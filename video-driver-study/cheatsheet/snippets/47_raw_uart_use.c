/* S47 | 구조 예시
 * board_io·board_cfg는 실제 명령과 map에서 구성. 전원·clock·reset·pinmux 선행.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

RawUart uart = {0};
MemStatus st = RawUart_Init(
    &uart, &board_io, &board_cfg, 100U);
if (st == MEM_OK) {
    const uint8_t msg[] = {'O', 'K'};
    size_t sent = 0U;
    st = RawUart_Write(&uart, msg,
        sizeof msg, &sent, 100U);
    if (st == MEM_OK) {
        st = RawUart_Flush(&uart, 100U);
    }
    /* Handle st and partial sent. */
}
