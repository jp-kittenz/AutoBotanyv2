#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "Config.h"

namespace AB {
struct CalibrationData {
  uint32_t version=1;
  int32_t endpoints[6]={SOIL_DRY_RAW,SOIL_WET_RAW,WATER_EMPTY_RAW,WATER_FULL_RAW,RAIN_DRY_RAW,RAIN_WET_RAW};
  uint32_t mask=(SOIL_CALIBRATED ? 1U : 0U)|(WATER_CALIBRATED ? 2U : 0U)|(RAIN_CALIBRATED ? 4U : 0U);
  bool valid() const {
    if(version!=1 || mask>7)return false;
    for(int i=0;i<6;++i)if(endpoints[i]<0 || endpoints[i]>4095)return false;
    for(int i=0;i<3;++i)if((mask&(1U<<i)) && abs(endpoints[i*2+1]-endpoints[i*2])<200)return false;
    return true;
  }
  bool ready(int target) const {return target>=0 && target<3 && (mask&(1U<<target));}
};

struct StableCapture {
  static constexpr uint8_t SIZE=32;
  int readings[SIZE]={};
  uint8_t count=0;
  int mean=0,spread=0;
  void reset(){count=0;mean=0;spread=0;}
  bool feed(int raw) {
    if(raw<0 || raw>4095)return false;
    readings[count++]=raw;
    if(count<SIZE)return false;
    int sorted[SIZE];
    long early=0,late=0;
    for(int i=0;i<SIZE;++i){sorted[i]=readings[i];if(i<SIZE/2)early+=readings[i];else late+=readings[i];}
    for(int i=1;i<SIZE;++i){int value=sorted[i],j=i-1;while(j>=0 && sorted[j]>value){sorted[j+1]=sorted[j];--j;}sorted[j+1]=value;}
    long sum=0;for(int i=4;i<SIZE-4;++i)sum+=sorted[i];
    mean=static_cast<int>((sum+12)/24);
    spread=sorted[SIZE-4]-sorted[3];
    count=0;
    return spread<=100 && labs(early-late)/(SIZE/2)<=50;
  }
};

struct CalibrationSession {
  int target=-1,point=0;
  uint8_t captured=0;
  int endpoints[2]={0,0};
  bool active=false,running=false;
  uint64_t startedMs=0,nextMs=0,timeoutMs=0,idleUntilMs=0;
  StableCapture capture;
  const char *message="Choose a sensor to begin.";
};

struct HistoryPoint {
  uint32_t uptime=0,epoch=0;
  float values[6]={NAN,NAN,NAN,NAN,NAN,NAN};
};
}
