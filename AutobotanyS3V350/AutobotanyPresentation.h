#pragma once
#include <Arduino.h>

namespace ABPresentation {
struct Asset { const char *path; const char *mime; const uint8_t *data; size_t size; bool gzip; bool cached; };
extern const uint8_t GardenCSS[1139] PROGMEM;
extern const uint8_t GardenJS[8024] PROGMEM;
extern const uint8_t Entrance[1823] PROGMEM;
extern const uint8_t Credits[2726] PROGMEM;
extern const uint8_t StoryCSS[6226] PROGMEM;
extern const uint8_t MotionJS[2627] PROGMEM;
extern const uint8_t Logo[96305] PROGMEM;
extern const uint8_t School[327906] PROGMEM;
extern const uint8_t Archdiocese[41890] PROGMEM;
extern const uint8_t GSAP[28302] PROGMEM;
extern const Asset Assets[12];
extern const size_t Count;
}
