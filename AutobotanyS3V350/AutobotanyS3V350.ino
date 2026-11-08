#include <Arduino.h>
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Select an ESP32-S3 board for ProjectAutobotany"
#endif
#include "AutobotanyApp.h"
#include "AutobotanyWeb.h"
#include "AutobotanyDisplay.h"

AutobotanyBoot startup;
AutobotanyApp autobotany;
AutobotanyWeb dashboard(autobotany,startup);
AutobotanyDisplay display(autobotany);

void setup() {
  autobotany.fan.prepareOff();
  autobotany.pump.prepareOff();
  startup.begin();
  autobotany.beginCore(&startup);
  autobotany.beginNetwork();
  dashboard.begin();
}

void loop() {
  if(startup.ready)display.tick();
  dashboard.tick();
  autobotany.retryNetwork();
  if(startup.ready){autobotany.tick();}
  else if(autobotany.apReady && startup.due()) {
    uint8_t stage=startup.nextStep;
    if(startup.guard(stage)) {
      if(stage==1){autobotany.beginSpi();display.begin();display.startupStage("SENSORS NEXT");}
      if(stage==2){display.startupStage("STARTING SENSORS");autobotany.beginSensors();}
      if(stage==3){display.startupStage("LOADING SETTINGS");autobotany.beginStorage();}
      if(stage==4){display.startupStage(startup.skipSd ? "SD CARD SKIPPED" : "CHECKING SD CARD");if(startup.skipSd)autobotany.skipSd();else autobotany.beginSd();}
      startup.completeStep();
      autobotany.hardwareReady=startup.ready;
    }
  }
  delay(1);
}
