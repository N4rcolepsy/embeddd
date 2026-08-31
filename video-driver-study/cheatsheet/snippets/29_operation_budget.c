/* S29 | 구조 예시
 * S28. 각 step의 timeout 계약이 전체 budget에 부합해야 함.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint32_t start = tick_ms();
uint32_t left = time_left(
    tick_ms(), start, operation_ms);
if (!left) return DRV_ETIMEOUT;
st = step_one(left);
if (st != DRV_OK) return st;
left = time_left(
    tick_ms(), start, operation_ms);
if (!left) return DRV_ETIMEOUT;
return step_two(left);
