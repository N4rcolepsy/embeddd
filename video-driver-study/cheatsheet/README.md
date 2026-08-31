# 인쇄·구현용 치트시트

- [인쇄용 PDF](../../output/pdf/embedded-driver-cheatsheet.pdf)
- [복사용 Markdown](embedded-driver-cheatsheet.md)
- [스니펫 목차](snippet-index.md): PDF의 S01~S50와 대응
- [현재 업무용 전체 템플릿](../examples/memory_io/README.md): ST HAL 없이 메모리 명령으로 구현
- [07 상세 설명](../07-memory-io-from-scratch.md): 주소·폭·RMW/W1C·초기화·FIFO·배리어
- `existing/`: 이전 STM32 UART 비교 템플릿, decode·가변 인자 실습 원본
- `source-manifest.json`: 압축 정리한 기존 Markdown 파일과 SHA-256

인쇄 PDF는 작업공간의 `output/pdf/embedded-driver-cheatsheet.pdf`에 있다. A4 세로, 100%/실제 크기, 긴 쪽 넘김 양면 인쇄를 권장한다. 22쪽이며 양면으로 11장이다. 얇은 회색 코드 배경과 청록색 표식은 흑백에서도 구분된다.

**2~7쪽이 현재 업무의 우선 경로다.** 주소와 메모리 read/write 명령만 사용하는 구현을 다룬다. 12~15쪽의 STM32 HAL/LL은 영상 비교와 문법 학습용이다. 실제 칩 map이 없으므로 새 UART 역시 교육용 모델이며 데이터시트에 맞춰 순서를 포팅해야 한다.

각 페이지를 독립적으로 찾아볼 수 있도록 용어와 전제를 일부 반복했다. 원문 전체의 주요 개념을 압축한 자료이며 원문을 그대로 재인쇄한 것은 아니다.

스니펫을 전부 하나의 소스에 합치지 않는다. `FRAGMENT`는 문맥에 넣거나 전체 소스 파일을 참고할 발췌다. HAL 태그는 부품·HAL 버전·보드 핀/클록 준비가 필요하다. 검증 범위는 각 예제 README와 quality-check.json에 구분했다. 실물 시험은 수행하지 않았다.
