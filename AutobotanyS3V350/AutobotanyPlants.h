#pragma once
#include <stdint.h>
#include "AutobotanyCore.h"

namespace ABPlant {
constexpr uint32_t Version=1;
enum Stage:uint8_t {Seed,Sprout,Seedling,Vegetative,Flowering,Fruiting,Mature,StageCount};
enum Medium:uint8_t {PottingMix,CocoBlend,SoilCompost,MediumCount};
enum Event:uint8_t {Established,Sown,Transplanted,Cutting,EventCount};
enum Exposure:uint8_t {MorningSun,FullSun,BrightShade,ExposureCount};
struct Preset {
  const char *id,*name,*scientific,*group,*note;
  float minPotL; uint16_t stages;
  float moistureLow,moistureHigh,lightLow,lightHigh,tempLow,tempHigh,humidityLow,humidityHigh;
};
extern const Preset Presets[];
extern const uint8_t Count;
const char *stageName(uint8_t stage);
const char *mediumName(uint8_t medium);
const char *eventName(uint8_t event);
const char *exposureName(uint8_t exposure);
int find(const char *id);
struct Profile {
  uint32_t version=Version,revision=0,cycle=0;
  uint16_t preset=0;
  uint8_t stage=Seedling,medium=PottingMix,event=Established,exposure=MorningSun;
  uint8_t ageKnown=0,customLimits=0,reserved[2]={0,0};
  float potL=5;
  uint32_t ageSeconds=0;
  int64_t anchorEpoch=0;
  AB::Thresholds limits;
  bool valid() const;
};
bool juvenile(uint8_t stage);
bool needsRepot(const Profile &p);
AB::Thresholds suggest(const Profile &p,const AB::Thresholds &hardware);
double ageSeconds(const Profile &p,uint32_t epoch,uint64_t uptimeMs,uint64_t savedMs);
}
