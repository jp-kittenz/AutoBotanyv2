#include "AutobotanyTFTDrawing.h"
#include "AutobotanyPlants.h"

// Card 14 "YOUR PLANT": procedural sprite for each of the 34 presets.
// The sprite grows with the saved stage and recorded age. Everything is drawn
// inside a 52x116 pane: a growth-stage rail on the far left and the plant on
// its right. A clip surface keeps foliage off the rail and the pane border.
namespace ABTFT {
namespace {
using namespace ABTFTDraw;
constexpr uint16_t rgb(uint8_t r,uint8_t g,uint8_t b){return static_cast<uint16_t>(((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3));}
constexpr uint16_t LIME=0x9FB3,LINE=0x2A69,DIM=0x9535,TEXT=0xF7BE,AMBER=0xFE6D;
constexpr int C=31,G=98; // Plant centre and soil line, pane coordinates.

enum Arch:uint8_t {Rosette,Bush,Vine,Trail,Grass,Ginger,Tree};
enum Shape:uint8_t {Oval,Lance,Heart,Round,Spoon,Frilly,Lacy,Strap,Tube,Lobed};
enum Bloom:uint8_t {NoBloom,Star5,Umbel,Spike,Ball,Trumpet,Tassel};
enum Fruit:uint8_t {NoFruit,Tomato,Eggplant,ChiliUp,ChiliLong,Bell,Okra,Bean,Cucumber,Bitter,Citrus};
enum Flag:uint16_t {Stake=1,Trellis=2,RootWhite=4,RootOrange=8,Rhizome=16,TwoTone=32,Pale=64,Crown=128,Opposite=256};
struct Look {uint8_t arch,shape,bloom,fruit;uint16_t leaf,shade,stem,flower,young,ripe;uint8_t height,leafLen,leafWide;uint16_t flags;};

// One row per preset, in AutobotanyPlants.cpp order.
const Look Looks[]={
 {Rosette,Spoon ,Star5  ,NoFruit  ,rgb(70,165,70) ,rgb(40,118,52) ,rgb(222,240,206),rgb(250,220,60) ,0,0,46,36,6,Pale},             // pechay
 {Trail  ,Lance ,Trumpet,NoFruit  ,rgb(92,180,80) ,rgb(52,128,58) ,rgb(150,200,120),rgb(245,215,235),0,0,52,16,3,0},                // kangkong
 {Rosette,Frilly,Star5  ,NoFruit  ,rgb(96,170,70) ,rgb(56,124,52) ,rgb(170,215,140),rgb(250,220,60) ,0,0,48,32,6,0},                // mustasa
 {Vine   ,Round ,Star5  ,NoFruit  ,rgb(60,150,70) ,rgb(36,108,52) ,rgb(176,52,104) ,rgb(250,200,225),0,0,86,10,5,Trellis},          // alugbati
 {Bush   ,Lance ,Star5  ,NoFruit  ,rgb(80,160,60) ,rgb(46,112,44) ,rgb(120,150,70) ,rgb(250,215,50) ,0,0,70,13,3,0},                // saluyot
 {Bush   ,Oval  ,Tassel ,NoFruit  ,rgb(100,170,70),rgb(150,50,70) ,rgb(170,80,90)  ,rgb(200,60,90)  ,0,0,64,12,5,TwoTone},          // kulitis
 {Rosette,Frilly,Star5  ,NoFruit  ,rgb(150,215,90),rgb(98,170,60) ,rgb(200,235,160),rgb(250,225,70) ,0,0,36,24,7,0},                // lettuce
 {Tree   ,Lacy  ,Umbel  ,NoFruit  ,rgb(110,180,80),rgb(64,130,56) ,rgb(150,110,70) ,rgb(245,240,200),0,0,76,16,2,0},                // malunggay
 {Bush   ,Lobed ,Star5  ,Tomato   ,rgb(76,156,64) ,rgb(44,110,48) ,rgb(110,160,70) ,rgb(250,220,40) ,rgb(120,190,70),rgb(232,60,48) ,80,12,4,Stake},  // kamatis
 {Bush   ,Oval  ,Star5  ,Eggplant ,rgb(100,150,90),rgb(62,104,66) ,rgb(110,80,120) ,rgb(176,120,220),rgb(150,90,190),rgb(96,40,130) ,72,15,6,0},       // talong
 {Bush   ,Lance ,Star5  ,ChiliUp  ,rgb(64,150,60) ,rgb(36,104,44) ,rgb(90,140,60)  ,rgb(245,245,235),rgb(90,175,60) ,rgb(230,40,36) ,58,10,3,0},       // siling labuyo
 {Bush   ,Lance ,Star5  ,ChiliLong,rgb(70,155,62) ,rgb(40,108,46) ,rgb(96,146,64)  ,rgb(245,245,235),rgb(110,190,70),rgb(60,150,50) ,66,12,3,0},       // siling haba
 {Bush   ,Oval  ,Star5  ,Bell     ,rgb(70,150,64) ,rgb(40,104,48) ,rgb(96,146,64)  ,rgb(245,245,235),rgb(80,170,60) ,rgb(222,44,40) ,60,13,5,0},       // bell pepper
 {Bush   ,Lobed ,Star5  ,Okra     ,rgb(86,160,70) ,rgb(50,112,52) ,rgb(120,150,80) ,rgb(250,235,150),rgb(130,200,90),rgb(80,160,60) ,88,13,6,0},       // okra
 {Vine   ,Oval  ,Star5  ,Bean     ,rgb(80,165,70) ,rgb(46,116,52) ,rgb(110,160,80) ,rgb(200,170,235),rgb(130,200,90),rgb(70,150,50) ,70,9,4,Trellis}, // sitaw
 {Vine   ,Lobed ,Star5  ,Cucumber ,rgb(80,160,64) ,rgb(46,112,48) ,rgb(110,160,70) ,rgb(250,225,50) ,rgb(120,190,80),rgb(50,130,50) ,80,11,6,Trellis}, // pipino
 {Vine   ,Lobed ,Star5  ,Bitter   ,rgb(88,170,70) ,rgb(52,120,52) ,rgb(110,160,70) ,rgb(250,225,50) ,rgb(130,200,90),rgb(245,140,40),88,9,5,Trellis},  // ampalaya
 {Rosette,Spoon ,Star5  ,NoFruit  ,rgb(90,165,70) ,rgb(52,118,52) ,rgb(176,210,150),rgb(240,230,250),0,0,40,28,5,RootWhite},        // labanos
 {Rosette,Lacy  ,Umbel  ,NoFruit  ,rgb(90,175,70) ,rgb(52,124,52) ,rgb(120,180,90) ,rgb(250,250,245),0,0,48,30,2,RootOrange},       // carrot
 {Trail  ,Heart ,Trumpet,NoFruit  ,rgb(96,160,70) ,rgb(120,60,110),rgb(140,90,120) ,rgb(220,180,240),0,0,44,11,6,TwoTone},          // kamote
 {Ginger ,Lance ,NoBloom,NoFruit  ,rgb(76,160,70) ,rgb(44,112,50) ,rgb(140,170,90) ,0,0,0,70,13,3,Rhizome},                        // luya
 {Rosette,Oval  ,NoBloom,NoFruit  ,rgb(86,170,76) ,rgb(48,120,56) ,rgb(140,180,100),0,0,0,70,30,6,Rhizome|RootOrange},                        // luyang dilaw
 {Bush   ,Oval  ,Spike  ,NoFruit  ,rgb(70,180,70) ,rgb(40,130,52) ,rgb(100,160,70) ,rgb(245,245,245),0,0,50,13,5,Opposite},                // basil
 {Bush   ,Oval  ,Spike  ,NoFruit  ,rgb(90,175,90) ,rgb(52,124,66) ,rgb(110,140,90) ,rgb(200,170,235),0,0,40,11,4,Opposite},                 // mint
 {Bush   ,Round ,Spike  ,NoFruit  ,rgb(140,200,120),rgb(96,160,90),rgb(130,170,110),rgb(200,170,235),0,0,42,11,5,Opposite},              // oregano
 {Rosette,Lacy  ,Umbel  ,NoFruit  ,rgb(96,180,72) ,rgb(54,128,52) ,rgb(130,190,100),rgb(250,250,245),0,0,42,28,2,0},                // wansoy
 {Rosette,Lance ,Spike  ,NoFruit  ,rgb(70,150,64) ,rgb(40,108,48) ,rgb(110,160,90) ,rgb(140,190,180),0,0,34,28,4,0},                // kulantro
 {Rosette,Lacy  ,Umbel  ,NoFruit  ,rgb(90,170,70) ,rgb(50,120,50) ,rgb(170,215,140),rgb(250,250,245),0,0,46,32,3,Pale},             // kinchay
 {Grass  ,Strap ,NoBloom,NoFruit  ,rgb(130,190,90),rgb(90,150,70) ,rgb(230,200,200),0,0,0,88,0,1,Pale},                            // tanglad
 {Grass  ,Strap ,NoBloom,NoFruit  ,rgb(50,150,60) ,rgb(30,104,44) ,rgb(90,150,70)  ,0,0,0,70,0,3,0},                               // pandan
 {Grass  ,Tube  ,Ball   ,NoFruit  ,rgb(100,175,110),rgb(64,130,80),rgb(236,240,226),rgb(245,245,240),0,0,62,0,1,Pale},            // spring onion
 {Grass  ,Tube  ,Ball   ,NoFruit  ,rgb(80,165,80) ,rgb(50,120,60) ,rgb(200,225,190),rgb(190,120,220),0,0,46,0,0,0},                // chives
 {Grass  ,Strap ,Ball   ,NoFruit  ,rgb(110,170,100),rgb(70,126,72),rgb(240,236,226),rgb(240,230,240),0,0,52,0,1,RootWhite},       // bawang
 {Tree   ,Oval  ,Star5  ,Citrus   ,rgb(40,130,56) ,rgb(24,92,42)  ,rgb(120,90,60)  ,rgb(250,250,245),rgb(90,170,60) ,rgb(250,150,30),80,7,3,Crown}    // calamansi
};
static_assert(sizeof(Looks)/sizeof(Looks[0])==34,"One look per plant preset");

// Clipped, offset drawing target: foliage never reaches the rail or pane edge.
class Clip:public Adafruit_GFX {
  Adafruit_GFX &out_;int ox_,oy_,x0_,y0_,x1_,y1_;
public:
  Clip(Adafruit_GFX &out,int ox,int oy,int x0,int y0,int x1,int y1):Adafruit_GFX(PlantPaneW,PlantPaneH),out_(out),ox_(ox),oy_(oy),x0_(x0),y0_(y0),x1_(x1),y1_(y1){setTextWrap(false);}
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=x0_&&x<=x1_&&y>=y0_&&y<=y1_)out_.drawPixel(ox_+x,oy_+y,c);}
};

struct Pt{float x,y;};
inline void tri(Adafruit_GFX &g,Pt a,Pt b,Pt c,uint16_t col){g.fillTriangle(px(a.x),px(a.y),px(b.x),px(b.y),px(c.x),px(c.y),col);}
inline float hash(int n){uint32_t h=static_cast<uint32_t>(n)*2654435761UL;h^=h>>15;return float(h%1000)/999.0f;}
inline float clampf(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}

float profile(uint8_t shape,float u){
  switch(shape){
    case Lance:return sinf(kPi*powf(u,.6f));
    case Heart:return u<.2f ? .55f+.45f*u/.2f : cosf((u-.2f)/.8f*kPi*.5f);
    case Round:case Frilly:return sqrtf(fmaxf(0,sinf(kPi*u)));
    case Spoon:return u<.38f ? .16f : sqrtf(fmaxf(0,sinf(kPi*(u-.38f)/.62f)));
    case Strap:return u<.78f ? 1-.25f*u : (1-.25f*.78f)*(1-u)/.22f;
    case Tube:return u<.9f ? 1 : (1-u)/.1f;
    case Lobed:return (u<.2f ? .55f+.45f*u/.2f : cosf((u-.2f)/.8f*kPi*.5f))*(.78f+.22f*cosf(u*kPi*6));
    default:return sinf(kPi*powf(u,.8f));
  }
}
// A curved, two-tone leaf: light upper half, shaded lower half and a midrib.
void leaf(Adafruit_GFX &g,float bx,float by,float angle,float len,float wid,float droop,uint8_t shape,uint16_t c,uint16_t d,uint16_t rib){
  if(len<1.5f){g.drawPixel(px(bx),px(by),c);return;}
  if(shape==Lacy){
    float dx=cosf(angle),dy=sinf(angle);Pt last{bx,by};
    for(int k=1;k<=6;++k){float u=k/6.0f;Pt p{bx+dx*len*u,by+dy*len*u+droop*len*u*u};line(g,last.x,last.y,p.x,p.y,rib);last=p;
      if(k>=2){float s=len*(.34f-.03f*k);float tx=dx,ty=dy+2*droop*u;float a=atan2f(ty,tx);
        leaf(g,p.x,p.y,a-1.0f,s,s*.38f+.4f,droop*.4f,Round,c,d,c);leaf(g,p.x,p.y,a+1.0f,s,s*.38f+.4f,droop*.4f,Round,d,d,d);}}
    leaf(g,last.x,last.y,atan2f(dy+2*droop,dx),len*.2f,len*.09f+.6f,0,Round,c,d,c);return;
  }
  const int N=len>14 ? 7 : len>6 ? 5 : 3;
  float dx=cosf(angle),dy=sinf(angle);Pt pl{bx,by},pr{bx,by},pc{bx,by};
  for(int k=1;k<=N;++k){
    float u=float(k)/N;float cx=bx+dx*len*u,cy=by+dy*len*u+droop*len*u*u;
    float tx=dx,ty=dy+2*droop*u,tl=sqrtf(tx*tx+ty*ty);float mx=-ty/tl,my=tx/tl;
    float w=wid*profile(shape,u);Pt l{cx+mx*w,cy+my*w},r{cx-mx*w,cy-my*w},m{cx,cy};
    tri(g,pc,pl,l,c);tri(g,pc,l,m,c);tri(g,pc,pr,r,d);tri(g,pc,r,m,d);
    if(shape==Frilly&&k<N&&wid>2.5f){g.fillCircle(px(l.x),px(l.y),1,c);g.fillCircle(px(r.x),px(r.y),1,d);}
    pl=l;pr=r;pc=m;
  }
  if(len>=6&&shape!=Tube&&shape!=Strap){Pt last{bx,by};for(int k=1;k<=4;++k){float u=.85f*k/4;Pt p{bx+dx*len*u,by+dy*len*u+droop*len*u*u};line(g,last.x,last.y,p.x,p.y,rib);last=p;}}
}
void flower(Adafruit_GFX &g,uint8_t type,float x,float y,uint16_t col,float open,uint64_t t,int seed){
  int ix=px(x),iy=px(y);if(open<=0)return;
  switch(type){
    case Star5:{int r=open>.6f ? 2 : 1;for(int k=0;k<5;++k){float a=k*kTau/5+phase(t+seed*90,9000)*.6f;g.fillCircle(px(x+cosf(a)*r),px(y+sinf(a)*r),open>.6f?1:0,col);}g.drawPixel(ix,iy,Yellow);break;}
    case Umbel:{for(int k=-2;k<=2;++k){int hx=ix+k*2,hy=iy-2+(k*k)/2;line(g,ix,iy+2,hx,hy,Stem);g.drawPixel(hx,hy,col);g.drawPixel(hx,hy-1,col);}break;}
    case Spike:{int n=2+px(open*3);g.drawFastVLine(ix,iy-n*2-1,n*2+2,Stem);for(int k=0;k<n;++k){g.drawPixel(ix-1,iy-k*2,col);g.drawPixel(ix+1,iy-k*2,col);}g.drawPixel(ix,iy-n*2-1,col);break;}
    case Ball:{int r=open>.6f ? 3 : 2;g.fillCircle(ix,iy,r,col);g.drawPixel(ix-1,iy-1,White);g.drawPixel(ix+1,iy+1,blend(col,0,.3f));break;}
    case Trumpet:{g.fillTriangle(ix,iy+2,ix-3,iy-2,ix+3,iy-2,col);g.drawFastHLine(ix-3,iy-2,7,White);g.drawPixel(ix,iy-1,Yellow);break;}
    case Tassel:{int n=3+px(open*5);for(int k=0;k<n;++k){float s=sinf(k*.6f+phase(t+seed*70,3000)*kTau)*.8f;g.fillCircle(px(x+s+k*.5f),iy+k,1,blend(col,Green,k/float(n+2)));}break;}
    default:break;
  }
}
void fruit(Adafruit_GFX &g,uint8_t type,float x,float y,float size,bool ripe,const Look &k,uint64_t t,int seed){
  float sw=wave(t+seed*311,2900)*.8f;x+=sw;int ix=px(x),iy=px(y);uint16_t c=ripe ? k.ripe : k.young;if(size<=.05f)return;
  switch(type){
    case Tomato:{int r=1+px(size*2.6f);g.drawFastVLine(ix,iy-r-1,2,Stem);g.fillCircle(ix,iy,r,c);g.drawPixel(ix-1,iy-1,blend(c,White,.6f));g.drawPixel(ix-1,iy-r,Green);g.drawPixel(ix+1,iy-r,Green);break;}
    case Eggplant:{int r=1+px(size*2);g.fillCircle(ix,iy+r,r+1,c);g.fillCircle(ix,iy,r,c);g.drawPixel(ix-1,iy+r,blend(c,White,.5f));g.fillRect(ix-1,iy-r-1,3,2,Green);break;}
    case ChiliUp:{int h=3+px(size*4);g.fillTriangle(ix-1,iy,ix+1,iy,ix,iy-h,c);g.drawPixel(ix,iy+1,Green);break;}
    case ChiliLong:{int h=3+px(size*9);for(int j=0;j<h;++j){int cx=ix+px(sinf(j*.3f)*1.2f);g.drawFastHLine(cx-(j<h-2?1:0),iy+j,j<h-2?2:1,c);}g.drawPixel(ix,iy-1,Green);break;}
    case Bell:{int r=1+px(size*2.5f);g.fillRoundRect(ix-r,iy-r,2*r+1,2*r+2,r>1?2:0,c);g.drawPixel(ix-r+1,iy-r+1,blend(c,White,.5f));g.drawFastHLine(ix-1,iy-r-1,3,Green);break;}
    case Okra:{int h=3+px(size*6);g.fillTriangle(ix-1,iy,ix+1,iy,ix,iy-h,c);g.drawLine(ix,iy-1,ix,iy-h+1,blend(c,White,.3f));g.drawPixel(ix,iy+1,Green);break;}
    case Bean:{int h=4+px(size*11);for(int s=-1;s<=1;s+=2){for(int j=0;j<h;++j)g.drawPixel(ix+s+px(sinf(j*.25f+s)*.8f),iy+j,c);}break;}
    case Cucumber:{int h=3+px(size*7);g.fillRoundRect(ix-1-(size>.5f),iy,3+(size>.5f)*2,h,1,c);for(int j=1;j<h;j+=2)g.drawPixel(ix,iy+j,blend(c,White,.45f));g.drawPixel(ix,iy-1,Green);break;}
    case Bitter:{int h=3+px(size*8);g.fillRoundRect(ix-1-(size>.5f),iy,3+(size>.5f)*2,h,1,c);for(int j=1;j<h;++j)g.drawPixel(ix+(j%2?1:-1),iy+j,blend(c,White,.35f));if(ripe&&size>.8f)g.drawPixel(ix,iy+h-1,Red);break;}
    case Citrus:{int r=size>.6f ? 2 : 1;g.fillCircle(ix,iy,r,c);g.drawPixel(ix-1,iy-1,blend(c,White,.55f));break;}
    default:break;
  }
}

struct Scene {
  Adafruit_GFX &g;const Look &k;float grow,p,sway,droop;uint8_t stage;uint64_t t;
  float topY; // highest drawn point, for sparkles and visitors
  Scene(Adafruit_GFX &out,const Look &look,float gr,float pr,float sw,float dr,uint8_t st,uint64_t ms):g(out),k(look),grow(gr),p(pr),sway(sw),droop(dr),stage(st),t(ms),topY(G){}
};
float swayAt(const Scene &s,float frac,int seed){return s.sway*frac*frac+wave(s.t+seed*373,2600+seed*97)*.05f;}
void note(Scene &s,float y){if(y<s.topY)s.topY=y;}
float leafLen(const Scene &s){return s.k.leafLen*(.42f+.58f*s.grow);}
float leafWide(const Scene &s){return s.k.leafWide*(.5f+.5f*s.grow)+.5f;}
bool blooming(const Scene &s){return s.k.bloom!=NoBloom&&(s.stage==ABPlant::Flowering||(s.stage==ABPlant::Fruiting&&s.p<.6f)||(s.stage==ABPlant::Mature&&s.k.fruit==NoFruit&&s.k.bloom==Ball));}
bool fruiting(const Scene &s){return s.k.fruit!=NoFruit&&s.stage>=ABPlant::Fruiting;}
float fruitSize(const Scene &s){return s.stage==ABPlant::Fruiting ? .3f+.7f*s.p : 1;}
bool ripe(const Scene &s){return s.stage==ABPlant::Mature||(s.stage==ABPlant::Fruiting&&s.p>.85f);}

void cotyledons(Scene &s,float x,float y,float size){
  uint16_t c=blend(s.k.leaf,White,.25f),d=s.k.leaf;float a=swayAt(s,1,7)*1.5f;
  leaf(s.g,x,y,-.35f+a,size,size*.42f,.05f+s.droop*.3f,Oval,c,d,d);leaf(s.g,x,y,kPi+.35f+a,size,size*.42f,.05f+s.droop*.3f,Oval,d,d,c);note(s,y-size*.4f);
}
void rootShoulder(Scene &s){
  if(!(s.k.flags&(RootWhite|RootOrange|Rhizome))||s.stage<ABPlant::Seedling)return;
  float r=1.5f+s.grow*4.5f;
  if(s.k.flags&Rhizome){uint16_t c=(s.k.flags&RootOrange) ? rgb(232,170,44) : rgb(205,170,120);s.g.fillRoundRect(C-px(r*1.6f),G-2,px(r*3.2f),4,2,c);s.g.drawPixel(C-px(r),G-2,blend(c,White,.5f));return;}
  uint16_t c=(s.k.flags&RootOrange) ? rgb(245,130,30) : rgb(245,245,240),top=(s.k.flags&RootOrange) ? rgb(250,160,60) : rgb(206,232,186);
  s.g.fillCircle(C,G,px(r),c);s.g.fillCircle(C,G-1,px(r*.7f),top);s.g.drawPixel(C-px(r*.5f),G-px(r*.6f),White);
}
void rosette(Scene &s){
  rootShoulder(s);
  const Look &k=s.k;int n=3+px(s.grow*5);float L=leafLen(s),W=leafWide(s);
  if(k.flags&Rhizome){n=2+px(s.grow*3);}
  if(blooming(s)||(s.stage==ABPlant::Mature&&k.bloom==Umbel&&s.p>.7f)){
    float h=k.height*(.55f+.4f*s.grow);float sw=swayAt(s,1,3)*6;float tx=C+sw,ty=G-h;
    line(s.g,C,G,tx,ty,k.stem);line(s.g,C+1,G,tx+1,ty,blend(k.stem,0,.25f));
    for(int j=1;j<=3;++j){float f=j/4.0f;float x=C+sw*f*f,y=G-h*f;leaf(s.g,x,y,(j%2?-.5f:-kPi+.5f),L*.35f,W*.4f,.1f,Lance,k.leaf,k.shade,k.shade);}
    for(int j=0;j<(k.bloom==Umbel ? 3 : 5);++j){float a=-kPi/2+(j-1.5f)*.55f;float fx=tx+cosf(a)*(4+j%2*2),fy=ty+sinf(a)*3+2;line(s.g,tx,ty+3,fx,fy,k.stem);flower(s.g,k.bloom,fx,fy,k.flower,s.stage==ABPlant::Flowering?.4f+.6f*s.p:1,s.t,j);}
    note(s,ty-4);
  }
  for(int j=0;j<n;++j){
    int side=(j%2)?1:-1;float order=1-float(j)/n;float spread=(k.flags&Rhizome) ? .35f : k.shape==Frilly ? .85f : .7f;
    float a=-kPi/2+side*(spread*order+.12f)+swayAt(s,1,j)*.9f;float len=L*(.65f+.35f*(1-order*.6f))*(.9f+.2f*hash(j+k.height));
    float dr=.10f+s.droop+(k.shape==Spoon ? .12f : 0)+order*.12f;
    if(k.shape==Spoon){float u=.38f;float ex=C+cosf(a)*len*u,ey=G+sinf(a)*len*u+dr*len*u*u;line(s.g,C,G,ex,ey,k.stem);line(s.g,C+side,G,ex+side*.6f,ey,k.stem);}
    if(k.flags&Rhizome){float sx=C+side*(1+j),sy=G-L*.18f;line(s.g,C+side*j*.5f,G,sx,sy,k.stem);leaf(s.g,sx,sy,a,len*.85f,W,dr,k.shape,k.leaf,k.shade,k.stem);note(s,sy-len*.85f);}
    else{leaf(s.g,C,G,a,len,W,dr,k.shape,j%3==0 ? blend(k.leaf,White,.12f) : k.leaf,k.shade,k.shape==Lacy ? k.stem : blend(k.leaf,White,.25f));note(s,G+sinf(a)*len);}
  }
}
void bush(Scene &s){
  // Alternate (or opposite) leaves on short petioles; herbs branch once they fill out.
  const Look &k=s.k;float h=fmaxf(12,k.height*s.grow);float L=leafLen(s),W=leafWide(s);bool opposite=k.flags&Opposite;
  int n=static_cast<int>(h/(opposite ? 11 : 7.5f));if(n<2)n=2;if(n>9)n=9;
  if(k.flags&Stake){s.g.drawFastVLine(C+9,px(G-k.height*.9f),px(k.height*.9f),rgb(180,150,100));s.g.drawPixel(C+9,px(G-k.height*.9f),rgb(220,190,140));}
  auto at=[&](float f)->Pt{Pt p{C+swayAt(s,f,1)*h*.35f+sinf(f*5+k.height)*.9f,G-h*f};return p;};
  Pt last=at(0);for(int j=1;j<=10;++j){Pt p=at(j/10.0f);line(s.g,last.x,last.y,p.x,p.y,k.stem);if(s.grow>.45f&&j<6)line(s.g,last.x+1,last.y,p.x+1,p.y,blend(k.stem,0,.3f));last=p;}
  if(k.flags&Stake){for(int j=1;j<=2;++j){Pt p=at(j*.33f);s.g.drawFastHLine(px(p.x)+1,px(p.y),C+9-px(p.x)-1,rgb(220,200,160));}}
  auto petioleLeaf=[&](Pt p,float a,float size,float wid,uint16_t c,float f){
    float pl=fminf(3,size*.18f);Pt q{p.x+cosf(a)*pl,p.y+sinf(a)*pl};line(s.g,p.x,p.y,q.x,q.y,k.stem);
    leaf(s.g,q.x,q.y,a,size,wid,.16f+s.droop+.12f*(1-f),k.shape,c,k.shade,(k.flags&TwoTone) ? rgb(170,60,80) : blend(k.leaf,White,.22f));};
  if(opposite&&s.grow>.5f){
    // Two side shoots make basil, mint and oregano bushy instead of a single spire.
    for(int side=-1;side<=1;side+=2){Pt b=at(.28f+(side>0 ? .1f : 0));Pt tip{b.x+side*(9+s.grow*4),b.y-h*.28f};line(s.g,b.x,b.y,tip.x,tip.y,k.stem);
      for(int j=0;j<2;++j){float u=.55f+.45f*j;Pt p{b.x+(tip.x-b.x)*u,b.y+(tip.y-b.y)*u};float sz=L*(.7f-.12f*j);
        petioleLeaf(p,-kPi/2-1.1f+swayAt(s,.5f,j+side*3)*.6f,sz,W*.85f,k.leaf,.5f);petioleLeaf(p,-kPi/2+1.1f+swayAt(s,.5f,j-side*3)*.6f,sz,W*.85f,k.shade,.5f);}
      leaf(s.g,tip.x,tip.y,-kPi/2+side*.3f,L*.45f,W*.6f,.05f,k.shape,blend(k.leaf,White,.15f),k.leaf,k.leaf);
      if(blooming(s))flower(s.g,k.bloom,tip.x,tip.y-3,k.flower,s.stage==ABPlant::Flowering ? .35f+.65f*s.p : .8f,s.t,side+5);}
  }
  int fruitNode=0;
  for(int i=0;i<n;++i){
    float f=(i+.8f)/(n+.6f)*.92f;Pt p=at(f);float size=L*(1.05f-.4f*f)*(.88f+.24f*hash(i*7+k.height));
    float spreadA=1.25f-.55f*f;
    uint16_t c=(k.flags&TwoTone)&&i%2 ? blend(k.leaf,k.shade,.5f) : (i%3==1 ? blend(k.leaf,White,.1f) : k.leaf);
    if(opposite){for(int side=-1;side<=1;side+=2)petioleLeaf(p,-kPi/2+side*(spreadA+((i%2)?.15f:0))+swayAt(s,f,i+side)*.7f,size*.9f,W,c,f);}
    else{int side=(i%2)?1:-1;petioleLeaf(p,-kPi/2+side*spreadA+swayAt(s,f,i)*.7f,size,W,c,f);}
    if(fruiting(s)&&i>0&&i<n-1&&fruitNode<4&&(i%2==0||n<5)){int side=(i%2)?-1:1;fruit(s.g,k.fruit,p.x+side*(2+px(size*.15f)),p.y+(k.fruit==ChiliUp||k.fruit==Okra ? -1 : 3),fruitSize(s)*(.75f+.25f*hash(i)),ripe(s),k,s.t,i);fruitNode++;}
  }
  Pt top=at(1);leaf(s.g,top.x,top.y,-kPi/2-.45f,L*.45f,W*.6f,.05f,k.shape,blend(k.leaf,White,.18f),k.leaf,k.leaf);leaf(s.g,top.x,top.y,-kPi/2+.45f,L*.45f,W*.6f,.05f,k.shape,k.leaf,k.shade,k.shade);note(s,top.y-L*.45f);
  if(blooming(s)){float open=s.stage==ABPlant::Flowering ? .35f+.65f*s.p : .8f;
    flower(s.g,k.bloom,top.x,top.y-2,k.flower,open,s.t,1);
    if(!opposite)for(int i=n/2;i<n;i+=2){float f=(i+.8f)/(n+.6f)*.92f;Pt p=at(f);int side=(i%2)?-1:1;flower(s.g,k.bloom,p.x+side*3,p.y-2,k.flower,open,s.t,i+3);}
    note(s,top.y-8);}
}
void vine(Scene &s){
  const Look &k=s.k;float h=fmaxf(10,k.height*s.grow);float L=leafLen(s),W=leafWide(s);
  uint16_t pole=rgb(176,146,98);int topY=px(G-k.height*.96f);
  if(k.flags&Trellis){for(int x:{C-9,C+11})s.g.drawFastVLine(x,topY,G-topY,pole);for(int y=G-12;y>topY;y-=16)s.g.drawFastHLine(C-9,y,21,blend(pole,0,.2f));}
  auto at=[&](float f)->Pt{Pt p{C+1+9*sinf(f*kPi*2.3f+.4f)*fminf(1,f*4)+swayAt(s,f,2)*6,G-h*f};return p;};
  Pt last=at(0);for(int j=1;j<=18;++j){Pt p=at(j/18.0f);line(s.g,last.x,last.y,p.x,p.y,k.stem);last=p;}
  int n=3+px(s.grow*8);int fruits=0;
  for(int i=0;i<n;++i){float f=(i+.6f)/n;Pt p=at(f);float side=cosf(f*kPi*2.3f+.4f)>0 ? 1 : -1;if(i%2)side=-side;
    float a=(side>0 ? -.25f : kPi+.25f)+swayAt(s,f,i)*.8f;float size=L*(.75f+.25f*hash(i+k.height));
    leaf(s.g,p.x,p.y,a,size,W,.3f+s.droop,k.shape,i%3==1 ? blend(k.leaf,White,.12f) : k.leaf,k.shade,blend(k.leaf,White,.22f));
    if(i%3==2&&s.grow>.3f)arc(s.g,p.x-side*3,p.y-2,2,2,0,kPi*1.5f,blend(k.stem,White,.3f),6);
    if(fruiting(s)&&i>0&&i%2==0&&fruits<3&&f<.85f){fruit(s.g,k.fruit,p.x-side*3,p.y+2,fruitSize(s)*(.8f+.2f*hash(i)),ripe(s),k,s.t,i);fruits++;}
    if(blooming(s)&&i%2==1)flower(s.g,k.bloom,p.x-side*4,p.y-2,k.flower,s.stage==ABPlant::Flowering ? .4f+.6f*s.p : .8f,s.t,i);
  }
  Pt top=at(1);leaf(s.g,top.x,top.y,-kPi/2,L*.45f,W*.55f,.1f,k.shape,blend(k.leaf,White,.2f),k.leaf,k.leaf);note(s,fminf(top.y-L*.45f,(k.flags&Trellis) ? topY : G));
}
void trail(Scene &s){
  const Look &k=s.k;float L=leafLen(s),W=leafWide(s);int runners=2+px(s.grow*2);
  for(int r=0;r<runners;++r){
    int side=(r%2)?1:-1;float reach=8+s.grow*(r<2 ? 9 : 5);float rise=k.height*s.grow*(r<2 ? .7f : 1.0f)+6;
    Pt a{C,G},b{C+side*3.0f,G-rise},c{C+side*reach,G-rise*.45f+s.grow*12};
    Pt last=a;int leaves=2+px(s.grow*4);
    for(int j=1;j<=12;++j){float u=j/12.0f,v=1-u;float sw=swayAt(s,u,r)*5;Pt p{v*v*a.x+2*v*u*b.x+u*u*c.x+sw,v*v*a.y+2*v*u*b.y+u*u*c.y};line(s.g,last.x,last.y,p.x,p.y,k.stem);
      if(j%(12/leaves>0?12/leaves:1)==0){float ang=(j%2 ? -kPi/2-side*.9f : -kPi/2+side*1.3f)+swayAt(s,u,j)*.7f;leaf(s.g,p.x,p.y,ang,L*(.8f+.2f*hash(j+r)),W,.25f+s.droop,k.shape,(k.flags&TwoTone)&&j%3==0 ? k.shade : k.leaf,blend(k.leaf,0,.25f),blend(k.leaf,White,.22f));
        if(blooming(s)&&j%4==0)flower(s.g,k.bloom,p.x+side*2,p.y-3,k.flower,s.stage==ABPlant::Flowering ? .4f+.6f*s.p : .8f,s.t,j+r);}
      note(s,p.y-L*.8f);last=p;}
  }
}
void grass(Scene &s){
  const Look &k=s.k;rootShoulder(s);bool tube=k.shape==Tube;int n=3+px(s.grow*(tube ? 4 : 7));
  float droop=k.height>80 ? .55f : k.leafWide>=3 ? .32f : tube ? .03f : .12f;float wid=k.leafWide*(.6f+.4f*s.grow)+(tube ? .9f : .6f);
  for(int j=0;j<n;++j){
    float off=(j-(n-1)*.5f)/fmaxf(1,(n-1)*.5f);float a=-kPi/2+off*(tube ? .32f : .55f)+swayAt(s,1,j)*.7f;
    float len=k.height*s.grow*(.72f+.28f*hash(j*5+k.height))*(1-.15f*fabsf(off));
    uint16_t c=j%2 ? k.leaf : blend(k.leaf,White,.12f);
    leaf(s.g,C+off*2,G,a,len,wid,(droop+s.droop)*(off<0 ? 1 : 1),tube ? Tube : Strap,c,k.shade,c);
    if(k.flags&Pale)line(s.g,C+off*2,G,C+off*2+cosf(a)*5,G+sinf(a)*5,k.stem);
    note(s,G-len);
  }
  if(blooming(s)){float h=k.height*s.grow*1.05f;float sw=swayAt(s,1,9)*5;line(s.g,C,G,C+sw,G-h,k.leaf);flower(s.g,Ball,C+sw,G-h-2,k.flower,s.stage==ABPlant::Flowering ? .5f+.5f*s.p : 1,s.t,1);note(s,G-h-5);}
}
void ginger(Scene &s){
  const Look &k=s.k;rootShoulder(s);int stems=1+px(s.grow*2);float L=leafLen(s),W=leafWide(s);
  for(int st=0;st<stems;++st){float off=(st-(stems-1)*.5f)*6;float h=k.height*s.grow*(1-.18f*st);float sw=swayAt(s,1,st)*6;
    line(s.g,C+off,G,C+off+sw,G-h,k.stem);line(s.g,C+off+1,G,C+off+sw+1,G-h,blend(k.stem,0,.3f));
    int n=2+px(s.grow*4);for(int i=0;i<n;++i){float f=.3f+.7f*(i+1.0f)/(n+1);int side=(i%2)?1:-1;float x=C+off+sw*f*f,y=G-h*f;
      leaf(s.g,x,y,-kPi/2+side*1.0f+swayAt(s,f,i+st)*.6f,L*(1.1f-.3f*f),W,.22f+s.droop,Lance,k.leaf,k.shade,blend(k.leaf,White,.22f));}
    note(s,G-h-L*.5f);}
}
void tree(Scene &s){
  const Look &k=s.k;float h=fmaxf(16,k.height*s.grow);float L=leafLen(s),W=leafWide(s);uint16_t bark=k.stem;
  float trunk=h*(k.flags&Crown ? .42f : .55f);float sw=swayAt(s,1,4)*5;
  Pt top{C+sw*.5f,G-trunk};line(s.g,C,G,top.x,top.y,bark);line(s.g,C+1,G,top.x+1,top.y,blend(bark,0,.3f));if(s.grow>.5f)line(s.g,C-1,G,top.x-1,top.y,blend(bark,White,.15f));
  if(k.flags&Crown){
    float r=5+s.grow*12;Pt cen{top.x+sw*.5f,top.y-r*.7f};
    for(int b=-1;b<=1;++b)line(s.g,top.x,top.y,cen.x+b*r*.5f,cen.y+2,bark);
    int n=8+px(s.grow*16);for(int i=0;i<n;++i){float a=i*2.39996f;float rr=r*sqrtf((i+.5f)/n);float x=cen.x+cosf(a)*rr,y=cen.y+sinf(a)*rr*.8f;
      leaf(s.g,x,y,a+swayAt(s,1,i)*.8f,L*(.9f+.3f*hash(i)),W,.08f+s.droop*.5f,Oval,i%3 ? k.leaf : blend(k.leaf,White,.18f),k.shade,k.shade);}
    if(fruiting(s))for(int i=0;i<5;++i){float a=i*1.7f+.6f;float rr=r*(.35f+.12f*i);fruit(s.g,Citrus,cen.x+cosf(a)*rr,cen.y+sinf(a)*rr*.8f+2,fruitSize(s),ripe(s),k,s.t,i);}
    if(blooming(s))for(int i=0;i<4;++i){float a=i*1.9f+.3f;flower(s.g,Star5,cen.x+cosf(a)*r*.7f,cen.y+sinf(a)*r*.6f,k.flower,.8f,s.t,i);}
    note(s,cen.y-r*.8f-L*.4f);return;
  }
  int branches=2+px(s.grow*2);
  for(int b=0;b<branches;++b){float side=(b%2)?1:-1;float bh=h*(.62f+.38f*(b+1)/branches);Pt tip{C+side*(5+4*b)+sw,G-bh};Pt from{top.x,top.y+b*2};line(s.g,from.x,from.y,tip.x,tip.y,bark);
    for(int j=0;j<3;++j){float a=-kPi/2+side*(.3f+j*.55f)+swayAt(s,1,b*3+j)*.8f;leaf(s.g,tip.x,tip.y,a,L*(.85f+.15f*hash(j+b)),W,.45f+s.droop,Lacy,k.leaf,k.shade,blend(k.stem,White,.2f));}
    if(blooming(s))flower(s.g,Umbel,tip.x,tip.y-3,k.flower,.8f,s.t,b);
    note(s,tip.y-L*.6f);}
}
void sprout(Scene &s){
  const Look &k=s.k;
  if(k.arch==Grass){float h=6+s.p*8;float sw=swayAt(s,1,1)*3;leaf(s.g,C,G,-kPi/2+.15f+sw,h,.9f,.25f,k.shape==Tube ? Tube : Strap,blend(k.leaf,White,.2f),k.leaf,k.leaf);note(s,G-h);return;}
  if(k.arch==Ginger||(k.flags&Rhizome)){rootShoulder(s);float h=5+s.p*7;s.g.fillTriangle(C-1,G,C+1,G,C+px(swayAt(s,1,1)*3),px(G-h),blend(k.leaf,White,.15f));note(s,G-h);return;}
  float h=6+s.p*7;float sw=swayAt(s,1,1)*4;line(s.g,C,G,C+sw*.5f,G-h*.6f,blend(k.stem,White,.3f));line(s.g,C+sw*.5f,G-h*.6f,C+sw,G-h,blend(k.stem,White,.3f));
  cotyledons(s,C+sw,G-h,4+s.p*2);
}
void seed(Scene &s,uint16_t soil){
  int wob=(s.t%3200<300) ? px(wave(s.t,150)) : 0;uint16_t c=rgb(150,110,70);
  s.g.fillCircle(C+wob,G-1,2,c);s.g.fillRect(C-2+wob,G,5,2,soil);s.g.drawPixel(C-1+wob,G-2,blend(c,White,.5f));
  if(s.p>.3f){int r=px(s.p*4);s.g.drawLine(C+1+wob,G-1,C+2+wob,G-1-r,rgb(240,236,220));}
  note(s,G-6);
}
void pot(Adafruit_GFX &g,float potL,uint64_t t,bool thirsty,bool known){
  // Pot width follows the saved pot volume (1-20 L), centred under the plant.
  int half=known ? 11+px(fminf(20,fmaxf(1,potL))/20*6) : 14;int top=G+1,bottom=112;
  uint16_t clay=rgb(196,108,66),rim=rgb(166,86,52),soil=rgb(92,62,40);
  g.fillTriangle(C-half+2,top+3,C+half-2,top+3,C-half+5,bottom,clay);g.fillTriangle(C+half-2,top+3,C+half-5,bottom,C-half+5,bottom,clay);
  g.fillRect(C-half,top,2*half+1,4,rim);g.drawFastHLine(C-half,top,2*half+1,blend(rim,White,.25f));g.fillRect(C-half+1,G-1,2*half-1,2,soil);
  g.drawFastHLine(C-half+6,bottom-3,2*half-11,blend(clay,0,.2f));
  if(thirsty){bool on=t%1400<900;drop(g,C,top+10,2,on ? AMBER : blend(clay,AMBER,.4f));}
}
void rail(Adafruit_GFX &g,const PlantVisual &v,float p,uint64_t t,uint32_t tap,uint16_t bg){
  // Bottom = seed, top = mature. Only the stages valid for this preset get a dot.
  const int x=5;auto yOf=[](int s)->int{return 104-s*16;};
  g.drawFastVLine(x,yOf(6),yOf(0)-yOf(6)+1,LINE);
  if(v.preset!=255){
    int next=v.stage;for(int s=v.stage+1;s<7;++s)if(v.stageMask&(1<<s)){next=s;break;}
    float yp=yOf(v.stage)-(next>v.stage ? p*(yOf(v.stage)-yOf(next)) : 0);
    g.fillRect(x-1,px(yp),3,yOf(0)-px(yp)+1,blend(bg,LIME,.55f));g.drawFastVLine(x,px(yp),yOf(0)-px(yp)+1,LIME);
    float u=phase(t,2600);int sap=px(yOf(0)-(yOf(0)-yp)*u);g.drawFastVLine(x,sap-2,3,White);
    for(int s=0;s<7;++s){int y=yOf(s);bool ok=v.stageMask&(1<<s);
      if(!ok){g.drawFastHLine(x-1,y,3,LINE);continue;}
      if(s<v.stage)g.fillCircle(x,y,2,LIME);
      else if(s==v.stage){float pulse=.5f+.5f*wave(t,1600);g.drawCircle(x,y,3+(pulse>.75f),blend(bg,LIME,.4f+.6f*pulse));g.fillCircle(x,y,2,White);}
      else{g.drawCircle(x,y,2,DIM);}
    }
    if(tap<ReactionMs){float u2=bound(tap/700.0f);int y=px(yOf(0)-(yOf(0)-yOf(v.stage))*ease(u2));g.fillCircle(x,y,2,White);
      if(u2>=1){float r=3+(tap-700)/90.0f;if(r<12)g.drawCircle(x,yOf(v.stage),px(r),blend(LIME,bg,bound((r-3)/9)));}}
  }else for(int s=0;s<7;++s)g.drawCircle(x,yOf(s),1,LINE);
}
}

float plantGrowth(uint8_t stage,int ageDays,float &progress){
  static const float lo[]={0,.08f,.18f,.36f,.70f,.80f,.90f},hi[]={.05f,.16f,.32f,.70f,.82f,.92f,1};
  static const int dayLo[]={0,5,10,21,40,55,60},dayHi[]={7,14,30,60,90,120,150};
  if(stage>6)stage=6;
  progress=ageDays<0 ? .5f : clampf(float(ageDays-dayLo[stage])/(dayHi[stage]-dayLo[stage]),0,1);
  return lo[stage]+(hi[stage]-lo[stage])*progress;
}

void drawPlantPane(Adafruit_GFX &target,int x,int y,const PlantVisual &v,uint64_t t,uint16_t panel,uint16_t outer,uint32_t tap,uint32_t enterMs){
  target.fillRect(x,y,PlantPaneW,PlantPaneH,outer);target.fillRoundRect(x,y,PlantPaneW,PlantPaneH,6,panel);
  Clip whole(target,x,y,0,0,PlantPaneW-1,PlantPaneH-1);
  float p=.5f;float grow=v.preset!=255 ? plantGrowth(v.stage,v.ageDays,p) : 0;
  float enter=enterMs>=700 ? 1 : ease(enterMs/700.0f);
  rail(whole,v,p,t,tap,panel);
  Clip g(target,x,y,10,2,PlantPaneW-3,PlantPaneH-3);
  // Night fireflies stay behind the foliage.
  if(v.night)for(int i=0;i<2;++i){float a=phase(t+i*2300,5200+i*900)*kTau;int fx=px(C+12*cosf(a)+3*sinf(a*3)),fy=px(30+i*24+8*sinf(a*2));uint16_t glow=blend(panel,rgb(220,255,120),.5f+.5f*wave(t+i*700,900));g.drawPixel(fx,fy,glow);g.drawPixel(fx+1,fy,blend(panel,glow,.4f));}
  bool thirsty=v.mood==1;
  pot(g,v.potL,t,thirsty,v.preset!=255);
  if(v.preset==255){
    // No profile: a seed packet bobs above an empty pot.
    int bob=px(wave(t,2200)*2);g.fillRoundRect(C-7,60+bob,15,19,2,rgb(236,220,180));g.fillRect(C-7,60+bob,15,4,LIME);g.drawRect(C-7,60+bob,15,19,blend(rgb(236,220,180),0,.3f));
    text(g,C-2,68+bob,"?",rgb(120,90,60));for(int i=0;i<3;++i){float u=phase(t+i*700,2100);g.drawPixel(C-2+i*3,px(G-2-u*10),blend(rgb(150,110,70),panel,u));}
    return;
  }
  const Look &k=Looks[v.preset<34 ? v.preset : 0];
  float wig=0;if(tap<ReactionMs){float age=tap/1000.0f;wig=sinf(age*13)*expf(-age*2.2f)*.8f;}
  Scene s(g,k,grow*(.55f+.45f*enter),p,wig+wave(t,4200)*.05f,thirsty ? .55f : 0,v.stage,t);
  if(v.stage==ABPlant::Seed)seed(s,rgb(92,62,40));
  else if(v.stage==ABPlant::Sprout)sprout(s);
  else{
    if(v.stage==ABPlant::Seedling&&k.arch!=Grass&&k.arch!=Ginger&&!(k.flags&Rhizome)&&k.shape!=Tube)cotyledons(s,C,G-3,3.5f);
    switch(k.arch){case Rosette:rosette(s);break;case Bush:bush(s);break;case Vine:vine(s);break;case Trail:trail(s);break;case Grass:grass(s);break;case Ginger:ginger(s);break;default:tree(s);break;}
  }
  // Visitors: a butterfly while flowering, a bee near fruit, by day only.
  if(!v.night&&v.stage>=ABPlant::Flowering&&(k.bloom!=NoBloom||k.fruit!=NoFruit)&&s.topY<G-12){
    float a=phase(t,6400)*kTau;int bx=px(C+11*cosf(a)),by=px(fmaxf(8,s.topY)+4+5*sinf(a*2));bool up=t%260<130;
    if(v.stage==ABPlant::Flowering){uint16_t w=rgb(250,190,80);g.fillTriangle(bx,by,bx-3,by-(up?3:1),bx-3,by+1,w);g.fillTriangle(bx,by,bx+3,by-(up?3:1),bx+3,by+1,w);g.drawFastVLine(bx,by-1,3,0);}
    else{g.fillRect(bx-1,by,3,2,Yellow);g.drawPixel(bx,by,0);g.drawPixel(bx,by-1,up ? White : blend(White,panel,.5f));}
  }
  if(tap<ReactionMs){
    // SELECT: the plant waves and a few sparkles rise from the canopy.
    float age=tap/float(ReactionMs);for(int i=0;i<6;++i){float u=bound(age*1.6f-i*.08f);if(u<=0||u>=1)continue;
      float sx=C+(i-2.5f)*5+sinf(u*6+i)*3,sy=s.topY-2-u*18-(i%3)*2;star(g,px(sx),px(sy),u<.5f ? 1 : 0,blend(panel,i%2 ? Yellow : White,1-u));}
  }
}
}
