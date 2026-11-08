#include "AutobotanyTFTVisuals.h"
namespace ABTFT {
bool isNightHour(int hour){return hour>=0&&hour<24&&(hour>=18||hour<6);}
static bool rainNow(const VisualInput &i){return i.rain.current&&i.rain.calibrated&&i.rain.value>0;}
static Sky sky(const VisualInput &i){
  bool night=i.clockValid&&i.night;
  if(rainNow(i))return night ? Sky::MoonRain : Sky::Rain;
  if(night)return Sky::Moon;
  if(!i.light.current||!i.luxPresent)return Sky::Unknown;
  if(i.light.value>=10000)return Sky::Sun;
  if(i.light.value>=1000)return Sky::SunCloud;
  return Sky::Cloud;
}
Visual chooseVisual(uint8_t page,const VisualInput &i){
  Visual v;v.page=page;
  switch(page){
    case 0:
      if(!i.water.calibrated||!i.water.current)v.art=Art::WaterUnknown;
      else if(i.water.value<=0)v.art=Art::WaterEmpty;
      else {v.art=Art::WaterDrops;v.count=i.water.value<=25 ? 1 : i.water.value<=50 ? 2 : 4;}
      break;
    case 1:
      if(!i.soil.calibrated||!i.soil.current)v.art=Art::GrassUnknown;
      else v.art=i.soil.value<=0 ? Art::FlowerDead : i.soil.value<=25 ? Art::FlowerPetals : i.soil.value<=50 ? Art::FlowerWilt : Art::FlowerHealthy;
      break;
    case 2:
      v.art=Art::Weather;v.sky=rainNow(i) ? Sky::Rain : Sky::Cloud;
      v.warning=!i.rain.current||!i.rain.calibrated;break;
    case 3:
      v.art=!i.temperature.current ? Art::TempUnknown : i.temperature.value<10 ? Art::TempSnow : i.temperature.value<22 ? Art::TempWind : i.temperature.value<32 ? Art::TempSun : Art::TempHeat;break;
    case 4:
      v.art=!i.humidity.current ? Art::HumUnknown : i.humidity.value<35 ? Art::HumDry : i.humidity.value<70 ? Art::HumBubbles : Art::HumDrops;break;
    case 5:
      if(!i.luxPresent||!i.light.current)v.art=Art::BulbBroken;
      else if(!(i.clockValid&&i.night)&&!rainNow(i)&&i.light.value<50)v.art=Art::Bulb;
      else {v.art=Art::Weather;v.sky=sky(i);if(v.sky==Sky::Rain)v.sky=Sky::SunRain;}
      break;
    case 6:v.art=i.fanOn ? Art::FanSpin : Art::FanStill;break;
    case 7:
      v.art=!i.heat.current ? Art::Unknown : i.heat.value<=0 ? Art::FaceColdSad : i.heat.value<15 ? Art::FaceColdHappy : i.heat.value<27 ? Art::FaceHappy : i.heat.value<35 ? Art::FaceHotHappy : Art::FaceHotSad;break;
    case 8:
      if(!i.sunKnown)break;
      v.art=Art::Weather;v.sky=sky(i);v.dome=i.sunCovered;v.warning=v.sky==Sky::Unknown;break;
    case 9:
      if(!i.rainKnown)break;
      if(i.rainCovered&&!rainNow(i)){v.art=Art::DomeQuestion;v.dome=true;}
      else {v.art=Art::Weather;v.sky=sky(i);v.dome=i.rainCovered;v.warning=v.sky==Sky::Unknown;}
      break;
    case 10:
      if(!i.water.current||!i.water.calibrated)v.art=Art::PumpUnknown;
      else if(i.water.value<=0)v.art=Art::PumpEmpty;
      else if(!i.pumpOn)v.art=Art::PumpStill;
      else v.art=i.water.value<i.waterMin ? Art::PumpSlow : Art::PumpFlow;
      break;
    case 11:
      if(!i.chip.current)break;
      v.art=i.chip.value<10 ? Art::CircuitCold : i.chip.value>=i.fanHot ? (i.fanOn ? Art::CircuitCool : Art::CircuitHot) : Art::Circuit;break;
    case 12:
      v.art=i.apRunning ? Art::Network : Art::NetworkOff;
      v.camera=i.apRunning&&i.cameraOnline&&i.stations>0;
      v.count=userDevices(i.apRunning,i.stations,v.camera);break;
  }
  return v;
}
bool animated(const Visual &v){return v.art!=Art::FanStill&&v.art!=Art::PumpStill;}
bool playfulPage(uint8_t page){return page==2||page==3||page==4||page==5||page==7||page==11||page==12||page==13;}
bool cameraFresh(bool apRunning,uint8_t stations,uint64_t seenMs,uint64_t now){return apRunning&&stations>0&&seenMs>0&&now>=seenMs&&now-seenMs<=45000;}
uint8_t userDevices(bool apRunning,uint8_t stations,bool cameraOnline){return !apRunning ? 0 : stations-(cameraOnline&&stations>0 ? 1 : 0);}
uint8_t tipIndex(uint8_t page,uint64_t elapsedMs){return static_cast<uint8_t>((page%12)*3+(elapsedMs/8000)%3);}
const Tip Tips[36] PROGMEM={
 {"Calibrate empty and","full tank levels."},
 {"Refill before water","drops below limit."},
 {"Clean the probe for","steady readings."},
 {"Place the probe in","the root zone."},
 {"Soil type changes","moisture readings."},
 {"Check drainage if","soil stays wet."},
 {"Keep the rain plate","exposed to the sky."},
 {"Clean off dirt for","reliable rain data."},
 {"A stale reading is","not live weather."},
 {"Shade the DHT from","direct sunlight."},
 {"Keep air moving","around the sensor."},
 {"Ambient temperature","differs from chip C"},
 {"Keep water spray","away from the DHT."},
 {"High humidity can","reduce evaporation."},
 {"Compare humidity","over several hours."},
 {"Keep the BH1750","window clean."},
 {"Lux is brightness,","not plant PAR."},
 {"Set the RTC for","correct moon icons."},
 {"Leave space for","airflow at the fan."},
 {"Fan ON is a command","not RPM feedback."},
 {"The fan follows","ESP32 chip heat."},
 {"Heat index combines","heat and humidity."},
 {"Check DHT readings","if heat data fails."},
 {"Face icons are","visual cues only."},
 {"Keep the cover path","free of obstacles."},
 {"Use a separate","servo power supply."},
 {"Check shade needs","for your plant."},
 {"Keep drainage clear","under the cover."},
 {"Manual cover state","is your command."},
 {"Rain icons require","fresh sensor data."},
 {"Never run the pump","with an empty tank."},
 {"Check tubes for","kinks or air locks."},
 {"Flow art shows a","command, not flow."},
 {"Chip heat differs","from air heat."},
 {"Keep vents and the","heatsink clear."},
 {"Low supply voltage","can cause resets."}
};
}
