/* ADAPTER EXAMPLE ONLY. Rename/implement Platform_* using your actual API.
 * No HAL library is needed. These declarations are NOT existing platform APIs.
 * Compile this separately from the fake test; link real definitions on target.
 */
#include "mem_io.h"

extern MemStatus Platform_Read32(void *, MemAddr, uint32_t *, uint32_t);
extern MemStatus Platform_Write32(void *, MemAddr, uint32_t, uint32_t);
extern MemStatus Platform_SyncWrites(void *, uint32_t);
extern uint32_t Platform_MonotonicMs(void *);

MemIo BindPlatformMemoryCommands(void *command_context)
{
    MemIo io = {
        .ctx = command_context,
        .read32 = Platform_Read32,
        .write32 = Platform_Write32,
        .sync = Platform_SyncWrites,
        .now_ms = Platform_MonotonicMs
    };
    return io;
}
