#ifndef STUDY_UART_TEMPLATE_H
#define STUDY_UART_TEMPLATE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original teaching wrapper for classic STM32F4 HAL.
 * Fixed mode: 8 data bits, no parity, 1 stop bit, TX/RX, no flow control.
 * Board clocks, pins and HAL_UART_MspInit are supplied by your project.
 */
typedef struct {
    USART_TypeDef *instance;
    uint32_t baud_rate;
} UART_Config;

/* handle must be zero-initialized or successfully HAL_UART_DeInit'd.
 * Use exactly one live HAL handle for a peripheral instance.
 * Serialize access externally; do not call from an ISR.
 */
HAL_StatusTypeDef UART_Init(UART_HandleTypeDef *handle,
                           const UART_Config *config);

/* Synchronous polling TX for 8N1 with TX enabled.
 * length is 1..UINT16_MAX bytes; other frame formats are rejected.
 * Caller provides length readable bytes and exclusive peripheral access.
 * Failure may occur after some bytes were transmitted; no rollback.
 */
HAL_StatusTypeDef UART_Write(UART_HandleTypeDef *handle,
                            const uint8_t *data,
                            uint16_t length,
                            uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
