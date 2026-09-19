#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define DEFAULT_VID 0xCAFE
#define DEFAULT_PID 0x401B
#define MAX_PACKET  1024
#define ESP32_BOOT_SETTLE_MS  600
#define ESP32_RESET_SETTLE_MS 500

#define CMD_SET_TARGET              0x80
#define CMD_GET_TARGET              0x81
#define CMD_ESP32_BOOT              0x82
#define CMD_ESP32_RESET             0x83
#define CMD_GET_VTREF               0x84
#define CMD_GET_UART_STATS          0x88
#define CMD_CLEAR_UART_STATS        0x89
#define CMD_GET_UART_RX_SAMPLE      0x8A
#define CMD_RESET_ALL               0x8B
#define CMD_DAP_CONNECT             0x02
#define CMD_DAP_TRANSFER_CONFIGURE  0x04
#define CMD_DAP_TRANSFER            0x05
#define CMD_DAP_SWJ_PINS            0x10
#define CMD_DAP_SWJ_CLOCK           0x11
#define CMD_DAP_SWJ_SEQUENCE        0x12
#define CMD_DAP_SWD_CONFIGURE       0x13

#define PIN_SWCLK   (1u << 0)
#define PIN_SWDIO   (1u << 1)
#define PIN_NRESET  (1u << 7)

#define UART_ID_G474   0
#define UART_ID_F429   1
#define UART_ID_ESP32  2

typedef struct {
    HANDLE handle;
    DWORD input_len;
    DWORD output_len;
    wchar_t path[1024];
} uf4_hid_t;

static void usage(void)
{
    puts("uf4-target list");
    puts("uf4-target get");
    puts("uf4-target set <g474|f429|1|2>");
    puts("uf4-target esp32-boot");
    puts("uf4-target esp32-reset");
    puts("uf4-target reset");
    puts("uf4-target vtref");
    puts("uf4-target vref-adc");
    puts("uf4-target adc");
    puts("uf4-target uart-stats <g474|f429|esp32>");
    puts("uf4-target uart-clear-stats <g474|f429|esp32>");
    puts("uf4-target uart-rx-dump <g474|f429|esp32>");
    puts("uf4-target pins [--swclk 0|1] [--swdio 0|1] [--nrst 0|1]");
    puts("uf4-target probe-idcode [--target f429|g474] [--clock-khz N] [--repeat N]");
}

static int parse_target(const char *s, uint8_t *target)
{
    if (!s || !target) return 0;
    if (_stricmp(s, "g474") == 0 || _stricmp(s, "stm32g474") == 0 || strcmp(s, "1") == 0) {
        *target = 1; return 1;
    }
    if (_stricmp(s, "f429") == 0 || _stricmp(s, "stm32f429") == 0 || strcmp(s, "2") == 0) {
        *target = 2; return 1;
    }
    return 0;
}

static const char *target_name(uint8_t target)
{
    switch (target) {
    case 1: return "G474";
    case 2: return "F429";
    default: return "unknown";
    }
}

static int parse_uart_id(const char *s, uint8_t *id)
{
    if (!s || !id) return 0;
    if (_stricmp(s, "g474") == 0 || _stricmp(s, "target-g474") == 0 || strcmp(s, "0") == 0) {
        *id = UART_ID_G474; return 1;
    }
    if (_stricmp(s, "f429") == 0 || _stricmp(s, "target-f429") == 0 || strcmp(s, "1") == 0) {
        *id = UART_ID_F429; return 1;
    }
    if (_stricmp(s, "esp32") == 0 || _stricmp(s, "esp32c6") == 0 || strcmp(s, "2") == 0) {
        *id = UART_ID_ESP32; return 1;
    }
    return 0;
}

static const char *uart_name(uint8_t id)
{
    switch (id) {
    case UART_ID_G474: return "G474";
    case UART_ID_F429: return "F429";
    case UART_ID_ESP32: return "ESP32";
    default: return "unknown";
    }
}

static int parse_bit(const char *s, uint8_t *bit)
{
    if (!s || !bit) return 0;
    if (strcmp(s, "0") == 0) { *bit = 0; return 1; }
    if (strcmp(s, "1") == 0) { *bit = 1; return 1; }
    return 0;
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void print_last_error(const char *what)
{
    fprintf(stderr, "ERROR: %s failed, GetLastError=%lu\n", what, GetLastError());
}

static int get_caps(HANDLE h, DWORD *input_len, DWORD *output_len)
{
    PHIDP_PREPARSED_DATA ppd = NULL;
    HIDP_CAPS caps;

    if (!HidD_GetPreparsedData(h, &ppd)) return 0;
    if (HidP_GetCaps(ppd, &caps) != HIDP_STATUS_SUCCESS) {
        HidD_FreePreparsedData(ppd);
        return 0;
    }
    HidD_FreePreparsedData(ppd);
    *input_len = caps.InputReportByteLength ? caps.InputReportByteLength : 65;
    *output_len = caps.OutputReportByteLength ? caps.OutputReportByteLength : 65;
    if (*input_len > MAX_PACKET || *output_len > MAX_PACKET) return 0;
    return 1;
}

static HANDLE open_path(const wchar_t *path)
{
    return CreateFileW(path,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                       NULL);
}

static int enumerate_devices(wchar_t paths[][1024], int max_paths, int print_list)
{
    GUID guid;
    HDEVINFO devs;
    SP_DEVICE_INTERFACE_DATA ifdata;
    int count = 0;

    HidD_GetHidGuid(&guid);
    devs = SetupDiGetClassDevsW(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (devs == INVALID_HANDLE_VALUE) return 0;

    memset(&ifdata, 0, sizeof(ifdata));
    ifdata.cbSize = sizeof(ifdata);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devs, NULL, &guid, i, &ifdata); i++) {
        DWORD needed = 0;
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail;
        HANDLE h;
        HIDD_ATTRIBUTES attr;
        wchar_t product[128] = L"";
        wchar_t serial[128] = L"";
        DWORD in_len = 0, out_len = 0;

        SetupDiGetDeviceInterfaceDetailW(devs, &ifdata, NULL, 0, &needed, NULL);
        detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)calloc(1, needed);
        if (!detail) continue;
        detail->cbSize = sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetailW(devs, &ifdata, detail, needed, NULL, NULL)) {
            free(detail);
            continue;
        }

        h = open_path(detail->DevicePath);
        if (h == INVALID_HANDLE_VALUE) {
            free(detail);
            continue;
        }

        memset(&attr, 0, sizeof(attr));
        attr.Size = sizeof(attr);
        if (HidD_GetAttributes(h, &attr) &&
            attr.VendorID == DEFAULT_VID &&
            attr.ProductID == DEFAULT_PID) {
            HidD_GetProductString(h, product, sizeof(product));
            HidD_GetSerialNumberString(h, serial, sizeof(serial));
            get_caps(h, &in_len, &out_len);
            if (count < max_paths) {
                wcsncpy(paths[count], detail->DevicePath, 1023);
                paths[count][1023] = 0;
            }
            if (print_list) {
                wprintf(L"[%d] VID:PID=%04X:%04X serial=%ls product=%ls in=%lu out=%lu\n",
                        count, attr.VendorID, attr.ProductID,
                        serial[0] ? serial : L"-", product[0] ? product : L"-",
                        in_len, out_len);
            }
            count++;
        }

        CloseHandle(h);
        free(detail);
    }

    SetupDiDestroyDeviceInfoList(devs);
    return count;
}

static int open_first(uf4_hid_t *dev)
{
    wchar_t paths[16][1024];
    int count = enumerate_devices(paths, 16, 0);
    if (count <= 0) {
        fprintf(stderr, "ERROR: UF4 CMSIS-DAP HID not found (%04X:%04X)\n", DEFAULT_VID, DEFAULT_PID);
        return 0;
    }

    memset(dev, 0, sizeof(*dev));
    wcsncpy(dev->path, paths[0], 1023);
    dev->handle = open_path(paths[0]);
    if (dev->handle == INVALID_HANDLE_VALUE) {
        print_last_error("CreateFile");
        return 0;
    }
    if (!get_caps(dev->handle, &dev->input_len, &dev->output_len)) {
        CloseHandle(dev->handle);
        fprintf(stderr, "ERROR: cannot get HID report lengths\n");
        return 0;
    }
    HidD_SetNumInputBuffers(dev->handle, 16);
    return 1;
}

static int rw_overlapped(HANDLE h, int write, uint8_t *buf, DWORD len, DWORD timeout_ms, DWORD *done)
{
    OVERLAPPED ov;
    BOOL ok;
    DWORD err;

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ov.hEvent) return 0;

    ok = write ? WriteFile(h, buf, len, done, &ov) : ReadFile(h, buf, len, done, &ov);
    if (!ok) {
        err = GetLastError();
        if (err != ERROR_IO_PENDING) {
            CloseHandle(ov.hEvent);
            SetLastError(err);
            return 0;
        }
        if (WaitForSingleObject(ov.hEvent, timeout_ms) != WAIT_OBJECT_0) {
            CancelIo(h);
            CloseHandle(ov.hEvent);
            SetLastError(ERROR_TIMEOUT);
            return 0;
        }
        ok = GetOverlappedResult(h, &ov, done, FALSE);
    }

    CloseHandle(ov.hEvent);
    return ok ? 1 : 0;
}

static int hid_write_report(uf4_hid_t *dev, uint8_t *buf)
{
    DWORD done = 0;
    if (!rw_overlapped(dev->handle, 1, buf, dev->output_len, 1000, &done)) {
        print_last_error("WriteFile");
        return 0;
    }
    return 1;
}

static int hid_read_report(uf4_hid_t *dev, uint8_t *buf, DWORD timeout_ms, DWORD *done)
{
    if (!rw_overlapped(dev->handle, 0, buf, dev->input_len, timeout_ms, done)) {
        return 0;
    }
    return 1;
}

static int send_packet(uf4_hid_t *dev, const uint8_t *payload, size_t payload_len,
                       uint8_t *response, size_t response_cap, DWORD timeout_ms)
{
    uint8_t out[MAX_PACKET];
    uint8_t in[MAX_PACKET];
    DWORD done = 0;
    DWORD start = GetTickCount();

    if (payload_len + 1 > dev->output_len || response_cap < 64) return 0;
    memset(out, 0, sizeof(out));
    out[0] = 0; /* report ID */
    memcpy(out + 1, payload, payload_len);

    if (!hid_write_report(dev, out)) return 0;

    while ((GetTickCount() - start) < timeout_ms) {
        memset(in, 0, sizeof(in));
        if (!hid_read_report(dev, in, 100, &done)) continue;

        if (done >= 2 && in[0] == 0 && in[1] == payload[0]) {
            size_t n = done - 1;
            if (n > response_cap) n = response_cap;
            memcpy(response, in + 1, n);
            return (int)n;
        }
        if (done >= 1 && in[0] == payload[0]) {
            size_t n = done;
            if (n > response_cap) n = response_cap;
            memcpy(response, in, n);
            return (int)n;
        }
    }

    fprintf(stderr, "ERROR: timeout waiting for command 0x%02X\n", payload[0]);
    return 0;
}

static int cmd_get(uf4_hid_t *dev, uint8_t *target)
{
    uint8_t req[] = { CMD_GET_TARGET };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
    if (n < 3 || rsp[1] != 0) {
        fprintf(stderr, "ERROR: GET_TARGET failed\n");
        return 0;
    }
    *target = rsp[2];
    return 1;
}

static int cmd_set(uf4_hid_t *dev, uint8_t target)
{
    uint8_t req[] = { CMD_SET_TARGET, target };
    uint8_t rsp[64];
    uint8_t actual = 0;
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
    if (n < 2 || rsp[1] != 0) {
        fprintf(stderr, "ERROR: SET_TARGET failed\n");
        return 0;
    }
    Sleep(100);
    if (!cmd_get(dev, &actual)) return 0;
    printf("Selected target: %s (%u) on CMSIS-DAP\n", target_name(actual), actual);
    return actual == target;
}

static int cmd_simple_status(uf4_hid_t *dev, uint8_t cmd, const char *ok_text)
{
    uint8_t req[] = { cmd };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
    if (n < 2 || rsp[1] != 0) {
        fprintf(stderr, "ERROR: command 0x%02X failed\n", cmd);
        return 0;
    }
    puts(ok_text);
    return 1;
}

static int cmd_esp32_boot(uf4_hid_t *dev)
{
    if (!cmd_simple_status(dev, CMD_ESP32_BOOT, "ESP32 entered download mode")) {
        return 0;
    }

    Sleep(ESP32_BOOT_SETTLE_MS);
    return 1;
}

static int cmd_esp32_reset(uf4_hid_t *dev)
{
    if (!cmd_simple_status(dev, CMD_ESP32_RESET, "ESP32 reset")) {
        return 0;
    }

    Sleep(ESP32_RESET_SETTLE_MS);
    return 1;
}

static int cmd_reset_all(uf4_hid_t *dev)
{
    return cmd_simple_status(dev, CMD_RESET_ALL, "G474 and F429 reset");
}

static int cmd_vtref(uf4_hid_t *dev)
{
    uint8_t req[] = { CMD_GET_VTREF };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
    uint16_t mv;

    if ((n < 4) || (rsp[1] != 0)) {
        fprintf(stderr, "ERROR: GET_VTREF failed\n");
        return 0;
    }

    mv = (uint16_t)rsp[2] | ((uint16_t)rsp[3] << 8);
    printf("VTREF=%u mV\n", (unsigned)mv);
    return 1;
}

static int cmd_uart_stats(uf4_hid_t *dev, uint8_t id)
{
    uint8_t req[] = { CMD_GET_UART_STATS, id };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);

    if ((n < 35) || (rsp[1] != 0) || (rsp[2] != id)) {
        fprintf(stderr, "ERROR: GET_UART_STATS failed\n");
        return 0;
    }

    printf("%s UART stats:\n", uart_name(id));
    printf("  usb_to_uart_bytes     = %lu\n", (unsigned long)read_le32(&rsp[3]));
    printf("  usb_to_uart_dropped   = %lu\n", (unsigned long)read_le32(&rsp[7]));
    printf("  uart_tx_bytes         = %lu\n", (unsigned long)read_le32(&rsp[11]));
    printf("  uart_tx_errors        = %lu\n", (unsigned long)read_le32(&rsp[15]));
    printf("  uart_rx_bytes         = %lu\n", (unsigned long)read_le32(&rsp[19]));
    printf("  uart_rx_dropped       = %lu\n", (unsigned long)read_le32(&rsp[23]));
    printf("  usb_from_uart_bytes   = %lu\n", (unsigned long)read_le32(&rsp[27]));
    printf("  uart_rx_start_errors  = %lu\n", (unsigned long)read_le32(&rsp[31]));
    return 1;
}

static int cmd_uart_clear_stats(uf4_hid_t *dev, uint8_t id)
{
    uint8_t req[] = { CMD_CLEAR_UART_STATS, id };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);

    if ((n < 2) || (rsp[1] != 0)) {
        fprintf(stderr, "ERROR: CLEAR_UART_STATS failed\n");
        return 0;
    }

    printf("%s UART stats cleared\n", uart_name(id));
    return 1;
}

static void print_dump_line(uint16_t offset, const uint8_t *data, size_t len)
{
    printf("%04X  ", offset);
    for (size_t i = 0u; i < 16u; i++) {
        if (i < len) {
            printf("%02X ", data[i]);
        } else {
            printf("   ");
        }
    }
    printf(" ");
    for (size_t i = 0u; i < len; i++) {
        uint8_t c = data[i];
        putchar((c >= 32u && c <= 126u) ? c : '.');
    }
    putchar('\n');
}

static int cmd_uart_rx_dump(uf4_hid_t *dev, uint8_t id)
{
    uint8_t sample[256];
    size_t total = 0u;

    while (total < sizeof(sample)) {
        uint8_t req[] = {
            CMD_GET_UART_RX_SAMPLE,
            id,
            (uint8_t)(total & 0xFFu),
            (uint8_t)((total >> 8u) & 0xFFu)
        };
        uint8_t rsp[64];
        int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
        uint8_t count;

        if ((n < 6) || (rsp[1] != 0) || (rsp[2] != id)) {
            fprintf(stderr, "ERROR: GET_UART_RX_SAMPLE failed\n");
            return 0;
        }

        count = rsp[5];
        if ((count == 0u) || (n < (int)(6u + count))) {
            break;
        }

        if ((total + count) > sizeof(sample)) {
            count = (uint8_t)(sizeof(sample) - total);
        }
        memcpy(&sample[total], &rsp[6], count);
        total += count;
    }

    printf("%s UART RX sample: %lu bytes\n", uart_name(id), (unsigned long)total);
    for (size_t off = 0u; off < total; off += 16u) {
        size_t line_len = total - off;
        if (line_len > 16u) {
            line_len = 16u;
        }
        print_dump_line((uint16_t)off, &sample[off], line_len);
    }

    return 1;
}

static int dap_connect_swd(uf4_hid_t *dev)
{
    uint8_t req[] = { CMD_DAP_CONNECT, 1 };
    uint8_t rsp[64];
    int n = send_packet(dev, req, sizeof(req), rsp, sizeof(rsp), 1000);
    return (n >= 2 && rsp[1] == 1);
}

static int cmd_pins(uf4_hid_t *dev, int argc, char **argv)
{
    uint8_t value = 0, select = 0, rsp[64];
    uint8_t req[8];

    if (!dap_connect_swd(dev)) {
        fprintf(stderr, "ERROR: DAP_Connect(SWD) failed\n");
        return 0;
    }

    for (int i = 2; i < argc; i++) {
        uint8_t bit;
        if (strcmp(argv[i], "--swclk") == 0 && i + 1 < argc && parse_bit(argv[++i], &bit)) {
            select |= PIN_SWCLK; if (bit) value |= PIN_SWCLK;
        } else if (strcmp(argv[i], "--swdio") == 0 && i + 1 < argc && parse_bit(argv[++i], &bit)) {
            select |= PIN_SWDIO; if (bit) value |= PIN_SWDIO;
        } else if (strcmp(argv[i], "--nrst") == 0 && i + 1 < argc && parse_bit(argv[++i], &bit)) {
            select |= PIN_NRESET; if (bit) value |= PIN_NRESET;
        } else {
            fprintf(stderr, "ERROR: bad pins argument: %s\n", argv[i]);
            return 0;
        }
    }

    req[0] = CMD_DAP_SWJ_PINS;
    req[1] = value;
    req[2] = select;
    req[3] = req[4] = req[5] = req[6] = 0;

    if (send_packet(dev, req, 7, rsp, sizeof(rsp), 1000) < 2) return 0;
    printf("SWCLK=%u SWDIO=%u nRESET=%u raw=0x%02X\n",
           (rsp[1] & PIN_SWCLK) ? 1 : 0,
           (rsp[1] & PIN_SWDIO) ? 1 : 0,
           (rsp[1] & PIN_NRESET) ? 1 : 0,
           rsp[1]);
    return 1;
}

static int dap_status(uf4_hid_t *dev, const uint8_t *req, size_t req_len)
{
    uint8_t rsp[64];
    int n = send_packet(dev, req, req_len, rsp, sizeof(rsp), 1000);
    return (n >= 2 && rsp[1] == 0);
}

static int swj_sequence(uf4_hid_t *dev, uint8_t bits, const uint8_t *data, size_t data_len)
{
    uint8_t req[16];
    req[0] = CMD_DAP_SWJ_SEQUENCE;
    req[1] = bits;
    memcpy(req + 2, data, data_len);
    return dap_status(dev, req, 2 + data_len);
}

static const char *ack_text(uint8_t ack)
{
    switch (ack & 7) {
    case 1: return "ACK=001 OK";
    case 2: return "ACK=010 WAIT";
    case 4: return "ACK=100 FAULT";
    case 0: return "ACK=000 LINE_LOW/NO_ACK";
    case 7: return "ACK=111 LINE_HIGH/NO_ACK";
    default: return "ACK=invalid";
    }
}

static int cmd_probe(uf4_hid_t *dev, int argc, char **argv)
{
    int clock_khz = 1000;
    int repeat = 3;
    uint8_t target = 255;
    uint8_t req[8], rsp[64];
    uint8_t ones[7] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    uint8_t jtag_to_swd[2] = {0x9e, 0xe7};
    uint8_t zero[1] = {0};

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--clock-khz") == 0 && i + 1 < argc) {
            clock_khz = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
            repeat = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc && parse_target(argv[++i], &target)) {
        } else {
            fprintf(stderr, "ERROR: bad probe-idcode argument: %s\n", argv[i]);
            return 0;
        }
    }

    if (target != 255 && !cmd_set(dev, target)) return 0;
    if (!dap_connect_swd(dev)) {
        fprintf(stderr, "ERROR: DAP_Connect(SWD) failed\n");
        return 0;
    }

    uint32_t hz = (uint32_t)clock_khz * 1000u;
    req[0] = CMD_DAP_SWJ_CLOCK;
    req[1] = (uint8_t)hz; req[2] = (uint8_t)(hz >> 8);
    req[3] = (uint8_t)(hz >> 16); req[4] = (uint8_t)(hz >> 24);
    if (!dap_status(dev, req, 5)) return 0;
    req[0] = CMD_DAP_SWD_CONFIGURE; req[1] = 0;
    if (!dap_status(dev, req, 2)) return 0;
    req[0] = CMD_DAP_TRANSFER_CONFIGURE; req[1] = 0; req[2] = 100; req[3] = 0; req[4] = 0; req[5] = 0;
    if (!dap_status(dev, req, 6)) return 0;

    swj_sequence(dev, 56, ones, 7);
    swj_sequence(dev, 16, jtag_to_swd, 2);
    swj_sequence(dev, 56, ones, 7);
    swj_sequence(dev, 8, zero, 1);

    for (int i = 0; i < repeat; i++) {
        req[0] = CMD_DAP_TRANSFER; req[1] = 0; req[2] = 1; req[3] = 0x02;
        int n = send_packet(dev, req, 4, rsp, sizeof(rsp), 1000);
        if (n >= 7 && rsp[1] == 1 && rsp[2] == 1) {
            uint32_t id = (uint32_t)rsp[3] | ((uint32_t)rsp[4] << 8) |
                          ((uint32_t)rsp[5] << 16) | ((uint32_t)rsp[6] << 24);
            printf("IDCODE[%d]: 0x%08lX, %s\n", i, (unsigned long)id, ack_text(rsp[2]));
        } else if (n >= 3) {
            printf("IDCODE[%d]: %s, raw=%02X %02X %02X\n", i, ack_text(rsp[2]), rsp[0], rsp[1], rsp[2]);
        } else {
            printf("IDCODE[%d]: short response\n", i);
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    uf4_hid_t dev;
    int ok = 0;

    if ((argc < 2) ||
        (strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0) ||
        (strcmp(argv[1], "help") == 0)) {
        usage();
        return (argc < 2) ? 2 : 0;
    }

    if (strcmp(argv[1], "list") == 0) {
        int n;
        wchar_t paths[16][1024];
        n = enumerate_devices(paths, 16, 1);
        return n > 0 ? 0 : 1;
    }

    if (!open_first(&dev)) return 1;

    if (strcmp(argv[1], "get") == 0) {
        uint8_t target;
        ok = cmd_get(&dev, &target);
        if (ok) printf("Current target: %s (%u)\n", target_name(target), target);
    } else if (strcmp(argv[1], "set") == 0) {
        uint8_t target;
        if (argc < 3 || !parse_target(argv[2], &target)) {
            fprintf(stderr, "ERROR: expected target g474/f429\n");
            ok = 0;
        } else {
            ok = cmd_set(&dev, target);
        }
    } else if (strcmp(argv[1], "esp32-boot") == 0 || strcmp(argv[1], "boot-esp32") == 0) {
        ok = cmd_esp32_boot(&dev);
    } else if (strcmp(argv[1], "esp32-reset") == 0 || strcmp(argv[1], "reset-esp32") == 0) {
        ok = cmd_esp32_reset(&dev);
    } else if (strcmp(argv[1], "reset") == 0 || strcmp(argv[1], "reset-all") == 0) {
        ok = cmd_reset_all(&dev);
    } else if (strcmp(argv[1], "vtref") == 0 || strcmp(argv[1], "vref-adc") == 0 || strcmp(argv[1], "adc") == 0) {
        ok = cmd_vtref(&dev);
    } else if (strcmp(argv[1], "uart-stats") == 0 || strcmp(argv[1], "esp32-stats") == 0) {
        uint8_t id = UART_ID_ESP32;
        if ((strcmp(argv[1], "uart-stats") == 0) &&
            (argc < 3 || !parse_uart_id(argv[2], &id))) {
            fprintf(stderr, "ERROR: expected UART g474/f429/esp32\n");
            ok = 0;
        } else {
            ok = cmd_uart_stats(&dev, id);
        }
    } else if (strcmp(argv[1], "uart-clear-stats") == 0 || strcmp(argv[1], "esp32-clear-stats") == 0) {
        uint8_t id = UART_ID_ESP32;
        if ((strcmp(argv[1], "uart-clear-stats") == 0) &&
            (argc < 3 || !parse_uart_id(argv[2], &id))) {
            fprintf(stderr, "ERROR: expected UART g474/f429/esp32\n");
            ok = 0;
        } else {
            ok = cmd_uart_clear_stats(&dev, id);
        }
    } else if (strcmp(argv[1], "uart-rx-dump") == 0 || strcmp(argv[1], "esp32-rx-dump") == 0) {
        uint8_t id = UART_ID_ESP32;
        if ((strcmp(argv[1], "uart-rx-dump") == 0) &&
            (argc < 3 || !parse_uart_id(argv[2], &id))) {
            fprintf(stderr, "ERROR: expected UART g474/f429/esp32\n");
            ok = 0;
        } else {
            ok = cmd_uart_rx_dump(&dev, id);
        }
    } else if (strcmp(argv[1], "pins") == 0) {
        ok = cmd_pins(&dev, argc, argv);
    } else if (strcmp(argv[1], "probe-idcode") == 0) {
        ok = cmd_probe(&dev, argc, argv);
    } else {
        usage();
        ok = 0;
    }

    CloseHandle(dev.handle);
    return ok ? 0 : 1;
}
