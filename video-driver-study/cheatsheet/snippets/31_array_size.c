/* S31 | 구조 예시
 * stdint.h + stddef.h. 선언·호출 위치의 차이를 보는 조각.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint16_t values[6] = {0};
size_t count = sizeof values /
               sizeof values[0];
size_t bytes = sizeof values;
/* count=6; bytes=6*sizeof(uint16_t) */

void parse(const uint8_t *p, size_t n);
/* Parameter uint8_t p[6] also adjusts */
/* to pointer; sizeof p is NOT six.   */
