"""Build A4 print PDF, Markdown and snippet files from one curated source."""
from pathlib import Path
import hashlib
import html
import json
import re
import shutil
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor, Color
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from cheatsheet_content import PAGES, SOURCE_URLS

ROOT = Path(__file__).resolve().parents[2]
STUDY = ROOT / 'video-driver-study'
OUT = STUDY / 'cheatsheet'
PDF = ROOT / 'output/pdf/embedded-driver-cheatsheet.pdf'
OUT.mkdir(parents=True, exist_ok=True)
PDF.parent.mkdir(parents=True, exist_ok=True)

pdfmetrics.registerFont(TTFont('Ko', 'C:/Windows/Fonts/malgun.ttf'))
pdfmetrics.registerFont(TTFont('KoBold', 'C:/Windows/Fonts/malgunbd.ttf'))
pdfmetrics.registerFont(TTFont('Mono', 'C:/Windows/Fonts/consola.ttf'))
pdfmetrics.registerFont(TTFont('MonoBold', 'C:/Windows/Fonts/consolab.ttf'))
pdfmetrics.registerFontFamily('Ko', normal='Ko', bold='KoBold')
pdfmetrics.registerFontFamily('Mono', normal='Mono', bold='MonoBold')

INK = HexColor('#182F3B')
TEAL = HexColor('#14796F')
GREY = HexColor('#596971')
LINE = HexColor('#CCD7DB')
PALE = HexColor('#F1F5F6')
WHITE = HexColor('#FFFFFF')
FONT = 9.0
LEADING = 13.2
CODE = 8.05
CODE_LEADING = 10.3
MARGIN = 32
GUTTER = 16
W, H = A4
COL = (W - 2*MARGIN - GUTTER)/2
TOP = H - 98
BOTTOM = 53
AVAILABLE = TOP - BOTTOM

styles = {
    'body': ParagraphStyle('body', fontName='Ko', fontSize=FONT,
        leading=LEADING, textColor=INK, wordWrap='CJK'),
    'note': ParagraphStyle('note', fontName='Ko', fontSize=7.9,
        leading=10.8, textColor=GREY, wordWrap='CJK'),
    'table': ParagraphStyle('table', fontName='Ko', fontSize=8.65,
        leading=12.2, textColor=INK, wordWrap='CJK'),
    'tablekey': ParagraphStyle('tablekey', fontName='KoBold', fontSize=8.35,
        leading=12.0, textColor=INK, wordWrap='CJK'),
    'footer': ParagraphStyle('footer', fontName='Ko', fontSize=7.0,
        leading=9.0, textColor=GREY, wordWrap='CJK'),
}

def clean(s):
    # Print skill: ordinary hyphens, no special dash glyphs.
    return s.replace('\u2011','-').replace('\u2013','-').replace('\u2014','-')

def para(s, style='body', width=COL):
    p=Paragraph(html.escape(clean(s)), styles[style])
    _,h=p.wrap(width, 10000)
    return p,h

def card_layout(d, width):
    blocks=[]
    y=23 if not d.get('sid') else 37
    if d.get('text'):
        p,h=para(d['text'],width=width)
        blocks.append(('p',p,y,h,0));y+=h+6
    for s in d.get('bullets') or []:
        p,h=para(s,width=width-10)
        blocks.append(('bullet',p,y,h,10));y+=h+3
    if d.get('rows'):
        rows=[]
        for a,b in d['rows']:
            rows.append([para(a,'tablekey',width*0.32-9)[0],
                         para(b,'table',width*0.68-9)[0]])
        table=Table(rows,colWidths=[width*.32,width*.68],hAlign='LEFT')
        table.setStyle(TableStyle([
            ('VALIGN',(0,0),(-1,-1),'TOP'),
            ('LEFTPADDING',(0,0),(-1,-1),4),
            ('RIGHTPADDING',(0,0),(-1,-1),4),
            ('TOPPADDING',(0,0),(-1,-1),5),
            ('BOTTOMPADDING',(0,0),(-1,-1),5),
            ('LINEBELOW',(0,0),(-1,-1),.35,LINE),
            ('BACKGROUND',(0,0),(0,-1),PALE),
        ]))
        _,h=table.wrap(width,10000)
        blocks.append(('table',table,y,h,0));y+=h+6
    if d.get('code'):
        lines=d['code'].splitlines()
        longest=max(pdfmetrics.stringWidth(l,'Mono',CODE) for l in lines)
        if longest>width-14:
            raise ValueError(f"Code line too wide {d['sid']}: {longest:.1f}>{width-14:.1f}")
        h=len(lines)*CODE_LEADING+14
        blocks.append(('code',lines,y,h,0));y+=h+5
    if d.get('needs'):
        p,h=para(d['needs'],'note',width=width)
        blocks.append(('p',p,y,h,0));y+=h+3
    return blocks,y+7

def draw_card(c,d,x,top,width):
    blocks,total=card_layout(d,width)
    c.setFillColor(TEAL)
    c.rect(x,top-12,3,11,fill=1,stroke=0)
    c.setFillColor(INK); c.setFont('KoBold',10.4)
    c.drawString(x+8,top-11,clean(d['title']))
    if d.get('sid'):
        c.setFont('MonoBold',7.7);c.setFillColor(TEAL)
        c.drawString(x,top-27,d['sid'])
        c.setFont('Ko',7.7)
        c.drawString(x+24,top-27,clean(d['mode']))
        c.setFillColor(GREY);c.setFont('Mono',7.1)
        c.drawRightString(x+width,top-27,d['file'])
    for kind,obj,offset,h,pad in blocks:
        base=top-offset-h
        if kind in ('p','bullet'):
            if kind=='bullet':
                c.setFillColor(TEAL);c.circle(x+2,top-offset-5,1.2,fill=1,stroke=0)
            obj.drawOn(c,x+pad,base)
        elif kind=='table':
            obj.drawOn(c,x,base)
        elif kind=='code':
            c.setFillColor(PALE);c.roundRect(x,base,width,h,4,fill=1,stroke=0)
            c.setFillColor(INK);c.setFont('Mono',CODE)
            for i,line in enumerate(obj):
                c.drawString(x+7,top-offset-8-CODE,i*0+line) if False else None
                c.drawString(x+7,top-offset-8-CODE-i*CODE_LEADING,line)
    return total

def render_pdf():
    c=canvas.Canvas(str(PDF),pagesize=A4,pageCompression=1)
    c.setTitle('임베디드 드라이버 구현 치트시트')
    c.setAuthor('Embedded driver onboarding notes')
    c.setSubject('A4 print reference: C17, C++17, STM32F4 HAL/LL, datasheet-driven drivers')
    qa=[]
    for page_no,p in enumerate(PAGES,1):
        heights=[sum(card_layout(d,COL)[1] for d in col) for col in p['columns']]
        if max(heights)>AVAILABLE:
            raise ValueError(f'Page {page_no} overflow {heights}, max {AVAILABLE:.1f}')
        c.setFillColor(TEAL);c.rect(0,H-8,W,8,fill=1,stroke=0)
        c.setFont('MonoBold',8);c.drawString(MARGIN,H-28,'EMBEDDED DRIVER / IMPLEMENTATION CHEATSHEET')
        c.setFillColor(INK);c.setFont('KoBold',22)
        c.drawString(MARGIN,H-57,p['title'])
        c.setFillColor(GREY);c.setFont('Ko',8.7)
        c.drawString(MARGIN,H-76,clean(p['subtitle']))
        c.setStrokeColor(LINE);c.setLineWidth(.6)
        c.line(MARGIN,H-86,W-MARGIN,H-86)
        for colidx,column in enumerate(p['columns']):
            x=MARGIN+colidx*(COL+GUTTER)
            y=TOP
            for d in column:
                y-=draw_card(c,d,x,y,COL)
        c.setStrokeColor(LINE);c.line(MARGIN,43,W-MARGIN,43)
        foot,h=para('원문: '+p['sources'],'footer',W-2*MARGIN-52)
        if h>28: raise ValueError('Footer too tall')
        foot.drawOn(c,MARGIN,38-h)
        c.setFillColor(TEAL);c.setFont('MonoBold',9)
        c.drawRightString(W-MARGIN,27,f'{page_no:02d} / {len(PAGES):02d}')
        # Invisible useful links on reference-page source rows are supplemented
        # by explicit links in the Markdown source index.
        qa.append({'page':page_no,'title':p['title'],'column_heights_pt':heights,
                   'available_pt':AVAILABLE,'font_body_pt':FONT,'font_code_pt':CODE})
        c.showPage()
    c.save()
    return qa

PREAMBLE={
 '01_status.h':'',
 '02_api_shape.h':'#include "01_status.h"',
 '03_handle_demo.c':'',
 '04_reg_bus.h':'#include "01_status.h"',
 '06_fake_read.h':'#include "01_status.h"',
 '10_fields.h':'#include <stdint.h>',
 '13_i2c_port.h':'#include "01_status.h"\n#include "stm32f4xx_hal.h"',
 '17_uart_api.h':'#include "stm32f4xx_hal.h"',
 '18_uart_init.c':'#include "17_uart_api.h"',
 '19_uart_start.c':'#include "17_uart_api.h"',
 '20_uart_write.c':'#include "stm32f4xx_hal.h"',
 '24_endian.h':'#include <stdint.h>',
 '25_signed20.h':'#include <stdint.h>',
 '26_adxl355_units.h':'#include <stdint.h>',
 '28_timeout.h':'#include <stdint.h>\n#include <stdbool.h>',
 '32_bounds.h':'#include <stddef.h>\n#include <stdbool.h>',
 '34_event.h':'#include <stdint.h>',
}

def export_text():
    snippets=OUT/'snippets';snippets.mkdir(exist_ok=True)
    lines=['# 임베디드 드라이버 구현 치트시트','',
           'A4 인쇄본과 같은 순서의 복사용 원본. **C17 / C++17 / 기존 STM32F4 HAL**을 기준으로 한다.',
           '', '교육용 코드이며 전체 C/C++ 컴파일·실물 검증은 하지 않았다. `구조 예시`는 실제 프로젝트 타입과 함수로 완성해야 한다.',
           '', '## 페이지 목차','']
    for i,p in enumerate(PAGES,1):
        lines.append(f'{i:02d}. {p["title"]}')
    catalog=['# 스니펫 찾아보기','','S번호는 PDF와 Markdown에서 동일하다. **전체를 한 프로젝트에 한꺼번에 추가하지 않는다.** 같은 API의 대안·문법 조각이 포함되어 있다.','',
             'UART는 `existing/uart_template.h`, `.c`, `uart_usage.c`를 한 세트로 사용하면 된다. 같은 함수의 S17~S21 예시와 중복 링크하지 않는다.','',
             '| ID | 페이지 | 코드 | 구분 | 필요한 조건 |','|---|---:|---|---|---|']
    meta=[]
    for i,p in enumerate(PAGES,1):
        lines+=['',f'## {i:02d}. {p["title"]}','',p['subtitle'],'']
        for column in p['columns']:
            for d in column:
                title=d['title']+(f' - {d["sid"]} / {d["mode"]}' if d.get('sid') else '')
                lines += [f'### {title}','']
                if d.get('text'):lines += [d['text'],'']
                if d.get('bullets'):lines += ['- '+s for s in d['bullets']]+['']
                if d.get('rows'):
                    lines += ['| 항목 | 빠른 참조 |','|---|---|']
                    lines += ['| '+' | '.join(s.replace('|','\\|') for s in row)+' |' for row in d['rows']]+['']
                if d.get('code'):
                    suffix=Path(d['file']).suffix
                    lang='cpp' if suffix in ('.cpp','.hpp') else ('text' if suffix=='.txt' else 'c')
                    lines += [f'```{lang}',d['code'],'```','',f'**전제:** {d["needs"]}', '',
                              f'[스니펫 파일](snippets/{d["file"]})','']
                    comment=f'/* {d["sid"]} | {d["mode"]}\n * {d["needs"]}\n * Educational snippet; not compiled or hardware-tested.\n */\n'
                    if d['mode']=='구조 예시' or d['file'] in {'14_adxl355_read.c','15_gpio_compare.c','16_hal_handle.c','21_uart_send.c'}:
                        comment += '/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */\n'
                    pre=PREAMBLE.get(d['file'],'')
                    source=comment+'\n'+(pre+'\n\n' if pre else '')+d['code']+'\n'
                    if suffix in ('.h','.hpp'):
                        guard='STUDY_SNIPPET_'+re.sub(r'\W','_',d['file']).upper()
                        source=f'#ifndef {guard}\n#define {guard}\n\n'+source+f'\n#endif /* {guard} */\n'
                    (snippets/d['file']).write_text(source,encoding='utf-8')
                    catalog.append(f'| {d["sid"]} | {i} | [{d["file"]}](snippets/{d["file"]}) | {d["mode"]} | {d["needs"].replace("|","/")} |')
                    meta.append({'id':d['sid'],'page':i,'file':d['file'],'mode':d['mode'],'requirements':d['needs']})
        lines+=['원문 대응: '+p['sources'],'']
    lines+=['## 원문 및 공식 출처','']
    for p in sorted(STUDY.glob('0[0-6]-*.md')):
        lines.append(f'- [{p.name}](../{p.name})')
    lines+=['- [목차](../README.md)','- [출처와 확인 범위](../sources/README.md)','']
    lines += [f'- [{name}]({url})' for name,url in SOURCE_URLS.items()]
    (OUT/'embedded-driver-cheatsheet.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (OUT/'snippet-index.md').write_text('\n'.join(catalog)+'\n',encoding='utf-8')
    (OUT/'snippet-manifest.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
    existing=OUT/'existing';existing.mkdir(exist_ok=True)
    for p in sorted((STUDY/'examples').glob('*')):
        if p.is_file():shutil.copy2(p,existing/p.name)
    input_files=sorted(STUDY.glob('*.md'))+[STUDY/'sources/README.md']
    manifest=[]
    for p in input_files:
        b=p.read_bytes()
        manifest.append({'path':str(p.relative_to(STUDY)).replace('\\','/'),
                         'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),
                         'role':'duplicate compilation' if p.name=='driver-onboarding-complete.md' else 'source'})
    (OUT/'source-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    (OUT/'README.md').write_text('''# 인쇄·구현용 치트시트

- [복사용 Markdown](embedded-driver-cheatsheet.md)
- [스니펫 목차](snippet-index.md): PDF의 S01~S39와 대응
- `existing/`: 앞서 만든 UART 템플릿, decode·가변 인자 실습 원본
- `source-manifest.json`: 압축 정리한 기존 Markdown 파일과 SHA-256

인쇄 PDF는 작업공간의 `output/pdf/embedded-driver-cheatsheet.pdf`에 있다. A4 세로, 100%/실제 크기, 긴 쪽 넘김 양면 인쇄를 권장한다. 16쪽이며 양면으로 8장이다. 얇은 회색 코드 배경과 청록색 표식은 흑백에서도 구분된다.

각 페이지를 독립적으로 찾아볼 수 있도록 용어와 전제를 일부 반복했다. 원문 전체의 주요 개념을 압축한 자료이며 원문을 그대로 재인쇄한 것은 아니다.

스니펫을 전부 하나의 소스에 합치지 않는다. `FRAGMENT`는 함수 안에 넣거나 실제 프로젝트 API로 완성해야 한다. HAL 태그는 부품·HAL 버전·보드 핀/클록 준비가 필요하다. C/C++ 컴파일과 실물 시험은 별도로 수행해야 한다.
''',encoding='utf-8')

if __name__=='__main__':
    qa=render_pdf()
    export_text()
    (OUT/'layout-check.json').write_text(json.dumps(qa,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'pdf':str(PDF),'pages':len(PAGES),'snippets':sum(1 for p in PAGES for col in p['columns'] for d in col if d.get('sid')),'max_column_height':max(max(p['column_heights_pt']) for p in qa),'available':AVAILABLE},ensure_ascii=False))
