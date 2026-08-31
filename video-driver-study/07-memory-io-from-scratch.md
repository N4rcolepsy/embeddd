# 주소 + memory read/write에서 시작하는 드라이버 구현

> **현재 업무 기준의 우선 문서.** STMicroelectronics의 HAL·LL 인터페이스를 사용하지 않고, 제공된 메모리 read/write 명령과 레지스터 주소로 직접 구현한다. 기존 01의 영상 설명과 05·06의 STM32 코드는 비교 학습용으로 보존한다. 이 문서는 영상에서 직접 다룬 내용이 아니라 업무 조건에 맞춘 확장이다.

실제 칩 이름·레지스터 맵·read/write 함수 원형은 아직 제공되지 않았다. 따라서 특정 주소, baud 계산식, reset 순서를 실제 장치 값처럼 가정하지 않는다. 아래 UART는 **계약을 명시한 교육용 모델**이며 실제 부품에 맞게 알고리즘까지 포팅해야 한다.

복사할 전체 파일: [memory_io 템플릿](examples/memory_io/README.md). 빠른 참조: [치트시트](cheatsheet/embedded-driver-cheatsheet.md).

## 1. 무엇을 직접 만들어야 하는가

```text
응용 코드             RawUart_Init / Write / Flush, 정책·사용 시점
내 장치 드라이버       offset, field, 순서, timeout, 상태, FIFO/IRQ 규칙
내 메모리 접근 계층    read32(address), write32(address, value), sync
제공된 primitive      실제 memory read/write 명령 또는 CPU MMIO 접근
하드웨어              interconnect → UART/I2C/SPI/GPIO controller
```

`MemIo`는 ST 인터페이스가 아니라 이 자료에서 직접 정의한 작은 함수 집합이다. 플랫폼이 한 개뿐이면 함수 포인터 없이 `platform_read32()`를 직접 호출해도 된다. 함수 포인터는 필수 패턴이 아니다. **바뀌는 접근 방법을 레지스터 의미와 분리**하고 fake 장치로 순서를 확인하는 데 쓸 수 있다.

| 직접 작성할 책임 | 최소 내용 |
|---|---|
| 보드·SoC 준비 | 전원 도메인, clock gate/source, reset 해제, pinmux, 전압·pull 설정 |
| 접근 primitive 연결 | 주소 영역, 8/16/32/64비트 접근 폭, endian, 실패 반환, 시간 상한 |
| register access | base+offset, 허용 범위, RW/W1C/RC 등의 동작 의미 |
| 장치 초기화 | ID/version 확인 가능 여부, disable/reset/설정/enable 순서, ready 대기 |
| 데이터 경로 | FIFO 조건, 한 번에 전송 가능한 양, 상태·오류 판별 |
| 운영 계약 | handle 수명, 단일 소유자/lock, 재초기화·timeout·부분 실패 |

HAL이 없다는 이유로 handle이나 config 구조체를 없앨 필요는 없다. 예전 HAL이 관리하던 상태와 순서를 이제 **내 코드가 명시적으로 관리**한다.

## 2. 첫 단계: “이 주소가 어떤 주소인가?”

| 받은 정보 | 안전하게 접근하는 방법 | 금지할 추측 |
|---|---|---|
| `read32(address)` 같은 명령 API | 해당 API에 정의된 주소를 그대로 전달 | address를 바로 C 포인터로 캐스팅 |
| CPU에서 접근 가능한 MMIO 가상/고정 주소 | 플랫폼이 허용한 accessor 또는 volatile load/store | 접근 가능성·권한·매핑을 생략 |
| OS가 관리하는 물리 주소/BAR | OS가 제공하는 매핑·I/O accessor 사용 | 일반 앱에서 물리 주소 직접 역참조 |
| 디버그 probe·원격 장치의 주소 | probe/transport 명령으로 읽기·쓰기 | 호스트의 메모리와 같은 주소 공간이라고 가정 |

이 템플릿의 `MemAddr`는 `uint64_t`다. **명령 주소를 보관하는 정수**이지 포인터가 아니다. 실제 API의 주소 폭이 더 좁다면 변환 전에 범위를 검사한다. CPU 포인터로 왕복 변환하는 경우에는 그 구현에서 제공하는 `uintptr_t`와 매핑 규칙을 따른다. `uint32_t`가 모든 시스템의 주소 타입이라는 보장은 없다.

Linux에서는 I/O 메모리를 매핑하고 전용 accessor로 접근한다. 여기의 bare-metal 템플릿에 Linux API를 도입하라는 뜻이 아니라, 물리 주소와 C 포인터를 구분해야 하는 실제 사례다. [Linux Device I/O 문서](https://docs.kernel.org/driver-api/device-io.html)

## 3. 데이터시트에서 추출할 표

숫자를 코드에 넣기 전에 사용할 레지스터만 아래 표에 채운다. 사용하지 않는 레지스터까지 한 번에 구현할 필요는 없다.

| 항목 | 확인 질문 | 코드로 옮길 곳 |
|---|---|---|
| 주소 단위 | offset이 byte인가 word인가? base는 어느 address space인가? | `MemAddr`, 주소 합성 |
| 영역 길이 | base부터 몇 byte까지 허용되는가? | `span_bytes` |
| 접근 폭·정렬 | 8/16/32-bit 중 어떤 접근이 허용되는가? | read8/read32 선택, 정렬 검사 |
| 필드 | mask, position, 허용 encoding, reserved 정책은? | 상수·입력 검증·encode |
| 접근 의미 | RO, RW, WO, W1C, W0C, RC, FIFO인가? | 전용 read/write 함수 |
| 초기화 | clock/reset/unlock 순서, 각 단계의 대기는? | Init 상태 전이 |
| 동시성 | CPU·ISR·DMA·다른 코어·장치 자체가 바꾸는가? | lock, alias register, 소유권 |
| 완료 | 명령 ACK, bus 전달, 상태 변화 중 무엇을 기다리는가? | sync, ready/idle polling |
| 실패 | 읽기 실패 시 출력, 쓰기 timeout 시 실제 반영 여부는? | 반환값·FAULT·복구 정책 |

**예:** “DATA는 8비트 데이터”와 “DATA 레지스터를 8비트 bus transaction으로 접근해도 됨”은 별도 정보다. 유효 데이터가 8비트여도 32비트 접근만 허용할 수 있고, 반대 상황도 있다. 8비트 레지스터에 편의상 32비트 쓰기를 하면 인접 레지스터까지 건드릴 수 있다.

## 4. `UART_Init(uint8 *handle, ...)` 대신 타입을 직접 정의한다

표준 고정 폭 타입은 `<stdint.h>`의 `uint8_t`다. `uint8`는 플랫폼에서 별도로 typedef하지 않았다면 존재하지 않는다. handle은 바이트 버퍼가 아니라 **현재 장치의 주소·접근 방법·설정·상태를 묶은 객체**다.

```c
typedef struct {
    MemIo io;          /* read/write 함수 포인터 + context */
    RawUartConfig cfg; /* base, offset, mask, 적용할 설정 */
    RawUartState state;
} RawUart;

MemStatus RawUart_Init(RawUart *h, const MemIo *io,
                      const RawUartConfig *cfg, uint32_t timeout_ms);
MemStatus RawUart_Write(RawUart *h, const uint8_t *src, size_t n,
                       size_t *sent, uint32_t timeout_ms);
```

호출 모양은 다음과 같다. `board_io`와 `board_cfg`는 실제 명령 계약·레지스터 맵에서 작성할 객체이며, 이미 존재하는 라이브러리 객체가 아니다.

```c
RawUart uart = {0};
MemStatus st = RawUart_Init(&uart, &board_io, &board_cfg, 100U);
if (st == MEM_OK) {
    const uint8_t msg[] = {'O', 'K', '\r', '\n'};
    size_t accepted = 0U;
    st = RawUart_Write(&uart, msg, sizeof msg, &accepted, 100U);
    if (st == MEM_OK) {
        st = RawUart_Flush(&uart, 100U); /* line-idle까지 필요할 때 */
    }
    /* st와 accepted를 함께 처리한다. 실패하면 무조건 재송신하지 않는다. */
}
```

`&uart`는 객체의 주소다. 함수는 받은 포인터를 통해 원래 객체를 갱신한다. C는 포인터 자체도 **값으로 전달**한다. `h->state`는 `(*h).state`와 같다. `const RawUartConfig *`는 이 포인터를 통해 설정을 수정하지 않겠다는 뜻이다. init은 설정과 함수 포인터를 복사하지만, `io.ctx`가 가리키는 객체는 빌린다. context는 드라이버 사용 기간 내내 살아 있어야 한다.

`...`는 문법상 가변 인자 목록이다. `f(int a, ...)`는 a 뒤의 인자 개수·타입을 C가 자동 추적한다는 뜻이 아니다. `va_start/va_arg/va_end`와 별도의 count/format 계약이 필요하다. `float`는 `double`, 좁은 정수는 정수 승격 규칙에 따라 전달된다. 초기화 옵션은 가변 인자보다 **타입이 있는 config 구조체**로 표현한다. 자세한 문법·실습은 [06](06-uart-handle-and-variadic.md)의 가변 인자 절을 참고한다.

## 5. 이미 있는 메모리 명령을 연결하는 자리

```c
typedef struct {
    void *ctx;
    MemStatus (*read32)(void *, MemAddr, uint32_t *, uint32_t);
    MemStatus (*write32)(void *, MemAddr, uint32_t, uint32_t);
    MemStatus (*sync)(void *, uint32_t);
    uint32_t (*now_ms)(void *);
} MemIo;
```

| 멤버 | 인자와 반환 계약 |
|---|---|
| `read32(ctx, addr, out, budget_ms)` | 정확히 한 번의 허용된 32비트 읽기, 성공 시 CPU 순서 정수 |
| `write32(ctx, addr, value, budget_ms)` | 정확히 한 번의 32비트 쓰기, 성공은 접근 계층의 수락 |
| `sync(ctx, budget_ms)` | 선행 쓰기의 순서·전달을 플랫폼 계약에 따라 보장 |
| `now_ms(ctx)` | polling 중에도 진행하는 단조 증가 ms tick, uint32 wrap 허용 |

콜백은 동기 함수이며 버퍼를 보관하지 않는다. `budget_ms == 0`은 기다리지 않는 접근 계약이다. 상위 UART의 비어 있지 않은 작업에는 1~INT32_MAX ms를 준다. UART 작업은 전체 시간 예산을 공유하고 각 명령에는 남은 시간만 전달한다.

기존 명령이 `void write(addr, value)`라면 접근 실패를 반환할 수 없는 API다. wrapper가 `MEM_OK`를 반환하더라도 “명령을 발행했다” 이상의 오류 검출을 새로 만들어 낼 수는 없다. fault 처리·전송 완료 확인이 별도로 필요한지 확인한다. 반환값이 bool인지 음수 오류인지도 실제 정의를 보고 변환해야 한다.

이미 제공된 접근 함수가 쓰기 완료·순서를 충분히 보장한다면 `sync`가 아무 일도 하지 않고 성공을 반환해도 된다. **보장 여부를 모른다는 이유로 no-op으로 채우지는 않는다.** sync는 플랫폼의 접근 순서를 보장하는 기능이고, UART가 보낸 마지막 stop bit까지 완료되었는지는 STATUS로 따로 확인한다.

## 6. CPU에서 MMIO 포인터를 직접 쓰는 경우

아래는 **조건부 backend 조각**이다. `mapped_addr`가 CPU 접근 가능한 올바른 장치 매핑이고, 해당 주소에서 정렬된 32비트 load/store가 허용되며, toolchain·CPU·버스의 규칙을 확인한 경우에만 해당한다.

```c
#include <stdint.h>

static inline uint32_t cpu_load32(uintptr_t mapped_addr)
{
    return *(volatile const uint32_t *)mapped_addr;
}

static inline void cpu_store32(uintptr_t mapped_addr, uint32_t value)
{
    *(volatile uint32_t *)mapped_addr = value;
}
```

이 코드만으로 MMU/MPU 속성, endian, bus fault의 오류 반환, CPU ordering, cache 유지보수, posted write 완료는 해결되지 않는다. 특히 일반 메모리의 unaligned 접근이 되는 CPU라도 MMIO까지 허용되는 것은 아니다. **이 예제를 범용 `MemIo` backend로 자동 연결하지 않았다.**

명령 API를 쓰는 환경에서는 위 포인터 접근 코드를 건너뛰고 그 API를 연결한다. CPU MMIO 환경에서도 platform accessor가 있다면 그것이 제공하는 보장을 먼저 확인한다.

## 7. base + offset: 단위와 범위를 검사한다

```c
MemAddr address;
if (!MemAddr_At32(base, span_bytes, offset_bytes, &address)) {
    return MEM_EARG;
}
uint32_t value;
MemStatus st = Mem_Read32(&io, address, &value, remaining_ms);
```

`MemAddr_At32`는 base/offset의 4-byte 정렬, 전체 4-byte 접근의 영역 포함, 주소 덧셈 overflow를 검사한다. “주소가 범위 안”과 “그 주소를 실제로 읽어도 안전함”은 별개다. RO인지 RC인지, 해당 offset이 구현되었는지는 register policy가 판단한다.

```c
/* offset_bytes가 0x10이면 byte 주소에는 16을 더한다. */
MemAddr addr = base + offset_bytes;

/* 주의: uint32_t *p에서 p + 0x10은 64 byte 앞으로 이동한다. */
```

단순히 `#define REG (*(volatile uint32_t *)...)`를 반복하는 방식도 가능하지만, 접근 감사·fake 테스트·오류 전달·폭 제한이 필요하면 중앙 accessor가 유리하다.

## 8. 레지스터의 “쓰기 의미”별로 함수를 나눈다

| 종류 | 구현 패턴 | 복사 전 조건 |
|---|---|---|
| RO, 부작용 없는 상태 | 필요할 때 read | 최신 하드웨어 값을 읽는 접근이어야 함 |
| 일반 RW | 전체 값 write 또는 보호된 RMW | 모든 필드·reserved bit 정책 확인 |
| WO | 문서에서 요구하는 값 write | 읽어서 보존할 수 있다고 가정하지 않음 |
| 전용 W1C | 지울 비트에만 1을 써서 write | 나머지 비트에 0을 쓰는 것이 안전해야 함 |
| W0C | 지울 필드에 0, 다른 필드는 문서 규칙 | 단순 `~mask`는 reserved bit까지 1로 만들 수 있음 |
| RC/read-to-clear | 필요한 소유자가 한 번 읽고 snapshot 보관 | 로그·디버거·register dump도 소비자가 됨 |
| FIFO/data port | 같은 주소에 지정 폭으로 반복 접근 | 주소 자동 증가·memcpy 금지 여부 확인 |
| self-clearing command | 명령 write 후 지정 조건 대기 | 같은 값을 다시 쓰면 재시작할 수 있음 |
| RW/W1C 혼합 | 필드별 의미를 합친 장치 전용 함수 | 범용 update_bits에 넘기지 않음 |

### 일반 RW의 필드 교체

```c
uint32_t old_value, next;
MemStatus st = Mem_Read32(&io, config_addr, &old_value, read_budget);
if (st != MEM_OK) { return st; }
if (!Mem_FieldReplace32(old_value, field_mask, shifted_bits, &next)) {
    return MEM_EARG;
}
/* 전체 read-modify-write 동안 단일 소유자 또는 lock 유지.
 * 남은 시간 재계산. 이 register 전체가 안전한 RW일 때만 사용. */
st = Mem_Write32(&io, config_addr, next, write_budget);
```

필드 교체 식은 `(old & ~mask) | shifted_bits`다. `shifted_bits`가 mask 밖의 비트를 포함하지 않도록 검증한다. 필드 위치가 pos이면 보통 unsigned 값을 검증한 뒤 `value << pos`로 합성한다. pos는 타입 비트 폭보다 작아야 한다. RAM 안의 식 계산과 하드웨어 RMW 전체의 원자성은 별개다.

### 전용 W1C의 acknowledge

```c
/* 전제: 전체 register가 전용 W1C이고, mask 외 0 쓰기가 안전. */
st = Mem_Write32(&io, irq_ack_addr, handled_flags, remaining_ms);
```

`old | handled_flags`를 쓰면 이미 1이었던 다른 이벤트까지 clear할 수 있다. 하드웨어 SET/CLEAR alias가 있으면 소프트웨어 RMW 없이 지정 비트만 갱신할 수 있다. 단, alias의 의미와 접근 폭을 확인한다. lock은 다른 소프트웨어 접근을 막을 뿐 하드웨어가 스스로 갱신하는 필드를 멈추지 않는다.

## 9. UART 모델에서 초기화가 하는 일

전체 코드: [raw_uart.c](examples/memory_io/raw_uart.c).

```text
인자·map 검증 → handle에 설정 복사, FAULT로 시작
CTRL(disable) write → sync
BAUD(encoded value) write → sync
CTRL(enable) write → sync
모두 성공한 마지막에 READY
```

이는 **이 자료의 모델에 한정한 초기화 순서**다. 실제 UART에 DLAB, bank selection, fractional divider, FIFO reset, busy 해제 대기, unlock key 등이 있으면 추가해야 한다. 16550 계열처럼 같은 offset을 모드에 따라 공유하는 경우, 현재 모델의 “서로 다른 4개 register” 검증부터 바꿔야 한다.

`baud_value`는 원하는 baud rate 자체가 아니라 이미 데이터시트대로 encode한 값이다. `clock/(16*baud)`를 모든 UART에 적용하면 안 된다. 실제 입력 clock, prescaler, oversampling, 정수/분수 divider, rounding, 허용 오차를 해당 식으로 계산한다.

clock/reset/pinmux 설정은 board 단계에서 수행해야 하며 숨겨진 HAL 호출이 없다. 장치 ID register가 없는 UART에는 ID 읽기를 강제하지 않는다. 문서에 안전하다고 명시된 version/status/default register를 사용할 수 있다면 접근 확인에 활용한다.

## 10. TX: FIFO 수락과 전송 완료를 구분한다

```text
한 번의 Write 예산 시작
  STATUS 읽기 → TX_READY 대기 → TXDATA 한 byte write → sync
  다음 byte도 같은 예산에서 반복
Write OK = 요청한 byte들을 TX FIFO에 수락시킴
Flush: TX_IDLE 대기 = 모델에서 shift register까지 idle
```

STATUS는 반복 읽어도 부작용이 없고, TX_READY가 1이면 최소 한 byte를 넣을 수 있다는 모델 계약이다. polling, ISR, DMA가 동시에 같은 FIFO를 쓰면 status 확인 뒤 조건이 달라질 수 있다. 이 버전은 **단일 소유자가 모든 작업을 직렬화**한다.

다른 UART에서는 TX-empty가 FIFO만 가리킬 수 있다. 마지막 stop bit까지 완료되었는지는 shift register·line idle 조건을 확인해야 한다. TX_IDLE이라는 이름을 임의로 붙이는 것만으로 보장이 생기지 않는다.

`RawUart_Write`는 src를 반환 후 보관하지 않는 동기 함수다. 데이터 레지스터가 32비트이고 [7:0]만 TX byte이며 상위 비트 0 쓰기가 안전하다는 가정이다. RX·IRQ·DMA는 구현하지 않았으며 해당 기능을 맡을 때 별도 경로를 추가한다.

## 11. timeout과 부분 실패

`while (!ready) {}`는 단독으로 쓰지 않는다. 단조 증가 tick에서 `elapsed = (uint32_t)(now - start)`를 계산하고 전체 작업의 remaining budget을 사용한다. 개별 read마다 원래 timeout을 새로 주면 총 대기 시간이 불어나므로, 같은 start를 공유한다.

이 모델은 timeout을 1~INT32_MAX ms로 제한하고 한 번의 전체 tick 주기를 넘어 멈추지 않는 것을 전제로 한다. 각 callback도 받은 예산 내에 반환해야 한다. CPU bus access 자체가 영원히 멈추거나 now_ms가 중단되면 C polling 코드가 그 상황을 해결할 수 없다. IRQ를 막은 문맥에서 ISR 기반 tick을 기다리지 않는다.

| 상황 | 반환값/상태 | 호출자가 할 일 |
|---|---|---|
| 잘못된 인자·map | EARG, 하드웨어 접근 전 거절 | 설정 수정 |
| READY 객체에 Init | ESTATE | 사용 중 장치의 무단 재설정 방지 |
| Init 중 실패 | FAULT | 일부 설정이 적용됐을 수 있으므로 정해진 recovery 수행 |
| TX/Flush timeout·I/O 실패 | FAULT | 무조건 retry하지 말고 상태·출력·복구 정책 확인 |
| 성공한 write 뒤 sync 실패 | sent에는 그 byte 포함 | 이미 수락한 쓰기의 완료가 불확실함 |
| write 자체가 실패 | sent에는 해당 byte 미포함 | 그래도 실제 장치에 도달했을 수 있음 |

`sent`는 성공 반환을 확인한 쓰기의 개수다. 실패한 접근이 실제 하드웨어에 도달했을 수 있으므로 정확한 재전송 시작 위치가 아니다. 메시지 전체의 중복을 방지해야 한다면 상위 프로토콜의 sequence/ACK 같은 정책이 필요하다. timeout이 발생했다고 하드웨어 작업이 자동 취소되는 것도 아니다.

## 12. volatile, barrier, cache, completion을 분리한다

| 수단 | 해결하려는 문제 | 대신 해결하지 않는 것 |
|---|---|---|
| volatile MMIO 접근 | 구현이 정의한 관찰 가능한 장치 접근 | thread lock, 일반 RAM 원자성, 모든 CPU 재정렬 |
| compiler barrier | 컴파일러의 특정 메모리 재배치 제한 | bus 전달·DMA cache coherence |
| CPU memory barrier | 아키텍처가 정한 접근 순서/완료 | UART 자체의 기능 완료 판정 |
| 올바른 MMU/MPU 속성 | 장치 영역의 cache·speculation 등 접근 속성 | register별 쓰기 의미 |
| DMA cache clean/invalidate | CPU cache와 DMA가 보는 RAM 일관성 | DMA 시작/종료 상태 관리 |
| safe readback / platform sync | 문서화된 쓰기 전달·순서 보장 | 임의 register의 안전한 읽기 보장 |

Arm의 DMB는 메모리 접근의 순서, DSB는 더 강한 완료 조건, ISB는 명령 실행 문맥의 동기화와 관련된다. 이것들은 모든 곳에 같은 순서로 붙이는 마법의 고정 코드가 아니다. CMSIS는 해당 명령의 wrapper를 설명하는 공식 참고 자료이며, **이 템플릿은 CMSIS에 의존하지 않는다**. 대상 ISA와 platform의 계약에 맞는 구현이 필요하다. [Arm CMSIS CPU instruction reference](https://arm-software.github.io/CMSIS_6/main/Core/group__intrinsic__CPU__gr.html)

일부 bus의 posted write는 명령 반환 뒤 장치에 도달한다. 해당 platform이 요구할 때 안전한 register readback 등으로 동기화한다. RC/FIFO/전원 차단 중 register를 임의로 읽어서 flush하지 않는다. 일반 `memcpy`로 I/O register 배열을 복사하는 것 역시 접근 폭·순서 계약을 보장하지 못한다. [Linux Device I/O의 접근·posted write 설명](https://docs.kernel.org/driver-api/device-io.html)

## 13. 구조체·union을 레지스터에 덮어씌울 때

```c
typedef struct {
    volatile uint32_t STATUS;
    volatile uint32_t TXDATA;
    volatile uint32_t CTRL;
    volatile uint32_t BAUD;
} ExampleRegs; /* 오직 이 교육용 4-register 배치 */

_Static_assert(offsetof(ExampleRegs, CTRL) == 8U, "CTRL offset");
```

`offsetof`는 `<stddef.h>`에 있다. 이런 overlay는 CPU MMIO 매핑이 유효하고 실제 offset·폭·padding을 검증할 수 있을 때 선택할 수 있다. 현재 명령 기반 템플릿은 overlay가 없어도 동작하도록 **명시적 offset**을 사용한다.

- C bit-field의 배치·할당 순서는 구현에 영향을 받으므로 외부 register layout을 그대로 표현하는 기본 수단으로 삼지 않는다. mask/shift를 쓰면 어떤 비트를 건드리는지 명확하다.
- `packed`를 붙이면 장치가 요구하는 정렬·접근 폭이 보장되는 것이 아니다. unaligned 접근을 유발할 수 있다.
- `union { uint32_t raw; Bits bits; }`는 하드웨어 RMW를 원자적으로 만들어 주지 않는다. C/C++의 union 멤버 해석 규칙도 같지 않다.
- `memset(regs, 0, sizeof *regs)`로 장치 초기화를 대체하지 않는다. RO, W1C, command, reserved까지 건드릴 수 있다.
- `const`는 C 코드의 수정 경로를 제한한다. 하드웨어가 값 자체를 바꾸지 않는다는 뜻은 아니다.

## 14. C/C++ 문법을 실제 템플릿에 연결하기

| 문법 | 이 코드에서의 역할 | 흔한 혼동 |
|---|---|---|
| `typedef struct` | 장치/config/접근 함수 집합 정의 | typedef가 객체를 생성하는 것은 아님 |
| `uint32_t`, `uint64_t` | 값·주소 폭을 명시 | 주소 폭과 data width는 서로 다름 |
| `size_t` | 버퍼 길이·전송 개수 | 바이트 수인지 원소 수인지 계약 필요 |
| `&`, `*`, `->` | handle 주소·역참조·멤버 접근 | C에는 자동 참조 전달이 없음 |
| 함수 포인터 | 실제 메모리 명령을 주입 | 포인터 선언에 `(*name)` 괄호 필요 |
| `void *ctx` | probe 세션·backend 객체를 전달 | 임의 타입을 안전하게 판별해 주지 않음 |
| `enum` | OK/error 및 OFF/READY/FAULT 표현 | enum 크기가 hardware field 폭과 같다는 보장 없음 |
| designated initializer | `.base`, `.read32`처럼 의미를 드러냄 | C++17에 C 초기화 문법을 그대로 복사하지 않음 |
| `static` 함수 | 해당 .c 내부 helper | 지역 static은 호출마다 새 객체가 아님 |
| `static inline` | 작은 helper를 header에 정의 | 반드시 inline 기계어로 확장된다는 보장 없음 |
| `extern`, include guard | 선언/정의 분리, 중복 포함 방지 | header의 일반 전역 정의는 중복 symbol 위험 |
| `const`, `volatile`, `_Atomic` | 수정 경로·MMIO·일반 RAM 동시성의 다른 목적 | 서로 대체할 수 없음 |
| unsigned mask/shift | 명시적인 비트 encode/decode | signed shift/승격·shift 폭 확인 |
| `sizeof` | 실제 배열 byte 수 | 함수 인자의 배열은 포인터로 조정됨 |
| `...`와 stdarg | count/format 계약이 있는 가변 인자 | config 옵션이나 타입 안전성을 자동 제공하지 않음 |
| C++ `extern "C"` | C 함수와의 linkage 연결 | C 코드를 C++ 문법으로 바꿔 주지 않음 |
| C++ RAII | lock·자원 반환을 scope에 연결 | MMIO 완료·DMA 버퍼 수명은 별도 계약 |

상세 설명과 문법별 복사용 예시는 [04 C/C++ 핵심](04-c-cpp-core.md), 치트시트의 C/C++ 페이지를 참고한다. C의 `_Atomic`을 장치 register에 그대로 덧씌우지 않는다. atomic 명령의 read-modify-write가 그 장치에서 허용되는지 별도 문제다.

## 15. 다른 주변장치에도 적용되는 패턴

| 대상 | 같은 설계 골격 | 장치별로 바꿀 부분 |
|---|---|---|
| GPIO | base+offset, config, read/write, 소유권 | pinmux, direction, set/clear alias |
| Timer/PWM | clock 확인, encode, 적용, ready/enable | prescaler/period, shadow update, latch 시점 |
| UART | 상태 polling, FIFO, timeout, 오류·진행량 | baud, FIFO 폭, idle, RX error clear |
| SPI controller | control/TX/RX/status를 직접 접근 | CS, clock mode, TX/RX 동시 진행, FIFO drain |
| I2C controller | command/status/IRQ를 직접 접근 | START/STOP, address, NACK, arbitration, bus recovery |
| DMA controller | descriptor 준비, 시작, 완료, 복구 | 주소 제약, alignment, cache·barrier·버퍼 수명 |

**I2C/SPI 외부 센서의 register 주소와 CPU의 controller register 주소는 다르다.** 센서에 쓰려면 controller 레지스터를 조작해 버스 transaction을 먼저 만들어야 한다. 영상의 `HAL_I2C_Mem_Read`가 하던 부분을 직접 구현한다면 START→주소→register 선택→데이터→STOP 및 오류 처리를 맡아야 한다. 센서 offset을 CPU base에 더하는 것으로 대체되지 않는다.

도출할 공통 패턴은 “모든 장치에 같은 Init 코드”가 아니다. **접근 계약 → register 의미 → 초기화 상태 전이 → 데이터 경로 → 완료/실패 계약**을 분리하고, 장치마다 데이터시트로 구체화하는 방식이다.

## 16. 온보딩 첫날 체크리스트

- [ ] 실제 명령의 함수 원형·주소 공간·폭·endian·시간 상한을 확인한다.
- [ ] 사용 가능한 register에서 한 번 읽고 실제 transaction 주소를 확인한다.
- [ ] write할 register의 부작용과 reserved bit 정책을 확인한다.
- [ ] board clock/reset/pinmux와 장치 init을 구분해서 구현한다.
- [ ] UART라면 한 byte를 내보내고 logic analyzer/상대 수신기로 확인한다.
- [ ] timeout·부분 실패 때 출력과 handle 상태가 어떻게 되는지 문서화한다.
- [ ] polling부터 성공시키고 필요한 경우 RX→IRQ→DMA 순서로 확장한다.
- [ ] C 테스트와 실제 board 시험을 구분해서 기록한다.

현재 제공 범위: 메모리 접근·UART 모델 소스, fake C 테스트, 문서·치트시트. 실제 address/mask/divider 입력, 플랫폼 sync 구현, board bring-up, RX/IRQ/DMA, 실물 검증은 제공 정보와 장치가 있어야 수행할 수 있다.
