/* S03 | C17
 * 독립 함수 예. handle_demo()는 1을 반환.
 * Educational snippet; not compiled or hardware-tested.
 */

typedef struct { int ready; } Demo;

int handle_demo(void)
{
    Demo d = {0};
    Demo *p = &d;
    p->ready = 1;
    return d.ready;  /* returns 1 */
}
/* p->ready == (*p).ready */
