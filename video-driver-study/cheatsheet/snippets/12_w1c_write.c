/* S12 | 구조 예시
 * S04. 실제 register별 W1C·예약 비트 규칙 확인.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

/* Fictional register: all status bits */
/* are W1C, unused bits accept zero.  */
uint8_t clear_mask = 0x04U;
st = bus.ops->write(
    bus.ctx, status_reg,
    &clear_mask, 1U, 20U);
