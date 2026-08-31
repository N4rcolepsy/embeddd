# UART 핸들 템플릿과 C의 가변 인자

`UART_Init(uint8 *handle, ...)`처럼 시작하려는 의도라면 **핸들의 타입과 나머지 설정의 타입을 구체적으로 표현**하는 것이 좋다.

```c
HAL_StatusTypeDef UART_Init(UART_HandleTypeDef *handle,
                           const UART_Config *config);
```

이 자료의 템플릿은 STM32F4의 기존 HAL을 사용한다. `UART_Init()`는 여기서 만든 편의 함수이고, `HAL_UART_Init()`는 ST가 제공한 함수다. 두 이름을 구분한다.

## 1. 제공 파일과 범위

| 파일 | 역할 |
|---|---|
| [uart_template.h](examples/uart_template.h) | 설정 구조체와 공개 함수 선언 |
| [uart_template.c](examples/uart_template.c) | 설정 검증, HAL 초기화, 동기 송신 |
| [uart_usage.c](examples/uart_usage.c) | handle 생성 → 설정 전달 → 문자열 송신 예 |
| [variadic_demo.c](examples/variadic_demo.c) | UART와 독립적인 C17 가변 인자 실습 |

UART 템플릿은 **8N1, TX/RX, flow control 없음, oversampling 16, polling 송신**으로 범위를 고정했다. 수신·IRQ·DMA·RTOS mutex·재연결 정책까지 포함한 제품 드라이버는 아니다. C/C++ 컴파일러가 확인되지 않은 환경이므로 컴파일·실행·실물 보드 시험은 하지 않았다.

프로젝트의 STM32F4 HAL 및 CMSIS 헤더·소스·부품 선택 매크로가 필요하다. 이 파일만 호스트 C 컴파일러에 넣어 실행할 수 있는 예제가 아니다. 호스트 실습은 `variadic_demo.c`를 따로 사용한다.

## 2. `uint8_t *`와 handle 포인터

`uint8`은 표준 C의 기본 타입 이름이 아니다. 프로젝트에서 별도로 typedef했다면 사용할 수 있다. STM32처럼 해당 타입을 제공하는 환경에서는 `<stdint.h>`의 `uint8_t`를 사용할 수 있다.

```c
uint8_t *data;                 /* 8비트 데이터에 접근할 포인터 */
UART_HandleTypeDef *handle;    /* UART 관리 구조체에 접근할 포인터 */
```

버퍼와 handle을 둘 다 포인터로 넘기더라도, 가리키는 대상과 의미가 다르다. `uint8_t *`를 쓴다고 UART 정보가 자동으로 생기는 것은 아니다. 다른 객체를 바이트 포인터로 캐스팅하여 타입 정보를 지우는 방식은 이 API에서 필요 없다.

템플릿에서는 이미 HAL이 정의한 `UART_HandleTypeDef`를 재사용한다. 이 객체가 주변장치 instance, 설정, 전송 상태 등을 관리한다. 이름에 Handle이 붙었다고 동적 할당이 필요한 것은 아니다. [ST UART handle 정의](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Inc/stm32f4xx_hal_uart.h)

```c
static UART_HandleTypeDef uart = {0};

/* uart 자체가 아니라 uart의 주소를 전달 */
UART_Init(&uart, &config);
```

함수의 매개변수 `handle`에는 주소 값이 복사된다. `handle->Init.BaudRate`에 쓰면 호출자가 만든 `uart`의 실제 멤버가 바뀐다. C가 객체 전체를 참조 전달하는 별도 문법을 제공하는 것이 아니라 **포인터 값을 전달해서 원본 객체에 접근**하는 것이다.

`{0}`으로 시작하는 것은 이 템플릿의 사용 계약이다. 초기화되지 않은 지역 객체를 넘기지 않는다. 초기화 후 handle을 복사해서 같은 하드웨어를 독립적으로 관리하는 것도 피한다.

## 3. 설정 구조체를 넘기는 이유

```c
typedef struct {
    USART_TypeDef *instance;
    uint32_t baud_rate;
} UART_Config;

const UART_Config config = {
    .instance = USART2,
    .baud_rate = 115200U
};
```

설정 객체는 사용자가 원하는 값이고, handle은 동작 중인 UART의 관리 객체다. 둘을 구분하면 API의 의도가 명확하다.

| 표현 | 의미 |
|---|---|
| `&uart` | 초기화 결과를 기록할 handle의 주소 |
| `&config` | 읽어서 적용할 설정의 주소 |
| `const UART_Config *` | 함수가 이 포인터를 통해 설정을 수정하지 않겠다는 타입 계약 |
| `USART2` | 사용할 MCU의 레지스터 블록 |
| `.baud_rate = 115200U` | 해당 필드를 이름으로 지정하는 초기화 |

템플릿은 `config`의 주소를 저장하지 않고 필요한 필드 값을 handle에 옮긴다. 따라서 호출이 끝난 뒤 config 자체의 수명을 길게 유지할 필요는 없다. handle은 이후 송신에 계속 쓰므로 살아 있어야 한다.

향후 parity나 stop bits를 설정 가능하게 만들려면 config 필드·입력 검증·하드웨어 적용을 함께 추가한다. 설정 항목의 타입을 없애는 가변 인자가 필요한 상황은 아니다.

## 4. 실제 호출 예

```c
static UART_HandleTypeDef uart = {0};

const UART_Config config = {
    .instance = USART2,
    .baud_rate = 115200U
};

HAL_StatusTypeDef status = UART_Init(&uart, &config);
if (status != HAL_OK) {
    /* 초기화 실패 처리 */
}
```

송신은 초기화 성공 뒤에 수행한다. 전체 분기 흐름은 `uart_usage.c`를 참고한다.

```c
const uint8_t message[] = "Hello UART\r\n";

status = UART_Write(&uart,
                    message,
                    (uint16_t)(sizeof message - 1U),
                    100U);
```

문자열 배열 끝의 `\0`을 전송하지 않으려고 1을 뺐다. 이 송신은 polling 방식이므로 호출 동안 완료 또는 실패를 기다린다. 실패 전에 일부 바이트가 이미 전송되었을 수 있다. 자동으로 되돌리거나 재전송하지 않는다.

## 5. 보드에 붙일 때 필요한 작업

1. 프로젝트에서 `HAL_Init()`와 시스템 클록 설정을 수행한다.
2. `HAL_UART_MspInit()` 또는 등록한 MSP 초기화 경로에서 실제 보드의 USART 클록과 TX/RX alternate-function 핀을 준비한다.
3. 사용 중인 부품·클록에서 baud rate가 유효한지 확인한다. 매크로 범위 검사만으로 baud 오차와 전기 조건까지 확인되지는 않는다.
4. 한 peripheral instance에 한 HAL handle을 사용하고 외부에서 접근을 직렬화한다. 템플릿의 상태 검사는 mutex가 아니다.
5. 이 예제는 정상 task/main 문맥에서 사용한다. polling API를 ISR에서 실행하는 예제로 사용하지 않는다.

ST의 `HAL_UART_Init()`는 UART 설정을 적용하고 MSP 초기화 경로와 연결된다. 실제 핀 설정은 보드에 맞게 제공해야 한다. [ST HAL UART 구현](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_uart.c)

**CubeMX가 이미 `huart2`와 `MX_USART2_UART_Init()`를 만들었다면 같은 USART2용 handle을 하나 더 만들지 않는다.** 기존 초기화가 **8N1이고 TX가 활성화된 구성**이면 `UART_Write(&huart2, ...)`를 쓰거나, 프로젝트 초기화 구조를 명확히 정리한 뒤 이 템플릿을 사용한다. `UART_Write()`에서도 프레임 설정을 검사한다. HAL의 9비트·패리티 없음 모드는 데이터 원소를 16비트로 읽으므로 이 바이트 버퍼 템플릿의 대상에서 제외한다.

`UART_Init()`는 RESET 상태의 handle만 받도록 제한했다. 초기화된 객체를 다시 초기화하려 하면 `HAL_BUSY`를 반환한다. 재설정이 필요하면 활성 작업이 없는 것을 보장하고 `HAL_UART_DeInit()`의 성공을 확인한 뒤 적용한다. 오류 뒤 정리와 복구는 제품 요구에 맞춰 별도로 구현한다.

## 6. `(int a, ...)`는 함수 호출이 아니라 선언의 형태

```c
void f(int a, ...);  /* 선언 */

f(10);              /* 호출: 추가 인자 0개 */
f(10, 20);          /* 호출: 추가 인자 1개 */
f(10, 20, 30);      /* 호출: 추가 인자 2개 */
```

위 호출들은 인수 개수 관점의 문법 예다. 구현이 어떤 추가 인자를 요구하는지는 별도 계약이다.

- `int a`: 이름과 타입이 정해진 고정 매개변수.
- `,`: 매개변수 목록의 구분자. 여기서는 comma operator를 뜻하지 않는다.
- `...`: 그 뒤에 가변 개수의 추가 인자를 받을 수 있다는 ellipsis 문법.

호출할 때는 `f(10, ...)`처럼 쓰지 않고 실제 값을 전달한다. C의 기본 인자나 자동 옵션 처리 기능도 아니다. `a`가 자동으로 추가 인자 개수를 뜻하는 것도 아니다.

설명 문장에서 다른 매개변수를 생략하려고 `...`를 쓸 수는 있지만, **C 선언 안에 실제로 넣으면 언어 문법이 된다.** C17에서는 마지막에 두고 앞에 고정 매개변수가 있어야 한다. 이후 C23 규칙은 일부 완화되었으나, 여기서는 임베디드 프로젝트에서 사용할 C17 형태를 설명한다. [WG14 N1570 §6.7.6.3·§7.16](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)

## 7. 가변 인자는 어떻게 꺼내는가?

```c
#include <stdarg.h>
#include <stdio.h>

void PrintInts(int count, ...)
{
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; ++i) {
        int value = va_arg(args, int);
        printf("%d\n", value);
    }

    va_end(args);
}
```

호출 예:

```c
PrintInts(3, 10, 20, 30);
PrintInts(1, 99);
PrintInts(0);
```

이 함수에서는 개발자가 **첫 인자 count 뒤에 int를 count개 전달한다**는 규칙을 정했다. C가 개수를 자동으로 세어 전달한 것이 아니다. count는 0 이상이어야 한다.

| 도구 | 역할 |
|---|---|
| `va_list` | 가변 인자를 읽는 데 필요한 상태 |
| `va_start(args, count)` | C17에서 마지막 고정 매개변수를 기준으로 읽기 시작 |
| `va_arg(args, int)` | 다음 인자가 int라고 가정하여 읽고 다음 위치로 이동 |
| `va_end(args)` | 가변 인자 접근 종료 |

`va_list`를 단순한 배열이나 항상 스택을 가리키는 포인터라고 가정하지 않는다. ABI에 따라 인자가 레지스터와 스택 등에 배치되며 표준 매크로가 그 차이를 처리한다.

## 8. 개수와 타입은 별도 규칙으로 맞춰야 한다

```c
PrintInts(3, 10, 20);       /* 잘못됨: 세 번째 int가 없음 */
PrintInts(1, 3.14);         /* 잘못됨: double을 int로 꺼내려 함 */
```

`va_arg`는 자동 타입 판별이나 숫자 변환을 하지 않는다. 다음 인자의 승격된 타입과 맞게 읽어야 한다. 존재하지 않는 인자를 읽거나 허용되지 않는 타입으로 읽으면 정의되지 않은 동작이 된다. 컴파일러가 일반 가변 인자 함수의 이 계약을 자동으로 검증해 주지는 않는다. [SEI CERT EXP47-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp47-c/)

추가 인자에는 기본 승격이 적용된다.

| 전달한 값 | 추가 인자로 전달될 때 | 읽는 타입 |
|---|---|---|
| `float` | `double` | `va_arg(args, double)` |
| STM32의 `uint8_t`, `char`, `short` 등 작은 정수 | 정수 승격으로 보통 `int` | `va_arg(args, int)` |
| `int` | `int` | `va_arg(args, int)` |

일반 C의 정수 승격은 `int`가 모든 값을 표현할 수 있으면 `int`, 그렇지 않으면 `unsigned int`가 된다. 포인터도 원하는 타입으로 자동 판별되는 것이 아니다. 이러한 이유로 타입이 고정된 UART 초기화에는 가변 인자보다 구조체가 적합하다.

대표적인 가변 인자 함수는 `printf`다.

```c
printf("value=%d, temp=%.1f\n", 10, 25.0);
```

여기서는 format 문자열이 추가 인자의 타입과 사용 방식을 알려 주는 계약이다. 컴파일러가 printf 계열에 형식 검사를 제공할 수 있지만, 그것이 모든 가변 인자 함수의 타입을 런타임에 자동 추적한다는 뜻은 아니다.

관련 자료: [C/C++ 문법 가이드](04-c-cpp-core.md) · [HAL과 LL](05-hal-and-ll-explained.md)
