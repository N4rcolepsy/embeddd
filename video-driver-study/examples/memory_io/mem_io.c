#include "mem_io.h"

bool MemIo_Valid(const MemIo *io)
{
    return io && io->read32 && io->write32 && io->sync && io->now_ms;
}

bool MemAddr_At32(MemAddr base, uint32_t span_bytes,
                  uint32_t offset_bytes, MemAddr *out)
{
    if (!out || (base & UINT64_C(3)) != 0U ||
        (offset_bytes & UINT32_C(3)) != 0U || span_bytes < 4U ||
        offset_bytes > span_bytes - 4U ||
        base > UINT64_MAX - offset_bytes - UINT64_C(3)) {
        return false;
    }
    *out = base + offset_bytes;
    return true;
}

MemStatus Mem_Read32(const MemIo *io, MemAddr addr,
                     uint32_t *out, uint32_t budget_ms)
{
    uint32_t value = 0U;
    if (!MemIo_Valid(io) || !out || (addr & UINT64_C(3)) != 0U ||
        addr > UINT64_MAX - UINT64_C(3)) {
        return MEM_EARG;
    }
    MemStatus st = io->read32(io->ctx, addr, &value, budget_ms);
    if (st == MEM_OK) {
        *out = value; /* Preserve caller output on failure. */
    }
    return st;
}

MemStatus Mem_Write32(const MemIo *io, MemAddr addr,
                      uint32_t value, uint32_t budget_ms)
{
    if (!MemIo_Valid(io) || (addr & UINT64_C(3)) != 0U ||
        addr > UINT64_MAX - UINT64_C(3)) {
        return MEM_EARG;
    }
    return io->write32(io->ctx, addr, value, budget_ms);
}

bool Mem_FieldReplace32(uint32_t old_value, uint32_t mask,
                        uint32_t shifted_value, uint32_t *out)
{
    if (!out || mask == 0U || (shifted_value & ~mask) != 0U) {
        return false;
    }
    *out = (old_value & ~mask) | shifted_value;
    return true;
}

MemStatus Mem_Remaining(const MemIo *io, uint32_t start_ms,
                        uint32_t timeout_ms, uint32_t *remaining_ms)
{
    if (!MemIo_Valid(io) || !remaining_ms || timeout_ms == 0U ||
        timeout_ms > (uint32_t)INT32_MAX) {
        return MEM_EARG;
    }
    uint32_t elapsed = (uint32_t)(io->now_ms(io->ctx) - start_ms);
    if (elapsed >= timeout_ms) {
        *remaining_ms = 0U;
        return MEM_ETIMEOUT;
    }
    *remaining_ms = timeout_ms - elapsed;
    return MEM_OK;
}
