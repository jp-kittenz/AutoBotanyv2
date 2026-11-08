#include "AutobotanyTFTDrawing.h"
namespace ABTFT {
namespace {
using namespace ABTFTDraw;
constexpr uint16_t Storm=0x630C,StormTop=0x9CF3;
// A dark rain cloud with a lit rim; drift is a few pixels so it never leaves the art box.
void rainCloud(Adafruit_GFX &g,int x,int y,uint64_t t,float scale=1){
  int d=px(wave(t,5200)*2);int r1=px(4*scale),r2=px(6*scale),r3=px(5*scale);
  g.fillCircle(x-px(6*scale)+d,y+1,r1,Storm);g.fillCircle(x+d,y-1,r2,Storm);g.fillCircle(x+px(7*scale)+d,y+1,r3,Storm);g.fillRoundRect(x-px(9*scale)+d,y+1,px(19*scale),r1+1,2,Storm);
  ABTFTDraw::arc(g,x+d,y-1,r2,r2,kPi*1.15f,kPi*1.85f,StormTop,8);g.drawPixel(x-px(7*scale)+d,y-2,StormTop);
}
void rainBelow(Adafruit_GFX &g,int x0,int x1,int top,int bottom,uint64_t t,uint16_t bg){
  for(int j=0;x0+j*5<=x1;++j){float p=phase(t+j*263+(j%2)*500,900+(j%3)*140);int x=x0+j*5+(j%2);int y=top+px((bottom-top)*p);
    line(g,x,y,x-1,y+3,blend(bg,Blue,.55f+.45f*(1-p)));if(p>.86f)g.drawPixel(x-2,bottom,blend(White,bg,(p-.86f)/.14f));}
}
void reservoir(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool empty=v.art==Art::WaterEmpty,unknown=v.art==Art::WaterUnknown;
  uint16_t edge=unknown||empty ? blend(bg,empty ? Red : Yellow,.6f+.4f*wave(t,1700)) : Blue;
  g.drawRoundRect(4,17,32,21,4,edge);g.drawFastHLine(7,37,26,White);
  for(int n=0;n<3;++n)g.drawFastHLine(6,22+n*5,3,Gray);
  if(empty||unknown){text(g,18,24,empty ? "!" : "?",edge);if(empty){for(int x=12;x<32;++x)g.drawPixel(x,34+px(wave(t+x*90,2400)),Deep);}return;}
  int level=v.count==4 ? 23 : v.count==2 ? 28 : 32;
  for(int x=10;x<34;++x){int top=level+px(sinf(x*.31f-phase(t,1800)*kTau));g.fillRect(x,top,1,36-top,Deep);g.drawPixel(x,top,Blue);}
  for(int n=0;n<v.count;++n){float u=phase(t+n*401,1700);int x=12+n*6;
    if(u<.7f){float p=u/.7f;drop(g,x,px(-4+(level+3)*p*p),2,Blue);}
    else{float p=(u-.7f)/.3f;arc(g,x,level,1+p*7,1+p*2,0,kTau,blend(Blue,bg,p));for(int s=-1;s<=1;s+=2)g.drawPixel(px(x+s*5*p),px(level-8*p*(1-p)),White);}
  }
  g.drawLine(32,29,32,34,blend(White,Blue,.6f));
  if(tap<ReactionMs){
    // SELECT: the level probe dips in, the reading ripples outward, then it lifts away.
    float down=ease(tap/600.0f)*(1-ease((tap-1500.0f)/600.0f));int tip=px(2+(level-2)*down);
    g.drawFastVLine(25,0,tip,Gray);g.drawFastVLine(28,0,tip,Gray);g.fillRect(24,0,6,2,White);
    if(tap>600&&tap<1500){float r=(tap-600)/900.0f;arc(g,26,level,3+r*9,1+r*2,0,kTau,blend(White,bg,r));arc(g,26,level,1+r*4,1,0,kTau,blend(Blue,bg,r));}
  }
}
void soilProbe(Surface &g,uint32_t tap,uint16_t bg){
  // SELECT: a two-pin probe pushes into the soil, moisture pulses rise, then it withdraws.
  float down=ease(tap/550.0f)*(1-ease((tap-1500.0f)/600.0f));int tip=px(20+16*down);
  for(int x:{31,34})g.drawFastVLine(x,tip-12,12,Gray);g.fillRect(30,tip-14,6,3,White);
  if(tap>550&&tap<1500){float u=(tap-550)/950.0f;for(int j=0;j<3;++j){float q=bound(u*1.4f-j*.2f);if(q>0&&q<1)g.drawPixel(29+j*3,px(34-q*14),blend(Blue,bg,q));}arc(g,32,35,2+u*6,1,0,kTau,blend(Blue,bg,u));}
}
void soil(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool dead=v.art==Art::FlowerDead,weak=v.art==Art::FlowerWilt||v.art==Art::FlowerPetals,unknown=v.art==Art::GrassUnknown;
  g.fillRoundRect(3,33,34,6,2,Brown);for(int j=0;j<9;++j)g.drawPixel(5+j*4,35+j%3,Stem);
  if(unknown){for(int j=0;j<6;++j){int x=6+j*5;line(g,x,33,x+wave(t+j*200,2400)*2,21-j%3*3,Yellow);}text(g,17,8,"?",blend(bg,Yellow,.7f+.3f*wave(t,1900)));if(tap<ReactionMs)soilProbe(g,tap,bg);return;}
  float sway=dead ? 0 : wave(t,3000)*(weak ? 2 : 4);float hx=20+sway+(weak||dead ? 5 : 0),hy=dead ? 24 : weak ? 17 : 10;
  uint16_t stem=dead ? Brown : weak ? blend(Stem,Brown,.6f) : Green;
  float lx=20,ly=33;for(int j=1;j<=12;++j){float f=j/12.0f;float x=20+(hx-20)*f*f,y=33+(hy-33)*f;line(g,lx,ly,x,y,stem);lx=x;ly=y;}
  leaf(g,20,27,-1,stem);leaf(g,22,24,1,stem);
  line(g,20,34,16,38,Stem);line(g,20,34,25,38,Stem);
  if(dead){line(g,hx,hy,hx+5,hy+4,Red);line(g,hx,hy,hx-2,hy+5,Red);if(tap<ReactionMs)soilProbe(g,tap,bg);return;}
  for(int j=0;j<6;++j){float a=j*kTau/6-.3f+sway*.04f;float x=hx+cosf(a)*5,y=hy+sinf(a)*5;
    if(v.art==Art::FlowerPetals&&j%2==0){float p=phase(t+j*600,3000);x+=sinf(p*8)*3;y+=21*p*p;g.fillCircle(px(x),px(y),2,blend(Pink,bg,p));}
    else{g.fillCircle(px(x),px(y),3,weak ? Orange : Pink);g.drawPixel(px(x-1),px(y-1),White);}
  }
  g.fillCircle(px(hx),px(hy),3,Yellow);g.drawPixel(px(hx),px(hy),Brown);
  if(!weak){float a=phase(t,4500)*kTau;int x=px(20+16*cosf(a)),y=px(16+6*sinf(a));g.fillRect(x,y,3,2,Yellow);g.drawPixel(x+1,y,Stem);g.drawPixel(x,y-1,White);}
  if(tap<ReactionMs)soilProbe(g,tap,bg);
}
void rainGauge(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  float kick=reaction(tap);int dx=px(wave(t,4100)*2);int squash=px(kick*2*sinf(tap<ReactionMs ? tap*.025f : 0));
  g.fillCircle(10+dx,10,5,Gray);g.fillCircle(18+dx,7+squash,7,White);g.fillCircle(27+dx,10,6,Gray);g.fillRoundRect(6+dx,10,28,5,2,White);
  g.drawRect(14,23,13,15,Blue);g.drawFastHLine(12,23,17,White);
  for(int j=0;j<3;++j)g.drawFastHLine(23,27+j*4,3,Gray);
  bool wet=v.sky==Sky::Rain;
  if(wet&&!v.warning){g.fillRect(16,32,7,4,Deep);for(int n=0;n<6;++n){float p=phase(t+n*211,1100);int x=5+n*6;int y=px(15+19*p*p);line(g,x,y,x-1,y+3,Blue);if(p>.85f)arc(g,x,37,(p-.85f)*30,1,0,kTau,blend(White,bg,(p-.85f)/.15f));}}
  else{line(g,4,19+wave(t,2100),10,19,Gray);g.drawFastHLine(30,20+px(wave(t,2500)),7,Gray);}
  if(v.warning)text(g,18,28,"?",Yellow);
  if(kick>0){if(wet&&!v.warning){for(int j=0;j<5;++j){float p=bound(tap/800.0f);drop(g,px(20+(j-2)*5*p),px(15+20*p*p),1,blend(Blue,bg,1-kick));}}else for(int j=0;j<3;++j)arc(g,20,17,6+j*4+kick*6,2+j,0,kPi,blend(bg,White,kick));}
}
void thermometer(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool missing=v.art==Art::TempUnknown,cold=v.art==Art::TempSnow,cool=v.art==Art::TempWind,hot=v.art==Art::TempHeat;
  float kick=reaction(tap);int x=14+px(kick*sinf(tap<ReactionMs ? tap*.025f : 0)*3);
  uint16_t c=missing ? Gray : cold||cool ? Blue : hot ? Red : Orange;
  int level=missing ? 25 : cold ? 26 : cool ? 21 : hot ? 7 : 14;
  g.fillRoundRect(x-4,3,9,27,4,White);g.fillCircle(x,30,7,White);g.fillRoundRect(x-2,5,5,26,2,bg);g.fillCircle(x,30,5,c);g.fillRect(x-1,level,3,30-level,c);g.drawPixel(x-2,28,Yellow);
  for(int j=0;j<5;++j)g.drawFastHLine(x+6,7+j*4,j%2 ? 3 : 5,Gray);
  for(int j=0;j<3;++j){float p=phase(t+j*700,cold ? 3200 : 2100);int xx=29+j*3,yy=px(cold ? 2+34*p : 36-34*p);uint16_t ink=blend(bg,c,sinf(p*kPi));
    if(cold)star(g,xx+px(sinf(p*5)*2),yy,2,ink);
    else if(cool)line(g,25,yy,35,yy-2,ink);
    else{for(int k=0;k<8;++k)g.drawPixel(xx+px(sinf(k*.7f+p*kTau)*2),yy-k,ink);}}
  if(missing)text(g,27,17,"?",Yellow);
  if(kick>0){float r=7+tap/95.0f;arc(g,x,30,r,r,0,kTau,blend(bg,c,kick));}
}
void dew(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool dry=v.art==Art::HumDry,wet=v.art==Art::HumDrops,unknown=v.art==Art::HumUnknown;
  float kick=reaction(tap),sway=wave(t,3600)*2;uint16_t lc=dry ? Brown : Green;
  g.fillTriangle(5,30,18+px(sway),18,36,25,lc);g.fillTriangle(5,30,36,25,26,35,lc);line(g,5,30,32,27,Stem);
  for(int j=0;j<4;++j)line(g,12+j*5,29,17+j*4,24,Stem);
  if(unknown){text(g,17,5-px(kick*4),"?",blend(bg,Yellow,.7f+.3f*wave(t,1700)));if(kick>0)arc(g,20,17,5+kick*12,5+kick*12,0,kTau,blend(bg,Yellow,kick));return;}
  if(dry){for(int j=0;j<4;++j){float p=phase(t+j*430,2600);star(g,px(8+j*7+3*sinf(p*4)),px(22-20*p),1,blend(bg,Orange,sinf(p*kPi)));}line(g,20,28,23,31,Brown);}
  else{for(int j=0;j<(wet ? 5 : 3);++j){float p=phase(t+j*510,2900);float spread=kick*(j-2)*7;int x=px(7+j*6+spread),y=px(23-p*19-kick*5);int r=1+px((1-p)*2);g.drawCircle(x,y,r,blend(bg,Blue,1-p*.6f));g.drawPixel(x-1,y-1,White);}
    float p=phase(t,wet ? 1400 : 4200);drop(g,px(10+19*p),px(28-3*p),2,Blue);if(wet)drop(g,31,32+px(12*p*p),2,Blue);}
  if(kick>0)for(int j=0;j<6;++j){float a=j*kTau/6;star(g,px(20+cosf(a)*(5+kick*14)),px(17+sinf(a)*(5+kick*12)),1,blend(bg,White,kick));}
}
void sunlight(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  float kick=reaction(tap);bool moon=v.sky==Sky::Moon||v.sky==Sky::MoonRain;
  if(v.art==Art::Bulb||v.art==Art::BulbBroken){bool bad=v.art==Art::BulbBroken;uint16_t c=bad ? Yellow : blend(Orange,Yellow,.6f+.4f*wave(t,2500));g.drawCircle(20,14,9,c);arc(g,20,14,7,7,kPi,1.6f*kPi,White);g.drawLine(13,21,15,28,c);g.drawLine(27,21,25,28,c);for(int j=0;j<3;++j)g.drawFastHLine(15,28+j*3,11-j,Gray);line(g,17,18,20,21,c);line(g,20,21,23,18,c);if(bad){line(g,23,6,17,15,bg);text(g,32,2,"!",Red);if(kick>0){star(g,12-px(kick*4),8,2,Yellow);star(g,28+px(kick*4),19,2,White);}}else for(int j=0;j<6;++j){float a=j*kTau/6;line(g,20+cosf(a)*(12+kick*6),14+sinf(a)*(12+kick*6),20+cosf(a)*(15+kick*8),14+sinf(a)*(15+kick*8),c);}return;}
  bool wet=v.sky==Sky::Rain||v.sky==Sky::MoonRain||v.sky==Sky::SunRain;
  if(wet){
    // Rain by day or night: the sun or moon peeks out behind a dark rain cloud.
    if(moon){g.fillCircle(11,9,7,White);g.fillCircle(15,6,6,bg);for(int j=0;j<2;++j)star(g,30+j*6,3+j*4,phase(t+j*500,2200)<.4f ? 1 : 0,blend(bg,White,.3f+.4f*wave(t+j*600,2600)));}
    else{float breath=wave(t,2600);g.fillCircle(12,10,6,Yellow);g.fillCircle(11,9,3,Orange);for(int j=0;j<8;++j){float a=j*kTau/8+phase(t,24000)*kTau;float st=8+kick*8;line(g,12+cosf(a)*st,10+sinf(a)*st,12+cosf(a)*(st+2+breath),10+sinf(a)*(st+2+breath),blend(Yellow,bg,.3f+kick*.5f));}}
    rainCloud(g,23+px(kick*4),17,t);rainBelow(g,13,35,24,38,t,bg);
    if(kick>0)for(int j=0;j<4;++j){float p=bound(tap/900.0f);drop(g,px(16+j*6+(j-1.5f)*4*p),px(23+14*p*p),1,blend(Blue,bg,1-kick));}
    return;
  }
  if(moon){int y=17+px(wave(t,5000));g.fillCircle(17,y,10,White);g.fillCircle(22,y-4,9,bg);g.drawPixel(12,y+4,Gray);for(int j=0;j<4;++j){int x=5+j*9,yy=3+(j%3)*12;star(g,x,yy,phase(t+j*340,2000)<.4f ? 1 : 0,blend(bg,White,.4f+.6f*wave(t+j*450,2600)));}if(kick>0){float p=bound(tap/1300.0f);line(g,3+p*35,2+p*25,-5+p*35,-3+p*25,blend(bg,White,kick));}}
  else{bool cloudy=v.sky==Sky::Cloud||v.sky==Sky::SunCloud;int x=cloudy ? 13 : 20,y=cloudy ? 13 : 19;float breath=wave(t,2600);g.fillCircle(x,y,8,Yellow);g.fillCircle(x-1,y-1,5,Orange);g.fillCircle(x-2,y-2,3,Yellow);
    for(int j=0;j<12;++j){float a=j*kTau/12+phase(t,24000)*kTau;float start=11+kick*14,end=start+3+breath;line(g,x+cosf(a)*start,y+sinf(a)*start,x+cosf(a)*end,y+sinf(a)*end,blend(Yellow,bg,kick*.6f));}
    if(cloudy){int drift=px(wave(t,4400)*2+kick*10);g.fillCircle(19+drift,25,6,Gray);g.fillCircle(27+drift,22,7,White);g.fillRoundRect(12+drift,26,25,6,3,White);}}
}
// Rotor angle with an eased spin-up/spin-down after SELECT. The ramp length is a
// whole number of half blade pitches, so the steady angle is rejoined without a jump.
float rotor(bool on,uint64_t t,uint32_t tap,uint32_t period,float rest,int blades){
  if(tap>=ReactionMs)return on ? phase(t,period)*kTau : rest;
  int m=static_cast<int>(lroundf(960.0f*blades/(2.0f*period)));if(m<1)m=1;
  const float w=kTau/period,R=float(period)*2*m/blades,a=float(tap);
  const float t0=float((t-tap)%(uint64_t(period)*blades));
  const float pitch=kTau/blades;
  if(on){ // From rest, accelerate, then blend onto the steady angle (mod one blade pitch).
    if(a<R)return rest+w*a*a/(2*R);
    float lin=rest+w*(a-R/2),diff=fmodf(w*(t0+a)-lin,pitch);if(diff<0)diff+=pitch;
    return lin+diff*ease((a-(ReactionMs-600))/450.0f);
  }
  float end=w*(t0+R/2);if(a<R)return w*(t0+a-a*a/(2*R));
  float target=rest+ceilf((end-rest)/pitch)*pitch;return end+(target-end)*ease((a-R)/fmaxf(1,ReactionMs-R-200));
}
void coolingFan(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool on=v.art==Art::FanSpin;g.drawRoundRect(2,2,36,36,5,Gray);for(int x:{5,34})for(int y:{5,34})g.fillCircle(x,y,1,White);
  g.drawCircle(20,20,14,Deep);uint32_t ramp=v.before!=on ? tap : NoReaction;float a=rotor(on,t,ramp,1200,.3f,5);
  float speed=ramp<ReactionMs ? (on ? ease(ramp/960.0f) : 1-ease(ramp/960.0f)) : on ? 1.0f : 0.0f;
  if(tap<ReactionMs&&tap<500){float r=bound(tap/500.0f);g.drawCircle(20,20,px(14+r*5),blend(on ? Green : Gray,bg,r));}
  for(int j=0;j<5;++j){float q=a+j*kTau/5;int x=px(20+cosf(q)*8),y=px(20+sinf(q)*8);uint16_t blade=blend(Gray,Blue,speed);g.fillTriangle(20,20,px(20+cosf(q-.35f)*13),px(20+sinf(q-.35f)*13),px(20+cosf(q+.45f)*10),px(20+sinf(q+.45f)*10),blade);g.fillCircle(x,y,3,blade);line(g,20+cosf(q)*5,20+sinf(q)*5,20+cosf(q)*12,20+sinf(q)*12,White);}
  g.fillCircle(20,20,4,bg);g.fillCircle(20,20,2,White);
  if(speed>0)for(int j=0;j<3;++j){float p=phase(t+j*310,1400);int y=8+j*12;uint16_t c=blend(bg,Green,sinf(p*kPi)*speed);line(g,-8+14*p,y,-3+14*p,y-1,c);line(g,30+14*p,y+1,35+14*p,y,c);}
}
void comfort(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool unknown=v.art==Art::Unknown,cold=v.art==Art::FaceColdSad||v.art==Art::FaceColdHappy,hot=v.art==Art::FaceHotSad||v.art==Art::FaceHotHappy,sad=v.art==Art::FaceColdSad||v.art==Art::FaceHotSad;
  float kick=reaction(tap);int x=20+(cold ? px(wave(t,350)) : 0),y=21+px(wave(t,3200)-kick*4);
  uint16_t c=unknown ? Gray : cold ? Blue : hot ? Orange : Yellow;g.fillCircle(x,y,12,c);arc(g,x,y,12,12,kPi,1.9f*kPi,White);g.fillCircle(x-8,y+3,2,Pink);g.fillCircle(x+8,y+3,2,Pink);
  bool blink=t%4300<160,wink=kick>.25f;
  if(blink||wink)g.drawFastHLine(x-6,y-3,3,bg);else g.fillCircle(x-5,y-3,1,bg);
  if(blink)g.drawFastHLine(x+3,y-3,3,bg);else g.fillCircle(x+5,y-3,1,bg);
  arc(g,x,sad ? y+8 : y+3,5,3,sad ? kPi : 0,sad ? kTau : kPi,bg);
  if(hot){float p=phase(t,1800);drop(g,x+13,px(y-8+18*p*p),2,Blue);line(g,3,8+wave(t,1700),5,3,Red);}
  if(cold){g.fillRoundRect(x-10,y+9,20,4,1,Blue);g.fillRect(x+6,y+12,4,5,Blue);star(g,5,6,2,White);}
  if(unknown)text(g,17,3,"?",Yellow);
  if(kick>0)for(int j=0;j<3;++j)star(g,6+j*14,px(6-kick*4+(j%2)*3),1,blend(bg,c,kick));
}
void shade(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool night=v.sky==Sky::Moon||v.sky==Sky::MoonRain,unknown=v.art==Art::Unknown;
  bool wet=v.sky==Sky::Rain||v.sky==Sky::MoonRain||v.sky==Sky::SunRain;
  bool cloudy=v.sky==Sky::Cloud||v.sky==Sky::SunCloud||v.sky==Sky::Unknown;
  if(night){g.fillCircle(7,6,4,White);g.fillCircle(9,4,4,bg);}
  else if(!unknown&&(!cloudy||v.sky==Sky::SunCloud)&&!wet){g.fillCircle(7,6,4,Yellow);for(int j=0;j<4;++j){float a=j*kPi/2+phase(t,10000)*kTau;line(g,7+cosf(a)*6,6+sinf(a)*6,7+cosf(a)*8,6+sinf(a)*8,Yellow);}}
  if(wet)rainCloud(g,10,6,t,.7f);
  else if(!night&&cloudy){int dx=px(wave(t,4700));g.fillCircle(6+dx,6,3,Gray);g.fillCircle(11+dx,4,4,Gray);g.drawFastHLine(4+dx,8,12,White);}
  g.fillRect(4,15,2,23,Gray);g.fillRect(34,15,2,23,Gray);line(g,4,15,20,10,White);line(g,20,10,36,15,White);
  // The shade cloth unrolls from the left post; SELECT animates the change with easing.
  float cover=v.dome ? 1 : 0;if(tap<ReactionMs&&v.before!=v.dome){float u=ease(tap/1200.0f);cover=v.dome ? u : 1-u;}
  int edge=6+px(28*cover);
  if(edge>6){g.fillRect(6,15,edge-6,3,Stem);for(int j=0;7+j*6<edge;++j)line(g,7+j*6,14,fminf(11+j*5,edge-1),18,Green);g.drawFastVLine(edge-1,14,5,White);}
  for(int j=0;6+j*2<34;++j){int x=6+j*2;if(x>=edge&&j%3!=1)g.drawLine(x,15,x,19,blend(bg,Green,.7f));}
  for(int j=0;j<4;++j){float p=phase(t+j*500,wet ? 1500 : 2400);float yy=8+28*(wet ? p*p : p);int x=15+j*5;uint16_t c=wet ? Blue : night||cloudy||unknown ? Gray : Yellow;if(x>=edge||yy<15)line(g,x,yy,x-2,yy+3,blend(bg,c,.6f));else if(j%2==0&&yy>22)g.drawPixel(x-2,px(yy),blend(bg,c,.18f));}
  int sway=px(wave(t,3900)*(cover>.5f ? 1 : 2));line(g,21,37,21+sway,26,Stem);leaf(g,21+sway,30,-1,Green);leaf(g,21+sway,28,1,Green);g.drawFastHLine(10,38,22,Brown);
  if(unknown||v.warning)text(g,28,3,"?",Yellow);
}
void rainShelter(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool wet=v.sky==Sky::Rain||v.sky==Sky::MoonRain||v.sky==Sky::SunRain;bool unknown=v.art==Art::Unknown;
  if(v.sky==Sky::Moon||v.sky==Sky::MoonRain){g.fillCircle(6,5,4,White);g.fillCircle(8,3,4,bg);g.drawPixel(35,4,blend(bg,White,.5f+.5f*wave(t,2900)));}
  if(wet)rainCloud(g,24,4,t,.75f);
  g.drawFastHLine(3,37,34,Deep);
  // SELECT: the umbrella glides between its folded and open positions.
  float open=v.dome ? 1 : 0;if(tap<ReactionMs&&v.before!=v.dome){float u=ease(tap/1200.0f);open=v.dome ? u : 1-u;}
  int ux=px(32-12*open);
  if(open>.15f){float rx=15*open,ry=9*open;int rings=1+px(7*open);for(int r=0;r<rings;++r)arc(g,ux,20,rx-r,fmaxf(1,ry-r*.5f),kPi,kTau,r<2 ? Blue : Deep);for(int j=-1;j<=1;++j)line(g,ux,20-ry,ux+j*rx*.93f,20,White);g.drawLine(ux,20,ux,30,Gray);arc(g,ux-3,30,3,4,0,kPi,Gray);}
  else{g.drawLine(ux,10,ux,32,Gray);g.fillTriangle(ux,10,ux-4,25,ux+3,25,Blue);line(g,ux-2,13,ux,24,White);arc(g,ux-3,32,3,3,0,kPi,Gray);}
  line(g,12,36,12+wave(t,3000),29,Stem);leaf(g,12,32,-1,Green);leaf(g,12,30,1,Green);
  if(wet)for(int j=0;j<6;++j){float p=phase(t+j*177,1250);int x=3+j*7;int y=px(8+32*p*p);bool intercepted=open>.6f&&x>ux-15*open&&x<ux+15*open&&y>12&&y<30;if(!intercepted)line(g,x,y,x-1,y+3,Blue);else if(p>.6f){float f=(p-.6f)/.4f;g.drawPixel(px(x<ux ? ux-15-3*f : ux+15+3*f),px(21+15*f*f),White);}}
  else{for(int j=0;j<3;++j)arc(g,6+j*14,37,1+phase(t+j*500,2400)*4,1,0,kPi,Deep);}
  if(v.art==Art::DomeQuestion||unknown||v.warning)text(g,2,2,"?",blend(bg,Yellow,.7f+.3f*wave(t,2100)));
}
void plumbing(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool flow=v.art==Art::PumpFlow||v.art==Art::PumpSlow,empty=v.art==Art::PumpEmpty,unknown=v.art==Art::PumpUnknown;uint16_t c=empty ? Red : unknown ? Yellow : Blue;
  g.drawRoundRect(2,5,12,29,2,Gray);for(int j=0;j<4;++j)g.drawFastHLine(3,9+j*6,3,Gray);
  if(!empty&&!unknown)g.fillRect(5,v.art==Art::PumpSlow ? 27 : 17,6,v.art==Art::PumpSlow ? 4 : 14,Deep);
  g.drawRect(13,25,9,5,c);g.drawCircle(25,27,7,c);g.fillCircle(25,27,5,Deep);g.fillCircle(25,27,2,White);g.drawRect(30,13,6,14,c);g.drawRect(30,10,9,5,c);
  // SELECT: the impeller spins up or coasts down; a pressure ring marks the command.
  float a=v.art==Art::PumpSlow ? (flow ? phase(t,3000)*kTau : 0) : rotor(flow,t,v.before!=flow ? tap : NoReaction,1100,0,3);
  if(tap<ReactionMs&&tap<700){float r=bound(tap/700.0f);g.drawCircle(25,27,px(7+r*8),blend(flow ? Blue : Gray,bg,r));}
  for(int j=0;j<3;++j){float q=a+j*kTau/3;line(g,25,27,25+cosf(q)*5,27+sinf(q)*5,White);}
  if(flow){float p=phase(t,v.art==Art::PumpSlow ? 3000 : 1000);g.fillRect(14+px(6*p),27,2,1,Blue);g.fillRect(32,24-px(11*p),2,2,White);drop(g,37,17+px(17*p*p),1,Blue);arc(g,35,37,2+p*4,1,0,kTau,blend(Blue,bg,p));}
  if(empty||unknown)text(g,6,14,empty ? "!" : "?",blend(bg,c,.6f+.4f*wave(t,1600)));
}
void processor(Surface &g,const Visual &v,uint64_t t,uint16_t bg,uint32_t tap){
  bool hot=v.art==Art::CircuitHot||v.art==Art::CircuitCool,cold=v.art==Art::CircuitCold,unknown=v.art==Art::Unknown;float kick=reaction(tap);
  uint16_t c=unknown ? Gray : hot ? Orange : cold ? Blue : Green;
  for(int j=0;j<4;++j){int p=11+j*6;line(g,p,1,p,10,Stem);line(g,p,30,p,38,Stem);line(g,1,p,10,p,Stem);line(g,30,p,38,p,Stem);float u=phase(t+j*210,1900);int z=px(u*8);g.drawPixel(p,2+z,c);g.drawPixel(37-z,p,c);g.drawPixel(p,37-z,c);}
  g.fillRoundRect(9,9,22,22,3,Deep);g.drawRoundRect(9,9,22,22,3,c);g.drawRect(13,13,14,14,c);
  for(int j=0;j<3;++j){int h=3+px((wave(t+j*230,1800)+1)*3);g.fillRect(16+j*3,24-h,2,h,c);}
  if(hot){for(int j=0;j<3;++j){float p=phase(t+j*450,1700);g.drawPixel(px(14+j*6+sinf(p*6)),px(7-9*p),blend(bg,Red,1-p));}}
  if(v.art==Art::CircuitCool){for(int j=0;j<3;++j){float p=phase(t+j*440,1600);g.drawFastHLine(px(-8+p*48),3+j*3,6,Blue);}}
  if(cold){star(g,4,4,2,White);star(g,35,35,1,White);}
  if(unknown)text(g,17,16,"?",Yellow);
  if(kick>0){int r=px(7+bound(tap/1100.0f)*23);g.drawRoundRect(20-r,20-r,2*r,2*r,3,blend(bg,White,kick));}
}
uint8_t inferredPage(Art a){int n=static_cast<int>(a);if(n<=static_cast<int>(Art::WaterUnknown))return 0;if(n<=static_cast<int>(Art::GrassUnknown))return 1;if(a==Art::Weather||a==Art::Bulb||a==Art::BulbBroken)return 5;if(n<=static_cast<int>(Art::TempUnknown))return 3;if(n<=static_cast<int>(Art::HumUnknown))return 4;if(n<=static_cast<int>(Art::FaceColdSad))return 7;if(a==Art::DomeQuestion)return 9;if(n<=static_cast<int>(Art::PumpUnknown))return 10;if(n<=static_cast<int>(Art::FanStill))return 6;if(a==Art::Network||a==Art::NetworkOff)return 12;return 11;}
}
void drawVisual(Adafruit_GFX &target,int x,int y,const Visual &v,uint64_t ms,uint16_t bg,uint32_t tap){
  Surface g(target,x,y);uint8_t page=v.page==255 ? inferredPage(v.art) : v.page;
  switch(page){
    case 0:reservoir(g,v,ms,bg,tap);break;case 1:soil(g,v,ms,bg,tap);break;
    case 2:rainGauge(g,v,ms,bg,tap);break;case 3:thermometer(g,v,ms,bg,tap);break;
    case 4:dew(g,v,ms,bg,tap);break;case 5:sunlight(g,v,ms,bg,tap);break;
    case 6:coolingFan(g,v,ms,bg,tap);break;case 7:comfort(g,v,ms,bg,tap);break;
    case 8:shade(g,v,ms,bg,tap);break;case 9:rainShelter(g,v,ms,bg,tap);break;
    case 10:plumbing(g,v,ms,bg,tap);break;case 11:processor(g,v,ms,bg,tap);break;
    case 12:drawNetwork(target,x,y,v,ms,bg,tap);break;
  }
}
}
