/* S19 | HAL
 * S17+S18 또는 existing 템플릿. 최초 초기화용.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "17_uart_api.h"

static UART_HandleTypeDef uart = {0};

HAL_StatusTypeDef start_uart(void)
{
    const UART_Config cfg = {
        .instance = USART2,
        .baud_rate = 115200U
    };
    return UART_Init(&uart, &cfg);
}
