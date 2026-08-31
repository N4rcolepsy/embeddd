/* S23 | C17
 * count>=0, 실제 추가 인자는 count개의 int. stdout 연결 별도.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdarg.h>
#include <stdio.h>

void PrintInts(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; ++i) {
        int v = va_arg(ap, int);
        printf("%d\n", v);
    }
    va_end(ap);
}
/* PrintInts(3, 10, 20, 30); */
