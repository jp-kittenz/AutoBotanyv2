#include "AutobotanyVision.h"
#include <math.h>
namespace ABVision {
const char *conditionName(uint8_t n){static const char *s[]={"normal-appearance","yellowing","browning","drooping"};return n<Conditions?s[n]:"unknown";}
const char *qualityName(uint8_t n){static const char *s[]={"usable","too-dark","too-bright","plant-not-clear"};return n<4?s[n]:"unknown";}
bool Model::valid()const{
  if(version!=1||count>Capacity||crop>2)return false;
  for(uint8_t i=0;i<count;i++){
    const Example &r=rows[i];if(r.condition>=Conditions&&r.condition!=Unknown)return false;
    if((r.stage==ABPlant::Seed||r.stage>=ABPlant::StageCount)&&r.stage!=Unknown)return false;
    for(uint8_t k=0;k<Features;k++)if(r.x.v[k]>1000)return false;
  }return true;
}
static int nibble(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static uint16_t fraction(uint32_t n,uint32_t total){return total?uint16_t(n*1000/total):0;}
bool extract(const char *hex,size_t length,Observation &out){
  if(length!=Side*Side*4)return false;
  out=Observation();uint32_t gr=0,ye=0,br=0,plant=0,dark=0,light=0,rSum=0,gSum=0,bSum=0,xSum=0,ySum=0,lower=0,left=0,centre=0,edge=0;
  int minX=Side,minY=Side,maxX=-1,maxY=-1;
  for(int y=0;y<Side;y++)for(int x=0;x<Side;x++){
    uint16_t color=0;for(int k=0;k<4;k++){int v=nibble(hex[(y*Side+x)*4+k]);if(v<0)return false;color=uint16_t((color<<4)|v);}
    int r=((color>>11)&31)*255/31,g=((color>>5)&63)*255/63,b=(color&31)*255/31;
    int mx=r>g?r:g;mx=mx>b?mx:b;int mn=r<g?r:g;mn=mn<b?mn:b;
    rSum+=r;gSum+=g;bSum+=b;if(mx<38)dark++;if(mn>235)light++;
    bool green=g>r*1.10f&&g>b*1.12f&&g>45;
    bool yellow=!green&&r>85&&g>70&&b<g*0.72f&&r>g*0.82f&&r<g*1.55f;
    bool brown=!green&&!yellow&&r>45&&r<190&&g>25&&g<150&&r>g*1.2f&&g>b*1.15f;
    gr+=green;ye+=yellow;br+=brown;
    if(green||yellow||brown){plant++;xSum+=x;ySum+=y;lower+=y>=Side/2;left+=x<Side/2;centre+=x>=Side/4&&x<Side*3/4;edge+=x<2||y<2||x>=Side-2||y>=Side-2;if(x<minX)minX=x;if(x>maxX)maxX=x;if(y<minY)minY=y;if(y>maxY)maxY=y;}
  }
  const uint32_t pixels=Side*Side;
  out.green=fraction(gr,pixels);out.yellow=fraction(ye,pixels);out.brown=fraction(br,pixels);out.coverage=fraction(plant,pixels);out.dark=fraction(dark,pixels);out.bright=fraction(light,pixels);
  uint16_t *v=out.x.v;v[0]=out.green;v[1]=out.yellow;v[2]=out.brown;v[3]=out.coverage;
  v[4]=fraction(rSum,pixels*255);v[5]=fraction(gSum,pixels*255);v[6]=fraction(bSum,pixels*255);
  v[7]=plant?fraction(maxX-minX+1,Side):0;v[8]=plant?fraction(maxY-minY+1,Side):0;
  v[9]=fraction(xSum,plant*(Side-1));v[10]=fraction(ySum,plant*(Side-1));
  v[11]=fraction(lower,plant);v[12]=fraction(left,plant);v[13]=fraction(centre,plant);v[14]=fraction(edge,plant);v[15]=plant?fraction(plant,(maxX-minX+1)*(maxY-minY+1)):0;
  out.quality=out.dark>650?1:out.bright>650?2:out.coverage<35?3:0;
  return true;
}
uint8_t examplesFor(const Model &m,bool stage,uint8_t label){uint8_t n=0;for(uint8_t i=0;i<m.count;i++)if((stage?m.rows[i].stage:m.rows[i].condition)==label)n++;return n;}
Result predict(const Model &m,const Vector &x,bool stage){
  Result result;uint8_t counts[ABPlant::StageCount]={0};
  for(uint8_t k=0;k<(stage?ABPlant::StageCount:Conditions);k++){counts[k]=examplesFor(m,stage,k);if(counts[k]>=3)result.classes++;}
  if(result.classes<2)return result;
  uint32_t distances[3]={0xffffffffUL,0xffffffffUL,0xffffffffUL};uint8_t labels[3]={Unknown,Unknown,Unknown};
  for(uint8_t i=0;i<m.count;i++){
    uint8_t label=stage?m.rows[i].stage:m.rows[i].condition;
    if(label==Unknown||counts[label]<3)continue;
    uint32_t distance=0;for(uint8_t k=0;k<Features;k++){int32_t d=int32_t(x.v[k])-m.rows[i].x.v[k];distance+=d*d;}distance/=Features;
    for(uint8_t j=0;j<3;j++)if(distance<distances[j]){for(int k=2;k>j;k--){distances[k]=distances[k-1];labels[k]=labels[k-1];}distances[j]=distance;labels[j]=label;break;}
  }
  result.nearest=uint16_t(sqrt(double(distances[0])));
  uint8_t winner=Unknown,best=0;for(uint8_t i=0;i<3;i++){uint8_t votes=0;for(uint8_t j=0;j<3;j++)votes+=labels[i]==labels[j];if(votes>best){best=votes;winner=labels[i];}}
  result.agreement=best; // Neighbor votes, NOT an accuracy or probability estimate.
  // Reject both distant scenes and neighbours without a majority.
  if(best>=2&&distances[0]<=10000&&distances[2]<=22500)result.label=winner;
  return result;
}
}
