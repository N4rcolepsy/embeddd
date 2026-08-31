/* S50 | 구조 예시
 * Platform_*는 예시 이름이며 실제 API가 아님. port_binding.example.c의 원형에 맞춰 구현.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

MemIo io = {
    .ctx = command_context,
    .read32 = Platform_Read32,
    .write32 = Platform_Write32,
    .sync = Platform_SyncWrites,
    .now_ms = Platform_MonotonicMs
};
