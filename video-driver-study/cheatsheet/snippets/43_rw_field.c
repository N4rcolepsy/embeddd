/* S43 | 구조 예시
 * 전체 RMW 동안 소유권 보호. bits는 이미 shift된 값. plain RW·reserved 정책 확인. sync 필요성 별도.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint32_t old, next;
st = Mem_Read32(&io, addr, &old, left);
if (st != MEM_OK) return st;
if (!Mem_FieldReplace32(old, mask,
                        bits, &next)) {
    return MEM_EARG;
}
/* Recompute remaining budget here. */
st = Mem_Write32(&io, addr, next, left);
