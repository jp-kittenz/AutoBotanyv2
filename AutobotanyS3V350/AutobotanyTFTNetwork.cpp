#include "AutobotanyTFTDrawing.h"
namespace ABTFT {
namespace {
using namespace ABTFTDraw;
void wifiGlyph(Surface &g,int x,int y,int size,uint64_t t,uint16_t bg,uint16_t color,float boost=0){
  for(int j=0;j<3;++j){int r=size+j*(size>5 ? 4 : 3);float brightness=.35f+.65f*(.5f+.5f*sinf(phase(t,1600)*kTau-j*1.2f));uint16_t c=blend(bg,color,brightness);arc(g,x,y,r+boost,r+boost,1.18f*kPi,1.82f*kPi,c);}
  g.fillCircle(x,y,2,color);
}
void phone(Surface &g,int x,int y,uint16_t c,uint64_t t,bool active){
  g.fillRoundRect(x,y,10,18,2,Deep);g.drawRoundRect(x,y,10,18,2,c);g.fillRect(x+2,y+3,6,10,active ? blend(Deep,Blue,.25f+.15f*wave(t,1800)) : 0x1125);g.drawFastHLine(x+3,y+1,4,Gray);g.drawPixel(x+5,y+15,White);
  if(active){int h=2+int((t/220)%6);g.fillRect(x+3,y+12-h,1,h,c);g.fillRect(x+6,y+9,1,3,c);}
}
void laptop(Surface &g,int x,int y,uint64_t t){g.fillRect(x,y,18,12,Deep);g.drawRoundRect(x,y,18,12,2,White);for(int n=0;n<3;++n)g.drawFastHLine(x+3,y+3+n*3,4+int((t/250+n*2)%9),Blue);g.fillTriangle(x,y+12,x+17,y+12,x+20,y+15,Gray);g.fillTriangle(x,y+12,x-3,y+15,x+20,y+15,Gray);g.drawFastHLine(x+5,y+13,7,White);}
void globe(Surface &g,uint64_t t,uint16_t bg,float kick){
  int x=20,y=21,r=14;g.drawCircle(x,y,r,Blue);arc(g,x,y,r,5,0,kTau,Deep);arc(g,x,y,r,10,0,kTau,Deep);
  float spin=phase(t,6500)*kTau+kick*2;
  for(int j=0;j<4;++j){float a=spin+j*kPi/4;float width=cosf(a)*r;arc(g,x,y,width,r,-kPi/2,kPi/2,blend(Deep,Blue,.7f));}
  // Rotating longitude/latitude samples draw continental clusters on the front hemisphere.
  static const int16_t land[][2]={{-110,45},{-100,35},{-90,25},{-80,10},{-70,-5},{-65,-20},{-60,-35},{10,50},{20,35},{25,15},{25,0},{30,-20},{45,40},{65,40},{85,35},{105,25},{120,-25},{135,-30}};
  for(const auto &point:land){float lon=point[0]*kPi/180+spin,lat=point[1]*kPi/180;if(cosf(lon)>0){int xx=x+px(sinf(lon)*cosf(lat)*12),yy=y-px(sinf(lat)*12);g.fillRect(xx,yy,2,2,Green);}}
  dotTrail(g,x,y,phase(t,3900)*kTau,17,Yellow,bg);
}
}
void drawNetwork(Adafruit_GFX &out,int x,int y,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  Surface g(out,x,y);float kick=reaction(tap);
  if(v.art==Art::NetworkOff){bool question=(t/1100)%2;float p=phase(t,1100);uint16_t c=blend(bg,question ? Yellow : Red,.3f+.7f*sinf(p*kPi));g.drawRoundRect(5,3,30,34,7,blend(bg,c,.45f));text(g,15,10+px(wave(t,2200)*2),question ? "?" : "!",c,2);if(kick>0)for(int j=0;j<4;++j)star(g,4+j*10,px(4+kick*30),1,Yellow);return;}
  wifiGlyph(g,20,27,6,t,bg,Green,kick*6);
  g.drawFastHLine(13,34,15,Deep);g.drawPixel(20,35,Blue);
  if(v.camera){dotTrail(g,20,20,phase(t,3300)*kTau,17,Red,bg);}
  if(kick>0){float p=bound(tap/1400.0f);arc(g,20,27,4+p*25,4+p*25,kPi,kTau,blend(bg,White,kick));}
}
void drawClients(Adafruit_GFX &out,int x,int y,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  Surface g(out,x,y);float kick=reaction(tap);bool online=v.art!=Art::NetworkOff;
  if(v.count>=4&&online){globe(g,t,bg,kick);return;}
  // Zero clients is the waiting-phone state; display text always gives the count.
  if(v.count<2){int y=15+px(wave(t,3300)-kick*3);phone(g,15,y,online&&v.count ? White : Gray,t,online&&v.count);wifiGlyph(g,20,12,3,t,bg,online&&v.count ? Blue : Gray,kick*3);if(v.count==0)for(int j=0;j<3;++j)g.drawPixel(16+j*4,37,blend(bg,Gray,phase(t+j*450,1800)));}
  else{int lift=px(kick*3);laptop(g,17,20-lift,t);phone(g,3,17+px(wave(t,3100)),White,t,true);if(v.count>=3)phone(g,23,13-lift,Blue,t+400,true);wifiGlyph(g,20,10,3,t,bg,Blue,kick*2);for(int j=0;j<v.count;++j){float p=phase(t+j*440,1600);int xx=px(8+22*p),yy=px(35-5*sinf(p*kPi));g.drawPixel(xx,yy,Green);}}
  if(kick>0)for(int j=0;j<4;++j){float p=bound(tap/1500.0f);int xx=px(20+(j-1.5f)*11*p),yy=px(20-17*p);g.drawRect(xx,yy,2,2,blend(bg,Yellow,kick));}
}
}
