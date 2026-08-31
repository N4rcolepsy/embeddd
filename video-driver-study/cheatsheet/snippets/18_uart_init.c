/* S18 | HAL
 * S17. 기존 전체 템플릿과 중복 링크하지 말 것.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "17_uart_api.h"

HAL_StatusTypeDef UART_Init(
    UART_HandleTypeDef *h,
    const UART_Config *c)
{
    if (!h || !c) return HAL_ERROR;
    if (!IS_UART_INSTANCE(c->instance) ||
        !c->baud_rate ||
        !IS_UART_BAUDRATE(c->baud_rate)) {
        return HAL_ERROR;
    }
    if (h->gState != HAL_UART_STATE_RESET ||
        h->RxState != HAL_UART_STATE_RESET) {
        return HAL_BUSY;
    }
    h->Instance = c->instance;
    h->Init.BaudRate = c->baud_rate;
    h->Init.WordLength = UART_WORDLENGTH_8B;
    h->Init.StopBits = UART_STOPBITS_1;
    h->Init.Parity = UART_PARITY_NONE;
    h->Init.Mode = UART_MODE_TX_RX;
    h->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    h->Init.OverSampling = UART_OVERSAMPLING_16;
    return HAL_UART_Init(h);
}
