#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include "Config.h"

class AutobotanyTimedOutput {
  int pin_;
  bool activeHigh_;
  bool on_=false;
  uint64_t deadline_=0,started_=0;
  double lastSeconds_=0;
  esp_timer_handle_t timer_=nullptr;
  portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
  static void timeout(void *arg) { static_cast<AutobotanyTimedOutput *>(arg)->expire(); }
  void drive(bool value) { gpio_set_level(static_cast<gpio_num_t>(pin_),value==activeHigh_ ? 1 : 0); }
public:
  AutobotanyTimedOutput(int pin,bool activeHigh):pin_(pin),activeHigh_(activeHigh){}
  void prepareOff() {
    // Preload the GPIO latch through ESP-IDF. Arduino-ESP32 3.x digitalWrite()
    // can reject writes before pinMode() has assigned the pin to GPIO.
    drive(false);
    pinMode(pin_,OUTPUT);
    drive(false);
  }
  bool begin(const char *name) {
    prepareOff();
    esp_timer_create_args_t args={};
    args.callback=timeout;
    args.arg=this;
    args.dispatch_method=ESP_TIMER_TASK;
    args.name=name;
    return esp_timer_create(&args,&timer_)==ESP_OK;
  }
  bool start(uint64_t now,uint64_t durationMs) {
    if (durationMs && !timer_) return false;
    if (timer_) esp_timer_stop(timer_);
    portENTER_CRITICAL(&mux_);
    if (!on_) started_=now;
    deadline_=durationMs ? now+durationMs : 0;
    on_=true;
    drive(true);
    portEXIT_CRITICAL(&mux_);
    if (durationMs && esp_timer_start_once(timer_,durationMs*1000)!=ESP_OK) {stop(now);return false;}
    return true;
  }
  void stop(uint64_t now) {
    if (timer_) esp_timer_stop(timer_);
    portENTER_CRITICAL(&mux_);
    if (on_) lastSeconds_=(now-started_)/1000.0;
    on_=false;
    deadline_=0;
    drive(false);
    portEXIT_CRITICAL(&mux_);
  }
  void expire() {
    uint64_t now=static_cast<uint64_t>(esp_timer_get_time()/1000);
    portENTER_CRITICAL(&mux_);
    if (on_ && deadline_ && now>=deadline_) {
      lastSeconds_=(now-started_)/1000.0;
      on_=false;
      deadline_=0;
      drive(false);
    }
    portEXIT_CRITICAL(&mux_);
  }
  bool on() {portENTER_CRITICAL(&mux_);bool value=on_;portEXIT_CRITICAL(&mux_);return value;}
  double runSeconds(uint64_t now) {portENTER_CRITICAL(&mux_);double value=on_ ? (now-started_)/1000.0 : lastSeconds_;portEXIT_CRITICAL(&mux_);return value;}
  uint64_t remainingMs(uint64_t now) {portENTER_CRITICAL(&mux_);uint64_t value=on_ && deadline_>now ? deadline_-now : 0;portEXIT_CRITICAL(&mux_);return value;}
};
