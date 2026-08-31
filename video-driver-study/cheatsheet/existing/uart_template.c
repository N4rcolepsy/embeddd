#include "uart_template.h"
#include <stddef.h>

HAL_StatusTypeDef UART_Init(UART_HandleTypeDef *handle,
                           const UART_Config *config)
{
    if (handle == NULL || config == NULL) {
        return HAL_ERROR;
    }

    if (!IS_UART_INSTANCE(config->instance) ||
        config->baud_rate == 0U ||
        !IS_UART_BAUDRATE(config->baud_rate)) {
        return HAL_ERROR;
    }

    /* This template only initializes a reset handle.
     * The check is not a mutex; caller must serialize access.
     */
    if (handle->gState != HAL_UART_STATE_RESET ||
        handle->RxState != HAL_UART_STATE_RESET) {
        return HAL_BUSY;
    }

    handle->Instance = config->instance;
    handle->Init.BaudRate = config->baud_rate;
    handle->Init.WordLength = UART_WORDLENGTH_8B;
    handle->Init.StopBits = UART_STOPBITS_1;
    handle->Init.Parity = UART_PARITY_NONE;
    handle->Init.Mode = UART_MODE_TX_RX;
    handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle->Init.OverSampling = UART_OVERSAMPLING_16;

    return HAL_UART_Init(handle);
}

HAL_StatusTypeDef UART_Write(UART_HandleTypeDef *handle,
                            const uint8_t *data,
                            uint16_t length,
                            uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || length == 0U) {
        return HAL_ERROR;
    }

    /* Keep the byte-buffer contract even for an existing CubeMX handle.
     * HAL's 9-bit/no-parity mode reads 16-bit elements instead.
     */
    if (handle->Init.WordLength != UART_WORDLENGTH_8B ||
        handle->Init.Parity != UART_PARITY_NONE ||
        handle->Init.StopBits != UART_STOPBITS_1 ||
        (handle->Init.Mode & UART_MODE_TX) == 0U) {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(handle, data, length, timeout_ms);
}
