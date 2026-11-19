#include "stm32g4xx.h"

/* Separate test image: no application GPIO/peripheral initialization. */
#define STRINGIFY_(value) #value
#define STRINGIFY(value) STRINGIFY_(value)
__attribute__((used, section(".test_identity")))
const char test_identity[] = "UF4 G474 upload test 0.1.3 / " STRINGIFY(UPLOAD_TEST_KIB) " KiB / HSI16";

int main(void)
{
    /* Reset starts on the internal 16 MHz HSI; no external crystal/PLL needed. */
    SCB->VTOR = FLASH_BASE;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;
    GPIOC->BSRR = (1U << 0) | (1U << (1 + 16));
    GPIOC->MODER = (GPIOC->MODER & ~0xFU) | 0x5U;
    GPIOC->OTYPER &= ~0x3U;
    GPIOC->OSPEEDR &= ~0xFU;
    GPIOC->PUPDR &= ~0xFU;

    /* Poll the 500 ms counter without enabling any interrupts. */
    SysTick->LOAD = 16000000U / 2U - 1U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
    for (;;)
    {
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
        {
        }
        GPIOC->ODR ^= 0x3U;
    }
}
