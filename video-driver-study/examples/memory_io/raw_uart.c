#include "raw_uart.h"

static bool one_bit(uint32_t value)
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

static bool valid_config(const RawUartConfig *c)
{
    if (!c || !one_bit(c->tx_ready_mask) || !one_bit(c->tx_idle_mask) ||
        c->tx_ready_mask == c->tx_idle_mask) {
        return false;
    }
    uint32_t offsets[] = {c->status_off, c->txdata_off, c->ctrl_off, c->baud_off};
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        MemAddr address;
        if (!MemAddr_At32(c->base, c->span_bytes, offsets[i], &address)) {
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (offsets[i] == offsets[j]) {
                return false;
            }
        }
    }
    return true;
}

static MemStatus fail(RawUart *h, MemStatus st)
{
    h->state = RAW_UART_FAULT;
    return st;
}

static MemStatus sync_until(RawUart *h, uint32_t start, uint32_t timeout)
{
    uint32_t left;
    MemStatus st = Mem_Remaining(&h->io, start, timeout, &left);
    if (st != MEM_OK) {
        return st;
    }
    st = h->io.sync(h->io.ctx, left);
    if (st != MEM_OK) {
        return st;
    }
    /* Strict deadline: budget expiry is reported even if effects occurred. */
    return Mem_Remaining(&h->io, start, timeout, &left);
}

static MemStatus write_until(RawUart *h, uint32_t offset, uint32_t value,
                             uint32_t start, uint32_t timeout)
{
    uint32_t left;
    MemStatus st = Mem_Remaining(&h->io, start, timeout, &left);
    if (st != MEM_OK) {
        return st;
    }
    return Mem_Write32(&h->io, h->cfg.base + offset, value, left);
}

static MemStatus wait_flag(RawUart *h, uint32_t mask,
                           uint32_t start, uint32_t timeout)
{
    for (;;) {
        uint32_t left, status;
        MemStatus st = Mem_Remaining(&h->io, start, timeout, &left);
        if (st != MEM_OK) {
            return st;
        }
        st = Mem_Read32(&h->io, h->cfg.base + h->cfg.status_off, &status, left);
        if (st != MEM_OK) {
            return st;
        }
        st = Mem_Remaining(&h->io, start, timeout, &left);
        if (st != MEM_OK) {
            return st;
        }
        if ((status & mask) != 0U) {
            return MEM_OK;
        }
        /* Busy polling. An RTOS may add bounded yield/backoff here.
         * Do not use if now_ms stops while this context is running. */
    }
}

MemStatus RawUart_Init(RawUart *h, const MemIo *io,
                       const RawUartConfig *cfg, uint32_t timeout_ms)
{
    if (!h || !MemIo_Valid(io) || !valid_config(cfg) ||
        timeout_ms == 0U || timeout_ms > (uint32_t)INT32_MAX) {
        return MEM_EARG;
    }
    if (h->state == RAW_UART_READY) {
        return MEM_ESTATE;
    }
    /* Copy before touching h, so passing &h->io / &h->cfg is well-defined. */
    MemIo saved_io = *io;
    RawUartConfig saved_cfg = *cfg;
    h->io = saved_io;
    h->cfg = saved_cfg;
    h->state = RAW_UART_FAULT;
    uint32_t start = h->io.now_ms(h->io.ctx);
    const uint32_t offsets[] = {cfg->ctrl_off, cfg->baud_off, cfg->ctrl_off};
    const uint32_t values[] = {cfg->ctrl_disabled, cfg->baud_value, cfg->ctrl_enabled};
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        MemStatus st = write_until(h, offsets[i], values[i], start, timeout_ms);
        if (st != MEM_OK) {
            return st;
        }
        st = sync_until(h, start, timeout_ms);
        if (st != MEM_OK) {
            return st;
        }
    }
    h->state = RAW_UART_READY;
    return MEM_OK;
}

MemStatus RawUart_Write(RawUart *h, const uint8_t *src, size_t n,
                        size_t *sent, uint32_t timeout_ms)
{
    if (!sent) {
        return MEM_EARG;
    }
    *sent = 0U;
    if (!h || (n != 0U && !src)) {
        return MEM_EARG;
    }
    if (h->state != RAW_UART_READY) {
        return MEM_ESTATE;
    }
    if (n == 0U) {
        return MEM_OK;
    }
    if (timeout_ms == 0U || timeout_ms > (uint32_t)INT32_MAX) {
        return MEM_EARG;
    }
    uint32_t start = h->io.now_ms(h->io.ctx);
    for (size_t i = 0; i < n; ++i) {
        MemStatus st = wait_flag(h, h->cfg.tx_ready_mask, start, timeout_ms);
        if (st != MEM_OK) {
            return fail(h, st);
        }
        st = write_until(h, h->cfg.txdata_off, src[i], start, timeout_ms);
        if (st != MEM_OK) {
            return fail(h, st);
        }
        ++*sent;
        st = sync_until(h, start, timeout_ms);
        if (st != MEM_OK) {
            return fail(h, st);
        }
    }
    return MEM_OK;
}

MemStatus RawUart_Flush(RawUart *h, uint32_t timeout_ms)
{
    if (!h || timeout_ms == 0U || timeout_ms > (uint32_t)INT32_MAX) {
        return MEM_EARG;
    }
    if (h->state != RAW_UART_READY) {
        return MEM_ESTATE;
    }
    uint32_t start = h->io.now_ms(h->io.ctx);
    MemStatus st = wait_flag(h, h->cfg.tx_idle_mask, start, timeout_ms);
    return st == MEM_OK ? MEM_OK : fail(h, st);
}
