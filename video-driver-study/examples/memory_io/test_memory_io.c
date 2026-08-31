#include "raw_uart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Entire map below belongs to this RAM simulator, NOT to real hardware. */
enum { STATUS = 0, TXDATA = 4, CTRL = 8, BAUD = 12, W1C = 16 };
typedef struct {
    uint32_t tick, status, flags;
    unsigned reads, writes, syncs, fail_write, fail_sync;
    bool fail_read;
    uint8_t tx[8];
    size_t tx_n;
    uint32_t offsets[16], values[16];
    char trace[64];
    size_t trace_n;
} Fake;

static uint32_t now_ms(void *ctx) { return ((Fake *)ctx)->tick++; }
static MemStatus read32(void *ctx, MemAddr addr, uint32_t *out, uint32_t budget)
{
    Fake *f = ctx;
    (void)budget;
    ++f->reads;
    f->trace[f->trace_n++] = 'R';
    if (f->fail_read) { *out = 0xBADU; return MEM_EIO; }
    if (addr == STATUS) { *out = f->status; return MEM_OK; }
    if (addr == W1C) { *out = f->flags; return MEM_OK; }
    return MEM_EIO;
}
static MemStatus write32(void *ctx, MemAddr addr, uint32_t value, uint32_t budget)
{
    Fake *f = ctx;
    (void)budget;
    ++f->writes;
    f->trace[f->trace_n++] = 'W';
    /* Simulate ambiguous completion: mutate FIRST, then report failure. */
    if (addr == TXDATA) {
        assert(f->tx_n < sizeof f->tx);
        f->tx[f->tx_n++] = (uint8_t)value;
    }
    if (addr == W1C) { f->flags &= ~value; }
    assert(f->writes <= 16U);
    f->offsets[f->writes - 1U] = (uint32_t)addr;
    f->values[f->writes - 1U] = value;
    return f->writes == f->fail_write ? MEM_EIO : MEM_OK;
}
static MemStatus sync_io(void *ctx, uint32_t budget)
{
    Fake *f = ctx;
    (void)budget;
    ++f->syncs;
    f->trace[f->trace_n++] = 'S';
    return f->syncs == f->fail_sync ? MEM_EIO : MEM_OK;
}
static MemIo make_io(Fake *f)
{
    MemIo io = { f, read32, write32, sync_io, now_ms };
    return io;
}
static RawUartConfig config(void)
{
    RawUartConfig c = {
        .base = 0, .span_bytes = 20,
        .status_off = STATUS, .txdata_off = TXDATA,
        .ctrl_off = CTRL, .baud_off = BAUD,
        .tx_ready_mask = 1U, .tx_idle_mask = 2U,
        .ctrl_disabled = 0U, .ctrl_enabled = 1U, .baud_value = 27U
    };
    return c;
}

int main(void)
{
    MemAddr addr = 99U;
    assert(MemAddr_At32(0, 20, 16, &addr) && addr == 16U);
    assert(!MemAddr_At32(0, 20, 20, &addr) && addr == 16U);
    assert(!MemAddr_At32(2, 20, 0, &addr));
    assert(!MemAddr_At32(0, 20, 2, &addr));
    assert(!MemAddr_At32(UINT64_MAX - 3U, 8, 4, &addr));
    uint32_t field = 0U;
    assert(Mem_FieldReplace32(0xA5U, 0x0CU, 0x08U, &field));
    assert(field == 0xA9U);
    assert(!Mem_FieldReplace32(0xA5U, 0x0CU, 0x10U, &field));

    Fake f = { .status = 3U, .flags = 3U };
    MemIo io = make_io(&f);
    uint32_t out = 42U;
    f.fail_read = true;
    assert(Mem_Read32(&io, STATUS, &out, 20) == MEM_EIO && out == 42U);
    unsigned before = f.reads;
    assert(Mem_Read32(&io, 2, &out, 20) == MEM_EARG && f.reads == before);
    f.fail_read = false;
    before = f.reads;
    assert(Mem_Write32(&io, W1C, 1, 20) == MEM_OK);
    assert(f.flags == 2U && f.reads == before); /* clear only requested flag */

    f = (Fake){ .status = 3U };
    RawUartConfig cfg = config();
    RawUart h = {0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_OK);
    assert(h.state == RAW_UART_READY && f.writes == 3U && f.syncs == 3U);
    assert(f.offsets[0] == CTRL && f.values[0] == 0U);
    assert(f.offsets[1] == BAUD && f.values[1] == 27U);
    assert(f.offsets[2] == CTRL && f.values[2] == 1U);
    assert(memcmp(f.trace, "WSWSWS", 6) == 0);
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_ESTATE);
    const uint8_t message[] = {'O', 'K'};
    size_t sent = 99U;
    assert(RawUart_Write(&h, message, 2, &sent, 100) == MEM_OK);
    assert(sent == 2U && f.tx_n == 2U && memcmp(f.tx, message, 2) == 0);
    assert(memcmp(f.trace + 6, "RWSRWS", 6) == 0);
    assert(RawUart_Flush(&h, 100) == MEM_OK);
    assert(RawUart_Write(&h, NULL, 0, &sent, 0) == MEM_OK && sent == 0U);

    f.status = 1U; /* FIFO ready but line NOT idle */
    assert(RawUart_Flush(&h, 5) == MEM_ETIMEOUT);
    assert(h.state == RAW_UART_FAULT);

    f = (Fake){ .status = 3U, .tick = UINT32_MAX - 2U };
    h = (RawUart){0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_OK); /* tick wrap */
    f.fail_write = f.writes + 2U;
    assert(RawUart_Write(&h, message, 2, &sent, 100) == MEM_EIO);
    assert(sent == 1U && f.tx_n == 2U && h.state == RAW_UART_FAULT);
    /* Software count is a lower bound; resending from sent would duplicate. */
    assert(RawUart_Write(&h, message, 2, &sent, 100) == MEM_ESTATE);

    f = (Fake){ .status = 3U, .fail_sync = 2U };
    h = (RawUart){0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_EIO);
    assert(h.state == RAW_UART_FAULT && f.writes == 2U);
    f = (Fake){ .status = 3U };
    h = (RawUart){0};
    cfg.txdata_off = cfg.status_off;
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_EARG && f.writes == 0U);

    cfg = config();
    f = (Fake){ .status = 3U };
    h = (RawUart){0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_OK);
    f.status = 0U; /* never ready: no DATA write before timeout */
    before = f.writes;
    assert(RawUart_Write(&h, message, 2, &sent, 5) == MEM_ETIMEOUT);
    assert(sent == 0U && f.writes == before && h.state == RAW_UART_FAULT);

    f = (Fake){ .status = 3U };
    h = (RawUart){0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_OK);
    f.fail_sync = f.syncs + 1U;
    assert(RawUart_Write(&h, message, 2, &sent, 100) == MEM_EIO);
    assert(sent == 1U && f.tx_n == 1U && h.state == RAW_UART_FAULT);

    f = (Fake){ .status = 3U };
    h = (RawUart){0};
    assert(RawUart_Init(&h, &io, &cfg, 100) == MEM_OK);
    f.fail_read = true;
    before = f.writes;
    assert(RawUart_Write(&h, message, 2, &sent, 100) == MEM_EIO);
    assert(sent == 0U && f.writes == before && h.state == RAW_UART_FAULT);
    puts("memory_io tests passed");
    return 0;
}
