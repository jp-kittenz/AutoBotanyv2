#pragma once
#include <Arduino.h>
#include <esp_arduino_version.h>
#include "Config.h"

class AutobotanyBlocker {
  int pin_,channel_,openAngle_,coverAngle_;
  uint64_t detachAt_=0;
  bool attached_=false;
public:
  bool covering=false,automatic=true,known=false,moving=false;
  AutobotanyBlocker(int pin,int channel,int openAngle,int coverAngle):pin_(pin),channel_(channel),openAngle_(openAngle),coverAngle_(coverAngle){}
  bool command(bool cover,uint64_t now,bool manual=false) {
    if (!attached_) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
      attached_=ledcAttach(pin_,50,14);
#else
      attached_=ledcSetup(channel_,50,14)>0;
      if (attached_) ledcAttachPin(pin_,channel_);
#endif
      if (!attached_) {known=false;return false;}
    }
    int angle=cover ? coverAngle_ : openAngle_;
    angle=constrain(angle,0,180);
    uint32_t pulse=AB::SERVO_MIN_US+(AB::SERVO_MAX_US-AB::SERVO_MIN_US)*angle/180;
    uint32_t duty=(pulse*16383UL+10000UL)/20000UL;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!ledcWrite(pin_,duty)) {known=false;return false;}
#else
    ledcWrite(channel_,duty);
#endif
    covering=cover;
    known=true;
    moving=true;
    if (manual) automatic=false;
    detachAt_=now+AB::SERVO_DRIVE_MS;
    return true;
  }
  void tick(uint64_t now) {
    if (!attached_ || now<detachAt_) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcDetach(pin_);
#else
    ledcDetachPin(pin_);
#endif
    pinMode(pin_,OUTPUT);
    digitalWrite(pin_,LOW);
    attached_=false;
    moving=false;
  }
};
