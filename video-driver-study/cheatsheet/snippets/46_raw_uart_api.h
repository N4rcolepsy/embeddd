#ifndef STUDY_SNIPPET_46_RAW_UART_API_H
#define STUDY_SNIPPET_46_RAW_UART_API_H

/* S46 | C17
 * 전체 raw_uart.h/.c 사용. RawUart는 내 객체이며 ST handle이 아님.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "../../examples/memory_io/raw_uart.h"

MemStatus RawUart_Init(
    RawUart *h, const MemIo *io,
    const RawUartConfig *cfg,
    uint32_t timeout_ms);

MemStatus RawUart_Write(
    RawUart *h, const uint8_t *src,
    size_t n, size_t *sent,
    uint32_t timeout_ms);

MemStatus RawUart_Flush(
    RawUart *h, uint32_t timeout_ms);

#endif /* STUDY_SNIPPET_46_RAW_UART_API_H */
