#ifndef STUDY_MEM_IO_H
#define STUDY_MEM_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Byte address in the command's address space, NOT necessarily a CPU pointer. */
typedef uint64_t MemAddr;
typedef enum {
    MEM_OK = 0, MEM_EARG, MEM_ESTATE, MEM_ETIMEOUT, MEM_EIO
} MemStatus;

/* All callbacks are synchronous and bounded by budget_ms (0: no waiting).
 * read32/write32 perform ONE aligned, legal 32-bit register transaction.
 * Values are CPU-order integers; the backend handles transport byte order.
 * MEM_OK from write32 means accepted, not peripheral operation completed.
 * A failed write may still have reached hardware: never retry blindly.
 * sync completes/orders preceding writes as the target platform requires.
 * A no-op sync is allowed ONLY if the backend already gives this guarantee.
 * now_ms is monotonic modulo 2^32 and keeps advancing during polling.
 * No callback retains buffer pointers. ctx must outlive every use.
 */
typedef struct {
    void *ctx;
    MemStatus (*read32)(void *, MemAddr, uint32_t *, uint32_t);
    MemStatus (*write32)(void *, MemAddr, uint32_t, uint32_t);
    MemStatus (*sync)(void *, uint32_t);
    uint32_t (*now_ms)(void *);
} MemIo;

bool MemIo_Valid(const MemIo *io);
bool MemAddr_At32(MemAddr base, uint32_t span_bytes,
                  uint32_t offset_bytes, MemAddr *out);
MemStatus Mem_Read32(const MemIo *io, MemAddr addr,
                     uint32_t *out, uint32_t budget_ms);
MemStatus Mem_Write32(const MemIo *io, MemAddr addr,
                      uint32_t value, uint32_t budget_ms);

/* Pure software field replacement. mask and value are already shifted.
 * Hardware RMW is allowed only for documented plain RW registers under
 * exclusive ownership. This does not implement W1C/W0C/FIFO/RC policies.
 */
bool Mem_FieldReplace32(uint32_t old_value, uint32_t mask,
                        uint32_t shifted_value, uint32_t *out);

/* timeout_ms must be 1..INT32_MAX; elapsed must stay below one full wrap.
 * The time source and each callback must satisfy the above timing contract.
 */
MemStatus Mem_Remaining(const MemIo *io, uint32_t start_ms,
                        uint32_t timeout_ms, uint32_t *remaining_ms);
#endif
