#pragma once
#include <Arduino.h>
#include <esp_attr.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "Config.h"

namespace AB {
struct BootCheckpoint { uint32_t signature,stage,inverse; };
static RTC_NOINIT_ATTR BootCheckpoint checkpoint;
}

class AutobotanyBoot {
  static constexpr uint32_t SIGNATURE=0xAB310101;
  esp_timer_handle_t watchdog_=nullptr;
  volatile bool guarding_=false;
  volatile bool timedOut_=false;
  uint64_t nextStepAt_=0;
  bool stepRequested_=false;
  void mark(uint32_t stage) {
    AB::checkpoint.signature=0;
    AB::checkpoint.stage=stage;AB::checkpoint.inverse=~stage;
    AB::checkpoint.signature=SIGNATURE;
  }
  static void timedOut(void *arg) {
    AutobotanyBoot *self=static_cast<AutobotanyBoot*>(arg);
    if(!self->guarding_)return;
    self->timedOut_=true;self->guarding_=false;
    self->mark(AB::checkpoint.stage|0x100U);
    esp_restart();
  }
public:
  esp_reset_reason_t resetReason=ESP_RST_UNKNOWN;
  bool recovery=false,paused=false,ready=false,watchdogReady=false;
  bool previousTimedOut=false,skipSd=false;
  uint8_t previousStage=255,nextStep=1,currentStage=0;
  String note="Waiting for startup";
  static uint64_t now(){return static_cast<uint64_t>(esp_timer_get_time()/1000);}
  static const char *name(uint8_t stage) {
    static const char *names[]={"Wi-Fi","TFT display / SPI","I2C / RTC / light / DHT","Saved settings","SD card","Running"};
    return stage<6 ? names[stage] : "Unknown";
  }
  static const char *resetName(esp_reset_reason_t reason) {
    switch(reason) {
      case ESP_RST_POWERON:return "Power-on";
      case ESP_RST_EXT:return "External reset";
      case ESP_RST_SW:return "Software reset";
      case ESP_RST_PANIC:return "Panic / crash";
      case ESP_RST_INT_WDT:return "Interrupt watchdog";
      case ESP_RST_TASK_WDT:return "Task watchdog";
      case ESP_RST_WDT:return "Other watchdog";
      case ESP_RST_DEEPSLEEP:return "Deep-sleep wake";
      case ESP_RST_BROWNOUT:return "Brownout";
      default:return "Other / unknown";
    }
  }
  void begin() {
    if(watchdog_)esp_timer_stop(watchdog_);
    guarding_=false;timedOut_=false;stepRequested_=false;ready=false;skipSd=false;nextStep=1;currentStage=0;
    resetReason=esp_reset_reason();
    bool valid=AB::checkpoint.signature==SIGNATURE && AB::checkpoint.inverse==~AB::checkpoint.stage && (AB::checkpoint.stage&~0x100U)<=5;
    bool fault=resetReason==ESP_RST_PANIC || resetReason==ESP_RST_INT_WDT || resetReason==ESP_RST_TASK_WDT || resetReason==ESP_RST_WDT || resetReason==ESP_RST_BROWNOUT;
    previousStage=valid && resetReason!=ESP_RST_POWERON ? static_cast<uint8_t>(AB::checkpoint.stage&0xFF) : 255;
    previousTimedOut=valid && (AB::checkpoint.stage&0x100U)!=0 && resetReason==ESP_RST_SW;
    pinMode(AB::SELECT_PIN,INPUT_PULLUP);
    bool held=digitalRead(AB::SELECT_PIN)==LOW;
    recovery=held || previousTimedOut || (valid && fault);
    paused=recovery;
    esp_timer_create_args_t args={};args.callback=timedOut;args.arg=this;args.dispatch_method=ESP_TIMER_TASK;args.name="ab_startup";
    watchdogReady=esp_timer_create(&args,&watchdog_)==ESP_OK;
    if(!watchdogReady){recovery=paused=true;note="Startup watchdog unavailable";}
    else if(held)note="Recovery selected with the SELECT button";
    else if(recovery)note="Previous reset recorded. Hardware is paused.";
    else note="Wi-Fi first. Hardware starts in five seconds.";
    mark(0);nextStepAt_=now()+5000;
  }
  bool guarding()const{return guarding_;}
  bool guard(uint8_t stage) {
    if(timedOut_)return false;
    if(!watchdogReady){paused=recovery=true;note="Startup watchdog unavailable";return false;}
    esp_timer_stop(watchdog_);currentStage=stage;mark(stage);guarding_=true;
    if(esp_timer_start_once(watchdog_,15000000ULL)!=ESP_OK){guarding_=false;paused=recovery=true;note="Could not start the startup watchdog";return false;}
    return true;
  }
  void unguard() {
    if(timedOut_)return;
    guarding_=false;if(watchdog_)esp_timer_stop(watchdog_);mark(5);
  }
  bool due()const{return !ready && watchdogReady && (stepRequested_ || (!paused && now()>=nextStepAt_));}
  void completeStep() {
    unguard();stepRequested_=false;
    if(++nextStep>4){ready=true;currentStage=5;note="Startup complete";}
    else {currentStage=nextStep;note=paused ? "Stage completed. Continue when ready." : "Continuing startup";nextStepAt_=now()+200;}
  }
  String command(const String &action) {
    if(ready)return "Startup is already complete";
    if(action=="pause"){paused=true;note="Hardware startup paused";return String();}
    if(!watchdogReady)return "Startup watchdog unavailable. Restart the S3.";
    if(action=="next"){paused=true;stepRequested_=true;return String();}
    if(action=="skipSd"){if(nextStep!=4)return "Skip SD is available at the SD stage";skipSd=true;paused=true;stepRequested_=true;return String();}
    if(action=="continue"){paused=false;stepRequested_=false;nextStepAt_=now()+300;return String();}
    return "Unknown startup action";
  }
};

class AutobotanyStageScope {
  AutobotanyBoot *boot_;
  bool owns_=false,valid_=true;
public:
  AutobotanyStageScope(AutobotanyBoot *boot,uint8_t stage):boot_(boot) {
    if(boot_ && !boot_->guarding()){valid_=boot_->guard(stage);owns_=valid_;}
  }
  ~AutobotanyStageScope(){if(owns_)boot_->unguard();}
  bool valid()const{return valid_;}
};
