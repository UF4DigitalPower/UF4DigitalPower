#include "param_crc.h"

uint32_t ParamCRC32_Update(uint32_t crc, const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    size_t i;
    uint32_t bit;
    for (i = 0u; i < length; ++i)
    {
        crc ^= bytes[i];
        for (bit = 0u; bit < 8u; ++bit)
            crc = (crc >> 1u) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc;
}

uint32_t ParamCRC32(const void *data, size_t length)
{
    return ParamCRC32_Update(0xFFFFFFFFu, data, length) ^ 0xFFFFFFFFu;
}
