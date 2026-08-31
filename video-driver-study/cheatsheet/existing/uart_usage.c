#include "uart_template.h"

/* Call the demo once, from normal application/task context, after HAL_Init
 * and system clock setup. Your HAL_UART_MspInit must configure USART2's
 * clock and the board's TX/RX alternate-function pins.
 * Do NOT also initialize USART2 through a different CubeMX handle.
 */
static UART_HandleTypeDef debug_uart = {0};

HAL_StatusTypeDef UART_ExampleOnce(void)
{
    const UART_Config config = {
        .instance = USART2,
        .baud_rate = 115200U
    };

    HAL_StatusTypeDef status = UART_Init(&debug_uart, &config);
    if (status != HAL_OK) {
        return status;
    }

    const uint8_t message[] = "Hello UART\r\n";
    return UART_Write(&debug_uart,
                      message,
                      (uint16_t)(sizeof message - 1U),
                      100U);
}
