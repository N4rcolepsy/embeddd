/* S14 | HAL
 * 버스·센서가 준비된 후 호출. 주소는 ASEL 회로와 일치해야 함.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint8_t raw[9];
HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
    &hi2c1, (uint16_t)(0x1DU << 1),
    0x08U, I2C_MEMADD_SIZE_8BIT,
    raw, (uint16_t)sizeof raw, 20U);
if (st == HAL_OK) {
    /* Decode only complete read. */
}
