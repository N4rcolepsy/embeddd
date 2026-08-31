/* S21 | HAL
 * S20 선언 + 초기화된 uart. 실패 시 전송 rollback은 없음.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

const uint8_t msg[] = "Hello\r\n";
HAL_StatusTypeDef st = UART_Write(
    &uart, msg,
    (uint16_t)(sizeof msg - 1U), 100U);
if (st != HAL_OK) {
    /* Some bytes may already be sent. */
}
