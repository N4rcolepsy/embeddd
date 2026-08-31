/* S41 | C17
 * 전체 mem_io.h/.c와 사용. base·offset은 byte 주소. 범위 검사가 register 부작용을 검증하지는 않음.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "../../examples/memory_io/mem_io.h"

MemStatus read_at32(
    const MemIo *io, MemAddr base,
    uint32_t span, uint32_t offset,
    uint32_t *out, uint32_t budget)
{
    MemAddr address;
    if (!MemAddr_At32(base, span,
                      offset, &address)) {
        return MEM_EARG;
    }
    return Mem_Read32(io, address,
                      out, budget);
}
