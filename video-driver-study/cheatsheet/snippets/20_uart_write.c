/* S20 | HAL
 * 준비된 8N1 handle. task/main 전용, 접근은 외부 직렬화.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef UART_Write(
    UART_HandleTypeDef *h,
    const uint8_t *data,
    uint16_t n, uint32_t timeout_ms)
{
    if (!h || !data || !n) return HAL_ERROR;
    if (h->Init.WordLength !=
            UART_WORDLENGTH_8B ||
        h->Init.Parity != UART_PARITY_NONE ||
        h->Init.StopBits != UART_STOPBITS_1 ||
        !(h->Init.Mode & UART_MODE_TX)) {
        return HAL_ERROR;
    }
    return HAL_UART_Transmit(
        h, data, n, timeout_ms);
}
