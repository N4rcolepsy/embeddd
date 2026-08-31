/* Independent hosted C17 example. This uses stdout, not the UART wrapper.
 * Compile on a host with a C compiler:
 *   cc -std=c17 -Wall -Wextra -Wconversion -pedantic variadic_demo.c -o variadic_demo
 * Not compiled or executed in this workspace: no compiler was available.
 */
#include <stdarg.h>
#include <stdio.h>

/* Contract: count >= 0; exactly count further arguments are supplied,
 * each with promoted type int. The function cannot verify that contract.
 */
static void print_ints(int count, ...)
{
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; ++i) {
        int value = va_arg(args, int);
        printf("%d\n", value);
    }

    va_end(args);
}

int main(void)
{
    print_ints(3, 10, 20, 30);
    print_ints(1, 99);
    print_ints(0);
    return 0;
}
