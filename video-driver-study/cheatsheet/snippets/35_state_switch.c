/* S35 | 구조 예시
 * S01. state·함수·상태 enum은 프로젝트에서 정의.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

switch (state) {
case READY:
    return perform_read();
case STARTING:
    return DRV_EBUSY;
default:
    return DRV_ESTATE;
}
