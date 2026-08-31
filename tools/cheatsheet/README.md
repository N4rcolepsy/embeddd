# 자료 재생성

`cheatsheet_content.py`는 기존 학습 페이지, `memory_io_pages.py`는 현재 업무용 메모리 접근 페이지다. 한 소스에서 PDF·Markdown·스니펫을 함께 만든다. builder는 Windows의 Malgun Gothic/Consolas 폰트를 사용한다. 다른 OS에서는 `build_cheatsheet.py`의 폰트 경로를 조정한다.

필요한 Python 패키지: `reportlab`, `pypdf`, `pdfplumber`. 루트에서 실행한다.

```sh
python tools/cheatsheet/build_all.py
python tools/verify_c.py --zig /path/to/zig
python tools/cheatsheet/verify_materials.py
```

PDF는 Poppler 등으로 **22쪽 전체를 렌더링하여 확인한 뒤에만** 다음을 실행한다.

```sh
python tools/cheatsheet/verify_materials.py --visual-reviewed
```

마지막 명령은 검증 기록과 두 ZIP 배포본을 갱신한다. C 검증은 가짜 메모리와 호스트 프로그램만 실행하고 실제 MMIO에 접근하지 않는다. `-UNDEBUG`를 주어 최적화 빌드에서도 assert 검사가 사라지지 않게 한다. 임시 컴파일러·실행 파일·렌더 이미지·Python cache는 Git에서 제외한다.

생성 순서 주의: 원문 수정 → 통합본/PDF/Markdown 생성 → 소스가 바뀌었다면 C 재검증 → 전체 PDF 확인 → 검증/패키징. `source-manifest.json`과 C 실행 결과의 해시가 현재 파일과 다른 경우 검증이 실패한다.
