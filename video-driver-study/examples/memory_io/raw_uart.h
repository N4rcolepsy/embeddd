#ifndef STUDY_RAW_UART_H
#define STUDY_RAW_UART_H

#include "mem_io.h"

/* TEACHING UART MODEL, not a register map for a real chip.
 * Four distinct registers, all permit aligned 32-bit transactions:
 * STATUS: side-effect-free RO; READY/IDLE are active-high one-bit flags.
 * TXDATA: WO FIFO push of bits [7:0], upper written bits must be zero.
 * CTRL/BAUD: plain RW; caller supplies complete safe register values.
 * Required device sequence: disable -> baud -> enable, sync after each.
 * No unlock, bank, DLAB, reset wait, RX, IRQ, DMA, or clock/pin setup here.
 * If the datasheet differs, modify the algorithm as well as the map.
 */
typedef struct {
    MemAddr base;
    uint32_t span_bytes;
    uint32_t status_off, txdata_off, ctrl_off, baud_off;
    uint32_t tx_ready_mask, tx_idle_mask;
    uint32_t ctrl_disabled, ctrl_enabled, baud_value;
} RawUartConfig;

typedef enum { RAW_UART_OFF = 0, RAW_UART_READY, RAW_UART_FAULT } RawUartState;
typedef struct {
    MemIo io;             /* Copy callbacks; borrow ctx. */
    RawUartConfig cfg;     /* Own a copy; cfg argument may expire. */
    RawUartState state;
} RawUart;

/* Start with RawUart h = {0}. One owner; serialize ALL operations externally.
 * Init on READY is rejected. Before retrying a FAULT, perform any board/chip
 * recovery required by its datasheet. This module cannot undo partial writes.
 * Each nonempty operation has one shared timeout, 1..INT32_MAX milliseconds.
 * src/sent must not overlap the handle or each other. No ISR use assumed.
 */
MemStatus RawUart_Init(RawUart *h, const MemIo *io,
                       const RawUartConfig *cfg, uint32_t timeout_ms);
/* OK: all bytes accepted into TX FIFO. Use Flush for line-idle completion.
 * *sent counts only successful write32 callbacks, even if a later sync fails.
 * On failed write, one more byte MAY have reached hardware; count is a lower
 * bound, not a safe retry cursor. No automatic retries. Buffer not retained.
 * n==0 allows src==NULL and returns OK on a READY handle, with *sent==0.
 */
MemStatus RawUart_Write(RawUart *h, const uint8_t *src, size_t n,
                        size_t *sent, uint32_t timeout_ms);
MemStatus RawUart_Flush(RawUart *h, uint32_t timeout_ms);
#endif
