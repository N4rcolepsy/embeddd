/* S15 | HAL
 * HAL + stm32f4xx_ll_gpio.h. 클록·출력 설정 후 한 방법 선택.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

/* Alternatives; PA5 output is ready. */
HAL_GPIO_WritePin(
    GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_5);

GPIOA->BSRR = (1UL << 5);
