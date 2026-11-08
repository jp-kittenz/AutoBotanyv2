#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <DHT.h>
#include <RTClib.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>
#include "AutobotanyCore.h"
#include "AutobotanyCalibration.h"
#include "TimedOutput.h"
#include "ServoBlocker.h"
#include "AutobotanyBoot.h"
#include "AutobotanyPlants.h"
#include "AutobotanyVision.h"

class AutobotanyApp {
  Preferences prefs_;
  DHT dht_{AB::DHT_PIN,DHT22};
  RTC_DS3231 rtc_;
  uint64_t nextEnvironment_=0,nextSdRetry_=0,lastLog_=0,lastAutoWater_=0;
  uint64_t pendingSince_=0,pendingDeadline_=0;
  bool pendingAutomatic_=false,previousPumpOn_=false,logDirty_=false;
  uint64_t pendingDuration_=AB::MANUAL_RUN_MS;
  AB::HistoryPoint history_[120];
  uint16_t historyHead_=0,historyCount_=0;
  uint64_t nextHistory_=0;
  uint64_t nextNetworkRetry_=0;
  AutobotanyBoot *boot_=nullptr;
  bool i2cReady_=false;

  AB::Sample &probeSample(int target) {return target==0 ? moisture : target==1 ? water : rain;}
  AB::ProbeCycle &probeCycle(int target) {return target==0 ? soilCycle : target==1 ? waterCycle : rainCycle;}
  int probeEnable(int target) {return target==0 ? AB::SOIL_ENABLE : target==1 ? AB::WATER_ENABLE : AB::RAIN_ENABLE;}
  int probeAdc(int target) {return target==0 ? AB::SOIL_ADC : target==1 ? AB::WATER_ADC : AB::RAIN_ADC;}
  void stopCalibrationPower(uint64_t tick) {
    if(calibration.target<0)return;
    powerProbe(probeEnable(calibration.target),false);
    probeSample(calibration.target).active=false;
    probeCycle(calibration.target).cancel(tick);
    calibration.running=false;
  }
  void tickCalibration(uint64_t tick) {
    if(!calibration.active)return;
    if(tick>=calibration.idleUntilMs){stopCalibrationPower(tick);calibration.active=false;calibration.message="Session expired. Return the probe to its installed position.";return;}
    if(!calibration.running)return;
    if(tick>=calibration.timeoutMs){stopCalibrationPower(tick);calibration.message="Reading was unstable. Check contact and retry this reference.";return;}
    if(tick<calibration.nextMs)return;
    calibration.nextMs=tick+40;
    if(!calibration.capture.feed(analogRead(probeAdc(calibration.target))))return;
    calibration.endpoints[calibration.point]=calibration.capture.mean;
    calibration.captured|=1U<<calibration.point;
    stopCalibrationPower(tick);
    if(calibration.captured!=3){calibration.message="First reference captured. Prepare the second condition.";return;}
    if(abs(calibration.endpoints[1]-calibration.endpoints[0])<200){calibration.captured=1;calibration.message="References are too similar. Check the sensor and retry the second point.";return;}
    AB::CalibrationData next=calibrationData;
    next.endpoints[calibration.target*2]=calibration.endpoints[0];
    next.endpoints[calibration.target*2+1]=calibration.endpoints[1];
    next.mask|=1U<<calibration.target;
    if(!next.valid() || !storageReady || prefs_.putBytes("calibration1",&next,sizeof(next))!=sizeof(next)){
      calibration.message="Calibration could not be saved. Previous settings are retained.";return;
    }
    calibrationData=next;
    probeSample(calibration.target).calibrated=true;
    probeSample(calibration.target).valid=false;
    calibration.active=false;
    calibration.message="Saved. Return the probe to its installed position, then activate a fresh reading.";
    logDirty_=true;
  }
  void recordHistory(uint64_t tick) {
    if(tick<nextHistory_)return;
    nextHistory_=tick+60000;
    AB::HistoryPoint &point=history_[historyHead_];
    point.uptime=tick/1000;point.epoch=epoch();
    AB::Sample *samples[]={&temperature,&humidity,&sunlight,&moisture,&water,&boardTemperature};
    for(int i=0;i<6;++i)point.values[i]=samples[i]->current(tick) && samples[i]->calibrated ? samples[i]->value : NAN;
    historyHead_=(historyHead_+1)%120;
    if(historyCount_<120)++historyCount_;
  }

  void powerProbe(int pin,bool on) {digitalWrite(pin,on==AB::SENSOR_ACTIVE_HIGH ? HIGH : LOW);}
  bool luxCommand(uint8_t value) {Wire.beginTransmission(AB::BH1750_ADDRESS);Wire.write(value);return Wire.endTransmission()==0;}
  void initLux() {
    luxPresent=luxCommand(0x01);
    luxPresent=luxPresent && luxCommand(0x40 | (AB::BH1750_MTREG>>5));
    luxPresent=luxPresent && luxCommand(0x60 | (AB::BH1750_MTREG&31));
    luxPresent=luxPresent && luxCommand(0x10);
    sunlight.active=luxPresent;
  }
  void initSample(AB::Sample &s,bool calibrated,uint64_t interval) {
    s.scheduled=true;
    s.calibrated=calibrated;
    s.freshMs=60000;
    s.staleMs=interval+300000;
  }
  void tickProbe(AB::ProbeCycle &cycle,AB::Sample &sample,int adc,int enable,int low,int high,uint64_t now,bool rainProbe=false) {
    if (cycle.scheduledDue(now)) requestProbe(cycle,sample,enable,now);
    if (!cycle.sampleDue(now)) return;
    if (!cycle.feed(analogRead(adc),now)) return;
    int raw=cycle.mean();
    powerProbe(enable,false);
    sample.active=false;
    double value=AB::calibratedPercent(raw,low,high);
    if (rainProbe && isfinite(value)) value=value>=AB::RAIN_TRIGGER_PERCENT ? 1.0 : 0.0;
    sample.set(value,now,epoch(),raw);
    if (rainProbe && value>0 && sample.calibrated) lastRained=epoch();
    logDirty_=true;
  }
  void requestProbe(AB::ProbeCycle &cycle,AB::Sample &sample,int enable,uint64_t now) {
    if (!cycle.request(now)) return;
    sample.active=true;
    powerProbe(enable,true);
  }
  void environment(uint64_t now) {
    double rh=dht_.readHumidity();
    double air=dht_.readTemperature();
    if (isfinite(rh) && rh>=0 && rh<=100) humidity.set(rh,now,epoch());
    else humidity.valid=false;
    if (isfinite(air) && air>=-40 && air<=80) temperature.set(air,now,epoch());
    else temperature.valid=false;
    if (humidity.valid && temperature.valid) heat.set(dht_.computeHeatIndex(air,rh,false),now,epoch());
    else heat.valid=false;
    double chip=temperatureRead();
    if (isfinite(chip) && chip>=-40 && chip<=125) boardTemperature.set(chip,now,epoch());
    else boardTemperature.valid=false;
    if (!i2cReady_) {sunlight.valid=false;return;}
    if (!luxPresent) initLux();
    else if (Wire.requestFrom(AB::BH1750_ADDRESS,static_cast<uint8_t>(2))==2) {
      uint8_t hi=Wire.read(),lo=Wire.read();
      uint16_t raw=(static_cast<uint16_t>(hi)<<8)|lo;
      sunlight.set(raw/1.2*(69.0/AB::BH1750_MTREG),now,epoch(),raw);
    } else {sunlight.valid=false;luxPresent=false;}
  }
  void automations(uint64_t now) {
    if (fanAutomatic && fanReady && boardTemperature.current(now)) {
      bool next=AB::fanDecision(fan.on(),boardTemperature.value,limits);
      if (next && !fan.on()) fan.start(now,0);
      if (!next && fan.on()) fan.stop(now);
    }
    if (sunBlocker.automatic && !rainBlocker.moving && sunlight.current(now)) {
      bool next=sunBlocker.covering;
      if (sunlight.value>limits.get("sunlightMax")) next=true;
      if (sunlight.value<limits.get("sunlightMax")*0.8) next=false;
      if (!sunBlocker.known || next!=sunBlocker.covering) sunBlocker.command(next,now);
    }
    if (rainBlocker.automatic && !sunBlocker.moving && rain.calibrated && rain.current(now)) {
      bool next=rain.value>0;
      if (!rainBlocker.known || next!=rainBlocker.covering) rainBlocker.command(next,now);
    }
    if (AB::AUTOMATIC_WATERING && !calibration.active && moisture.calibrated && water.calibrated && !pump.on() && !pumpPending && moisture.current(now) && moisture.value<limits.get("moistureMin") && (!lastAutoWater_ || now-lastAutoWater_>=AB::AUTO_WATER_COOLDOWN_MS)) {
      if (!(rain.calibrated && rain.current(now) && rain.value>0)) {
        lastAutoWater_=now;
        requestPump(now,true);
      }
    }
  }
  void servicePump(uint64_t now) {
    if (pumpPending) {
      if (now>pendingDeadline_) {pumpPending=false;pumpNote="Reservoir sampling timed out";}
      else if (!water.active && water.acquiredMs>=pendingSince_ && water.current(now,15000)) {
        pumpPending=false;
        if (water.value<=0 || water.value<limits.get("waterMin")) pumpNote="Reservoir below minimum";
        else if (pump.start(now,pendingDuration_)) {pumpNote=pendingAutomatic_ ? "Automatic watering pulse" : "Timed manual run";logDirty_=true;}
        else pumpNote="Pump timer unavailable";
      }
    }
    if (pump.on()) {
      if (!water.current(now,15000) || water.value<=0 || water.value<limits.get("waterMin")) {pump.stop(now);pumpNote="Stopped: reservoir low or outdated";}
      else if (!waterCycle.busy() && now-water.acquiredMs>=8000) requestProbe(waterCycle,water,AB::WATER_ENABLE,now);
    }
    bool current=pump.on();
    if (previousPumpOn_ && !current) {
      lastWatered=epoch();
      logDirty_=true;
      if (pendingAutomatic_) requestProbe(soilCycle,moisture,AB::SOIL_ENABLE,now);
      if (pumpNote=="Timed manual run" || pumpNote=="Automatic watering pulse") pumpNote="Run completed";
    }
    previousPumpOn_=current;
  }
  void mountSd(uint64_t now) {
    nextSdRetry_=now+60000;
    if (sdReady) return;
    AutobotanyStageScope scope(boot_,4);if(!scope.valid())return;
    SD.end();
    sdReady=SD.begin(AB::SD_CS,SPI,10000000);
    if (sdReady) {
      sdReady=SD.cardType()!=CARD_NONE;
      if (sdReady && !SD.exists("/autobotany")) sdReady=SD.mkdir("/autobotany");
    }
  }
  String csvValue(const AB::Sample &s) {return s.valid && isfinite(s.value) ? String(s.value,2) : String("");}
  void log(uint64_t now) {
    if (!sdReady) return;
    AutobotanyStageScope scope(boot_,4);if(!scope.valid())return;
    String path="/autobotany/unsynced.csv";
    time_t stamp=time(nullptr);
    if (clockValid()) {
      struct tm local;
      localtime_r(&stamp,&local);
      char buffer[40];
      strftime(buffer,sizeof(buffer),"/autobotany/%Y-%m-%d.csv",&local);
      path=buffer;
    }
    File file=SD.open(path,FILE_APPEND);
    if (!file) {sdReady=false;return;}
    bool ok=true;
    if (!file.size()) ok=file.println("utc,uptime_s,soil_pct,soil_raw,soil_age_s,soil_calibrated,water_pct,water_raw,water_age_s,water_calibrated,rain,rain_raw,rain_age_s,rain_calibrated,temp_c,humidity_pct,heat_c,lux,chip_c,fan_on,pump_on,sun_cover_command,rain_cover_command")>0;
    String row=iso(epoch())+","+String(now/1000.0,0)+","+csvValue(moisture)+","+String(moisture.raw)+","+String(moisture.valid ? (now-moisture.acquiredMs)/1000.0 : -1,0)+","+String(moisture.calibrated ? 1 : 0);
    row+=","+csvValue(water)+","+String(water.raw)+","+String(water.valid ? (now-water.acquiredMs)/1000.0 : -1,0)+","+String(water.calibrated ? 1 : 0);
    row+=","+csvValue(rain)+","+String(rain.raw)+","+String(rain.valid ? (now-rain.acquiredMs)/1000.0 : -1,0)+","+String(rain.calibrated ? 1 : 0);
    row+=","+csvValue(temperature)+","+csvValue(humidity)+","+csvValue(heat)+","+csvValue(sunlight)+","+csvValue(boardTemperature);
    row+=","+String(fan.on() ? 1 : 0)+","+String(pump.on() ? 1 : 0)+","+String(sunBlocker.covering ? 1 : 0)+","+String(rainBlocker.covering ? 1 : 0);
    ok=ok && file.println(row)>0;
    file.flush();file.close();
    if (!ok) {sdReady=false;return;}
    lastSdWrite=epoch();lastLogPath=path;lastLog_=now;logDirty_=false;
  }

public:
  AB::CalibrationData calibrationData;
  AB::CalibrationSession calibration;
  AB::Sample moisture,water,rain,temperature,humidity,heat,sunlight,boardTemperature;
  AB::ProbeCycle soilCycle{AB::PROBE_INTERVAL_MS},waterCycle{AB::PROBE_INTERVAL_MS},rainCycle{AB::RAIN_INTERVAL_MS};
  AB::Thresholds limits;
  AutobotanyTimedOutput fan{AB::FAN_PIN,AB::FAN_ACTIVE_HIGH},pump{AB::PUMP_PIN,AB::PUMP_ACTIVE_HIGH};
  AutobotanyBlocker sunBlocker{AB::SUN_SERVO,0,AB::SUN_OPEN_DEGREES,AB::SUN_COVER_DEGREES};
  AutobotanyBlocker rainBlocker{AB::RAIN_SERVO,1,AB::RAIN_OPEN_DEGREES,AB::RAIN_COVER_DEGREES};
  bool fanAutomatic=true,fanReady=false,pumpReady=false,pumpPending=false,storageReady=false,sdReady=false,rtcPresent=false,luxPresent=false,apReady=false;
  bool hardwareReady=false,sdEnabled=true;
  uint32_t lastWatered=0,lastRained=0,lastSdWrite=0;
  String pumpNote="Ready",lastLogPath;
  uint64_t cameraSeenMs=0;
  String cameraAddress;
  bool cameraReady=true,cameraEnhanced=false;
  int cameraRssi=0,cameraProfile=0;
  double cameraFps=0;
  uint32_t cameraFrameBytes=0;
  struct Plant {
    uint32_t version=2,stage=0,ageDays=0;
    int64_t savedEpoch=0;
  } plant;
  bool plantSet=false;
  uint64_t plantSavedMs=0;
  ABPlant::Profile garden;
  bool gardenSet=false;
  uint64_t gardenSavedMs=0;
  ABVision::Model visionModel;
  ABVision::Observation visionObservation;
  uint32_t visionFrame=0,visionLabelledFrame=0;
  uint64_t visionSeenMs=0;
  uint8_t visionCrop=0;
  bool visionHasFrame=false;

  static uint64_t now() {return static_cast<uint64_t>(esp_timer_get_time()/1000);}
  static uint32_t epoch() {time_t t=time(nullptr);return t>=1700000000 ? static_cast<uint32_t>(t) : 0;}
  static bool clockValid() {return epoch()!=0;}
  static String localDate(uint32_t value) {
    if(!value)return String();time_t stamp=value;struct tm local;localtime_r(&stamp,&local);
    char buffer[16];strftime(buffer,sizeof(buffer),"%Y-%m-%d",&local);return String(buffer);
  }
  static String iso(uint32_t value) {
    if (!value) return String();
    time_t stamp=value;struct tm utc;gmtime_r(&stamp,&utc);
    char buffer[25];strftime(buffer,sizeof(buffer),"%Y-%m-%dT%H:%M:%SZ",&utc);return String(buffer);
  }
  double plantAge(uint64_t tick) const {
    if(gardenSet)return floor(ABPlant::ageSeconds(garden,epoch(),tick,gardenSavedMs)/86400.0);
    if (!plantSet) return NAN;
    uint32_t current=epoch();
    double elapsed=plant.savedEpoch>0 && current>=plant.savedEpoch ? (current-plant.savedEpoch)/86400.0 : (tick-plantSavedMs)/86400000.0;
    return plant.ageDays+floor(elapsed);
  }
  void beginCore(AutobotanyBoot *boot=nullptr) {
    boot_=boot;
    hardwareReady=false;
    fanReady=fan.begin("ab_fan");pumpReady=pump.begin("ab_pump");
    const int enables[]={AB::WATER_ENABLE,AB::SOIL_ENABLE,AB::RAIN_ENABLE};
    for (int pin:enables) {digitalWrite(pin,AB::SENSOR_ACTIVE_HIGH ? LOW : HIGH);pinMode(pin,OUTPUT);}
    for(int pin:{AB::SUN_SERVO,AB::RAIN_SERVO}){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
    analogReadResolution(12);
    analogSetPinAttenuation(AB::WATER_ADC,ADC_11db);analogSetPinAttenuation(AB::SOIL_ADC,ADC_11db);analogSetPinAttenuation(AB::RAIN_ADC,ADC_11db);
    initSample(moisture,AB::SOIL_CALIBRATED,AB::PROBE_INTERVAL_MS);
    initSample(water,AB::WATER_CALIBRATED,AB::PROBE_INTERVAL_MS);
    initSample(rain,AB::RAIN_CALIBRATED,AB::RAIN_INTERVAL_MS);
    temperature.active=humidity.active=heat.active=boardTemperature.active=true;
    setenv("TZ","PHT-8",1);tzset();
  }
  void beginNetwork() {
    WiFi.persistent(false);WiFi.mode(WIFI_AP);WiFi.setSleep(false);
    IPAddress address(192,168,4,1),mask(255,255,255,0);
    WiFi.softAPConfig(address,address,mask);
    apReady=WiFi.softAP(AB::AP_NAME,AB::AP_PASSWORD,AB::WIFI_CHANNEL,false,AB::MAX_CLIENTS);
    nextNetworkRetry_=now()+10000;
  }
  void retryNetwork(){if(!apReady && now()>=nextNetworkRetry_)beginNetwork();}
  void beginSpi() {
    digitalWrite(AB::TFT_CS,HIGH);pinMode(AB::TFT_CS,OUTPUT);digitalWrite(AB::SD_CS,HIGH);pinMode(AB::SD_CS,OUTPUT);
    SPI.begin(AB::TFT_SCK,AB::SD_MISO,AB::TFT_MOSI,AB::TFT_CS);
  }
  void beginSensors() {
    i2cReady_=Wire.begin(AB::I2C_SDA,AB::I2C_SCL);
    if(i2cReady_){Wire.setClock(100000);Wire.setTimeOut(30);}
    rtcPresent=i2cReady_ && rtc_.begin(&Wire);
    if (rtcPresent && !rtc_.lostPower()) {
      uint32_t value=rtc_.now().unixtime();
      if (value>=1700000000 && value<4102444800UL && (sizeof(time_t)>=8 || value<=2147483647UL)) {struct timeval tv={static_cast<time_t>(value),0};settimeofday(&tv,nullptr);}
    }
    if(i2cReady_)initLux();dht_.begin();
  }
  void beginStorage() {
    storageReady=prefs_.begin("autobotany",false);
    if (storageReady) {
      AB::Thresholds stored;
      if (prefs_.getBytesLength("limits1")==sizeof(stored.values) && prefs_.getBytes("limits1",stored.values,sizeof(stored.values))==sizeof(stored.values) && stored.valid()) limits=stored;
      Plant saved;
      if (prefs_.getBytesLength("plant2")==sizeof(saved) && prefs_.getBytes("plant2",&saved,sizeof(saved))==sizeof(saved) && saved.version==2 && saved.stage<6 && saved.ageDays<=36500) {plant=saved;plantSet=true;}
      ABPlant::Profile profile;
      if(prefs_.getBytesLength("garden1")==sizeof(profile) && prefs_.getBytes("garden1",&profile,sizeof(profile))==sizeof(profile) && profile.valid()) {garden=profile;gardenSet=true;limits=profile.limits;}
      ABVision::Model model;
      if(gardenSet && prefs_.getBytesLength("vision1")==sizeof(model) && prefs_.getBytes("vision1",&model,sizeof(model))==sizeof(model) && model.valid() && model.cycle==garden.cycle)visionModel=model;
      else {visionModel=ABVision::Model();visionModel.cycle=gardenSet?garden.cycle:0;}
    }
    if(storageReady && prefs_.getBytesLength("calibration1")==sizeof(calibrationData)) {
      AB::CalibrationData saved;
      if(prefs_.getBytes("calibration1",&saved,sizeof(saved))==sizeof(saved) && saved.valid())calibrationData=saved;
    }
    moisture.calibrated=calibrationData.ready(0);water.calibrated=calibrationData.ready(1);rain.calibrated=calibrationData.ready(2);
    plantSavedMs=now();
    gardenSavedMs=now();
  }
  void beginSd(){mountSd(now());}
  void skipSd(){sdEnabled=false;sdReady=false;}
  void begin(){beginCore();beginNetwork();beginSpi();beginSensors();beginStorage();beginSd();hardwareReady=true;}
  void tick() {
    if(!hardwareReady)return;
    uint64_t tick=now();fan.expire();pump.expire();
    sunBlocker.tick(tick);rainBlocker.tick(tick);
    tickCalibration(tick);
    if(!(calibration.active && calibration.target==1))tickProbe(waterCycle,water,AB::WATER_ADC,AB::WATER_ENABLE,calibrationData.endpoints[2],calibrationData.endpoints[3],tick);
    if(!(calibration.active && calibration.target==0))tickProbe(soilCycle,moisture,AB::SOIL_ADC,AB::SOIL_ENABLE,calibrationData.endpoints[0],calibrationData.endpoints[1],tick);
    if(!(calibration.active && calibration.target==2))tickProbe(rainCycle,rain,AB::RAIN_ADC,AB::RAIN_ENABLE,calibrationData.endpoints[4],calibrationData.endpoints[5],tick,true);
    if (tick>=nextEnvironment_) {nextEnvironment_=tick+AB::ENVIRONMENT_INTERVAL_MS;environment(tick);}
    servicePump(tick);automations(tick);recordHistory(tick);
    if (sdEnabled && !sdReady && tick>=nextSdRetry_ && !pump.on()) mountSd(tick);
    if (sdReady && (tick-lastLog_>=AB::SD_LOG_INTERVAL_MS || (logDirty_ && tick-lastLog_>=5000))) log(tick);
  }
  String probeCommand(const String &target,bool enabled) {
    AB::ProbeCycle *cycle=nullptr;AB::Sample *s=nullptr;int pin=-1;
    if (target=="moisture") {cycle=&soilCycle;s=&moisture;pin=AB::SOIL_ENABLE;}
    if (target=="water") {cycle=&waterCycle;s=&water;pin=AB::WATER_ENABLE;}
    if (target=="rain") {cycle=&rainCycle;s=&rain;pin=AB::RAIN_ENABLE;}
    if (!cycle) return "Unknown sensor";
    if(calibration.active && s==&probeSample(calibration.target))return "Finish or cancel this sensor calibration first";
    if (target=="water" && (pump.on() || pumpPending) && !enabled) return "Reservoir sampling is required during pump operation";
    if (enabled) requestProbe(*cycle,*s,pin,now());
    else {cycle->cancel(now());s->active=false;powerProbe(pin,false);}
    return String();
  }
  String fanCommand(const String &mode,bool enabled) {
    if (!fanReady) return "Fan timer unavailable";
    if (mode=="auto") {fanAutomatic=true;if(fan.on())fan.start(now(),0);return String();}
    if (mode!="manual") return "Invalid fan mode";
    fanAutomatic=false;
    if (enabled) {if (!fan.start(now(),AB::MANUAL_RUN_MS)) return "Could not start fan timer";}
    else fan.stop(now());
    logDirty_=true;return String();
  }
  String requestPump(uint64_t tick,bool automatic=false) {
    if (!pumpReady) return "Pump timer unavailable";
    if(calibration.active)return "Pump is held off during calibration";
    if (!water.calibrated) return "Calibrate the reservoir in Settings first";
    if (pump.on() || pumpPending) return String();
    if (water.current(tick,15000) && (water.value<=0 || water.value<limits.get("waterMin"))) return "Reservoir below minimum";
    pendingAutomatic_=automatic;pendingDuration_=automatic ? AB::AUTO_WATER_PULSE_MS : manualPumpMs();
    if (water.current(tick,15000) && !water.active) {
      if (!pump.start(tick,pendingDuration_)) return "Could not start pump timer";
      pumpNote=automatic ? "Automatic watering pulse" : "Timed manual run";
      logDirty_=true;
    } else {
      pendingSince_=tick;pendingDeadline_=tick+6000;pumpPending=true;pumpNote="Checking reservoir";
      requestProbe(waterCycle,water,AB::WATER_ENABLE,tick);
    }
    return String();
  }
  String pumpCommand(bool enabled) {
    if (enabled) return requestPump(now());
    pumpPending=false;pump.stop(now());pumpNote="Stopped manually";logDirty_=true;return String();
  }
  String blockerCommand(const String &target,const String &mode,bool cover) {
    AutobotanyBlocker *b=target=="sunBlocker" ? &sunBlocker : target=="rainBlocker" ? &rainBlocker : nullptr;
    if (!b) return "Unknown blocker";
    if (mode=="auto") {b->automatic=true;return String();}
    if (mode!="manual") return "Invalid blocker mode";
    if (!b->command(cover,now(),true)) return "Servo output could not be attached";
    logDirty_=true;return String();
  }
  uint16_t historyCount() const {return historyCount_;}
  const AB::HistoryPoint &historyAt(uint16_t index) const {return history_[(historyHead_+120-historyCount_+index)%120];}
  String calibrationCommand(const String &target,int point,bool cancel) {
    uint64_t tick=now();
    if(cancel){stopCalibrationPower(tick);calibration.active=false;calibration.message="Cancelled. Return the probe to its installed position.";return String();}
    int index=target=="moisture" ? 0 : target=="water" ? 1 : target=="rain" ? 2 : -1;
    if(index<0 || point<0 || point>1)return "Choose a sensor and reference point";
    if(!storageReady)return "Settings storage is unavailable";
    if(calibration.running)return "Wait for capture to finish or cancel it";
    if(calibration.active && calibration.target!=index)return "Finish or cancel the current sensor first";
    if(point==1 && (!calibration.active || calibration.target!=index || !(calibration.captured&1)))return "Capture the first reference before the second";
    pumpCommand(false);
    if(point==0){calibration=AB::CalibrationSession();calibration.target=index;}
    calibration.active=true;calibration.running=true;calibration.point=point;
    calibration.captured&=static_cast<uint8_t>(~(1U<<point));
    calibration.capture.reset();calibration.startedMs=tick;calibration.nextMs=tick+AB::PROBE_WARMUP_MS;
    calibration.timeoutMs=tick+12000;calibration.idleUntilMs=tick+600000;
    calibration.message="Sampling automatically. Keep the reference condition steady.";
    probeCycle(index).cancel(tick);probeSample(index).valid=false;probeSample(index).active=true;
    powerProbe(probeEnable(index),true);
    return String();
  }
  bool saveThresholds(const AB::Thresholds &next) {
    if(gardenSet){ABPlant::Profile copy=garden;copy.limits=next;copy.customLimits=1;copy.revision++;return saveGarden(copy,false);}
    if (!storageReady || !next.valid() || prefs_.putBytes("limits1",next.values,sizeof(next.values))!=sizeof(next.values)) return false;
    limits=next;return true;
  }
  bool savePlant(uint32_t stage,uint32_t days) {
    if(gardenSet)return false; // The guided profile owns stage/age once configured.
    if (!storageReady || stage>=6 || days>36500) return false;
    Plant next;next.stage=stage;next.ageDays=days;next.savedEpoch=epoch();
    if (prefs_.putBytes("plant2",&next,sizeof(next))!=sizeof(next)) return false;
    plant=next;plantSet=true;plantSavedMs=now();return true;
  }
  uint64_t manualPumpMs() const {return gardenSet?5000:AB::MANUAL_RUN_MS;}
  bool saveGarden(const ABPlant::Profile &next,bool resetAge=true) {
    if(!storageReady||!next.valid())return false;
    ABPlant::Profile copy=next;
    if(!resetAge&&gardenSet){double age=ABPlant::ageSeconds(garden,epoch(),now(),gardenSavedMs);copy.ageSeconds=isfinite(age)?static_cast<uint32_t>(fmin(age,3153600000.0)):0;copy.anchorEpoch=epoch();}
    if(prefs_.putBytes("garden1",&copy,sizeof(copy))!=sizeof(copy))return false;
    bool changed=!gardenSet||garden.cycle!=copy.cycle;
    garden=copy;gardenSet=true;limits=copy.limits;gardenSavedMs=now();
    if(changed){visionModel=ABVision::Model();visionModel.cycle=copy.cycle;visionHasFrame=false;}
    return true;
  }
  bool saveVision(const ABVision::Model &next) {
    if(!storageReady||!gardenSet||next.cycle!=garden.cycle||!next.valid())return false;
    if(prefs_.putBytes("vision1",&next,sizeof(next))!=sizeof(next))return false;
    visionModel=next;return true;
  }
  bool setClock(uint32_t value) {
    if (!rtcPresent || value<1700000000 || value>=4102444800UL || (sizeof(time_t)<8 && value>2147483647UL)) return false;
    uint32_t previous=epoch();
    double currentAge=plantAge(now());
    double gardenAge=gardenSet?ABPlant::ageSeconds(garden,previous,now(),gardenSavedMs):NAN;
    rtc_.adjust(DateTime(value));
    if (rtc_.lostPower() || rtc_.now().unixtime()+2<value || rtc_.now().unixtime()>value+2) return false;
    struct timeval tv={static_cast<time_t>(value),0};if (settimeofday(&tv,nullptr)!=0) return false;
    AB::Sample *samples[]={&moisture,&water,&rain,&temperature,&humidity,&heat,&sunlight,&boardTemperature};
    uint64_t tick=now();
    for(AB::Sample *s:samples)if(s->valid && tick>=s->acquiredMs){uint64_t age=(tick-s->acquiredMs)/1000;s->epoch=value>age ? value-static_cast<uint32_t>(age) : 0;}
    if(previous){
      int64_t correction=static_cast<int64_t>(value)-previous;
      uint32_t *events[]={&lastWatered,&lastRained,&lastSdWrite};
      for(uint32_t *event:events)if(*event){int64_t shifted=static_cast<int64_t>(*event)+correction;*event=shifted>0 && shifted<4102444800LL ? static_cast<uint32_t>(shifted) : 0;}
    }
    if (plantSet && !gardenSet && (!previous || previous!=value)) {plant.ageDays=static_cast<uint32_t>(currentAge);plant.savedEpoch=value;plantSavedMs=now();if(storageReady)prefs_.putBytes("plant2",&plant,sizeof(plant));}
    if(gardenSet&&previous!=value){garden.ageSeconds=isfinite(gardenAge)?static_cast<uint32_t>(fmin(gardenAge,3153600000.0)):0;garden.anchorEpoch=value;gardenSavedMs=now();if(storageReady)prefs_.putBytes("garden1",&garden,sizeof(garden));}
    return true;
  }
};
