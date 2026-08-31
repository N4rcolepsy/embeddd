/* S16 | HAL
 * 설명 조각. 이것만으로 I2C 초기화가 끝나지 않음.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

static I2C_HandleTypeDef hi2c1 = {0};
/* Part of initialization only: */
hi2c1.Instance = I2C1;

/* &hi2c1: RAM management object */
/* I2C1:   peripheral registers  */
