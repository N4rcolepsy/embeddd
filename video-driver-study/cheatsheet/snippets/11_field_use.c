/* S11 | 구조 예시
 * S10. old는 이미 읽은 일반 RW 값. 가상 필드 예.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

const uint8_t mask = 0x0EU;
uint8_t next = field_put8(
    old, mask, 1U, 5U);
uint8_t mode = field_get8(
    next, mask, 1U);
/* mode == 5; other bits preserved. */
