# 원자료와 검토 범위

## 1. 영상 근거

- [YouTube 영상](https://www.youtube.com/watch?v=_JQAve05o_0): Phil's Lab #30, 화면 제목은 `(Sponsored) How To Write A Driver (STM32, I2C, Datasheet)`.
- 영상 페이지에 표시된 게시일: 2021-08-20. 길이: 약 38분 20초.
- 제공 링크의 시작점: 1298초, 즉 21:38. 노트는 전체 영상을 대상으로 한다.
- YouTube의 영어 자동 생성 자막을 00:01부터 38:19까지 읽었다. 자동 자막의 부품명·타입 이름·연산 용어는 코드와 문맥을 대조해 정리했다.
- 설명란의 공식 챕터를 확인했다. 가속도 변환 구간의 실제 코드 화면도 재확인했다. 전 구간을 연속 재생하거나 모든 화면을 프레임별로 검토한 것은 아니다.
- 대본 전체는 결과물에 재배포하지 않는다. 문서는 기술적인 학습 흐름을 재구성한 요약·분석이다.

| 공식 챕터 | 시작 |
|---|---:|
| Introduction | 00:00 |
| Sensor | 00:28 |
| Altium Designer 광고 | 01:22 |
| Sensor Board Schematic and PCB | 01:42 |
| STM32F4 Schematic | 03:38 |
| STM32CubeIDE Setup | 04:16 |
| Basic Project Structure | 06:40 |
| Driver Header File | 07:55 |
| Low-Level Functions | 16:11 |
| Sensor Initialisation and Setup | 18:41 |
| Temperature Measurement | 26:18 |
| Acceleration Measurements | 30:21 |
| Testing the Driver | 34:10 |

## 2. 제작자의 공개 코드

원자료: [LittleBrain_DriverExample](https://github.com/pms67/LittleBrain-STM32F4-Sensorboard/tree/master/LittleBrain_DriverExample).

2026-08-31에 다음 공개 `master` 파일을 읽기 전용으로 내려받아 비교했다. 이 사본의 파일명 앞에만 `original-`을 붙였으며 내용은 수정하지 않았다. 원저작자와 원래 주석·라이선스 표기를 유지했다. 이 폴더는 출처 추적용이며, 완성된 빌드 프로젝트가 아니다. 실제 재사용·재배포 시 저장소와 각 파일의 라이선스를 별도로 확인한다.

| 비교 사본 | 원본 |
|---|---|
| `original-ADXL355.h` | [Core/Inc/ADXL355.h](https://github.com/pms67/LittleBrain-STM32F4-Sensorboard/blob/master/LittleBrain_DriverExample/Core/Inc/ADXL355.h) |
| `original-ADXL355.c` | [Core/Src/ADXL355.c](https://github.com/pms67/LittleBrain-STM32F4-Sensorboard/blob/master/LittleBrain_DriverExample/Core/Src/ADXL355.c) |
| `original-main.c` | [Core/Src/main.c](https://github.com/pms67/LittleBrain-STM32F4-Sensorboard/blob/master/LittleBrain_DriverExample/Core/Src/main.c) |

이 소스는 영상 녹화 시점의 스냅샷이라고 단정할 수 없다. 현재 소스의 가속도 부호 변환 코드와 측정 호출 위치는 영상의 설명/화면과 다르다. 따라서 01은 영상의 진행을, 02의 새로운 교육 예제는 설계 원리를 설명한다.

로컬 비교 사본의 SHA-256은 `source-hashes.json`에 기록했다. 원격 URL은 변경될 수 있으므로 URL과 조회일, 실제 내려받은 파일을 함께 보관한다.

## 3. 주요 추가 자료

아래는 기본 근거이며, 03·04의 세부 문법과 설계 설명에는 해당 절 가까이에 추가 공식 출처가 연결돼 있다.

| 자료 | 활용 |
|---|---|
| [Analog Devices ADXL354/ADXL355 데이터시트](https://www.analog.com/media/en/technical-documentation/data-sheets/adxl354_adxl355.pdf) | Rev. D, 문서 개정 이력상 2025-06. 레지스터 의미·데이터 형식·설정 조건 확인 |
| [제조사 온도 기준값 정정 답변](https://ez.analog.com/mems/f/q-a/575682/adxl355-temperature-sensor-output-at-25c-1852-or-1885-lsb) | 영상의 1852와 현행 1885 LSB 차이 구분 |
| [STM32F4 HAL I²C 구현](https://github.com/STMicroelectronics/stm32f4xx-hal-driver/blob/master/Src/stm32f4xx_hal_i2c.c) | 핸들, 주소 형식, 메모리 읽기 인수 계약 |
| [NXP I²C 규격](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) | 주소·버스 거래·repeated START 용어 |
| [Analog Devices no-OS ADXL355](https://github.com/analogdevicesinc/no-OS/tree/main/drivers/accel/adxl355) | 영상 다음 단계에서 볼 수 있는 제조사 드라이버 구조 |

현행 데이터시트의 페이지 번호는 영상에서 언급하는 과거 판과 다르다. 페이지 숫자만 찾지 말고 `Register Map`, `FILTER`, `POWER_CTL`, `Temperature Data Registers` 같은 절 이름을 함께 찾는다.

## 4. 예제 검증 범위

- `examples/measurement_decode.c`: 이 자료에서 새로 작성한 교육용 C17 예제. 버스 전송·초기화·인터럽트를 포함하지 않는다.
- `examples/uart_template.h`, `uart_template.c`, `uart_usage.c`: 기존 STM32F4 HAL의 공개 인터페이스를 바탕으로 작성한 8N1 polling 송신 학습 템플릿. 보드별 클록·핀·MSP 초기화와 HAL 프로젝트가 필요하며 컴파일·실물 검증은 하지 않았다.
- `examples/variadic_demo.c`: UART와 독립적인 C17 가변 인자 실습. 호스트의 표준 출력용이며 이 환경에서는 컴파일·실행하지 않았다.
- `examples/verify_decoding.py`: 같은 해독 원리의 수학적 모델을 검사한다. 20비트 전체 패턴 1,048,576개, 가속도의 무시할 하위 비트 대표 조합 112개, 온도 12비트와 상위 예약 비트 조합 65,536개, 대표 단위 변환을 확인했다.
- 모델 검사 결과는 `verification.json`에 저장했다. 모두 통과했다.
- 이 환경에는 확인 가능한 C/C++ 컴파일러가 없어 C/C++ 컴파일·실행 검증은 하지 않았다. Python 모델 검사가 C 컴파일러 검증을 대신한다는 뜻은 아니다.
- STM32 보드, ADXL355 실물, 로직 애널라이저를 사용한 시험은 수행하지 않았다. 초기 동작 확인은 사용자 환경에서 별도로 해야 한다.
- 문서의 개념 예제는 각 절의 설명을 위한 코드 조각이다. 모두 이어 붙여 하나의 완성 드라이버가 되는 것은 아니다.

자료를 실제 개발에 적용할 때는 팀의 SDK 버전, 코딩 규칙, 동기화 방식, 부품 판과 회로도를 기준으로 조정한다.
