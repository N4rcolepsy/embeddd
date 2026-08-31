#ifndef STUDY_SNIPPET_45_UART_MAP_H
#define STUDY_SNIPPET_45_UART_MAP_H

/* S45 | 구조 예시
 * raw_uart.h 발췌. 실제 address·mask·divider를 제공하지 않는다. 전체 헤더와 중복 정의 금지.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

typedef struct {
    MemAddr base;
    uint32_t span_bytes;
    uint32_t status_off, txdata_off;
    uint32_t ctrl_off, baud_off;
    uint32_t tx_ready_mask;
    uint32_t tx_idle_mask;
    uint32_t ctrl_disabled;
    uint32_t ctrl_enabled;
    uint32_t baud_value;
} RawUartConfig;

#endif /* STUDY_SNIPPET_45_UART_MAP_H */
