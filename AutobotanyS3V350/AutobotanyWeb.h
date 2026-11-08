#pragma once
#include <WebServer.h>
#include "AutobotanyApp.h"
#include "AutobotanyUI.h"
#include "AutobotanyPresentation.h"
#include "AutobotanyBootUI.h"
#include "AutobotanyPlantWeb.h"

class AutobotanyWeb {
  AutobotanyApp &a_;
  AutobotanyBoot &boot_;
  WebServer server_{80};
  AutobotanyPlantWeb plants_{a_,server_};
  static String quote(const String &value) {
    String out;out.reserve(value.length()+4);out+='"';
    const char digits[]="0123456789abcdef";
    for (size_t i=0;i<value.length();++i) {
      uint8_t c=value[i];
      if (c=='"' || c=='\\') {out+='\\';out+=static_cast<char>(c);}
      else if (c<32) {out+="\\u00";out+=digits[c>>4];out+=digits[c&15];}
      else out+=static_cast<char>(c);
    }
    out+='"';return out;
  }
  static String number(double value,int places=2) {return isfinite(value) ? String(value,places) : String("null");}
  static String truth(bool value) {return value ? "true" : "false";}
  static String stamp(uint32_t epoch) {return epoch ? quote(AutobotanyApp::iso(epoch)) : String("null");}
  String sample(const AB::Sample &s,uint64_t now,bool booleanValue=false,const AB::ProbeCycle *cycle=nullptr) {
    String out="{\"value\":";
    out+=s.valid ? (booleanValue ? truth(s.value>0) : number(s.value)) : String("null");
    out+=",\"valid\":"+truth(s.valid)+",\"active\":"+truth(s.active)+",\"calibrated\":"+truth(s.calibrated)+",\"scheduled\":"+truth(s.scheduled);
    out+=",\"lastRead\":"+stamp(s.epoch)+",\"ageMs\":"+number(s.valid ? static_cast<double>(now-s.acquiredMs) : NAN,0);
    out+=",\"freshMs\":"+number(s.freshMs,0)+",\"staleMs\":"+number(s.staleMs,0)+",\"raw\":"+(s.raw>=0 ? String(s.raw) : String("null"));
    if (cycle) out+=",\"nextReadSeconds\":"+number(cycle->busy() ? 0 : (cycle->nextMs>now ? (cycle->nextMs-now)/1000.0 : 0),0);
    return out+"}";
  }
  String blocker(AutobotanyBlocker &b) {
    return String("{\"covering\":")+(b.known ? truth(b.covering) : String("null"))+",\"valid\":"+truth(b.known)+",\"ageMs\":0,\"mode\":"+quote(b.automatic ? "auto" : "manual")+",\"moving\":"+truth(b.moving)+",\"positionSource\":\"commanded\"}";
  }
  void respond(int code,const String &body) {server_.sendHeader("Cache-Control","no-store");server_.send(code,"application/json; charset=utf-8",body);}
  void fail(int code,const String &error) {respond(code,String("{\"ok\":false,\"error\":")+quote(error)+"}");}
  bool authorizedHeader() {
    if (server_.header("X-Autobotany-Request")!="1") {fail(403,"Use the local dashboard controls");return false;}
    return true;
  }
  bool allowed() {if(!authorizedHeader())return false;if(!a_.hardwareReady){fail(503,"Hardware startup is paused or incomplete. Open /startup");return false;}return true;}
  void startupPage(){server_.sendHeader("Cache-Control","no-store");server_.send_P(200,"text/html; charset=utf-8",AUTOBOTANY_BOOT_HTML,sizeof(AUTOBOTANY_BOOT_HTML)-1);}
  void startupStatus() {
    String out;out.reserve(1000);
    out="{\"version\":\"3.6.0\",\"ready\":"+truth(boot_.ready)+",\"paused\":"+truth(boot_.paused)+",\"recovery\":"+truth(boot_.recovery)+",\"watchdogReady\":"+truth(boot_.watchdogReady);
    out+=",\"stage\":"+quote(AutobotanyBoot::name(boot_.currentStage))+",\"nextStage\":"+quote(AutobotanyBoot::name(boot_.nextStep));
    out+=",\"previousStage\":"+quote(AutobotanyBoot::name(boot_.previousStage))+",\"previousTimedOut\":"+truth(boot_.previousTimedOut);
    out+=",\"resetReason\":"+quote(AutobotanyBoot::resetName(boot_.resetReason))+",\"resetCode\":"+String(static_cast<int>(boot_.resetReason));
    out+=",\"uptimeSeconds\":"+number(AutobotanyBoot::now()/1000.0,1)+",\"freeHeap\":"+String(ESP.getFreeHeap())+",\"psramBytes\":"+String(ESP.getPsramSize())+",\"note\":"+quote(boot_.note)+"}";
    respond(200,out);
  }
  void startupCommand() {
    if(!authorizedHeader())return;
    String error=boot_.command(server_.arg("action"));
    if(error.length()){fail(409,error);return;}
    respond(200,"{\"ok\":true}");
  }
  bool parse(const char *key,double &value) {
    if (!server_.hasArg(key)) return false;
    String raw=server_.arg(key);if (!raw.length() || raw.length()>24) return false;
    char *end=nullptr;value=strtod(raw.c_str(),&end);
    return end!=raw.c_str() && *end=='\0' && isfinite(value);
  }
  void status() {
    uint64_t now=AutobotanyApp::now();String out;out.reserve(6200);
    out="{\"schemaVersion\":1,\"deviceTime\":"+stamp(AutobotanyApp::epoch())+",\"sensors\":{";
    out+="\"moisture\":"+sample(a_.moisture,now,false,&a_.soilCycle)+",\"water\":"+sample(a_.water,now,false,&a_.waterCycle)+",\"rain\":"+sample(a_.rain,now,true,&a_.rainCycle);
    out+=",\"sunlight\":"+sample(a_.sunlight,now)+",\"humidity\":"+sample(a_.humidity,now)+",\"temperature\":"+sample(a_.temperature,now)+",\"heat\":"+sample(a_.heat,now)+",\"espTemperature\":"+sample(a_.boardTemperature,now)+"}";
    out+=",\"fan\":{\"on\":"+(a_.fanReady ? truth(a_.fan.on()) : String("null"))+",\"mode\":"+quote(a_.fanAutomatic ? "auto" : "manual")+",\"runSeconds\":"+number(a_.fan.runSeconds(now))+",\"manualRemainingSeconds\":"+number(a_.fan.remainingMs(now)/1000.0)+"}";
    out+=",\"blockers\":{\"sun\":"+blocker(a_.sunBlocker)+",\"rain\":"+blocker(a_.rainBlocker)+"}";
    static const char *stages[]={"seed","sprout","seedling","vegetative","flowering","mature"};
    out+=",\"plant\":{\"stage\":"+(a_.gardenSet?quote(ABPlant::stageName(a_.garden.stage)):a_.plantSet ? quote(stages[a_.plant.stage]) : String("null"))+",\"ageDays\":"+number(a_.plantAge(now),0)+",\"profile\":"+plants_.profileJson()+",\"ageClockReliable\":"+truth(a_.gardenSet?a_.garden.anchorEpoch>0&&AutobotanyApp::epoch()>=a_.garden.anchorEpoch:AutobotanyApp::clockValid())+"}";
    out+=",\"vision\":"+plants_.visionJson();
    out+=",\"thresholds\":{";
    for (size_t i=0;i<12;++i) {if (i) out+=',';out+=quote(AB::Thresholds::name(i))+":"+number(a_.limits.values[i]);}
    out+="},\"system\":{\"pumpOn\":"+truth(a_.pump.on())+",\"pumpPending\":"+truth(a_.pumpPending)+",\"pumpNote\":"+quote(a_.pumpNote)+",\"pumpRemainingSeconds\":"+number(a_.pump.remainingMs(now)/1000.0);
    out+=",\"pumpRunSeconds\":"+number(a_.pump.runSeconds(now))+",\"wifiConnected\":false,\"networkMode\":\"softap\",\"apActive\":"+truth(a_.apReady)+",\"apSsid\":"+quote(AB::AP_NAME)+",\"apClients\":"+String(WiFi.softAPgetStationNum())+",\"rssi\":null";
    out+=",\"manualPumpSeconds\":"+number(a_.manualPumpMs()/1000.0,0);
    out+=",\"uptimeSeconds\":"+number(now/1000.0)+",\"sdReady\":"+truth(a_.sdReady)+",\"lastSdWrite\":"+stamp(a_.lastSdWrite)+",\"lastWatered\":"+stamp(a_.lastWatered)+",\"lastRained\":"+stamp(a_.lastRained);
    out+=",\"rtcPresent\":"+truth(a_.rtcPresent)+",\"clockValid\":"+truth(AutobotanyApp::clockValid())+",\"freeHeap\":"+String(ESP.getFreeHeap())+",\"freePsram\":"+String(ESP.getFreePsram())+",\"automaticWatering\":"+truth(AB::AUTOMATIC_WATERING)+"}";
    bool cameraOnline=a_.cameraSeenMs && now-a_.cameraSeenMs<=45000;
    String cameraIp=a_.cameraAddress;
    String captureUrl=cameraIp.length() ? "http://"+cameraIp+"/capture" : String();
    String streamUrl=cameraIp.length() ? "http://"+cameraIp+":81/stream" : String();
    out+=",\"camera\":{\"ready\":"+truth(a_.cameraReady)+",\"enhanced\":"+truth(a_.cameraEnhanced)+",\"rssi\":"+number(cameraOnline && a_.cameraEnhanced ? a_.cameraRssi : NAN,0)+",\"fps\":"+number(cameraOnline && a_.cameraEnhanced ? a_.cameraFps : NAN,1)+",\"profile\":"+String(a_.cameraProfile)+",\"frameBytes\":"+String(a_.cameraFrameBytes)+",\"online\":"+truth(cameraOnline)+",\"snapshotUrl\":"+quote(captureUrl)+",\"streamUrl\":"+quote(streamUrl)+",\"ageMs\":"+number(a_.cameraSeenMs ? static_cast<double>(now-a_.cameraSeenMs) : NAN,0)+"}";
    AB::CalibrationSession &c=a_.calibration;
    out+=",\"calibration\":{\"active\":"+truth(c.active)+",\"running\":"+truth(c.running)+",\"target\":"+String(c.target)+",\"point\":"+String(c.point)+",\"captured\":"+String(c.captured)+",\"message\":"+quote(c.message);
    out+=",\"progress\":"+number(c.running ? fmin(95.0,100.0*c.capture.count/AB::StableCapture::SIZE) : c.captured==3 ? 100 : 0,0)+",\"spread\":"+String(c.capture.spread)+",\"endpoints\":[";
    for(int i=0;i<6;++i){if(i)out+=',';out+=String(a_.calibrationData.endpoints[i]);}
    out+="],\"mask\":"+String(a_.calibrationData.mask)+"}";
    out+=",\"capabilities\":{\"fanControl\":"+truth(a_.hardwareReady && a_.fanReady)+",\"pumpControl\":"+truth(a_.hardwareReady && a_.pumpReady && a_.water.calibrated)+",\"moistureControl\":"+truth(a_.hardwareReady)+",\"waterControl\":"+truth(a_.hardwareReady)+",\"rainControl\":"+truth(a_.hardwareReady)+",\"blockerControl\":"+truth(a_.hardwareReady)+",\"thresholds\":"+truth(a_.hardwareReady && a_.storageReady)+",\"growth\":"+truth(a_.hardwareReady && a_.storageReady)+",\"clock\":"+truth(a_.hardwareReady && a_.rtcPresent)+"}}";
    respond(200,out);
  }
  void control() {
    if (!allowed()) return;
    String target=server_.arg("target"),mode=server_.arg("mode"),enabled=server_.arg("enabled");
    bool isAuto=mode=="auto";
    if (!isAuto && enabled!="0" && enabled!="1") {fail(400,"enabled must be 0 or 1");return;}
    bool next=enabled=="1";String error;
    if(target=="sampleAll") {
      if(a_.calibration.active){fail(409,"Finish or cancel calibration first");return;}
      a_.probeCommand("moisture",true);a_.probeCommand("water",true);a_.probeCommand("rain",true);
    }
    else if (target=="fan") error=a_.fanCommand(mode,next);
    else if (target=="pump") {if(isAuto){fail(400,"Pump automatic policy is configured in Config.h");return;}error=a_.pumpCommand(next);}
    else if (target=="sunBlocker" || target=="rainBlocker") error=a_.blockerCommand(target,mode,next);
    else if (target=="water" || target=="moisture" || target=="rain") {if(isAuto){fail(400,"Use enabled for a sensor reading");return;}error=a_.probeCommand(target,next);}
    else error="Unknown component";
    if (error.length()) {fail(409,error);return;}
    respond(200,String("{\"ok\":true,\"pending\":")+truth(a_.pumpPending)+"}");
  }
  void thresholds() {
    if (!allowed()) return;
    AB::Thresholds next;
    for (size_t i=0;i<12;++i) {double value;if(!parse(AB::Thresholds::name(i),value)){fail(400,"Every threshold must be numeric");return;}next.values[i]=value;}
    if (!next.valid()) {fail(400,"Threshold ranges are invalid");return;}
    if (!a_.saveThresholds(next)) {fail(500,"Thresholds could not be saved");return;}
    respond(200,"{\"ok\":true}");
  }
  void growth() {
    if (!allowed()) return;
    if(a_.gardenSet){fail(409,"Use Plant setup to update the selected profile's stage and age.");return;}
    static const char *stages[]={"seed","sprout","seedling","vegetative","flowering","mature"};
    String stage=server_.arg("stage");int index=-1;for(int i=0;i<6;++i)if(stage==stages[i])index=i;
    double age;
    if(index<0 || !parse("ageDays",age) || age<0 || age>36500 || floor(age)!=age){fail(400,"Invalid stage or plant age");return;}
    if(!a_.savePlant(index,static_cast<uint32_t>(age))){fail(500,"Plant settings could not be saved");return;}
    respond(200,"{\"ok\":true}");
  }
  void clock() {
    if (!allowed()) return;
    double epoch;
    if (!parse("epoch",epoch) || floor(epoch)!=epoch || epoch<1700000000.0 || epoch>=4102444800.0 || (sizeof(time_t)<8 && epoch>2147483647.0)) {fail(400,"Date is outside this core's supported range");return;}
    if(!a_.setClock(static_cast<uint32_t>(epoch))){fail(503,"RTC could not be set; check the DS3231 wiring");return;}
    respond(200,"{\"ok\":true}");
  }
  void calibration() {
    if(!allowed())return;
    bool cancel=server_.arg("action")=="cancel";
    double point=0;
    if(!cancel && (!parse("point",point) || (point!=0 && point!=1))){fail(400,"Reference point must be 0 or 1");return;}
    String error=a_.calibrationCommand(server_.arg("target"),static_cast<int>(point),cancel);
    if(error.length()){fail(409,error);return;}
    respond(200,"{\"ok\":true}");
  }
  void history() {
    String out;out.reserve(11500);
    out="{\"intervalSeconds\":60,\"capacity\":120,\"keys\":[\"temperature\",\"humidity\",\"sunlight\",\"moisture\",\"water\",\"espTemperature\"],\"points\":[";
    for(uint16_t i=0;i<a_.historyCount();++i){
      if(i)out+=',';
      const AB::HistoryPoint &point=a_.historyAt(i);
      out+='[';out+=String(point.uptime)+","+String(point.epoch);
      for(int k=0;k<6;++k)out+=","+number(point.values[k],k==2 ? 0 : 1);
      out+=']';
    }
    respond(200,out+"]}");
  }
  void cameraHeartbeat() {
    if (!authorizedHeader()) return;
    IPAddress peer=server_.client().remoteIP();
    if(peer[0]!=192 || peer[1]!=168 || peer[2]!=4 || peer[3]==0 || peer[3]==1 || peer[3]==255){fail(403,"Camera must be connected to the Autobotany SoftAP");return;}
    double value;
    a_.cameraReady=!server_.hasArg("ready") || server_.arg("ready")=="1";
    a_.cameraEnhanced=server_.arg("protocol")=="3";
    if(parse("rssi",value) && value>=-127 && value<=0)a_.cameraRssi=static_cast<int>(value);
    if(parse("fps",value) && value>=0 && value<=60)a_.cameraFps=value;
    if(parse("profile",value) && value>=0 && value<=2 && floor(value)==value)a_.cameraProfile=static_cast<int>(value);
    if(parse("bytes",value) && value>=0 && value<=600000)a_.cameraFrameBytes=static_cast<uint32_t>(value);
    a_.cameraAddress=peer.toString();a_.cameraSeenMs=AutobotanyApp::now();respond(200,"{\"ok\":true}");
  }
public:
  explicit AutobotanyWeb(AutobotanyApp &app,AutobotanyBoot &boot):a_(app),boot_(boot){}
  void begin() {
    plants_.begin();
    const char *headers[]={"X-Autobotany-Request"};server_.collectHeaders(headers,1);
    for(size_t i=0;i<ABPresentation::Count;++i) {
      const ABPresentation::Asset *asset=&ABPresentation::Assets[i];
      server_.on(asset->path,HTTP_GET,[this,asset](){
        if(!a_.hardwareReady && (String(asset->path)=="/" || String(asset->path)=="/index.html")){startupPage();return;}
        server_.sendHeader("Cache-Control",asset->cached ? "public, max-age=31536000, immutable" : "no-cache");
        server_.sendHeader("X-Content-Type-Options","nosniff");
        if(asset->gzip)server_.sendHeader("Content-Encoding","gzip");
        server_.send_P(200,asset->mime,reinterpret_cast<const char*>(asset->data),asset->size);
      });
    }
    server_.on("/dashboard",HTTP_GET,[this](){server_.sendHeader("Cache-Control","no-cache");server_.send_P(200,"text/html; charset=utf-8",AUTOBOTANY_HTML,sizeof(AUTOBOTANY_HTML)-1);});
    server_.on("/dashboard.html",HTTP_GET,[this](){server_.sendHeader("Cache-Control","no-cache");server_.send_P(200,"text/html; charset=utf-8",AUTOBOTANY_HTML,sizeof(AUTOBOTANY_HTML)-1);});
    server_.on("/startup",HTTP_GET,[this](){startupPage();});
    server_.on("/api/startup/status",HTTP_GET,[this](){startupStatus();});
    server_.on("/api/startup",HTTP_POST,[this](){startupCommand();});
    server_.on("/api/status",HTTP_GET,[this](){status();});
    server_.on("/api/control",HTTP_POST,[this](){control();});
    server_.on("/api/thresholds",HTTP_POST,[this](){thresholds();});
    server_.on("/api/growth",HTTP_POST,[this](){growth();});
    server_.on("/api/time",HTTP_POST,[this](){clock();});
    server_.on("/api/camera",HTTP_POST,[this](){cameraHeartbeat();});
    server_.on("/api/calibration",HTTP_POST,[this](){calibration();});
    server_.on("/api/history",HTTP_GET,[this](){history();});
    server_.on("/logs/latest.csv",HTTP_GET,[this](){
      if(!a_.hardwareReady){server_.send(503,"text/plain","Hardware startup incomplete. Open /startup");return;}
      if(!a_.sdReady || !a_.lastLogPath.length()){server_.send(404,"text/plain","No SD log available");return;}
      File file=SD.open(a_.lastLogPath,FILE_READ);if(!file){server_.send(404,"text/plain","Log unavailable");return;}
      server_.sendHeader("Content-Disposition","attachment; filename=autobotany.csv");server_.streamFile(file,"text/csv");file.close();
    });
    server_.onNotFound([this](){fail(404,"Endpoint not found");});server_.begin();
  }
  void tick() {server_.handleClient();}
};
