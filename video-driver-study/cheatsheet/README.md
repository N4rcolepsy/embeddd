# 인쇄·구현용 치트시트

- [복사용 Markdown](embedded-driver-cheatsheet.md)
- [스니펫 목차](snippet-index.md): PDF의 S01~S39와 대응
- `existing/`: 앞서 만든 UART 템플릿, decode·가변 인자 실습 원본
- `source-manifest.json`: 압축 정리한 기존 Markdown 파일과 SHA-256

인쇄 PDF는 작업공간의 `output/pdf/embedded-driver-cheatsheet.pdf`에 있다. A4 세로, 100%/실제 크기, 긴 쪽 넘김 양면 인쇄를 권장한다. 16쪽이며 양면으로 8장이다. 얇은 회색 코드 배경과 청록색 표식은 흑백에서도 구분된다.

각 페이지를 독립적으로 찾아볼 수 있도록 용어와 전제를 일부 반복했다. 원문 전체의 주요 개념을 압축한 자료이며 원문을 그대로 재인쇄한 것은 아니다.

스니펫을 전부 하나의 소스에 합치지 않는다. `FRAGMENT`는 함수 안에 넣거나 실제 프로젝트 API로 완성해야 한다. HAL 태그는 부품·HAL 버전·보드 핀/클록 준비가 필요하다. C/C++ 컴파일과 실물 시험은 별도로 수행해야 한다.
