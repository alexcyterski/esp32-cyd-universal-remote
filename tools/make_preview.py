"""Render the native_check SVG output into a contact sheet and HTML gallery.
Optional desktop dependencies: pip install cairosvg pillow
"""
from pathlib import Path
import cairosvg
from PIL import Image,ImageDraw
files=sorted(Path('preview').glob('*.svg'))
sheet=Image.new('RGB',(792,358*((len(files)+2)//3)),'#17181D')
d=ImageDraw.Draw(sheet)
cards=[]
for i,p in enumerate(files):
    png=p.with_suffix('.png');cairosvg.svg2png(url=str(p),write_to=str(png))
    x=i%3*264+12;y=i//3*358+24
    sheet.paste(Image.open(png),(x,y));d.text((x,y-17),p.stem,fill='white')
    cards.append(f'<figure><figcaption>{p.stem[3:].replace("-"," / ")}</figcaption>{p.read_text()}</figure>')
sheet.save('preview/screens.png')
Path('preview/index.html').write_text('''<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>PULSE / interface preview</title><style>body{background:#0b0c0f;color:#f3f2ef;font:16px system-ui;margin:32px}h1{font-weight:500}p{color:#9b9da5;max-width:760px;line-height:1.5}main{display:grid;grid-template-columns:repeat(auto-fit,264px);gap:24px}figure{margin:0}figcaption{padding:12px 0;text-transform:uppercase;font-size:12px;letter-spacing:2px;color:#9b9da5}svg{border:1px solid #2b2d33}</style>
<h1>PULSE / E32R28T</h1><p>Inter typography, deep charcoal, and warm orange. 240 × 320 portrait with large square controls and three persistent tabs. These screens use the firmware's actual layout and antialiased glyph bitmaps. Example favorites are demonstration data.</p><main>'''+''.join(cards)+'</main>')
