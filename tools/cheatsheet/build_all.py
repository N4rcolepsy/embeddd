"""Regenerate the complete Markdown and the PDF/Markdown cheatsheet."""
from pathlib import Path
import json
import build_cheatsheet as build

ROOT = Path(__file__).resolve().parents[2]
STUDY = ROOT/'video-driver-study'

intro = '''# 임베디드 드라이버 개발 온보딩 — 통합본

현재 업무는 **주소와 memory read/write 명령만으로 직접 구현하는 드라이버**다. 먼저 07과 `examples/memory_io/`를 읽고, 데이터시트의 구체적 설계는 03, 문법은 04에서 찾아본다. 아래에는 00~07의 전체 내용을 수록했다.

| 부분 | 내용 |
|---|---|
| 00 | 영상 기반 온보딩 경로 |
| 01 | 영상 시간대별 상세 학습 노트 |
| 02 | 영상 코드의 디자인 패턴 |
| 03 | 데이터시트·레지스터 맵 기반 공통 패턴 |
| 04 | C/C++ 핵심 문법 |
| 05 | STM32 HAL/LL 비교 학습 |
| 06 | 이전 HAL UART 사례와 C 가변 인자 |
| 07 | 현재 업무: 메모리 read/write로 직접 구현 |

[개별 문서 목차](README.md) · [22쪽 치트시트](cheatsheet/README.md) · [메모리 드라이버 템플릿](examples/memory_io/README.md) · [출처와 검증 범위](sources/README.md)

영상은 [Phil's Lab #30](https://www.youtube.com/watch?v=_JQAve05o_0)이다. 영상 직접 내용과 확장 설계를 구분한다. 새 메모리 UART는 실제 칩 map이 없는 교육용 모델이다. memory_io·decode·가변 인자 호스트 C 예제는 빌드·실행했으며, HAL 프로젝트·C++·실물은 검증하지 않았다. 개별 본문의 과거 미실행 기록은 sources/README의 2026-09-01 추가 검증으로 갱신했다.
'''
parts = sorted(STUDY.glob('0[0-7]-*.md'))
assert len(parts) == 8
(STUDY/'driver-onboarding-complete.md').write_text(
    intro+'\n\n---\n\n'+'\n\n---\n\n'.join(p.read_text(encoding='utf-8').strip() for p in parts)+'\n',
    encoding='utf-8', newline='\n')
qa = build.render_pdf()
build.export_text()
(build.OUT/'layout-check.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps({'pages':len(qa), 'source_chapters':len(parts), 'pdf':str(build.PDF)}))
