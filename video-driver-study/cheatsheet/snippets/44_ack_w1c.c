/* S44 | 구조 예시
 * 전용 W1C이고 나머지 0 쓰기가 안전한 경우만. read한 전체 값에 OR하지 않는다.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

st = Mem_Write32(&io, ack_addr,
                 handled_flags, left);
