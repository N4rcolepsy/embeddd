"""Check links, source hashes, PDF bounds, test provenance, then package files.

Pass --visual-reviewed only after reviewing every final PDF page rendering.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import zipfile
from urllib.parse import unquote
from pypdf import PdfReader
import pdfplumber

ROOT = Path(__file__).resolve().parents[2]
STUDY = ROOT/'video-driver-study'
OUT = STUDY/'cheatsheet'
PDF = ROOT/'output/pdf/embedded-driver-cheatsheet.pdf'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--visual-reviewed', action='store_true')
    args = parser.parse_args()
    broken = []
    for path in STUDY.rglob('*.md'):
        value = path.read_text(encoding='utf-8')
        value = re.sub(r'^```[^\n]*\n.*?^```[^\n]*$', '', value, flags=re.M|re.S)
        value = re.sub(r'`[^`\n]+`', '', value)
        for link in re.findall(r'\[[^\]\n]*\]\(([^)\n]+)\)', value):
            link = link.strip('<>')
            if re.match(r'^(https?://|#|mailto:)', link):
                continue
            target = unquote(link.split('#')[0])
            if target and not (path.parent/target).exists():
                broken.append([str(path.relative_to(STUDY)),target])
    assert not broken, broken
    sources = json.loads((OUT/'source-manifest.json').read_text(encoding='utf-8'))
    for item in sources:
        assert hashlib.sha256((STUDY/item['path']).read_bytes()).hexdigest() == item['sha256'], item['path']
    original_sources = json.loads((STUDY/'sources/source-hashes.json').read_text(encoding='utf-8'))
    for item in original_sources:
        assert hashlib.sha256((STUDY/'sources'/item['file']).read_bytes()).hexdigest() == item['sha256'].lower()
    snippets = json.loads((OUT/'snippet-manifest.json').read_text(encoding='utf-8'))
    assert len(snippets) == len({s['id'] for s in snippets}) == 50
    for item in snippets:
        assert (OUT/'snippets'/item['file']).is_file()
    tests = json.loads((STUDY/'examples/memory_io/verification.json').read_text(encoding='utf-8'))
    for result in tests['results']:
        assert result['compile_exit'] == result['run_exit'] == 0
        assert '-UNDEBUG' in result['flags'], 'assert tests must remain active'
        for name, digest in result['source_sha256'].items():
            assert hashlib.sha256((STUDY/'examples'/name).read_bytes()).hexdigest() == digest, name
    reader = PdfReader(PDF)
    assert len(reader.pages) == 22
    for i,p in enumerate(reader.pages,1):
        assert abs(float(p.mediabox.width)-595.28) < 1
        assert abs(float(p.mediabox.height)-841.89) < 1
        content = p.extract_text()
        assert len(content)>500 and '\ufffd' not in content, i
    with pdfplumber.open(PDF) as doc:
        for i,p in enumerate(doc.pages,1):
            assert all(c['x0']>=30 and c['x1']<=p.width-30 for c in p.chars), i
            assert all(c['top']>=8 and c['bottom']<=p.height-15 for c in p.chars), i
    report = {
        'date':'2026-09-01', 'pages':len(reader.pages), 'snippets':len(snippets),
        'source_markdowns':len(sources), 'markdown_links':'PASS', 'source_hashes':'PASS',
        'pdf_text_bounds':'PASS', 'c_test_source_hashes':'PASS', 'original_video_sources':'SHA-256 preserved',
        'c_execution':[r['name']+': PASS' for r in tests['results']],
        'visual_review':('all 22 final pages reviewed' if args.visual_reviewed else 'pending'),
        'hardware':'Not performed; actual chip map and backend not provided',
        'not_compiled':'STM32 HAL project, C++ and all standalone snippet fragments',
        'pdf_sha256':hashlib.sha256(PDF.read_bytes()).hexdigest()
    }
    (OUT/'quality-check.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    if args.visual_reviewed:
        archive = ROOT/'embedded-driver-cheatsheet.zip'
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as package:
            for base in [STUDY, ROOT/'tools']:
                for p in sorted(base.rglob('*')):
                    if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc':
                        package.write(p,p.relative_to(ROOT).as_posix())
            package.write(PDF,PDF.relative_to(ROOT).as_posix())
            package.writestr('START-HERE.txt',
                '현재 업무: video-driver-study/07-memory-io-from-scratch.md\n'
                '치트시트: video-driver-study/cheatsheet/embedded-driver-cheatsheet.md\n'
                '인쇄 PDF: output/pdf/embedded-driver-cheatsheet.pdf (22쪽)\n'
                'C 코드: video-driver-study/examples/memory_io/\n'
                '재생성: tools/cheatsheet/README.md\n')
        with zipfile.ZipFile(archive) as package:
            assert package.testzip() is None
        (ROOT/'video-driver-study.zip').write_bytes(archive.read_bytes())
    print(json.dumps(report,ensure_ascii=False))


if __name__ == '__main__':
    main()
