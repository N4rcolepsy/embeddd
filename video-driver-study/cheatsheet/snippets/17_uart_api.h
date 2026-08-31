#ifndef STUDY_SNIPPET_17_UART_API_H
#define STUDY_SNIPPET_17_UART_API_H

/* S17 | HAL
 * 전체 파일은 existing/uart_template.h. 8N1·TX/RX 고정.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "stm32f4xx_hal.h"

typedef struct {
    USART_TypeDef *instance;
    uint32_t baud_rate;
} UART_Config;

HAL_StatusTypeDef UART_Init(
    UART_HandleTypeDef *handle,
    const UART_Config *config);

#endif /* STUDY_SNIPPET_17_UART_API_H */
