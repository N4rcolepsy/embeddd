/* S03 | C17
 * 문법 예. 함수 안에 두어 사용.
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
