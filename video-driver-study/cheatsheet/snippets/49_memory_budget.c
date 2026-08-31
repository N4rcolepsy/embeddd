/* S49 | 구조 예시
 * start는 작업 전체에서 공유. timeout 1..INT32_MAX ms. callback은 받은 예산 내 반환.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint32_t left;
MemStatus st = Mem_Remaining(
    &io, start, timeout_ms, &left);
if (st != MEM_OK) return st;
st = Mem_Read32(&io, addr, &value, left);
if (st != MEM_OK) return st;
/* Check deadline after access too. */
