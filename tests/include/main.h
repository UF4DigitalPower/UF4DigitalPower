/**
 * @file main.h
 * @brief Minimal host-test stub shared by protocol/control unit tests.
 */

#ifndef UF4DIGITALPOWER_TEST_MAIN_H
#define UF4DIGITALPOWER_TEST_MAIN_H

#include <stdint.h>

typedef struct
{
    uint32_t odr;
} GPIO_TypeDef;

typedef enum
{
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET = 1U,
} GPIO_PinState;

extern GPIO_TypeDef g_TEST_red_gpio_port;
extern GPIO_TypeDef g_TEST_green_gpio_port;
extern GPIO_TypeDef g_TEST_blue_gpio_port;

#define RED_GPIO_Port    (&g_TEST_red_gpio_port)
#define GREEN_GPIO_Port  (&g_TEST_green_gpio_port)
#define BLUE_GPIO_Port   (&g_TEST_blue_gpio_port)

#define RED_Pin          0x0001U
#define GREEN_Pin        0x0002U
#define BLUE_Pin         0x0004U

void Error_Handler(void);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
uint32_t HAL_GetTick(void);

#endif /* UF4DIGITALPOWER_TEST_MAIN_H */
