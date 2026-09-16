"""Rasterize the bundled OFL Inter font into compact 4-bit antialiased ASCII.
Rebuild: pip install pillow; python tools/build_fonts.py
Only the generated header is needed to compile firmware.
"""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
out=['#pragma once','#include <cstdint>','namespace pulse {',
     'struct Glyph {uint32_t offset;uint8_t width,height,advance;int8_t x,y;};',
     'struct Font {const uint8_t* pixels;const Glyph* glyphs;uint8_t height;};']
for idx,(size,weight,height) in enumerate([(11,400,13),(15,500,18),(23,600,26)]):
    font=ImageFont.truetype(str(root/'assets/fonts/Inter.ttf'),size)
    font.set_variation_by_axes([14,weight])
    baseline=-font.getbbox('H',anchor='ls')[1]
    pixels=[];glyphs=[]
    for code in range(32,127):
        char=chr(code);left,top,right,bottom=font.getbbox(char,anchor='ls');w=right-left;h=bottom-top
        image=Image.new('L',(max(w,1),max(h,1)));ImageDraw.Draw(image).text((-left,-top),char,font=font,fill=255,anchor='ls')
        samples=[(v+8)//17 for v in image.getdata()] if w*h else []
        off=len(pixels)
        for i in range(0,len(samples),2):pixels.append((samples[i]<<4)|(samples[i+1] if i+1<len(samples) else 0))
        glyphs.append((off,w,h,round(font.getlength(char)),left,top+baseline))
    out.append(f'inline constexpr uint8_t fontPixels{idx}[]={{'+','.join(map(str,pixels))+'};')
    out.append(f'inline constexpr Glyph fontGlyphs{idx}[]={{'+','.join('{'+','.join(map(str,g))+'}' for g in glyphs)+'};')
    print(size,'px:',len(pixels),'bitmap bytes')
out.append('inline constexpr Font fonts[]={'+','.join(f'{{fontPixels{i},fontGlyphs{i},{h}}}' for i,h in enumerate([13,18,26]))+'};')
out.append('}')
(root/'include/font_assets.h').write_text('\n'.join(out)+'\n')
