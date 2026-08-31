# 드라이버 코드를 읽고 작성하기 위한 C/C++ 핵심 문법

> **문서의 성격:** 영상에서 배운 드라이버 개발을 실제 코드에 적용하기 위한 **추가 학습 자료**다. 이 문서의 모든 문법과 예제가 영상에 등장한다는 뜻은 아니다. 예제의 `DemoSensor`, 레지스터 주소, 배율은 설명용이며 특정 칩의 사양이 아니다.
>
> **기준:** C17과 C++17. C 코드는 C17로, C++ 코드는 C++17로 구분하여 읽는다. STM32처럼 8비트 바이트와 32비트 `int`를 사용하는 환경을 주된 예로 삼되, 그 환경에 의존하는 부분은 따로 설명한다. 실제 펌웨어는 대상 컴파일러·ABI·HAL·데이터시트 요구사항을 추가로 확인해야 한다.
>
> **검증 범위:** 선언, 타입, 데이터 흐름과 수명 조건을 검토한 교육용 코드다. 본 작업 환경에는 C/C++ 컴파일러가 없어 아래 코드의 실제 컴파일·링크·타깃 실행을 검증했다고 주장하지 않는다. `platform_*`, `board_*` 등은 설명용 인터페이스이며 보드에 맞는 구현이 필요하다.

각 코드 블록은 해당 문법을 설명하는 조각이다. 같은 타입 이름을 반복해서 사용하는 블록도 있으므로 문서 전체를 한 파일에 그대로 이어 붙이는 용도가 아니다. C++ 예제의 `std::uint8_t`, `std::size_t`, `std::array`, `std::move`에는 각각 `<cstdint>`, `<cstddef>`, `<array>`, `<utility>` 등 해당 헤더가 필요하다.

## 1. 처음에는 무엇부터 알아야 하나

처음부터 C 표준 전체를 공부할 필요는 없다. 우선 `sensor_read(&sensor, &sample)` 한 줄이 어떤 객체를 읽고 쓰는지 설명할 수 있으면 드라이버 코드를 읽는 출발점에 도달한 것이다.

| 학습 순서 | 먼저 읽을 절 | 목표 | 처음에는 뒤로 미뤄도 되는 것 |
|---|---|---|---|
| 첫날 1 | 2–4절 | 구조체 하나에 장치 상태를 모으고 포인터로 전달하는 코드 읽기 | 불투명 타입의 메모리 할당 전략 |
| 첫날 2 | 5–7절 | 함수 포인터로 I2C 읽기 구현을 연결하고 버퍼를 주고받기 | 복잡한 함수 포인터 선언 |
| 첫날 3 | 8–10절 | 레지스터 비트와 수신 바이트를 안전하게 해석하기 | 일반화된 비트 필드 템플릿 |
| 첫 드라이버 작성 중 | 11–13절 | 헤더/구현 분리, 오류 처리, 초기화 상태 정리 | 공개 ABI 안정성 |
| 인터럽트·DMA를 붙일 때 | 14절 | 함수가 반환된 뒤에도 살아 있어야 할 데이터 찾기 | 세부 메모리 모델과 다중 코어 |
| C++ 프로젝트에 들어갈 때 | 15–18절 | 참조, 클래스, RAII, 콜백, 템플릿 이해하기 | 고급 메타프로그래밍 |

처음 읽을 때의 질문은 세 가지다.

1. **이 변수는 어떤 데이터를 표현하는가?** 장치 주소인가, 레지스터 값인가, 측정값인가, HAL 핸들인가?
2. **이 함수는 어느 객체를 바꾸는가?** 포인터가 가리키는 객체인가, 함수 안의 복사본인가?
3. **그 객체는 언제까지 존재해야 하는가?** 함수 반환까지인가, 드라이버를 사용하는 동안인가, DMA 완료까지인가?

이 세 질문에 답할 수 있게 만든 다음 범위 검사, 동시 접근, 오류 복구를 덧붙이는 순서가 이해하기 쉽다.

## 2. 작은 드라이버 하나를 먼저 읽어 보기

아래는 장치별 상태와 버스 함수를 분리한 C17 예제다. 실제 하드웨어 코드에 앞서 이 코드의 자료형과 호출 흐름을 읽는 것이 목표다.

### 2.1 드라이버에 등장하는 자료형

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    DRV_OK = 0,
    DRV_EINVAL,
    DRV_EIO,
    DRV_ETIMEOUT,
    DRV_ESTATE
} DrvStatus;

/* 동기식 읽기: 반환되면 dst에 대한 모든 접근이 끝난다. */
typedef DrvStatus (*RegReadFn)(
    void *context,
    uint8_t address7,
    uint8_t reg,
    uint8_t *dst,
    size_t length);

typedef struct {
    RegReadFn read;
} BusOps;

typedef struct {
    const BusOps *ops;  /* 호출할 버스 함수들의 표 */
    void *context;     /* 그 함수들이 사용할 플랫폼 객체 */
} RegisterBus;

typedef struct {
    RegisterBus bus;
    uint8_t address7;
    float units_per_lsb;
    bool ready;
} DemoSensor;

typedef struct {
    int32_t raw;
    float value;
} DemoSample;
```

이 자료형을 읽을 때는 다음 순서가 좋다.

- `DemoSensor`는 **장치 하나의 소프트웨어 상태**다. 버스 연결, 장치 주소, 변환 계수, 사용 가능 상태를 갖는다.
- `DemoSample`은 **한 번 읽은 측정 결과**다. 장치 상태와 측정 결과를 다른 타입으로 분리했다.
- `RegisterBus`는 **어떻게 통신할지에 대한 연결 정보**다. 함수 표와 그 함수가 사용할 객체를 함께 보관한다.
- `BusOps`는 동작 목록이다. 지금은 `read` 하나지만 필요에 따라 `write`, `delay_ms` 등을 추가할 수 있다.
- `RegReadFn`은 함수 포인터 타입이다. 특정 HAL 이름을 드라이버 본문에 직접 넣지 않아도 읽기를 수행하게 해 준다.
- `DrvStatus`는 함수가 성공했는지 알려 준다. 측정값과 성공 여부를 서로 다른 경로로 반환한다.

`address7`처럼 이름에 단위를 넣으면 7비트 I2C 주소와 HAL에 넘기는 주소 표현을 혼동하기 어렵다. 어떤 HAL이 주소를 이동한 값을 요구한다면 변환은 플랫폼 어댑터에서 한다. 여기서는 버스 인터페이스의 계약을 **7비트 주소**로 고정했다.

### 2.2 읽기 함수

```c
enum { DEMO_REG_DATA_L = 0x20 };  /* 가상의 레지스터 */

static int32_t decode_s16_le(const uint8_t bytes[2])
{
    uint32_t u = (uint32_t)bytes[0]
               | ((uint32_t)bytes[1] << 8);

    /* 장치가 16비트 2의 보수 형식을 보낸다고 가정한다. */
    return (u < UINT32_C(0x8000))
        ? (int32_t)u
        : (int32_t)u - INT32_C(65536);
}

DrvStatus demo_sensor_read(DemoSensor *sensor, DemoSample *out)
{
    if (sensor == NULL || out == NULL) {
        return DRV_EINVAL;
    }

    if (!sensor->ready || sensor->bus.ops == NULL
        || sensor->bus.ops->read == NULL) {
        return DRV_ESTATE;
    }

    uint8_t bytes[2];
    DrvStatus status = sensor->bus.ops->read(
        sensor->bus.context,
        sensor->address7,
        DEMO_REG_DATA_L,
        bytes,
        sizeof bytes);

    if (status != DRV_OK) {
        return status;
    }

    DemoSample next;
    next.raw = decode_s16_le(bytes);
    next.value = (float)next.raw * sensor->units_per_lsb;

    *out = next;  /* 모든 단계가 성공한 뒤 결과를 한 번에 전달한다. */
    return DRV_OK;
}
```

이 예제에서 통신 함수의 추가 계약은 다음과 같다. `DRV_OK`이면 요청한 바이트를 전부 채운다. 실패하면 `bytes`가 부분적으로 바뀌었을 수 있지만 `demo_sensor_read()`는 이를 사용하지 않는다. 출력 `out`은 `sensor`나 버스 내부 객체와 겹치지 않는 유효한 저장 공간이어야 한다.

### 2.3 호출부를 한국어로 읽기

```c
DemoSample sample;
DrvStatus status = demo_sensor_read(&sensor, &sample);

if (status == DRV_OK) {
    consume_measurement(sample.value);
}
```

`sensor`는 이미 올바르게 초기화되어 있고 `consume_measurement()`는 애플리케이션에 있다고 가정한 코드 조각이다.

`demo_sensor_read(&sensor, &sample)`을 풀어 쓰면 다음과 같다.

> “이 장치 객체의 주소를 전달한다. 함수는 그 주소를 통해 버스와 장치 주소를 확인한다. 결과를 저장할 객체의 주소도 전달한다. 함수는 성공하면 그 객체에 측정값을 써 준다.”

핸들을 넘기는 일은 결국 **어느 객체를 사용할지 알려 주는 일**이다. 특별한 복제나 숨겨진 장치 이동이 일어나는 것은 아니다.

## 3. 포인터, 주소, 역참조와 handle 전달

### 3.1 `&`, `*`, `.`, `->`의 네 가지 역할

```c
DemoSensor sensor;          /* 실제 구조체 객체 */
DemoSensor *p = &sensor;    /* sensor의 주소를 보관하는 포인터 객체 */

sensor.address7 = 0x48;
(*p).address7 = 0x48;
p->address7 = 0x48;
```

위의 마지막 세 줄은 같은 멤버를 지정한다. 나머지 멤버까지 초기화되었다는 뜻은 아니다.

| 표현 | 뜻 |
|---|---|
| `DemoSensor sensor` | `DemoSensor` 객체를 만든다 |
| `DemoSensor *p` | `DemoSensor`를 가리키는 포인터 객체를 만든다 |
| `&sensor` | `sensor`의 주소를 얻는다 |
| `*p` | `p`가 가리키는 객체를 지정한다 |
| `sensor.address7` | 구조체 객체의 멤버에 접근한다 |
| `p->address7` | 포인터가 가리키는 구조체의 멤버에 접근한다 |
| `&p` | 포인터 객체 `p` 자체의 주소다. 타입은 `DemoSensor **`다 |

선언의 `*`와 식의 `*`를 구분한다. `DemoSensor *p`의 `*`는 포인터 타입을 선언한다. `*p = other`의 `*`는 가리키는 객체에 접근한다.

### 3.2 C는 포인터도 값으로 전달한다

```c
void set_address(DemoSensor *s)
{
    s->address7 = 0x49;  /* 호출자가 가진 원래 객체를 변경 */
    s = NULL;           /* 이 함수 안의 포인터 복사본만 변경 */
}
```

호출자가 `set_address(&sensor)`를 실행하면 주소 값이 매개변수 `s`로 복사된다. `sensor` 전체가 복사되는 것이 아니다.

```text
호출자                          함수 내부

sensor ─────── 주소 A ◀──────── s = 주소 A
  address7: 0x48                 s->address7 = 0x49

원래 객체의 address7이 변경됨     s = NULL은 이 지역 포인터만 변경
```

반대로 다음 함수는 구조체를 값으로 받는다.

```c
void change_copy(DemoSensor s)
{
    s.address7 = 0x49;
}
```

이 함수의 `s.address7` 변경은 호출자의 구조체에 반영되지 않는다. 다만 구조체 안에 들어 있는 포인터의 값도 복사되므로, 복사된 포인터가 가리키는 외부 객체는 여전히 공유될 수 있다. **구조체 복사와 깊은 복사는 다르다.**

### 3.3 핸들은 대개 “상태 객체를 찾아가는 수단”이다

임베디드 코드에서 `handle`이라는 이름은 언어 문법이 아니다. 라이브러리의 설계 관례다. 다음 모두 핸들이라고 부를 수 있다.

- HAL 상태 구조체 자체: `I2C_HandleTypeDef hi2c1;`
- 그 구조체의 포인터: `I2C_HandleTypeDef *i2c;`
- 비공개 장치 구조체의 포인터: `Sensor *sensor;`
- 드라이버 내부 배열을 가리키는 정수 인덱스 또는 토큰.

따라서 `handle`이라는 단어만 보고 타입을 추측하지 말고 `typedef` 정의까지 확인해야 한다.

```c
/* 보드가 객체를 보유하고 드라이버는 포인터를 빌리는 개념 예제 */
typedef struct {
    I2C_HandleTypeDef *i2c;
    uint8_t address7;
} BoardSensorContext;
```

이 구조체의 `i2c` 포인터를 복사해도 I2C 주변장치나 HAL 상태 객체가 새로 생성되지 않는다. 두 객체가 같은 HAL 객체를 가리킬 수 있다. 이 경우 버스 접근 직렬화가 별도로 필요할 수 있다.

### 3.4 포인터를 받는 함수에서 확인할 것

| 질문 | API 문서에 적을 내용 |
|---|---|
| `NULL`을 허용하는가? | 선택 입력인지 필수 입력인지 |
| 읽기만 하는가? | `const T *`로 표현할 수 있는지 |
| 수정하는가? | 변경하는 멤버와 실패 시 상태 |
| 저장해 두는가? | 호출 반환 후에도 객체가 살아 있어야 하는지 |
| 소유권을 받는가? | 누가 해제·종료해야 하는지 |
| 같은 객체를 다른 곳에서도 쓰는가? | 동시 호출 가능 여부와 잠금 책임 |

`NULL` 검사만으로 유효한 포인터임을 증명할 수는 없다. 이미 수명이 끝난 객체의 주소, 잘못된 캐스팅, 짧은 버퍼는 모두 `NULL`이 아닐 수 있다. 자동 저장 기간을 가진 지역 객체를 밖으로 넘길 때는 특히 수명 조건을 확인한다. [SEI CERT DCL30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/declarations-and-initialization-dcl/dcl30-c/)

### 3.5 `T **`가 필요한 경우

```c
DrvStatus sensor_open(Sensor **out_sensor);
```

이 형식은 호출자가 가진 **포인터 변수 자체**를 함수가 바꿔야 할 때 쓴다. 예를 들어 내부 객체 풀에서 빈 항목을 찾아 그 주소를 돌려줄 수 있다. 반드시 동적 할당을 의미하지는 않는다.

```c
Sensor *sensor_ptr = NULL;
DrvStatus status = sensor_open(&sensor_ptr);
```

`&sensor_ptr`는 포인터 변수의 주소다. 함수 내부의 `*out_sensor = chosen;`이 호출자의 `sensor_ptr`를 바꾼다.

단순히 센서의 설정을 바꾸려는 함수라면 `Sensor *`면 충분하다. “원본을 바꾼다”는 이유만으로 언제나 이중 포인터가 필요한 것은 아니다.

## 4. `struct`, `typedef`, 초기화와 장치 객체

### 4.1 구조체는 관련된 상태를 한 단위로 묶는다

```c
struct SensorConfig {
    uint8_t address7;
    uint16_t sample_rate_hz;
};

typedef struct SensorConfig SensorConfig;
```

C에서는 `struct SensorConfig`가 원래 타입 이름이고 `SensorConfig`는 `typedef`로 만든 별칭이다. 흔히 아래처럼 한 번에 쓴다.

```c
typedef struct {
    uint8_t address7;
    uint16_t sample_rate_hz;
} SensorConfig;
```

자기 자신을 가리키거나 앞선 선언이 필요하면 태그를 주는 방식이 편하다.

```c
typedef struct Request Request;

struct Request {
    Request *next;
    uint8_t *buffer;
    size_t length;
};
```

`Request *next`는 다음 요청 객체의 주소만 저장하므로 현재 구조체의 크기가 아직 완성되지 않아도 선언할 수 있다. `Request next;`를 직접 넣으면 자기 자신을 끝없이 포함하는 모양이 되어 사용할 수 없다.

### 4.2 구성값과 실행 상태를 구분한다

```c
typedef struct {
    uint8_t address7;
    uint16_t sample_rate_hz;
} SensorConfig;

typedef struct {
    SensorConfig config;
    RegisterBus bus;
    uint32_t communication_errors;
    bool initialized;
} SensorState;
```

`config`는 “어떤 설정으로 쓰려는가”이고 `communication_errors`나 `initialized`는 “현재 어떤 상태인가”다. 이 구분은 초기화·재설정·진단 로직을 읽기 쉽게 한다.

데이터시트의 레지스터 하나를 무조건 구조체 멤버 하나에 대응시킬 필요는 없다. 소프트웨어의 구성값은 `sample_rate_hz`처럼 의미 있는 단위로 두고, 레지스터 코드로의 변환은 함수에서 할 수 있다.

### 4.3 C17 지정 초기화

```c
SensorConfig cfg = {
    .address7 = 0x48,
    .sample_rate_hz = 100
};
```

`.address7 = ...`는 C의 지정 초기화다. 멤버 이름을 명시하므로 값의 의미가 보이고 멤버 순서 변경에도 읽기 쉽다. **표준 C++17에서는 이 문법을 사용할 수 없다.** C++17에서는 집합 초기화 또는 생성자를 사용한다.

```cpp
SensorConfig cfg{0x48, 100};  // C++17: 멤버 순서에 따라 초기화
```

구조체를 값으로 초기화하면 생략한 멤버도 각 타입에 맞게 초기화된다.

```c
SensorState state = {0};  /* C: 나머지 멤버까지 영 값으로 초기화 */
```

```cpp
SensorState state{};     // C++: 값 초기화
```

이것은 하드웨어 초기화를 수행했다는 뜻이 아니다. 포인터가 널이고 상태가 0일 뿐이며, 실제 통신 설정과 장치 설정은 별도 단계다.

`memset(&state, 0, sizeof state)`는 모든 **바이트**를 0으로 만든다. 모든 C 구현에서 전부 0인 비트 패턴이 널 포인터나 부동소수점 0을 표현한다고 일반화할 수 없으므로, 타입이 섞인 객체의 기본 초기화에는 언어의 초기화 문법을 우선 사용한다. C++ 비단순 객체에는 생성자·소멸자와 객체 불변 조건도 있으므로 통째로 `memset`하지 않는다.

### 4.4 구조체 대입은 유용하지만 소유권을 복사하지는 않는다

```c
SensorState second = first;
```

숫자 멤버는 값이 복사되고 포인터 멤버는 주소 값이 복사된다. `first.bus.context`와 `second.bus.context`는 같은 객체를 가리킨다. 내부에 잠금, DMA 진행 상태, 콜백 등록 등이 있으면 구조체 통째 복사가 유효한 사용법인지 따로 결정해야 한다.

반면 `DemoSample`처럼 외부 자원과 무관한 작은 값 객체는 대입과 반환이 잘 맞는다. “구조체니까 반드시 포인터로 전달해야 한다”는 규칙은 없다. 크기, 목적, 수정 여부를 보고 선택한다.

### 4.5 불투명 구조체로 구현을 숨기기

공개 헤더에는 구조체의 이름만 공개할 수 있다.

```c
/* sensor.h */
typedef struct Sensor Sensor;

DrvStatus sensor_read(Sensor *sensor, DemoSample *out);
```

```c
/* sensor.c */
struct Sensor {
    RegisterBus bus;
    uint8_t address7;
    bool initialized;
};
```

사용자는 `Sensor *`를 전달할 수 있지만 멤버에 직접 접근하거나 `sizeof(Sensor)`를 계산할 수 없다. 내부 레이아웃을 바꿔도 공개 인터페이스를 유지하기 쉬워진다.

다만 사용자가 크기를 모르므로 `Sensor sensor;`를 직접 선언할 수 없다. 저장 공간을 제공하는 방식을 반드시 함께 설계한다.

- 내부의 고정 객체 풀에서 빌려 주기.
- 애플리케이션 전용 내부 헤더에서 구조체 정의를 제공하기.
- 초기화 시 호출자가 적절한 크기·정렬의 메모리를 제공하기.
- 시스템 정책이 허용하면 동적 할당하기.

처음 드라이버를 만드는 단계에서는 작은 공개 구조체가 더 단순할 수 있다. 불투명 타입은 실제로 구현 은닉이나 ABI 안정성이 필요할 때 도입하면 된다.

## 5. 함수 포인터: “무슨 일을 할지”를 데이터로 전달하기

### 5.1 선언을 읽는 방법

```c
int add(int a, int b);
int (*operation)(int a, int b);
```

첫 줄은 함수 선언이다. 두 번째 줄은 함수 포인터 변수 선언이다.

`operation`에서 출발해 다음과 같이 읽는다.

1. `(*operation)` → `operation`은 포인터다.
2. 뒤의 `(int a, int b)` → 두 `int`를 받는 함수를 가리킨다.
3. 맨 앞의 `int` → 그 함수는 `int`를 반환한다.

```c
operation = add;          /* &add라고 써도 된다. */
int result = operation(2, 3);
```

호출은 `operation(2, 3)` 또는 `(*operation)(2, 3)`으로 쓸 수 있다.

괄호 하나가 뜻을 바꾼다.

| 선언 | 뜻 |
|---|---|
| `int *f(void);` | 인자 없이 호출하고 `int *`를 반환하는 함수 |
| `int (*f)(void);` | 인자 없이 호출하고 `int`를 반환하는 함수의 포인터 |
| `int (*table[4])(void);` | 함수 포인터 4개를 담은 배열 |
| `uint8_t (*frame)[6];` | `uint8_t` 6개짜리 배열을 가리키는 포인터 |

매번 복잡한 선언을 읽기보다 의미 있는 타입 이름을 만든다.

```c
typedef int (*BinaryOperation)(int left, int right);
BinaryOperation operation = add;
```

### 5.2 버스 함수 포인터가 필요한 이유

장치 드라이버는 “레지스터를 읽는다”는 동작이 필요하다. STM32 HAL을 호출할지, RTOS의 I2C API를 호출할지, 시험용 배열에서 값을 읽을지는 장치의 레지스터 해석과 별개다.

```text
센서 드라이버
    │ read(context, address7, register, buffer, length)
    ▼
함수 포인터
    ├── STM32 어댑터 → HAL 호출
    ├── 다른 MCU 어댑터 → 해당 SDK 호출
    └── Fake 어댑터 → 메모리에 저장한 시험 값 반환
```

이 구조에서 바꾸는 것은 함수 포인터와 `context`다. 센서 값의 해석 코드는 그대로 둘 수 있다.

### 5.3 함수 포인터와 `void *context`는 한 쌍이다

함수 포인터만으로는 “어느 I2C 인스턴스인가”, “어느 fake 메모리인가”를 전달할 수 없다. 그래서 함수에 `void *context`를 같이 넘긴다.

```c
typedef struct {
    uint8_t registers[256];
    uint8_t expected_address7;
} FakeBus;

static DrvStatus fake_read(
    void *context,
    uint8_t address7,
    uint8_t reg,
    uint8_t *dst,
    size_t length)
{
    FakeBus *fake = context;  /* C: void* → 객체 포인터 변환 */

    if (fake == NULL || dst == NULL || length == 0) {
        return DRV_EINVAL;
    }
    if (address7 != fake->expected_address7) {
        return DRV_EIO;
    }
    if (length > sizeof fake->registers - (size_t)reg) {
        return DRV_EINVAL;
    }

    for (size_t i = 0; i < length; ++i) {
        dst[i] = fake->registers[(size_t)reg + i];
    }
    return DRV_OK;
}

static const BusOps fake_ops = {
    .read = fake_read
};
```

호출 예제다. 이 예제는 동기식으로 사용하며, 함수 반환 후에 드라이버 객체를 저장하거나 다시 쓰지 않는다.

```c
void run_demo(void)
{
    FakeBus fake = {0};
    fake.expected_address7 = 0x48;
    fake.registers[0x20] = 0x34;
    fake.registers[0x21] = 0x12;

    DemoSensor sensor = {
        .bus = { .ops = &fake_ops, .context = &fake },
        .address7 = 0x48,
        .units_per_lsb = 0.01f,
        .ready = true
    };

    DemoSample sample;
    DrvStatus status = demo_sensor_read(&sensor, &sample);
    /* 성공하면 sample.raw == 0x1234, 즉 4660 */
    (void)status;  /* 예제에서 미사용 경고를 피하기 위한 명시 */
}
```

`context`는 타입 정보를 자동 보관하는 특별한 객체가 아니다. `fake_read()`는 전달받은 주소가 **정말 `FakeBus`를 가리킨다는 계약**에 의존한다. 여기에 `I2C_HandleTypeDef *`를 연결하면 포인터의 크기가 같아도 올바르게 작동하지 않는다.

C++17에서는 같은 변환을 명시적으로 쓴다.

```cpp
auto* fake = static_cast<FakeBus*>(context);
```

### 5.4 `ops` 테이블과 `context`의 수명

```c
static const BusOps fake_ops = { .read = fake_read };
```

이 테이블은 정적 저장 기간을 가지므로 함수가 끝나도 존재한다. 여러 장치가 같은 읽기 구현을 사용하면 같은 `ops` 테이블을 공유해도 된다.

`context`는 장치마다 달라도 된다. 예를 들어 두 센서가 서로 다른 I2C 컨트롤러를 쓰면 다른 플랫폼 객체를 가리키게 한다.

| 구성 요소 | 보통 공유 가능한가? | 반드시 유지할 것 |
|---|---|---|
| 함수 코드 | 가능 | 올바른 함수 타입 |
| 읽기 전용 `ops` 테이블 | 가능 | 드라이버보다 짧지 않은 수명 |
| `context` 객체 | 설계에 따라 다름 | 실제 타입·수명·동시 접근 규칙 |
| 장치 주소·교정값 | 보통 장치별 | 서로 다른 장치 상태를 섞지 않기 |

### 5.5 함수 포인터를 캐스팅으로 억지로 맞추지 않는다

버스 함수는 매개변수 순서와 타입, 반환 타입, 호출 규약이 모두 인터페이스와 맞아야 한다. HAL 함수의 인자 목록이 다르면 **어댑터 함수**를 작성한다.

```c
/* 개념 예제: 이 함수 안에서만 HAL 형식으로 변환한다. */
static DrvStatus board_read(
    void *context, uint8_t address7, uint8_t reg,
    uint8_t *dst, size_t length);
```

잘못된 함수 포인터 타입으로 호출하는 것은 단순한 문체 문제가 아니다. 인자와 반환값이 ABI에 따라 다른 위치·폭으로 전달될 수 있다. 경고를 캐스팅으로 지우는 대신 타입이 맞는 함수로 감싼다. [SEI CERT EXP37-C](https://wiki.sei.cmu.edu/confluence/x/49UxBQ)

`void *`는 객체 포인터를 위한 통로다. ISO C의 일반적 이식성을 목표로 한다면 함수 포인터를 `void *`에 넣었다가 꺼내는 방식을 당연한 것으로 가정하지 않는다.

## 6. 배열, 버퍼, `sizeof`, 출력 매개변수

### 6.1 배열은 저장 공간이고 포인터는 주소 값이다

```c
uint8_t bytes[6];
uint8_t *p = bytes;
```

`bytes`는 원소 여섯 개의 저장 공간이다. `p`는 첫 원소를 가리키는 주소를 보관한다.

- `sizeof bytes` → 배열 전체의 바이트 수.
- `sizeof p` → 포인터 자체의 바이트 수.
- `sizeof bytes / sizeof bytes[0]` → 원소 개수.
- `bytes[i]` → `*(bytes + i)`와 같은 원소 접근.

`uint16_t values[6]`라면 원소 개수는 6이지만 전체 바이트 수는 `6 * sizeof(uint16_t)`다. 읽기 API의 `length`가 **바이트 수인지 원소 수인지** 이름과 주석으로 명확히 한다.

### 6.2 함수 매개변수의 `[]`는 크기를 보존하지 않는다

```c
void receive(uint8_t data[6]);
void receive(uint8_t *data);
```

이 두 선언의 매개변수 타입은 같은 포인터 타입으로 조정된다. 첫 번째 선언을 써도 함수 안에서 `sizeof data`로 6을 얻을 수 없다. 그래서 C API는 버퍼와 길이를 함께 넘기는 경우가 많다. [SEI CERT ARR01-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/arrays-arr/arr01-c/)

```c
DrvStatus read_fifo(uint8_t *out_bytes, size_t capacity, size_t *out_count);
```

각 인자의 뜻은 다르다.

- `out_bytes`: 결과를 쓸 저장 공간.
- `capacity`: 그 저장 공간에 쓸 수 있는 최대 바이트 수.
- `out_count`: 실제로 쓴 바이트 수를 돌려줄 위치.

고정 크기 배열을 가리키는 포인터는 별도 문법이다.

```c
void decode_frame(const uint8_t (*frame)[6]);

const uint8_t bytes[6] = {0};
decode_frame(&bytes);
```

이 경우 타입에 6개짜리 배열이라는 정보가 남지만 호출 문법이 다소 복잡하다. 단순 드라이버에서는 명시적 길이와 함수 계약이 더 읽기 쉬울 수 있다.

### 6.3 출력 매개변수는 “결과를 저장할 주소”다

```c
DrvStatus read_id(DemoSensor *sensor, uint8_t *out_id);

uint8_t id;
DrvStatus status = read_id(&sensor, &id);
```

반환값은 상태이고 `out_id`를 통해 읽은 값을 전달한다. 오류를 0이라는 측정값으로 표현하지 않아도 된다는 장점이 있다. 장치 ID나 측정값에 0이 정상적으로 등장할 수 있기 때문이다.

추천 계약은 **성공한 경우에만 출력이 유효하다**는 것이다. 더 강하게 “실패 시 출력 객체는 변경하지 않는다”고 정할 수도 있다. 2절의 `DemoSample next; ... *out = next;`는 그 계약을 구현한 예다.

출력 객체를 부분적으로 갱신해 놓고 실패하면 호출자가 이전 값과 새 값이 섞인 상태를 받게 된다. 축별 측정이나 여러 레지스터를 묶는 함수에서 임시 결과를 쓰는 이유다.

### 6.4 길이 검사는 더하기보다 빼기로 표현할 수 있다

```c
if (offset > capacity || count > capacity - offset) {
    return DRV_EINVAL;
}
```

이 형식은 `offset + count`가 정수 범위를 넘는 상황을 피한다. 왼쪽 조건이 참이면 `||`의 오른쪽은 평가하지 않으므로 `capacity - offset`의 잘못된 감산도 피한다.

그 뒤에도 실제 저장 공간이 `capacity`만큼 존재한다는 계약은 필요하다. 정수 검사로 잘못된 포인터를 유효하게 만들 수는 없다.

### 6.5 `memcpy`, `memmove`, `memset`의 역할

| 함수 | 용도 | 주의 |
|---|---|---|
| `memcpy(dst, src, n)` | 겹치지 않는 영역의 n바이트 복사 | 원본·대상 영역이 겹치면 안 됨 |
| `memmove(dst, src, n)` | 겹칠 수 있는 영역의 복사 | 그래도 양쪽 영역의 크기는 유효해야 함 |
| `memset(dst, value, n)` | 각 바이트를 같은 값으로 채움 | 다바이트 정수 값 채우기나 C++ 생성자를 대신하지 않음 |

예를 들어 `memset(values, 1, sizeof values)`는 각 `uint16_t` 원소를 숫자 1로 만드는 함수가 아니다. 8비트 바이트 환경에서는 각 바이트가 `0x01`이 되어 대개 원소가 `0x0101`이 된다.

일반 메모리 복사 함수는 MMIO 레지스터의 접근 폭·횟수·순서를 보장하기 위한 인터페이스가 아니다. 장치 레지스터 블록을 `memcpy`로 읽거나 쓰는 것은 대상의 명시적인 지원 없이는 하지 않는다.

## 7. `const`, `volatile`, `restrict`: 서로 다른 약속

### 7.1 `const`는 이 접근 경로로 수정하지 않겠다는 표현이다

```c
void parse(const uint8_t *bytes, size_t length);
```

`parse()`는 `bytes`를 통해 입력 바이트를 수정하지 않는다. 호출자가 가진 원본 메모리를 영구히 읽기 전용으로 만드는 것은 아니다. 다른 수정 가능한 포인터가 같은 메모리를 가리킬 수 있다.

| 선언 | 포인터 재지정 | 가리키는 값 수정 |
|---|---|---|
| `uint8_t *p` | 가능 | 가능 |
| `const uint8_t *p` | 가능 | 이 포인터를 통해 불가 |
| `uint8_t *const p` | 불가 | 가능 |
| `const uint8_t *const p` | 불가 | 이 포인터를 통해 불가 |

`const DemoSensor *sensor`도 모든 외부 자원을 깊이 읽기 전용으로 만드는 것은 아니다. 구조체 안의 `void *context`가 가리키는 객체는 별도의 객체다. 따라서 이 선언만으로 “장치나 버스에 부작용이 없다”고 약속하지 않는다.

실제 `const`로 정의된 객체에서 `const`를 제거해 쓰는 것은 정상적인 수정 방법이 아니다. 타입 캐스팅은 원래 객체의 성질을 바꾸지 않는다.

### 7.2 `volatile`은 주로 특별한 메모리 접근을 표현한다

```c
volatile uint32_t *status_reg;
```

MCU 주변장치 레지스터는 일반 RAM 변수와 다르게 외부 하드웨어가 값을 바꾸거나 읽기·쓰기 자체가 동작을 일으킬 수 있다. 컴파일러와 타깃의 MMIO 규약에 맞는 `volatile` 접근이 필요하다.

```c
/* 주소·접근 폭이 대상 메모리 맵에 맞다는 전제의 개념 예제 */
uint32_t status = *status_reg;
```

반면 I2C로 수신한 지역 배열 `uint8_t bytes[6]`는 보통 `volatile`일 필요가 없다. I2C 너머의 장치 레지스터와 CPU 주소 공간의 MMIO 레지스터는 구분해야 한다.

`volatile`이 다음을 자동으로 보장하지는 않는다.

- 여러 명령으로 나뉜 연산의 원자성.
- 스레드·ISR 사이의 잠금 또는 언어 수준 동기화.
- DMA와 CPU 캐시 사이의 일관성.
- 일반 메모리 접근 전체의 순서.

GCC 문서도 일반 메모리 접근을 `volatile` 접근으로 순서화할 수 없다고 설명한다. 특히 `volatile` 비트필드는 원하지 않는 읽기나 접근 폭을 유발할 수 있어 MMIO에 신중해야 한다. [GCC: Volatiles](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html)

### 7.3 `const volatile`은 모순이 아니다

```c
const volatile uint32_t *status;
```

이 포인터로는 쓸 수 없지만 읽을 때는 하드웨어 접근 규약이 적용된다. 하드웨어가 스스로 갱신하는 읽기 전용 상태 레지스터를 표현할 때 이런 조합을 볼 수 있다.

CMSIS 디바이스 헤더의 `__I`, `__O`, `__IO` 같은 매크로는 해당 헤더와 컴파일러 정의까지 확인해서 읽는다. 이름만으로 C 언어가 쓰기 전용 메모리를 완벽히 강제한다고 생각해서는 안 된다.

### 7.4 `restrict`는 C에서 사용하는 별칭 접근에 대한 계약이다

```c
void scale_samples(float *restrict out,
                   const float *restrict in,
                   size_t count,
                   float scale)
{
    for (size_t i = 0; i < count; ++i) {
        out[i] = in[i] * scale;
    }
}
```

이 예제의 계약은 처리하는 입력 영역과 출력 영역을 겹치지 않게 제공하는 것이다. 컴파일러는 그 조건을 바탕으로 최적화할 수 있다. 같은 배열을 입력과 출력으로 쓰게 하려면 이 인터페이스에 그런 약속을 붙이지 않는 편이 명확하다.

`restrict`의 실제 규칙은 단순히 “두 포인터 값이 다르다”보다 정교하며, 블록 실행 중 객체 접근 관계에 관한 것이다. 처음 드라이버를 만들 때는 의미를 확실히 이해한 버퍼 처리 함수에만 사용하면 된다. 표준 C++17에는 `restrict` 키워드가 없고 `__restrict` 등은 컴파일러 확장이다. [WG14 C 공개 초안 N1570, 6.7.3.1](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)

## 8. 정수 타입, 승격, 비트 연산과 레지스터 필드

### 8.1 어떤 타입을 언제 쓰는가

| 타입 | 드라이버에서 흔한 용도 | 확인할 점 |
|---|---|---|
| `uint8_t` | 8비트 레지스터, 수신 바이트 | 연산 중 `int`로 승격될 수 있음 |
| `uint16_t` | 16비트 코드·카운터 | 모든 연산이 16비트로 진행되는 것은 아님 |
| `uint32_t` | MMIO, 비트 마스크, 시간 카운터 | 하드웨어 접근 폭은 별도 사양 |
| `int16_t`, `int32_t` | 부호 있는 센서 원시값 | 수신 비트 해석과 타입 변환 구분 |
| `size_t` | 버퍼 크기, 배열 인덱스 | 부호 없는 타입; 음수 오류 코드와 혼합 금지 |
| `bool` | 참/거짓 상태 | 다중 상태는 `enum`이 더 명확 |
| `uintptr_t` | 구현이 지원하는 객체 포인터↔정수 변환 | 물리 주소가 항상 일반 포인터가 되는 것은 아님 |
| `float` | 물리 단위 변환 | FPU와 라이브러리 비용 확인 |

`uint8_t`, `uint16_t` 같은 정확한 폭의 타입은 해당 표현을 지원하는 구현에서 제공된다. STM32에서는 통상 제공되지만 모든 C 구현에 무조건 있다고 일반화하지 않는다. 정확한 바이트 전송을 전제하는 코드라면 그 전제를 빌드 시 확인하는 편이 좋다.

```c
#include <limits.h>
_Static_assert(CHAR_BIT == 8, "This driver requires 8-bit bytes");
```

C++17에서는 `static_assert(CHAR_BIT == 8, "...");`를 쓴다.

### 8.2 리터럴과 변수의 타입을 같이 읽는다

```c
0x80        /* 보통 int */
0x80u       /* unsigned 계열 리터럴 */
1.0f        /* float */
1.0         /* double */
UINT32_C(1) /* uint_least32_t에 대응하는 정수 상수 매크로 */
```

STM32 코드에서 `1 << 31`처럼 부호 있는 `int`를 왼쪽으로 이동하는 표현은 피한다. 마스크는 의도를 드러내는 부호 없는 상수로 만든다.

이 문서가 주로 다루는 32비트 `int` 환경에서는 `UINT32_C`를 32비트 이상 부호 없는 비트 계산에 활용할 수 있다. 매크로 결과의 엄밀한 타입은 `uint_least32_t`의 정수 승격과 연관되므로 특이한 정수 폭의 플랫폼까지 옮길 때는 해당 구현의 타입 정의를 확인한다.

```c
uint32_t top_bit = UINT32_C(1) << 31;
```

비트 연산을 주로 부호 없는 타입으로 수행하면 부호 표현과 부호 확장에 대한 혼동이 줄어든다. [SEI CERT INT13-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/integers-int/int13-c/)

### 8.3 정수 승격: `uint8_t` 계산이 8비트 계산인 것은 아니다

```c
uint8_t x = 0x80;
/* x << 8을 계산할 때 x는 보통 먼저 int로 승격된다. */
```

STM32의 32비트 `int`에서는 일부 표현이 문제없이 보이지만 16비트 `int` 환경에서는 같은 표현의 의미가 달라질 수 있다. 바이트를 합칠 때 계산할 폭을 먼저 정한다.

```c
uint16_t u = (uint16_t)(
    (uint32_t)bytes[0] |
    ((uint32_t)bytes[1] << 8));
```

여기서 캐스팅 순서가 중요하다. `(uint32_t)(bytes[1] << 8)`은 **이동 연산을 끝낸 뒤** 변환한다. 넓은 타입에서 이동하려면 이동하기 전에 변환해야 한다.

### 8.4 자주 쓰는 비트 연산

```c
uint32_t value = 0;
uint32_t mask = UINT32_C(1) << 3;

value |= mask;          /* 선택한 비트를 1로 */
value &= ~mask;         /* 선택한 비트를 0으로 */
value ^= mask;          /* 선택한 비트를 반전 */
bool set = (value & mask) != 0;
```

`&`와 `&&`는 다르다. `&`는 비트별 AND이고 `&&`는 논리 AND다. `|`와 `||`도 마찬가지다.

```c
/* 괄호로 비교 대상을 분명히 한다. */
if ((status & READY_MASK) != 0u) {
    /* ready */
}
```

`status & READY_MASK != 0`은 사람이 읽고 싶은 묶음과 연산자 우선순위가 다를 수 있으므로 이런 형태를 쓰지 않는다.

### 8.5 필드 값 추출과 삽입

가상의 8비트 설정 레지스터에서 ODR 필드가 `[5:3]`이라고 하자.

```c
enum { ODR_SHIFT = 3 };
#define ODR_MASK UINT32_C(0x38)

static uint8_t get_odr_code(uint8_t reg)
{
    return (uint8_t)(((uint32_t)reg & ODR_MASK) >> ODR_SHIFT);
}

static DrvStatus put_odr_code(uint8_t old_reg,
                              uint8_t code,
                              uint8_t *out_reg)
{
    if (out_reg == NULL || code > 7u) {
        return DRV_EINVAL;
    }

    uint32_t next = ((uint32_t)old_reg & ~ODR_MASK)
                  | ((uint32_t)code << ODR_SHIFT);
    *out_reg = (uint8_t)next;
    return DRV_OK;
}
```

읽는 순서는 다음과 같다.

1. `old_reg & ~ODR_MASK`로 변경하려는 필드만 비운다.
2. `code << ODR_SHIFT`로 필드 값을 올바른 위치에 놓는다.
3. `|`로 두 값을 합친다.

`code`를 마스크로 잘라 버리는 대신 범위를 검사한 것은 잘못된 입력을 발견하기 위해서다. 예를 들어 `code = 8`을 조용히 0으로 만들면 설정 실수를 찾기 어렵다.

이 계산은 **일반적인 RW 레지스터에 쓰려는 값을 만드는 연산**이다. 실제 버스에서 read-modify-write를 해도 되는지는 데이터시트를 확인한다. W1C, 읽으면 사라지는 상태, 예약 비트, 하드웨어가 갱신하는 필드에는 다른 규칙이 필요하다.

### 8.6 이동 횟수와 signed/unsigned 혼합

이동 횟수는 음수가 될 수 없고, 승격된 왼쪽 피연산자의 비트 폭 이상이어도 안 된다. 예를 들어 32비트 피연산자를 32만큼 이동하는 표현은 “0이 되겠지”라고 사용할 수 없다. [SEI CERT INT34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int34-c/)

음수를 오른쪽으로 이동하는 동작을 부호 해석의 유일한 근거로 쓰지 않는 편이 C17/C++17 코드의 의도를 설명하기 쉽다. 입력을 부호 없는 값으로 조합한 뒤, 데이터시트의 부호 표현에 따라 명시적으로 변환할 수 있다.

또한 다음 형태를 피한다.

```c
int count = get_count();
size_t capacity = sizeof buffer;
/* count < capacity만 검사하면 음수 count의 의미가 혼란스러워진다. */
```

정수의 부호가 다른 비교에서는 통상 산술 변환이 적용된다. 오류 가능한 `int`는 먼저 음수 여부를 확인하고, 그 다음 유효 범위의 값을 `size_t`와 비교한다.

```c
if (count < 0) {
    return DRV_EIO;
}
if ((size_t)count > capacity) {
    return DRV_EINVAL;
}
```

## 9. 바이트 순서, 부호 확장, 정렬과 별칭

### 9.1 통신 순서와 CPU 메모리 순서를 분리한다

장치가 낮은 바이트를 먼저 전송하면 little-endian 형식으로 조합한다.

```c
static uint16_t load_u16_le(const uint8_t bytes[2])
{
    return (uint16_t)((uint32_t)bytes[0]
                    | ((uint32_t)bytes[1] << 8));
}

static uint16_t load_u16_be(const uint8_t bytes[2])
{
    return (uint16_t)(((uint32_t)bytes[0] << 8)
                    | (uint32_t)bytes[1]);
}
```

이 함수는 CPU의 엔디언과 무관하게 입력 바이트의 의미를 코드로 표현한다. “STM32가 little-endian이니까 수신 배열을 정수 포인터로 바꾸면 된다”는 단축은 정렬과 타입 접근 문제를 함께 끌어들인다.

장치의 **주소 증가 순서**, **한 값의 바이트 순서**, SPI의 **비트 전송 순서**는 서로 다른 항목이다. 각각 데이터시트와 버스 설정을 확인한다.

### 9.2 16비트 2의 보수 값을 명시적으로 해석하기

2절의 디코더는 `0x8000` 미만을 양수로 해석하고, 나머지는 `65536`을 빼서 음수로 해석했다.

| 입력 비트 | 부호 없는 값 | 해석한 값 |
|---|---:|---:|
| `0x0000` | 0 | 0 |
| `0x7FFF` | 32767 | 32767 |
| `0x8000` | 32768 | -32768 |
| `0xFFFF` | 65535 | -1 |

이 방식은 `uint16_t`를 범위 밖 `int16_t`로 변환할 때의 구현 정의 동작에 의존하지 않고 의미를 보여 준다. 결과를 `int32_t`에 두므로 모든 중간값을 표현할 수 있다.

데이터가 12비트인데 16비트 레지스터에 왼쪽 정렬되어 있다면 다음 세 단계를 별도로 수행한다.

1. 두 바이트를 부호 없는 16비트 코드로 조합한다.
2. 데이터시트의 정렬 규칙에 따라 유효한 12비트를 추출한다.
3. 12번째 비트를 부호 비트로 해석하여 `4096`을 빼는 방식으로 변환한다.

```c
/* 16비트 단어의 [15:4]가 signed 12-bit 값인 가상의 형식 */
static int32_t decode_s12_left_aligned(uint16_t word)
{
    uint32_t u = ((uint32_t)word >> 4) & UINT32_C(0x0FFF);
    return (u < UINT32_C(0x0800))
        ? (int32_t)u
        : (int32_t)u - INT32_C(4096);
}
```

이 코드를 모든 센서에 그대로 쓰지는 않는다. 어떤 센서는 오른쪽 정렬이고, 어떤 센서는 상태 비트가 섞이며, 어떤 값은 offset binary 표현일 수 있다.

### 9.3 배열을 다른 타입 포인터로 바꾸는 것이 곧 해석은 아니다

```c
/* 피할 예: 다음 한 줄에 여러 전제를 숨긴다. */
uint16_t value = *(const uint16_t *)bytes;
```

확인해야 할 조건이 많다.

- `bytes` 주소가 `uint16_t`의 정렬 요구를 만족하는가?
- 해당 객체를 이 타입으로 읽는 것이 언어의 별칭 규칙에 맞는가?
- 충분한 바이트가 실제로 존재하는가?
- CPU 엔디언이 전송 형식과 같은가?

타입 캐스팅 자체가 이 조건을 만족시켜 주지는 않는다. 레지스터 수신 값은 위와 같이 바이트별로 조합하는 방식이 가장 직접적이다. [SEI CERT EXP39-C](https://wiki.sei.cmu.edu/confluence/display/c/EXP39-C.%2BDo%2Bnot%2Baccess%2Ba%2Bvariable%2Bthrough%2Ba%2Bpointer%2Bof%2Ban%2Bincompatible%2Btype)

`memcpy(&value, bytes, sizeof value)`는 적합한 대상 객체로 바이트를 복사하는 용도에서 정렬·별칭 문제를 피하는 수단이 될 수 있다. 그러나 **엔디언 변환은 해 주지 않는다.** 임의 비트 패턴이 대상 타입의 유효한 값인지도 별도 문제다.

### 9.4 `sizeof(struct)`는 멤버 크기의 합과 다를 수 있다

```c
typedef struct {
    uint8_t tag;
    uint32_t count;
} PacketInfo;
```

멤버 사이와 끝에 패딩이 들어갈 수 있다. 따라서 `sizeof(PacketInfo)`를 그대로 통신 패킷 길이라고 간주하거나, 구조체 전체 바이트를 보내는 방식은 외부 형식을 안정적으로 정의하지 못한다.

통신 형식은 “0번 바이트는 tag, 1–4번 바이트는 little-endian count”처럼 정의하고 직접 직렬화한다. `#pragma pack`과 `__attribute__((packed))`는 구현별 확장이며, 패딩을 줄여도 비정렬 접근과 엔디언 문제는 남는다.

MMIO용 구조체는 조금 다른 상황이다. 제조사 헤더가 주소 오프셋과 예약 영역까지 고려해 정의한 레지스터 블록이라면 그것을 사용한다. 직접 정의해야 한다면 접근 폭, `volatile`, `offsetof`, 정렬과 예약 공간을 모두 확인한다.

## 10. `enum`, `union`, 비트필드

### 10.1 `enum`은 숫자에 의미를 붙인다

```c
typedef enum {
    SENSOR_OFF,
    SENSOR_STARTING,
    SENSOR_READY,
    SENSOR_FAULT
} SensorStateCode;
```

`state == SENSOR_READY`는 `state == 2`보다 의도를 읽기 쉽다. 상태 전이는 `switch`와 함께 쓰면 빠진 상태를 검토하기 좋다.

C17에서는 enum 타입의 크기를 항상 1바이트나 4바이트로 가정하지 않는다. 통신 버퍼에 저장할 때는 명시적으로 정해진 정수 표현으로 변환한다. 특히 `enum`의 원시 메모리를 그대로 전송하지 않는다.

장치의 ODR 설정값이 `0, 1, 3, 5`처럼 불연속이면 enum도 그 코드를 명시할 수 있다. 다만 API에 공개할 값이 실제 Hz인지 레지스터 코드인지 이름으로 구분한다.

### 10.2 `union`은 여러 멤버가 저장 공간을 공유한다

```c
typedef union {
    uint32_t count;
    float temperature;
} EventPayload;
```

구조체는 두 값을 함께 담지만, 이 union은 같은 저장 영역을 서로 다른 멤버가 공유한다. “어떤 멤버가 의미 있는지”를 union 자체가 자동 기록하지는 않는다.

그래서 이벤트 큐에서는 태그를 붙이는 방식이 좋다.

```c
typedef enum {
    EVENT_COUNT,
    EVENT_TEMPERATURE
} EventKind;

typedef struct {
    EventKind kind;
    union {
        uint32_t count;
        float temperature;
    } payload;
} Event;

Event event = {
    .kind = EVENT_TEMPERATURE,
    .payload = { .temperature = 23.5f }
};

switch (event.kind) {
case EVENT_COUNT:
    use_count(event.payload.count);
    break;
case EVENT_TEMPERATURE:
    use_temperature(event.payload.temperature);
    break;
default:
    report_invalid_event();
    break;
}
```

`kind`와 실제로 쓴 멤버가 항상 일치하도록 생성 함수를 둘 수도 있다. 구조체와 union의 조합은 DMA 완료, 통신 오류, 샘플 준비처럼 서로 다른 이벤트 정보를 하나의 큐 항목으로 표현할 때 유용하다.

### 10.3 C와 C++의 union을 같은 규칙으로 취급하지 않는다

C에서는 다른 union 멤버를 읽는 방식의 표현 재해석에 관한 규정이 있지만 결과는 데이터 표현에 의존하고 트랩 표현 등의 문제가 생길 수 있다. 그렇다고 임의 포인터 캐스팅을 통한 접근까지 모두 허용되는 것은 아니다.

C++17에서는 활성 멤버와 객체 수명 규칙을 먼저 따라야 한다. 공통 초기 시퀀스 등 제한된 예외가 있지만, 정수를 쓴 후 같은 union의 `float`를 읽는 패턴을 일반적인 휴대용 비트 변환으로 쓰지 않는다. 비단순 클래스 멤버가 들어가면 생성·파괴 관리도 필요하다. [C++17 공개 초안 소스: classes](https://raw.githubusercontent.com/cplusplus/draft/n4659/source/classes.tex)

첫 드라이버에서는 union을 **태그에 따라 하나의 값만 저장하는 컨테이너**로 이해하면 충분하다. 바이트 해석에는 명시적 마스크·이동을, C++17 이벤트 값에는 필요에 따라 `std::variant`를 검토한다. `std::variant`를 쓸 수 있는지는 해당 표준 라이브러리와 프로젝트 정책도 확인한다.

### 10.4 비트필드는 레지스터 비트 그림과 같아 보이지만 주의가 필요하다

```c
struct ControlBits {
    unsigned enable : 1;
    unsigned mode   : 3;
    unsigned unused : 4;
};
```

소프트웨어 내부에서 작은 필드를 표현할 때는 유용할 수 있다. 그러나 컴파일러의 비트 배치·패딩·저장 단위 규칙을 확인하지 않고 “enable은 하드웨어 bit 0”이라고 단정할 수 없다.

MMIO나 통신 형식에는 접근 폭과 읽기·쓰기 부작용도 있으므로, 학습 단계에서는 정수 레지스터 값과 `MASK`/`SHIFT`를 사용하면 데이터시트와 코드를 더 명확히 연결할 수 있다.

## 11. 헤더, 구현 파일, `static`, `extern`, `inline`

### 11.1 공개 계약은 헤더, 동작은 구현 파일

```c
/* demo_sensor.h */
#ifndef DEMO_SENSOR_H
#define DEMO_SENSOR_H

#include <stdint.h>

/* 공개 타입 정의 */
/* 공개 함수 선언 */

#endif
```

```c
/* demo_sensor.c */
#include "demo_sensor.h"

/* 파일 내부 도우미 */
/* 공개 함수의 정의 */
```

include guard는 같은 번역 단위에서 헤더 내용이 중복 정의되는 것을 막는다. `#pragma once`도 흔하지만 표준 문법은 아니므로 프로젝트의 컴파일러 정책을 따른다.

헤더는 자신이 쓰는 타입의 선언을 직접 포함하도록 만드는 편이 좋다. 다른 헤더가 우연히 먼저 `stdint.h`를 포함해 주는 상황에 기대면 include 순서가 바뀔 때 문제가 생긴다.

### 11.2 `static`의 의미는 위치에 따라 다르다

| 위치 | 예 | 의미 |
|---|---|---|
| 파일 범위 함수 | `static DrvStatus read_reg(...)` | 다른 번역 단위에 직접 공개하지 않는 함수 |
| 파일 범위 변수 | `static uint32_t errors;` | 내부 연결을 가지며 프로그램 동안 존재 |
| 함수 내부 변수 | `static uint32_t calls;` | 이름은 지역 범위지만 값은 호출 사이에 유지 |

```c
static DrvStatus read_reg(DemoSensor *sensor,
                           uint8_t reg, uint8_t *out);
```

드라이버 내부의 저수준 도우미를 숨기면 외부에서 임의로 호출하지 못하게 하고 공개 API 수를 줄일 수 있다.

함수 내부 `static` 변수는 모든 호출이 공유한다. 인스턴스마다 필요한 상태를 여기에 두면 여러 센서를 추가할 때 문제가 된다.

```c
/* 이런 상태는 장치 구조체 멤버가 더 적합할 수 있다. */
static uint8_t current_address;
```

### 11.3 `extern`은 다른 곳에 있는 정의를 선언할 때 사용한다

```c
/* board.h */
extern I2C_HandleTypeDef hi2c1;
```

```c
/* board.c */
I2C_HandleTypeDef hi2c1;
```

실제 객체 정의는 하나다. 헤더에 수정 가능한 전역 변수의 정의를 넣어 여러 `.c` 파일이 각자 정의하게 만들지 않는다.

장치 드라이버가 `extern hi2c1`을 직접 참조하면 특정 보드에 묶인다. 보드 코드가 `&hi2c1`을 초기화 인자로 넘기면 어떤 컨트롤러를 사용할지 바깥에서 결정할 수 있다.

### 11.4 `static inline`과 매크로

```c
static inline uint8_t extract_mode(uint8_t reg)
{
    return (uint8_t)(((uint32_t)reg >> 1) & UINT32_C(0x07));
}
```

작고 순수한 계산 함수를 헤더에 두려면 C에서는 `static inline`이 이해하기 쉬운 선택이다. `inline`은 반드시 인라인 기계어로 치환하라는 명령이 아니다. C와 C++의 외부 연결 `inline` 정의 규칙도 완전히 같지 않으므로, 처음에는 공개 함수는 `.c`, 작은 헤더 도우미는 `static inline`으로 구분하면 단순하다.

매크로는 타입이 있는 함수와 다르게 텍스트 치환이다.

```c
#define BAD_SQUARE(x) x * x
/* BAD_SQUARE(1 + 2)는 기대한 9가 되지 않는다. */

#define SQUARE(x) ((x) * (x))
/* 괄호를 보완해도 SQUARE(i++)는 여전히 좋은 사용법이 아니다. */
```

입력을 한 번만 평가해야 하거나 타입 검사가 필요하면 함수가 더 적합하다. 다중 문장 매크로가 꼭 필요하면 보통 `do { ... } while (0)` 형태를 사용하지만, 매개변수의 부작용은 여전히 별도로 검토한다.

### 11.5 C 헤더를 C++에서 호출하기

```c
#ifndef DEMO_API_H
#define DEMO_API_H

#ifdef __cplusplus
extern "C" {
#endif

int demo_api_init(void);

#ifdef __cplusplus
}
#endif

#endif
```

`__cplusplus`는 C++ 컴파일일 때 정의된다. `extern "C"`는 해당 선언에 C 언어 연결을 부여하여 C로 컴파일한 함수와 연결할 수 있게 한다.

이 표기는 C++ 클래스·템플릿을 C 코드로 바꾸지 않는다. 공통 헤더는 C에서도 이해할 수 있는 타입과 함수 선언으로 구성해야 한다. 구조체 레이아웃, 호출 규약, 컴파일 옵션도 양쪽 ABI와 맞아야 한다.

## 12. 함수 선언, 제어 흐름과 오류 전달

### 12.1 C17의 `f(void)`와 `f()`

```c
int sensor_reset(void);  /* 인자가 없다는 명시적 프로토타입 */
int legacy_reset();      /* C17: 인자 정보를 지정하지 않는 선언 */
```

C17에서는 인자 없는 함수에 `(void)`를 쓰는 습관이 좋다. C++17의 `f()`는 인자 없는 함수 선언이다. 같은 표기를 C와 C++에서 무심코 같은 뜻으로 읽지 않는다.

함수는 호출 전에 올바른 선언이 보이도록 한다. 드라이버 공개 헤더의 함수 선언은 컴파일러가 잘못된 인자 사용을 발견하게 하는 첫 번째 계약이다.

### 12.2 읽을 수 있는 오류 코드

드라이버 오류는 처음부터 너무 세분화할 필요는 없지만, 호출자가 대응을 달리해야 하는 경우는 구분하는 편이 좋다.

| 상태 | 호출자의 일반적인 판단 |
|---|---|
| `DRV_EINVAL` | 인자 또는 구성 오류 수정 |
| `DRV_ESTATE` | 초기화·동작 순서 확인 |
| `DRV_EIO` | 버스 또는 장치 응답 확인 |
| `DRV_ETIMEOUT` | 제한 시간, 버스 정체, 장치 준비 상태 확인 |

오류 코드를 더하거나 OR 해서 “오류가 하나라도 있었음”으로만 만드는 방식은 원래 원인을 잃을 수 있다. 순차 초기화에서 다음 단계가 이전 단계의 성공을 전제로 한다면, 실패한 즉시 그 오류를 반환하는 흐름이 이해하기 쉽다.

```c
DrvStatus status = reset_device(sensor);
if (status != DRV_OK) {
    return status;
}

status = configure_device(sensor);
if (status != DRV_OK) {
    return status;
}
```

### 12.3 `switch`는 유한한 설정과 상태에 잘 맞는다

```c
switch (range) {
case RANGE_LOW:
    code = 0;
    scale = 0.001f;
    break;
case RANGE_HIGH:
    code = 1;
    scale = 0.004f;
    break;
default:
    return DRV_EINVAL;
}
```

한 설정에 대해 레지스터 코드와 물리 단위 변환 계수를 함께 결정하면 둘이 어긋날 가능성을 줄일 수 있다. 실제 소프트웨어 상태 갱신은 하드웨어 설정이 성공한 뒤 수행한다.

`break`를 빠뜨리면 다음 case로 흐름이 이어진다. 의도적 fallthrough는 프로젝트 규칙에 맞게 분명히 표시한다. C++17에는 `[[fallthrough]];`가 있지만 C17 표준 속성은 아니다.

### 12.4 `goto`는 정리 경로를 단순하게 만들 때 사용 가능하다

C에서 자원을 여러 단계로 얻는 함수는 실패 시 정리 경로가 필요하다. 한 함수 끝의 `cleanup:`으로 이동하는 제한된 사용은 중복 정리를 줄일 수 있다. 하지만 순수한 레지스터 읽기 함수에 꼭 필요한 문법은 아니다.

핵심은 어떤 경로로 나가도 잠금과 임시 자원이 올바르게 정리되는 것이다. C++에서는 16절의 RAII가 이 역할을 맡을 수 있다.

### 12.5 부동소수점 상수와 단위

```c
float value = (float)raw * units_per_lsb;
```

데이터시트가 `LSB/g`를 제공하면 곱하는 계수가 아니라 나누는 계수일 수 있다. 문법보다 단위 해석이 먼저다.

```c
float acceleration_g = (float)raw / lsb_per_g;
```

`1000`과 `1000.0f`는 같은 타입이 아니다. 정수 나눗셈이 먼저 일어난 뒤 float로 바뀌는 실수도 흔하다.

```c
float wrong = raw / 1000;           /* 정수 나눗셈 후 변환 */
float right = (float)raw / 1000.0f;  /* 부동소수점 나눗셈 */
```

변수 이름에 `_raw`, `_g`, `_mps2`, `_ms`, `_hz`처럼 단위를 붙이면 코드 리뷰가 쉬워진다.

## 13. 메모리 수명과 초기화: 어디에 객체를 둘 것인가

### 13.1 저장 위치보다 “언제까지 유효한가”를 먼저 본다

| 객체 생성 방식 | 일반적인 수명 | 드라이버에서의 용도 |
|---|---|---|
| 함수 지역 변수 | 해당 블록 실행 동안 | 동기 통신의 임시 바이트·결과 |
| 파일 범위/`static` 객체 | 프로그램 실행 동안 | 지속적인 장치·버스 상태 |
| 호출자가 보유한 구조체 | 호출자의 저장 기간에 따름 | 여러 장치 인스턴스 |
| 동적 할당 객체 | 할당부터 해제까지 | 정책이 허용하는 가변 수명 |
| C++ 클래스 멤버 | 바깥 객체의 수명에 포함 | DMA 버퍼·콜백 상태 |

“전부 전역으로 만들면 수명 문제를 해결한다”는 접근은 처음에는 간단해 보일 수 있지만, 다중 장치·시험·동시 접근을 어렵게 할 수 있다. 장치 구조체를 호출자가 보유하도록 하면 수명과 개수가 더 명확해진다.

### 13.2 지역 context를 저장해 두면 안 되는 경우

```c
/* 잘못된 형태를 설명하는 예: 함수 반환 후 ctx가 사라진다. */
void connect_bad(DemoSensor *sensor)
{
    PlatformContext ctx;
    /* ctx의 멤버 초기화 ... */
    sensor->bus.context = &ctx;
}
```

대신 호출자가 `PlatformContext`를 드라이버 사용 기간 동안 보유하거나, 장치 객체가 필요한 context를 값으로 포함하도록 설계한다.

`const`를 붙이거나 포인터 타입을 바꾸는 것은 수명을 연장하지 않는다. C++의 참조나 `std::span` 같은 비소유 뷰도 일반적으로 원본 객체를 대신 소유하지 않는다.

### 13.3 초기화는 소프트웨어와 하드웨어 두 층이다

소프트웨어 초기화는 함수 포인터, 주소, 상태, 버퍼를 준비한다. 하드웨어 초기화는 전원 안정화, 장치 ID 확인, 리셋, 모드 설정, 준비 상태 확인 등을 수행한다.

권장 흐름은 다음과 같다.

1. 인자와 필수 함수 포인터를 검사한다.
2. 장치 객체를 사용 불가 상태로 둔다.
3. 하드웨어 초기화를 수행한다.
4. 성공한 실제 설정과 변환 계수를 객체에 저장한다.
5. 마지막으로 준비 상태를 표시한다.

`ready = true`를 먼저 설정하면 초기화 도중 실패한 장치가 사용 가능하게 보일 수 있다. 한 개의 `bool`로 표현하기 어려워지면 10절의 상태 enum으로 확장한다.

## 14. ISR, DMA, 비동기 콜백을 붙일 때 추가로 필요한 문법 지식

이 절은 첫 번째 polling 드라이버를 읽은 뒤에 보면 된다. 여기서 새로 중요한 것은 문법의 종류보다 **여러 실행 흐름이 같은 객체에 접근한다는 사실**이다.

### 14.1 동기식과 비동기식은 버퍼 수명 계약이 다르다

```c
void read_sync_example(DemoSensor *sensor)
{
    uint8_t bytes[6];
    /* 동기 API가 반환 전에 bytes 접근을 완료하면 지역 배열 사용 가능 */
}
```

```c
/* 위험한 형태를 설명하는 예 */
void start_async_bad(void)
{
    uint8_t bytes[6];
    platform_start_dma(bytes, sizeof bytes);
    /* DMA 완료 전에 함수가 반환하면 bytes의 수명이 끝난다. */
}
```

비동기 API는 일반적으로 DMA 요청을 제출한 뒤 실제 전송이 끝나기 전에 반환한다. 이때 버퍼는 완료 또는 확정적인 취소까지 살아 있어야 하고, 전송 중에는 CPU가 임의로 재사용해서도 안 된다.

장치 멤버로 버퍼를 두는 예다.

```c
typedef struct {
    uint8_t receive_buffer[6];
    bool request_active;
    /* 상태의 동시 접근은 별도의 플랫폼 동기화 규칙으로 보호한다. */
} AsyncSensor;
```

`request_active`를 `volatile`로 바꾸는 것만으로 전체 비동기 프로토콜이 올바르게 되지는 않는다. 누가 언제 읽고 쓰는지와 완료 통지 순서를 함께 정한다.

### 14.2 읽기·수정·쓰기는 단일 동작이 아닐 수 있다

```c
counter++;
flags |= READY_FLAG;
```

이런 표현은 메모리 읽기, 계산, 메모리 쓰기로 나뉠 수 있다. 중간에 ISR이 같은 변수를 바꾸면 한쪽 갱신이 사라질 수 있다.

해결 방법은 상황에 따라 다르다.

- ISR과 main loop: 플랫폼이 보장하는 짧은 임계 구역 또는 ISR에 적합한 원자 연산.
- RTOS task 사이: mutex, semaphore, queue 또는 적합한 원자 연산.
- 여러 요청의 버스 사용: 버스 잠금과 요청 큐.
- 하드웨어가 제공하는 set/clear 전용 레지스터: 데이터시트가 정의한 쓰기 방식.

주변장치 W1C 레지스터에 C의 원자 연산을 적용하면 해결된다고 생각해서는 안 된다. 언어 수준 공유 메모리와 MMIO 부작용은 별개의 문제다.

### 14.3 C의 `_Atomic`과 C++의 `std::atomic`

```c
/* C: C11 이후, 해당 구현이 원자 라이브러리를 지원하는 경우 */
#include <stdatomic.h>
atomic_bool ready;
```

```cpp
// C++17
#include <atomic>
std::atomic<bool> ready{false};
```

원자 변수는 동시 접근하는 일반 메모리 값의 원자성과 순서 계약을 표현하는 도구다. C의 기능 지원 여부, 대상 폭의 lock-free 여부, 함수 호출로 구현되는지, ISR에서 안전한지는 컴파일러와 플랫폼 문서로 확인한다.

`memory_order_relaxed`는 주변의 일반 데이터까지 자동 공개해 주지 않는다. 데이터 버퍼 작성 후 준비 플래그를 통해 다른 실행 흐름에 전달하려면 release/acquire 같은 관계가 필요할 수 있다. 그러나 이는 DMA 캐시 관리까지 대신하지 않는다. 처음에는 RTOS가 제공하는 ISR용 큐·알림 API의 문서화된 계약을 따르는 편이 명확할 수 있다.

### 14.4 DMA는 C/C++의 일반 스레드와도 다르다

캐시가 있는 MCU에서는 CPU가 보는 메모리와 DMA가 접근하는 메모리의 최신 상태를 맞추는 작업이 필요할 수 있다. 전송 방향, 캐시 정책, DMA 접근 가능한 메모리 영역, 정렬과 캐시 라인 경계를 함께 확인한다.

- CPU가 만든 송신 데이터를 DMA가 읽기 전에 필요한 cache clean.
- DMA가 쓴 수신 데이터를 CPU가 읽을 때 필요한 cache invalidate 및 소유권 관리.
- 부분 캐시 라인을 다른 데이터와 공유할 때의 손실 위험.
- 전송 시작·완료·재사용 사이의 적절한 메모리 순서.

실제 순서는 MCU 제조사의 DMA·캐시 지침을 따른다. Arm CMSIS는 cache maintenance 함수를 제공하지만, 함수를 아무 위치에나 호출하는 것으로 정확한 프로토콜이 만들어지는 것은 아니다. [Arm CMSIS: Cache Functions](https://arm-software.github.io/CMSIS_6/latest/Core/group__cache__functions__m7.html)

`__DMB()`는 메모리 접근의 관측 순서를 위한 명령이고 `__DSB()`는 앞선 명시적 메모리 접근 완료를 기다리는 성격이 있다. 어느 것도 DMA 전송 전체가 끝났다는 일반적인 완료 통지 함수는 아니다. 장치의 완료 조건을 별도로 확인해야 한다. [Arm CMSIS: CPU Intrinsics](https://arm-software.github.io/CMSIS_6/v6.0.0/Core/group__intrinsic__CPU__gr.html)

### 14.5 콜백의 실행 문맥을 API에 적는다

```c
typedef void (*CompletionFn)(void *user, DrvStatus status);
```

함수 타입만 보면 콜백이 ISR에서 호출되는지 task에서 호출되는지 알 수 없다. 다음 조건을 주석이나 API 계약에 써야 한다.

- 콜백이 ISR 문맥인가?
- 콜백 안에서 같은 드라이버를 다시 호출할 수 있는가?
- 오래 걸리는 처리, 대기, 메모리 할당, 로깅을 허용하는가?
- 성공·실패·취소 시 각각 정확히 몇 번 호출하는가?
- 콜백 등록 해제 뒤에도 이미 실행 중인 콜백이 남을 수 있는가?
- `user` 객체와 버퍼를 언제부터 파괴하거나 재사용해도 되는가?

처음에는 콜백에서 결과·오류 이벤트만 전달하고 실제 처리는 main loop나 task에서 수행하는 구조가 추적하기 쉽다.

### 14.6 시간 계산과 wraparound

```c
uint32_t elapsed = (uint32_t)(now_ms - start_ms);
if (elapsed >= timeout_ms) {
    return DRV_ETIMEOUT;
}
```

같은 폭의 부호 없는 순환 카운터에서 경과 시간을 구하는 흔한 방식이다. `now_ms >= start_ms + timeout_ms`는 덧셈이 순환할 때 의도와 달라질 수 있다.

이 방식도 무한히 긴 시간을 추적하지는 못한다. 한 바퀴 전체를 지나도록 관측을 놓치지 않고, 허용 timeout과 검사 주기가 시스템의 카운터 범위에 맞아야 한다. “어느 시각이 더 미래인가” 같은 일반적 순서 비교는 추가 범위 제한이 필요하다. 시간 단위와 순환 조건을 API 계약으로 남긴다.

## 15. C++17을 읽기 위한 최소 문법

### 15.1 참조 `T&`는 기존 객체에 붙인 다른 이름이다

```cpp
DrvStatus read(DemoSensor& sensor, DemoSample& out);

DemoSample sample;
auto status = read(sensor, sample);
```

참조를 받는 함수는 호출 시 `&`를 쓰지 않는다. 함수 내부에서는 `sensor.address7`처럼 객체 문법으로 접근한다.

| C++ 표현 | 읽는 의미 |
|---|---|
| `T value` | 값 전달; 별도 객체가 만들어질 수 있음 |
| `T* ptr` | 주소 전달; API가 허용하면 널 가능 |
| `T& ref` | 기존 수정 가능한 객체 참조 |
| `const T& ref` | 이 참조를 통해 수정하지 않는 객체 참조 |
| `T&& ref` | rvalue reference; 이동·전달 문맥에서 사용 |

참조를 다른 객체에 다시 연결할 수는 없다. `ref = other;`는 참조가 가리키는 객체에 값을 대입하는 것이다. 참조가 있다고 해서 원본 객체의 수명이 자동으로 충분해지는 것은 아니다.

### 15.2 `class`와 `struct`

```cpp
class Sensor {
public:
    DrvStatus read(DemoSample& out);

private:
    RegisterBus bus_;
    std::uint8_t address7_;
};
```

C++의 `class`와 `struct`는 모두 멤버 함수, 생성자, 접근 제어를 가질 수 있다. 기본 접근 권한과 기본 상속 접근 권한이 다르다. `class`는 기본이 private이고 `struct`는 기본이 public이다.

설정을 외부에서 임의로 바꾸지 못하게 하고 싶으면 private 멤버와 제한된 공개 함수를 사용할 수 있다. 이것은 C의 불투명 구조체가 해결하려는 문제와 일부 겹친다.

### 15.3 생성자, 초기화 목록, `explicit`

```cpp
class Sensor {
public:
    explicit Sensor(RegisterBus bus, std::uint8_t address7) noexcept
        : bus_(bus), address7_(address7), ready_(false)
    {
    }

    [[nodiscard]] DrvStatus init() noexcept;

private:
    RegisterBus bus_;
    std::uint8_t address7_;
    bool ready_;
};
```

`:` 뒤는 멤버 초기화 목록이다. 멤버는 목록의 나열 순서가 아니라 **클래스에서 선언된 순서**로 초기화된다.

이 예제의 생성자는 소프트웨어 연결만 준비한다. 실패할 수 있는 버스 통신은 오류 코드를 반환하는 `init()`으로 분리했다. C++에서 생성자가 반드시 하드웨어 초기화까지 수행해야 하는 것은 아니다.

`explicit`은 의도하지 않은 암시적 변환에 생성자가 사용되는 일을 제한한다. 단일 인자로 호출 가능한 자원 래퍼에서 특히 중요하다.

`[[nodiscard]]`는 반환값을 무시하지 않도록 컴파일러 진단을 유도한다. 실패를 런타임에서 자동 처리해 주는 기능은 아니다.

### 15.4 `noexcept`의 정확한 역할

```cpp
DrvStatus read(DemoSample& out) noexcept;
```

이 함수는 예외를 밖으로 내보내지 않겠다고 약속한다. 버스 읽기가 반드시 성공한다는 뜻도, 함수 안에서 예외가 절대 발생할 수 없도록 모든 코드를 자동 변경한다는 뜻도 아니다. 예외가 `noexcept` 함수 밖으로 나가려 하면 종료 처리로 이어진다.

예외를 사용하지 않는 임베디드 프로젝트에서는 오류 코드 방식과 잘 어울린다. 하지만 컴파일러의 예외 비활성 옵션과 라이브러리 동작까지 검토해야 한다. “`noexcept`를 붙였으니 ISR에서 어떤 함수든 안전하다”는 결론은 성립하지 않는다.

### 15.5 `enum class`, `constexpr`, `auto`, `nullptr`

```cpp
enum class Range : std::uint8_t {
    low = 0,
    high = 1
};

constexpr std::uint8_t kWhoAmIRegister = 0x0F;
constexpr std::size_t kFrameBytes = 6;

auto* sensor_ptr = static_cast<Sensor*>(context);
Sensor* optional_sensor = nullptr;
```

- `enum class`는 열거자 이름을 `Range::low`처럼 범위 안에 두고 정수로의 암시적 변환을 제한한다.
- `constexpr`는 상수 식에 사용할 수 있는 값이나 함수를 표현한다. 함수에 붙였다고 모든 호출이 반드시 컴파일 시 계산되는 것은 아니다.
- `auto`는 초기화 식으로부터 타입을 추론한다. 참조를 유지하려면 필요에 따라 `auto&`, 읽기 전용 참조는 `const auto&`로 표현한다.
- `nullptr`는 널 포인터를 나타내는 C++ 타입 있는 값이다. C++ 오버로드 문맥에서는 `0`이나 전통적인 `NULL`보다 의도가 명확하다.

```cpp
auto copy = sensor;        // 복사 가능한 타입이라면 객체 복사
auto& alias = sensor;      // 같은 객체 참조
const auto& view = sensor; // 수정하지 않는 참조
```

드라이버처럼 복사가 허용되지 않아야 하는 타입에서는 다음 절의 `= delete`가 실수를 막을 수 있다.

## 16. C++ RAII, 소유권, 복사와 이동

### 16.1 RAII는 자원 정리를 객체 수명에 연결한다

동적 메모리만 자원이 아니다. 버스 잠금, 칩 선택 상태, 임시 클럭 사용권도 자원이다.

다음은 **task 문맥의 mutex성 잠금**을 가정한 예제다. `platform_lock()`은 반환하면 잠금을 획득한 상태이고 `platform_unlock()`은 실패하지 않는다고 가정한다. ISR에서 쓰는 예제가 아니다.

```cpp
struct PlatformMutex;
void platform_lock(PlatformMutex*) noexcept;
void platform_unlock(PlatformMutex*) noexcept;

class BusLock {
public:
    explicit BusLock(PlatformMutex& mutex) noexcept : mutex_(mutex)
    {
        platform_lock(&mutex_);
    }

    ~BusLock() noexcept
    {
        platform_unlock(&mutex_);
    }

    BusLock(const BusLock&) = delete;
    BusLock& operator=(const BusLock&) = delete;
    BusLock(BusLock&&) = delete;
    BusLock& operator=(BusLock&&) = delete;

private:
    PlatformMutex& mutex_;
};
```

아래 호출부는 `prepare()`와 `apply()`를 제공하는 센서 API가 있다고 가정한 제어 흐름 예제다.

```cpp
DrvStatus configure_with_lock(PlatformMutex& mutex, Sensor& sensor)
{
    BusLock lock{mutex};

    auto status = sensor.prepare();
    if (status != DRV_OK) {
        return status;  // lock의 소멸자가 잠금을 해제한다.
    }

    return sensor.apply(); // 여기서 반환할 때도 해제한다.
}
```

소멸자가 실행되는 정상적인 스코프 종료 경로에 정리를 연결하므로 조기 반환마다 unlock을 반복하지 않아도 된다. 하드 리셋이나 프로세스 강제 종료 같은 모든 상황에서 소멸자가 실행된다는 뜻은 아니다. RAII는 객체 수명과 자원 관리를 연결하는 C++의 대표적인 설계 방식이다. [C++ Core Guidelines: R.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-raii)

### 16.2 드라이버를 복사하면 무엇이 중복되는가

```cpp
Sensor second = first;
```

기본 복사가 허용되어 있으면 내부 포인터, 주소, 상태 값 등이 복사된다. 하지만 실제 장치, IRQ 등록, DMA 요청, mutex가 독립적으로 새로 생기는 것은 아니다.

자원 소유자이거나 주소 안정성이 필요한 드라이버는 다음과 같이 복사와 이동을 막을 수 있다.

```cpp
Sensor(const Sensor&) = delete;
Sensor& operator=(const Sensor&) = delete;
Sensor(Sensor&&) = delete;
Sensor& operator=(Sensor&&) = delete;
```

객체를 이동해도 괜찮은 설계라면 이동 생성자·이동 대입을 구현할 수 있다. 하지만 등록된 콜백의 `this` 주소, DMA 버퍼 주소, 외부 레지스트리의 포인터를 함께 처리해야 한다. 처음에는 진행 중인 비동기 작업을 가진 객체의 이동을 금지하는 편이 추적하기 쉽다.

### 16.3 `std::move`는 이동을 자동 구현하지 않는다

```cpp
auto next = std::move(current);
```

`std::move`는 객체를 이동 가능한 값 범주로 취급하도록 하는 캐스팅 성격의 도구다. 실제 동작은 선택된 생성자나 대입 연산자에 달려 있다. 이동 생성자가 장치 소유권을 어떻게 이전하는지는 클래스가 정해야 한다.

이동된 객체를 어떤 상태로 남길지, 소멸자가 무엇을 정리할지까지 설계되지 않았다면 `std::move`를 붙이는 것으로 해결되지 않는다.

### 16.4 비동기 종료는 소멸자만 생각하면 부족하다

객체를 파괴하기 전에 다음이 확정되어야 한다.

1. 새 요청 제출이 중단되었다.
2. IRQ·콜백 등록이 해제되거나 더 이상 해당 객체에 접근하지 않는다.
3. 진행 중 DMA가 완료되었거나 안전하게 취소되었다.
4. 이미 실행 중인 콜백이 끝났다.
5. 그 뒤 버퍼와 객체를 파괴한다.

시간이 걸리거나 실패할 수 있는 정리는 명시적인 `stop()`/`shutdown()`으로 처리하고, 소멸자에는 확인 가능한 짧은 정리만 남기는 설계도 가능하다. 어떤 방식이든 수명이 끝난 객체를 콜백이 다시 접근하면 안 된다. [SEI CERT EXP54-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/)

## 17. C++ 객체를 C 콜백에 연결하기

### 17.1 비정적 멤버 함수는 일반 함수 포인터와 다르다

```cpp
class Reader {
public:
    void on_complete(int status) noexcept;
};
```

`&Reader::on_complete`의 타입은 멤버 함수 포인터다. 호출하려면 어느 `Reader` 객체인지도 필요하다. 따라서 이것을 `void (*)(void*, int)`와 같은 C 콜백 슬롯에 그대로 넣지 않는다.

함수 포인터는 실행할 함수를 가리키고 `void *user`는 객체를 가리키도록 분리할 수 있다.

### 17.2 C 연결 shim을 사용하는 명시적인 예제

다음은 C/C++ 공용 API가 이러한 형식을 제공한다고 가정한다.

```cpp
extern "C" {
    typedef void Completion(void* user, int status);
    void platform_register_completion(Completion* fn, void* user);
}
```

```cpp
class Reader {
public:
    void on_complete(int status) noexcept
    {
        /* 실행 문맥에 맞는 완료 처리 또는 이벤트 전달 */
        (void)status;
    }
};

extern "C" void reader_completion(void* user, int status) noexcept
{
    if (user == nullptr) {
        return;
    }
    auto* reader = static_cast<Reader*>(user);
    reader->on_complete(status);
}

void register_reader(Reader& reader)
{
    platform_register_completion(&reader_completion, &reader);
}
```

호출 흐름은 다음과 같다.

```text
C 플랫폼 → reader_completion(user, status)
          → user를 Reader*로 복원
          → reader->on_complete(status)
```

이 방식은 함수 타입, C 언어 연결, 객체 포인터를 명시적으로 분리한다. 실제 벤더 콜백 typedef에 요구되는 호출 규약과 언어 연결을 따른다. 일부 API는 C++ 정적 멤버 함수를 직접 등록하는 방식을 지원하지만 모든 ABI의 일반 규칙으로 가정하지 않는다.

`Reader`는 콜백 등록이 유효한 동안 같은 주소에서 살아 있어야 한다. 등록 해제와 진행 중 콜백의 종료 조건 없이 지역 `Reader`를 넘겨 두고 함수에서 반환하면 안 된다.

### 17.3 lambda와 capture

```cpp
auto add_one = [](int value) { return value + 1; };
int (*fn)(int) = add_one;  // capture 없는 lambda는 호환 함수 포인터로 변환 가능
```

```cpp
int bias = 3;
auto add_bias = [bias](int value) { return value + bias; };
/* add_bias는 bias 상태를 담는다. 일반 함수 포인터로 직접 바꿀 수 없다. */
```

캡처 없는 lambda가 제공하는 함수 포인터 변환과 캡처하는 closure의 상태 보관은 다르다. C++17 규정에서도 캡처 없는 lambda의 변환을 별도로 정의한다. [C++17 공개 초안 소스: expressions, expr.prim.lambda](https://raw.githubusercontent.com/cplusplus/draft/n4659/source/expressions.tex)

특히 `[this]`는 객체 전체를 안전하게 복사해 소유하는 뜻이 아니다. 객체 주소를 보관하므로 원래 객체가 먼저 파괴되면 문제가 된다. `[&local]`도 지역 변수를 참조할 뿐 수명을 자동으로 연장하지 않는다.

C API에는 앞 절의 함수+context 패턴을 쓰고, C++ 내부에서는 필요한 소유권과 수명 정책에 맞는 callable을 선택한다.

### 17.4 `std::function`은 편리하지만 비용과 정책을 확인한다

`std::function<void(int)>`은 다양한 callable을 보관할 수 있다. 다만 구현과 캡처 크기에 따라 동적 할당이 발생할 수 있고, 호출 간접화와 수명 관리가 추가된다. 모든 임베디드 C++ 표준 라이브러리 구성이 이 기능을 동일하게 제공하는 것도 아니다.

고정된 콜백 하나에는 함수 포인터와 context가 더 단순할 수 있다. 반대로 애플리케이션 계층에서 라이브러리와 메모리 정책이 허용되면 `std::function`이 사용성을 높일 수 있다.

## 18. C++의 정적 다형성과 동적 다형성

### 18.1 C의 함수 표와 C++ virtual 함수

```cpp
class RegisterBusInterface {
public:
    virtual ~RegisterBusInterface() = default;

    virtual DrvStatus read(std::uint8_t address7,
                           std::uint8_t reg,
                           std::uint8_t* dst,
                           std::size_t length) noexcept = 0;
};
```

서로 다른 버스 구현을 같은 인터페이스로 다룰 수 있다. C의 `ops` 테이블이 수행한 역할을 C++ 언어가 지원하는 형태로 표현한 것이다.

virtual 함수를 쓴다고 반드시 heap을 써야 하는 것은 아니다. 구현 객체를 정적으로 또는 지역 변수로 만들고 참조로 전달할 수도 있다. 다만 호출 간접화, 객체 레이아웃, 링크 결과 등의 비용은 컴파일러와 최적화에 따라 확인한다.

기반 클래스 포인터로 파생 객체를 파괴하는 소유 구조라면 virtual 소멸자가 필요하다. 소유하지 않는 인터페이스라면 파괴 정책을 별도로 명확히 해야 한다.

### 18.2 템플릿으로 구현 타입을 컴파일 시 결정하기

```cpp
template<class Bus>
class TypedSensor {
public:
    TypedSensor(Bus& bus, std::uint8_t address7) noexcept
        : bus_(bus), address7_(address7)
    {
    }

    [[nodiscard]] DrvStatus read_id(std::uint8_t& out) noexcept
    {
        std::uint8_t next = 0;
        auto status = bus_.read(address7_, 0x0F, &next, 1);
        if (status == DRV_OK) {
            out = next;
        }
        return status;
    }

private:
    Bus& bus_;
    std::uint8_t address7_;
};
```

이 예제의 `Bus`는 해당 `read()`를 제공하고 예외를 밖으로 내보내지 않는다는 계약을 만족해야 한다. `Bus` 객체의 수명은 센서보다 길어야 한다.

`TypedSensor<Stm32Bus>`와 `TypedSensor<FakeBus>`는 서로 다른 타입이 된다. 컴파일러가 구체적 구현을 알아 인라인 최적화를 할 여지가 생기지만, 타입 조합마다 코드가 생성되어 코드 크기가 커질 수도 있다.

| 방식 | 선택 시점 | 장점 | 확인할 비용 |
|---|---|---|---|
| C 함수 포인터 + context | 보통 초기화/런타임 | C 호환, 간단한 ABI, 시험용 교체 | 간접 호출, context 타입 계약 |
| C++ virtual | 런타임 | 언어 수준 인터페이스·오버라이드 | 간접 호출, 객체 레이아웃 |
| C++ template | 컴파일 시 | 구체 타입 검사, 인라인 기회 | 코드 크기, 헤더 의존성, 오류 메시지 |

첫 드라이버에서 어느 하나가 무조건 정답은 아니다. 기존 프로젝트가 사용하는 방식, C API 필요 여부, 메모리·플래시 제약, 시험 전략에 맞춰 고른다.

### 18.3 `std::array`, `std::span`, `std::bit_cast`의 버전 구분

```cpp
std::array<std::uint8_t, 6> bytes{};  // C++17에서 사용 가능
auto status = bus.read(address7, reg, bytes.data(), bytes.size());
```

`std::array`는 고정 크기 배열을 값 타입으로 다루며 `.data()`와 `.size()`를 제공한다. heap 할당을 필수로 하지 않는다.

`std::span`과 `std::bit_cast`는 **C++20 기능**이다. C++17 학습 자료나 펌웨어에 표준 기능인 것처럼 바로 넣지 않는다. C++17 프로젝트는 자체 버퍼 뷰, 검증된 지원 라이브러리 또는 포인터+길이 인터페이스를 사용할 수 있다.

## 19. 코드에서 자주 만나는 문법 빠른 참조

| 문법 | 읽는 방법 | 드라이버에서 연결되는 개념 |
|---|---|---|
| `typedef struct { ... } T;` | 구조체 타입에 T라는 별칭 | 장치 상태·설정·측정 결과 |
| `T *p` | T 객체를 가리키는 포인터 | 장치 handle |
| `&object` | 객체의 주소 | 초기화 인자·출력 인자 |
| `p->member` | 가리키는 객체의 멤버 | 장치별 버스·주소·교정값 |
| `*out = next` | 출력 객체에 값 대입 | 성공 시 결과 전달 |
| `T **out` | 포인터 변수의 주소 | 객체 풀의 handle 반환 |
| `const T *p` | 이 경로로 원본을 수정하지 않음 | 입력 버퍼·설정 |
| `volatile T *p` | 특별한 접근 규약의 객체 | MMIO |
| `static` 함수 | 파일 내부 함수 | 레지스터 도우미 은닉 |
| `static` 지역 변수 | 호출 사이에 살아 있는 공유 객체 | 필요할 때만 공통 상태 |
| `extern T object` | 다른 곳의 객체 정의를 선언 | 보드가 보유한 HAL 객체 |
| `enum` | 이름 붙인 유한 값 | 상태·오류·설정 코드 |
| `union` + 태그 | 여러 형태 중 하나 저장 | 이벤트 메시지 |
| `(*fn)(...)` | 함수 포인터 | 버스 구현·완료 콜백 |
| `void *context` | 타입을 지운 객체 주소 | 플랫폼 상태 연결 |
| `uint8_t buffer[N]` | N개 바이트 저장 공간 | 수신·송신 프레임 |
| `sizeof buffer` | 현재 식의 타입 크기 | 배열이 보이는 위치에서 바이트 수 |
| `(value & mask) >> shift` | 필드 추출 | 상태·설정 디코딩 |
| `(old & ~mask) \| field` | 필드 교체 | 일반 RW 설정값 생성 |
| `_Static_assert` | C 컴파일 시 조건 확인 | 폭·레이아웃 전제 |
| `T&`, `const T&` | C++ 참조 | 필수 객체·읽기 전용 입력 |
| `explicit` | 암시적 생성자 변환 제한 | 자원 래퍼·장치 생성 |
| `noexcept` | 예외를 밖으로 내보내지 않음 | 오류 코드 API·콜백 |
| `= delete` | 특정 연산 금지 | 드라이버 복사·이동 제한 |
| 소멸자 `~T()` | 객체 수명 종료 처리 | 잠금·자원 정리 |
| `enum class` | 범위와 강한 타입을 가진 enum | 서로 다른 설정 타입 분리 |
| `constexpr` | 상수 식에 사용할 수 있는 정의 | 레지스터 상수·계산 |
| `template<class Bus>` | 버스 타입을 매개변수로 받음 | 정적 의존성 주입 |
| `extern "C"` | C 언어 연결 | C 라이브러리·HAL과 연결 |

## 20. 직접 설명해 보면 이해가 확인되는 질문

### 첫날 확인

1. `sensor_read(&sensor, &sample)`에서 두 `&`는 각각 어느 객체의 주소인가?
2. `sensor->bus.ops->read(...)`는 어떤 순서로 객체와 함수를 찾아가는가?
3. `void *context`를 왜 함수 포인터와 함께 저장하는가?
4. `DemoSensor copy = original;`을 하면 버스 컨트롤러도 복제되는가?
5. `uint8_t data[6]`를 함수 인자로 받았을 때 `sizeof data`가 왜 6이 아닐 수 있는가?
6. `const uint8_t *`와 `uint8_t *const` 중 어느 쪽이 수신 버퍼 입력에 더 적합한가?
7. `(reg & MASK) >> SHIFT`의 각 단계는 데이터시트의 어느 부분을 표현하는가?
8. 읽기에 실패했는데 `sample.value`를 그대로 출력하면 어떤 문제가 생기는가?

### 첫 드라이버를 완성하기 전 확인

1. 구조체에 저장한 모든 포인터의 실제 객체와 소유자를 이름으로 말할 수 있는가?
2. 센서를 하나 더 추가할 때 공유하면 안 되는 `static` 상태가 있는가?
3. 버스 API가 동기인지 비동기인지 알고 있는가?
4. 설정 코드와 실제 단위 변환 계수가 함께 바뀌는가?
5. 각 버퍼의 길이는 바이트 수인지 원소 수인지 분명한가?
6. signed 원시값을 데이터시트의 유효 비트 수와 정렬에 따라 해석하는가?
7. C17 문법과 C++17 문법을 같은 파일에 혼합하지 않았는가?
8. 콜백이 있는 객체를 파괴하거나 이동하기 전에 무엇을 끝내야 하는가?

## 21. 권장 학습 실습

1. **포인터 실습:** `set_address(DemoSensor*)`와 `change_copy(DemoSensor)`를 작성하고 호출자의 값이 어떻게 달라지는지 예측한다.
2. **버스 교체 실습:** 5절의 fake 버스에서 장치 주소 불일치와 읽기 실패를 반환하게 하고 출력 객체가 어떻게 유지되어야 하는지 확인한다.
3. **비트 실습:** `[5:3]` 필드의 모든 코드 0–7을 넣었다가 다시 추출한다. 필드 밖 비트가 유지되는지도 확인한다.
4. **바이트 실습:** `0x00 0x80`, `0xFF 0x7F`, `0xFF 0xFF`를 signed little-endian으로 해석한다.
5. **수명 실습:** 장치 구조체 안에 있는 모든 포인터를 그림으로 그리고 각 대상 객체가 언제 사라지는지 표시한다.
6. **C++ 실습:** 복사 금지 Sensor를 만들고 복사를 시도했을 때 컴파일러가 막도록 한다. callback의 `this` 주소와 이동 허용 여부를 연결해 설명한다.

실습의 목적은 어려운 문법을 많이 쓰는 것이 아니라, 데이터시트의 비트와 코드의 객체·함수·수명이 어떻게 연결되는지 손으로 따라갈 수 있게 되는 것이다.

## 22. 출처와 적용 범위

본문 예제와 설명 흐름은 이 학습 자료를 위해 작성한 것이다. 세부 언어 규칙과 플랫폼 접근 규약을 확인할 수 있도록 주장 가까이에 1차 출처를 연결했다.

- **언어 규칙:** [WG14 공개 C11 초안 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf), [WG21의 C++ 표준 문서 안내](https://www.open-std.org/jtc1/sc22/wg21/), [C++17 N4659 공식 초안 저장소](https://github.com/cplusplus/draft/tree/n4659). N1570은 C17 최종 표준 자체가 아니며, 이 문서에서 설명한 공통 기초 규칙의 공개 참고 자료로 사용했다. 기준 코드는 C17/C++17이다.
- **C 위험 지점 확인:** [SEI CERT C 규칙](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/). 본문에서 함수 타입, 배열 크기, 객체 수명, 별칭, shift 항목을 연결했다.
- **컴파일러 접근 규약:** [GCC Volatiles](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html). 다른 컴파일러를 쓰면 해당 벤더 문서도 확인해야 한다.
- **Arm 명령·캐시 함수:** [CMSIS CPU Intrinsics](https://arm-software.github.io/CMSIS_6/v6.0.0/Core/group__intrinsic__CPU__gr.html), [CMSIS Cache Functions](https://arm-software.github.io/CMSIS_6/latest/Core/group__cache__functions__m7.html). 개별 STM32 제품의 DMA와 캐시 처리 순서를 대신하는 자료는 아니다.
- **C++ 자원 설계:** [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). 언어 표준과 구별되는 설계 지침이며, 임베디드 환경의 자원·실시간·라이브러리 제약에 맞춰 적용한다.

완전한 문법 목록보다 첫 드라이버의 실제 호출 흐름을 설명할 수 있는 것이 우선이다. `장치 객체 → 버스 함수 → 수신 버퍼 → 원시값 → 물리 단위 → 호출자의 결과`를 이해한 뒤, ISR·DMA·C++ 자원 관리로 확장하면 된다.
