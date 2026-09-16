#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "font_assets.h"
namespace pulse {
constexpr uint32_t BG=0x0B0C0F,PANEL=0x191B20,TEXT=0xF3F2EF,MUTED=0x9B9DA5,ACCENT=0xFF8A36;
struct Canvas {virtual ~Canvas()=default;virtual void rect(int x,int y,int w,int h,uint32_t color)=0;virtual void text(int x,int y,const std::string&,int size,uint32_t color,uint32_t background=BG)=0;};
inline int textWidth(const std::string& s,int size){int w=0;for(unsigned char c:s)w+=fonts[size].glyphs[(c>=32&&c<=126?c:'?')-32].advance;return w;}
inline std::string fitPixels(std::string s,int width,int size){if(textWidth(s,size)<=width)return s;while(!s.empty()&&textWidth(s+"..",size)>width)s.pop_back();return s.empty()?"":s+"..";}
// Shared 4-bit antialiased Inter renderer: desktop and panel use identical glyphs.
inline void renderText(Canvas& c,int x,int y,const std::string& s,int size,uint32_t fg,uint32_t bg){
 uint32_t colors[16];for(int a=0;a<16;a++){colors[a]=0;for(int shift:{16,8,0})colors[a]|=((((fg>>shift)&255)*a+((bg>>shift)&255)*(15-a)+7)/15)<<shift;}
 const Font& f=fonts[size];
 for(unsigned char ch:s){const Glyph& g=f.glyphs[(ch>=32&&ch<=126?ch:'?')-32];
  auto alpha=[&](int col,int row){int i=row*g.width+col;auto b=f.pixels[g.offset+i/2];return i%2?b&15:b>>4;};
  for(int row=0;row<g.height;row++)for(int col=0;col<g.width;){int a=alpha(col,row),end=col+1;while(end<g.width&&alpha(end,row)==a)end++;if(a)c.rect(x+g.x+col,y+g.y+row,end-col,1,colors[a]);col=end;}
  x+=g.advance;
 }
}
struct Button {int x,y,w,h,action,param;std::string title,context;bool selected=false;};
struct View {std::string title,subtitle,status;std::vector<Button> buttons;std::vector<std::string> lines;int tab=0;bool keyboard=false;};
inline std::string fit(const std::string& s,size_t n){return s.size()>n?s.substr(0,n-2)+"..":s;}
inline void draw(Canvas& c,const View& v,bool pulse=false){
  c.rect(0,0,240,320,BG);c.rect(0,0,240,3,pulse?ACCENT:PANEL);
  bool back=false;for(auto& b:v.buttons)if(b.y==4)back=true;
  c.text(10,11,fitPixels(v.title,back?174:220,2),2,TEXT);c.text(10,37,fitPixels(v.status.empty()?v.subtitle:v.status,220,0),0,v.status.empty()?MUTED:ACCENT);
  for(size_t i=0;i<v.lines.size();i++)c.text(10,61+i*18,fitPixels(v.lines[i],220,0),0,MUTED);
  for(const auto& b:v.buttons){
    c.rect(b.x,b.y,b.w,b.h,b.selected?ACCENT:PANEL);
    auto fg=b.selected?BG:TEXT;
    bool small=(b.y>=272&&!v.keyboard)||(b.y==4&&b.title.size()>1);
    auto bg=b.selected?ACCENT:PANEL;int size=small?0:1;
    c.text(b.x+9,b.y+(b.context.empty()?(b.h-fonts[size].height)/2:8),fitPixels(b.title,b.w-18,size),size,fg,bg);
    if(!b.context.empty())c.text(b.x+9,b.y+29,fitPixels(b.context,b.w-18,0),0,b.selected?BG:MUTED,bg);
  }
}
inline int hit(const View& v,int x,int y){for(size_t i=0;i<v.buttons.size();i++){const auto& b=v.buttons[i];if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return i;}return -1;}
}
