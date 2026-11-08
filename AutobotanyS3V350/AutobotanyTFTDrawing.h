#pragma once
#include "AutobotanyTFTVisuals.h"
#include <math.h>

// Drawing primitives only. Each card composes its own scene and motion timeline.
namespace ABTFTDraw {
constexpr float kPi=3.14159265f,kTau=6.28318531f;
constexpr uint16_t White=0xF7BE,Blue=0x65FF,Deep=0x2476,Green=0x9FB3,Stem=0x3C8A,Yellow=0xFFE9,Orange=0xFDA8,Red=0xFAAA,Gray=0x9D16,Brown=0xA3CB,Pink=0xFCD8;
class Surface:public Adafruit_GFX {
  Adafruit_GFX &out_;int x_,y_;
public:
  Surface(Adafruit_GFX &out,int x,int y):Adafruit_GFX(40,40),out_(out),x_(x),y_(y){setTextWrap(false);}
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=0&&x<40&&y>=0&&y<40)out_.drawPixel(x_+x,y_+y,c);}
};
inline int px(float x){return static_cast<int>(lroundf(x));}
inline float phase(uint64_t ms,uint32_t period){return float(ms%period)/period;}
inline float wave(uint64_t ms,uint32_t period){return sinf(phase(ms,period)*kTau);}
inline float bound(float x){return x<0 ? 0 : x>1 ? 1 : x;}
inline float ease(float x){x=bound(x);return x*x*(3-2*x);}
inline float reaction(uint32_t age){return age>=ABTFT::ReactionMs ? 0 : sinf(kPi*bound(age/300.0f)*.5f)*(1-age/float(ABTFT::ReactionMs));}
inline uint16_t blend(uint16_t a,uint16_t b,float f){int k=px(bound(f)*255);return (((((a>>11)&31)*(255-k)+((b>>11)&31)*k)/255)<<11)|(((((a>>5)&63)*(255-k)+((b>>5)&63)*k)/255)<<5)|(((a&31)*(255-k)+(b&31)*k)/255);}
inline void line(Adafruit_GFX &g,float x,float y,float xx,float yy,uint16_t c){g.drawLine(px(x),px(y),px(xx),px(yy),c);}
inline void arc(Adafruit_GFX &g,float x,float y,float rx,float ry,float start,float end,uint16_t c,int steps=18){float lx=x+cosf(start)*rx,ly=y+sinf(start)*ry;for(int n=1;n<=steps;++n){float a=start+(end-start)*n/steps;float nx=x+cosf(a)*rx,ny=y+sinf(a)*ry;line(g,lx,ly,nx,ny,c);lx=nx;ly=ny;}}
inline void text(Adafruit_GFX &g,int x,int y,const char *s,uint16_t c,int size=1){g.setTextSize(size);g.setTextColor(c);g.setCursor(x,y);g.print(s);}
inline void drop(Adafruit_GFX &g,int x,int y,int r,uint16_t c){g.fillTriangle(x,y-2*r,x-r,y,x+r,y,c);g.fillCircle(x,y,r,c);if(r>1)g.drawPixel(x-1,y,White);}
inline void star(Adafruit_GFX &g,int x,int y,int r,uint16_t c){g.drawFastHLine(x-r,y,2*r+1,c);g.fillRect(x,y-r,1,2*r+1,c);}
inline void leaf(Adafruit_GFX &g,int x,int y,int side,uint16_t c){g.fillTriangle(x,y,x+side*7,y-5,x+side*6,y+1,c);line(g,x,y,x+side*5,y-2,Stem);}
inline void dotTrail(Adafruit_GFX &g,float x,float y,float angle,int radius,uint16_t c,uint16_t bg){for(int j=7;j>=0;--j){float a=angle-j*.11f;g.fillCircle(px(x+cosf(a)*radius),px(y+sinf(a)*radius),j==0 ? 2 : 1,blend(bg,c,1-j/8.0f));}}
}
