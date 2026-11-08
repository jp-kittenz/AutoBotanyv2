#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "Config.h"

namespace AB {
struct Sample {
  double value = NAN;
  int raw = -1;
  bool valid = false;
  bool active = false;
  bool calibrated = true;
  bool scheduled = false;
  uint64_t acquiredMs = 0;
  uint32_t epoch = 0;
  uint64_t freshMs = 5000;
  uint64_t staleMs = 20000;
  void set(double next, uint64_t now, uint32_t stamp, int adc = -1) {
    value = next;
    raw = adc;
    valid = isfinite(next);
    acquiredMs = now;
    epoch = stamp;
  }
  bool current(uint64_t now, uint64_t maxAge = 0) const {
    return valid && isfinite(value) && now >= acquiredMs && now - acquiredMs <= (maxAge ? maxAge : staleMs);
  }
};

struct Thresholds {
  float values[12] = {35,85,20,1000,35000,40,75,16,32,35,55,48};
  static const char *name(size_t i) {
    static const char *names[] = {"moistureMin","moistureMax","waterMin","sunlightMin","sunlightMax","humidityMin","humidityMax","tempMin","tempMax","heatMax","fanOnC","fanOffC"};
    return i < 12 ? names[i] : "";
  }
  float get(const char *key) const {
    for (size_t i=0;i<12;++i) if (strcmp(key,name(i))==0) return values[i];
    return NAN;
  }
  bool valid() const {
    for (size_t i=0;i<12;++i) if (!isfinite(values[i])) return false;
    const float *v=values;
    return v[0]>=0 && v[1]<=100 && v[0]<v[1] && v[2]>=0 && v[2]<=100 && v[3]>=0 && v[4]<=200000 && v[3]<v[4] && v[5]>=0 && v[6]<=100 && v[5]<v[6] && v[7]>=-40 && v[8]<=100 && v[7]<v[8] && v[9]>=0 && v[9]<=100 && v[11]>=0 && v[10]<=125 && v[11]<v[10];
  }
};

struct ProbeCycle {
  enum Phase {Idle,Warming,Sampling};
  Phase phase = Idle;
  uint64_t nextMs = 0, deadlineMs = 0;
  uint64_t intervalMs;
  uint32_t sum = 0;
  uint8_t count = 0;
  explicit ProbeCycle(uint64_t interval):intervalMs(interval){}
  bool busy() const { return phase != Idle; }
  bool scheduledDue(uint64_t now) const { return !busy() && now >= nextMs; }
  bool request(uint64_t now) {
    if (busy()) return false;
    phase=Warming;
    deadlineMs=now+PROBE_WARMUP_MS;
    sum=0;
    count=0;
    return true;
  }
  bool sampleDue(uint64_t now) const { return busy() && now>=deadlineMs; }
  bool feed(int adc,uint64_t now) {
    if (!sampleDue(now)) return false;
    phase=Sampling;
    sum+=static_cast<uint32_t>(adc);
    ++count;
    deadlineMs=now+ADC_SPACING_MS;
    if (count<ADC_SAMPLES) return false;
    phase=Idle;
    nextMs=now+intervalMs;
    return true;
  }
  int mean() const { return count ? static_cast<int>((sum+count/2)/count) : -1; }
  void cancel(uint64_t now) { phase=Idle;nextMs=now+intervalMs;sum=0;count=0; }
};

inline double calibratedPercent(int raw,int low,int high) {
  if (low==high || raw<0 || raw>4095) return NAN;
  double value=(raw-low)*100.0/(high-low);
  return fmax(0.0,fmin(100.0,value));
}

struct DebouncedButton {
  bool raw=false,stable=false;
  uint64_t changedMs=0;
  bool update(bool pressed,uint64_t now) {
    if (pressed!=raw) {raw=pressed;changedMs=now;}
    if (raw!=stable && now-changedMs>=35) {stable=raw;return stable;}
    return false;
  }
};

inline bool fanDecision(bool current,double chipC,const Thresholds &limits) {
  if (chipC>=limits.values[10]) return true;
  if (chipC<=limits.values[11]) return false;
  return current;
}

constexpr int TFT_PAGE_COUNT=14;
inline int wrapPage(int page) { return (page%TFT_PAGE_COUNT+TFT_PAGE_COUNT)%TFT_PAGE_COUNT; }
}
