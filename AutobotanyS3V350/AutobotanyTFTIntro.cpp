#include "AutobotanyTFTDrawing.h"
#include "AutobotanyTFTAssets.h"
#include "Config.h"
namespace ABTFT {
namespace {
using namespace ABTFTDraw;
constexpr uint16_t IntroBG=0x0863;
void bitmapBand(Adafruit_GFX &g,int x,int y,const uint16_t *pixels,int w,int h,int shift,int band){int first=shift>y ? shift-y : 0,last=shift+band-y;if(last>h)last=h;if(first<last)g.drawRGBBitmap(x,y+first-shift,pixels+first*w,w,last-first);}
}
void drawIntroFrame(Adafruit_GFX &g,int shift,uint64_t elapsed){
  using namespace ABTFTDraw;
  // Use the full 160-pixel span only in the direct-render fallback.
  int band=g.height();float t=float(elapsed),grow=ease(t/1500),arrive=ease(t/850);
  for(int n=0;n<18;++n){float p=phase(elapsed+n*233,3600+n*31);int x=(n*47+13)%128,y=px(170-p*190);uint16_t c=blend(IntroBG,n%3 ? Green : Blue,.18f+.38f*sinf(p*kPi));g.fillCircle(x,y-shift,n%5==0 ? 1 : 0,c);}
  // Opposing living stems rise, unfurl leaves, and carry little light packets.
  for(int side=0;side<2;++side){float lastX=side ? 121 : 6,lastY=159;
    for(int j=1;j<=35;++j){float f=j/35.0f;if(f>grow)break;float y=159-f*146;float x=(side ? 120 : 7)+(side ? -1 : 1)*(4*sinf(f*8+wave(elapsed,5200)*.2f));line(g,lastX,lastY-shift,x,y-shift,Stem);if(j%7==0){float open=ease((grow-f)*7);int s=side ? -1 : 1;g.fillTriangle(px(x),px(y)-shift,px(x+s*10*open),px(y-7*open)-shift,px(x+s*7*open),px(y+2)-shift,blend(Stem,Green,.6f));}lastX=x;lastY=y;}
    float p=phase(elapsed+side*1500,3000)*grow;int yy=px(159-p*146),xx=(side ? 120 : 7)+px((side ? -1 : 1)*4*sinf(p*8));g.fillCircle(xx,yy-shift,1,Green);
  }
  int schoolY=5-px((1-arrive)*67)+px(sinf(t*.0025f)*1.2f*arrive);
  if(elapsed>180){float a=phase(elapsed,5100)*kTau;arc(g,64,31-shift,30,29,a,a+1.5f,Green);arc(g,64,31-shift,30,29,a+kPi,a+kPi+1.0f,Blue);for(int j=4;j>=0;--j){float q=a-j*.09f;g.fillCircle(px(64+cosf(q)*30),px(31+sinf(q)*29)-shift,j==0 ? 2 : 1,blend(IntroBG,Yellow,1-j*.18f));}}
  bitmapBand(g,38,schoolY,School,52,52,shift,band);
  float slide=ease((t-470)/1050);int logoY=64+px((1-slide)*95);g.fillRoundRect(3,logoY-shift,122,64,5,0xFFFF);
  bitmapBand(g,5,logoY+1,Logo,118,62,shift,band);
  // Edge glints follow the white plate; the logo's actual artwork stays intact.
  if(slide==1){int x=px(5+phase(elapsed,2800)*116);g.drawFastHLine(x,64-shift,6,Yellow);g.drawFastHLine(122-x,127-shift,6,Green);}
  float credit=ease((t-1550)/700);
  if(credit>0)text(g,16,136-px((1-credit)*3)-shift,AB::TFT_CREDIT,blend(IntroBG,White,credit));
  int progress=px(bound(t/float(AB::TFT_INTRO_MS))*104);g.fillRoundRect(12,150-shift,104,3,1,Deep);if(progress)g.fillRoundRect(12,150-shift,progress,3,1,Green);
  if(elapsed>2400){for(int j=0;j<3;++j){float p=phase(elapsed+j*620,2000);star(g,30+j*34,146-px(p*4)-shift,1,blend(IntroBG,Yellow,sinf(p*kPi)*.8f));}}
}
}
