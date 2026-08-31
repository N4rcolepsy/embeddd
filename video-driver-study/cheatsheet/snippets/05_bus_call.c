/* S05 | 구조 예시
 * S04. bus·reg_id는 유효하게 설정.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint8_t id = 0;
DrvStatus st = bus.ops->read(
    bus.ctx, reg_id, &id, 1U, 20U);

if (st == DRV_OK) {
    /* Check id before using device. */
}
