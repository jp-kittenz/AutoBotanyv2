#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

namespace ABTFT {
enum class Art:uint8_t {
  WaterDrops,WaterEmpty,WaterUnknown,FlowerHealthy,FlowerWilt,FlowerPetals,FlowerDead,GrassUnknown,
  Weather,Bulb,BulbBroken,TempSun,TempHeat,TempWind,TempSnow,TempUnknown,
  HumBubbles,HumDrops,HumDry,HumUnknown,FaceHotSad,FaceHotHappy,FaceHappy,FaceColdHappy,FaceColdSad,
  DomeQuestion,PumpFlow,PumpSlow,PumpStill,PumpEmpty,PumpUnknown,FanSpin,FanStill,
  CircuitHot,Circuit,CircuitCool,CircuitCold,Network,NetworkOff,Unknown
};
enum class Sky:uint8_t {Sun,SunCloud,Cloud,Moon,Rain,MoonRain,SunRain,Unknown};
struct Visual {
  Art art=Art::Unknown;
  Sky sky=Sky::Sun;
  uint8_t count=1;
  uint8_t page=255;
  bool dome=false,warning=false,camera=false;
  bool before=false; // output/cover state just before SELECT; drives transition art, not compared
  bool operator==(const Visual &v)const{return art==v.art&&sky==v.sky&&count==v.count&&page==v.page&&dome==v.dome&&warning==v.warning&&camera==v.camera;}
  bool operator!=(const Visual &v)const{return !(*this==v);}
};
struct Reading {float value=0;bool current=false,calibrated=true;};
struct VisualInput {
  Reading water,soil,rain,light,temperature,humidity,heat,chip;
  bool luxPresent=false,night=false,clockValid=false;
  bool fanOn=false,pumpOn=false;
  bool sunKnown=false,sunCovered=false,rainKnown=false,rainCovered=false,rainAutomatic=true;
  float waterMin=20,fanHot=55;
  bool apRunning=false,cameraOnline=false;
  uint8_t stations=0;
};
Visual chooseVisual(uint8_t page,const VisualInput &input);
bool isNightHour(int hour); // Local RTC hours: 18:00..05:59.
bool animated(const Visual &visual);
constexpr uint32_t NoReaction=0xFFFFFFFFUL;
constexpr uint32_t ReactionMs=2200;
bool playfulPage(uint8_t page);
bool cameraFresh(bool apRunning,uint8_t stations,uint64_t seenMs,uint64_t now);
uint8_t userDevices(bool apRunning,uint8_t stations,bool cameraOnline);
void drawVisual(Adafruit_GFX &target,int x,int y,const Visual &visual,uint64_t ms,uint16_t background,uint32_t reactionMs=NoReaction);
void drawNetwork(Adafruit_GFX &target,int x,int y,const Visual &visual,uint64_t ms,uint16_t background,uint32_t reactionMs=NoReaction);
void drawClients(Adafruit_GFX &target,int x,int y,const Visual &visual,uint64_t ms,uint16_t background,uint32_t reactionMs=NoReaction);
void drawIntroFrame(Adafruit_GFX &target,int shift,uint64_t elapsedMs);
// Card 14 "YOUR PLANT": the left pane holds the stage rail and the plant sprite.
constexpr int PlantPaneW=52,PlantPaneH=116;
struct PlantVisual {
  uint8_t preset=255;   // 255 = no saved plant profile
  uint8_t stage=0;      // ABPlant::Stage
  uint16_t stageMask=0; // stages allowed for this preset
  int16_t ageDays=-1;   // -1 = unknown age
  uint8_t mood=0;       // 0 normal, 1 soil below its target (thirsty)
  bool night=false;
  float potL=5;
  bool operator==(const PlantVisual &o)const{return preset==o.preset&&stage==o.stage&&stageMask==o.stageMask&&ageDays==o.ageDays&&mood==o.mood&&night==o.night&&potL==o.potL;}
  bool operator!=(const PlantVisual &o)const{return !(*this==o);}
};
float plantGrowth(uint8_t stage,int ageDays,float &progress);
void drawPlantPane(Adafruit_GFX &target,int x,int y,const PlantVisual &plant,uint64_t ms,uint16_t panel,uint16_t outer,uint32_t reactionMs=NoReaction,uint32_t enterMs=0xFFFFFFFFUL);
struct Tip {char line1[20];char line2[20];};
extern const Tip Tips[36] PROGMEM;
uint8_t tipIndex(uint8_t page,uint64_t elapsedMs);
}
