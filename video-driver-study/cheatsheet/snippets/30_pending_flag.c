/* S30 | C17
 * 대상 compiler/ABI가 ISR atomic 사용을 허용해야 함. payload 없는 알림.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdatomic.h>
#include <stdbool.h>

_Static_assert(ATOMIC_BOOL_LOCK_FREE == 2,
    "Need always lock-free atomic bool");
static atomic_bool pending =
    ATOMIC_VAR_INIT(false);

void signal_from_isr(void)
{
    atomic_store_explicit(
        &pending, true, memory_order_relaxed);
}

bool take_pending(void)
{
    return atomic_exchange_explicit(
        &pending, false, memory_order_relaxed);
}
