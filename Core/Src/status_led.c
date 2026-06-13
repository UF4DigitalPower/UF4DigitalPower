#include "status_led.h"

#include "function.h"

#define STATUS_LED_BLINK_FAST_MS    100U
#define STATUS_LED_BLINK_SLOW_MS    500U

typedef struct
{
    GPIO_PinState red;
    GPIO_PinState yellow;
    GPIO_PinState green;
} StatusLedPattern;

static void StatusLed_Apply(StatusLedPattern pattern)
{
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, pattern.red);
    HAL_GPIO_WritePin(LED_Y_GPIO_Port, LED_Y_Pin, pattern.yellow);
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, pattern.green);
}

static GPIO_PinState StatusLed_Blink(uint32_t tickNow, uint32_t periodMs)
{
    return (((tickNow / periodMs) & 0x01U) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void StatusLed_Init(void)
{
    StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_RESET});
}

void StatusLed_Update(void)
{
    uint32_t tickNow = HAL_GetTick();
    GPIO_PinState slowBlink = StatusLed_Blink(tickNow, STATUS_LED_BLINK_SLOW_MS);
    GPIO_PinState fastBlink = StatusLed_Blink(tickNow, STATUS_LED_BLINK_FAST_MS);

    if (DF.ErrFlag != F_NOERR || DF.SMFlag == Err)
    {
        StatusLed_Apply((StatusLedPattern){fastBlink, GPIO_PIN_RESET, GPIO_PIN_RESET});
        return;
    }

    switch (DF.SMFlag)
    {
        case Init:
            StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_RESET});
            break;

        case Wait:
            if (DF.OUTPUT_Flag != 0U)
            {
                StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, fastBlink, GPIO_PIN_RESET});
            }
            else
            {
                StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, slowBlink, GPIO_PIN_RESET});
            }
            break;

        case Rise:
            StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, fastBlink, fastBlink});
            break;

        case Run:
            StatusLed_Apply((StatusLedPattern){GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET});
            break;

        case Err:
        default:
            StatusLed_Apply((StatusLedPattern){fastBlink, GPIO_PIN_RESET, GPIO_PIN_RESET});
            break;
    }
}
