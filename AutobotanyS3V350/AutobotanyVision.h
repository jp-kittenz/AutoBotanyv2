#pragma once
#include <stdint.h>
#include <stddef.h>
#include "AutobotanyPlants.h"
namespace ABVision {
constexpr uint8_t Side=24,Features=16,Capacity=64,Unknown=255,Conditions=4;
struct Vector {uint16_t v[Features]={0};};
struct Example {Vector x;uint8_t condition=Unknown,stage=Unknown;uint16_t reserved=0;};
struct Model {
  uint32_t version=1,cycle=0;
  uint8_t count=0,crop=0;uint16_t reserved=0;
  Example rows[Capacity];
  bool valid() const;
};
struct Result {uint8_t label=Unknown,agreement=0,classes=0;uint16_t nearest=1000;};
struct Observation {
  Vector x;uint16_t green=0,yellow=0,brown=0,coverage=0,dark=0,bright=0;
  uint8_t quality=0; // 0 usable, 1 dark, 2 glare, 3 too little plant-like colour
};
const char *conditionName(uint8_t label);
const char *qualityName(uint8_t quality);
bool extract(const char *hex,size_t length,Observation &out);
Result predict(const Model &model,const Vector &x,bool stage);
uint8_t examplesFor(const Model &model,bool stage,uint8_t label);
}
