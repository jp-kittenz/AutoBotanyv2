#pragma once
#include <WebServer.h>
#include "AutobotanyApp.h"

// Small synchronous payloads only. JPEG fetching/resizing happens in the browser;
// feature extraction and the teachable kNN model execute on the ESP32-S3.
// Photos can come from the camera or from a file on the viewer's phone/computer;
// both are reduced to the same 24x24 RGB565 crop before they reach the device.
class AutobotanyPlantWeb {
  AutobotanyApp &a_;WebServer &s_;
  static String q(const String &s){String o="\"";for(size_t i=0;i<s.length();i++){char c=s[i];if(c=='"'||c=='\\')o+='\\';if(static_cast<uint8_t>(c)>=32)o+=c;}o+='"';return o;}
  static String num(double x,int places=0){return isfinite(x)?String(x,places):String("null");}
  static String yes(bool v){return v?"true":"false";}
  void send(int code,const String &body){s_.sendHeader("Cache-Control","no-store");s_.send(code,"application/json; charset=utf-8",body);}
  void error(int code,const char *message){send(code,"{\"ok\":false,\"error\":"+q(message)+"}");}
  bool allowed(){if(s_.header("X-Autobotany-Request")!="1"){error(403,"Use the local dashboard controls");return false;}if(!a_.hardwareReady||!a_.storageReady){error(503,"Complete startup and check device storage first");return false;}return true;}
  bool integer(const char *key,uint32_t low,uint32_t high,uint32_t &out){String s=s_.arg(key);if(!s.length()||s.length()>10)return false;uint64_t n=0;for(size_t i=0;i<s.length();i++){if(s[i]<'0'||s[i]>'9')return false;n=n*10+s[i]-'0';if(n>high)return false;}if(n<low)return false;out=n;return true;}
  bool decimal(const char *key,double low,double high,double &out){String s=s_.arg(key);if(!s.length()||s.length()>20)return false;char *end=nullptr;out=strtod(s.c_str(),&end);return end!=s.c_str()&&!*end&&isfinite(out)&&out>=low&&out<=high;}
  bool revision(){uint32_t value;if(!integer("revision",0,0xfffffffeUL,value)||value!=(a_.gardenSet?a_.garden.revision:0)){error(409,"Plant settings changed. Reload the setup and review again.");return false;}return true;}
  static String limits(const AB::Thresholds &t){String out="{";for(int i=0;i<12;i++){if(i)out+=',';out+=q(AB::Thresholds::name(i))+":"+num(t.values[i],1);}return out+"}";}
  bool draft(ABPlant::Profile &p){
    if(!revision())return false;
    int index=ABPlant::find(s_.arg("preset").c_str());uint32_t stage,medium,event,exposure,known,age,newPlant;double volume;
    if(index<0||!integer("stage",0,ABPlant::StageCount-1,stage)||!integer("medium",0,ABPlant::MediumCount-1,medium)||!integer("event",0,ABPlant::EventCount-1,event)||!integer("exposure",0,ABPlant::ExposureCount-1,exposure)||!integer("ageKnown",0,1,known)||!integer("ageDays",0,36500,age)||!integer("newPlant",0,1,newPlant)||!decimal("potL",1,20,volume)){error(400,"Check the plant, stage, age and pot details.");return false;}
    if(a_.gardenSet&&!newPlant&&index!=a_.garden.preset){error(400,"Choose New plant when changing species.");return false;}
    p.preset=index;p.stage=stage;p.medium=medium;p.event=event;p.exposure=exposure;p.potL=volume;p.ageKnown=known;p.ageSeconds=known?age*86400UL:0;p.anchorEpoch=AutobotanyApp::epoch();
    if(a_.gardenSet&&!newPlant&&known&&a_.garden.ageKnown&&age==a_.plantAge(AutobotanyApp::now()))p.ageSeconds=static_cast<uint32_t>(fmin(3153600000.0,ABPlant::ageSeconds(a_.garden,AutobotanyApp::epoch(),AutobotanyApp::now(),a_.gardenSavedMs)));
    p.revision=a_.gardenSet?a_.garden.revision+1:1;
    p.cycle=a_.gardenSet?a_.garden.cycle+(newPlant?1:0):1;
    if(!p.cycle){error(409,"Plant cycle counter exhausted.");return false;}
    p.limits=ABPlant::suggest(p,a_.limits);
    if(!p.valid()){error(400,"That stage and planting event do not match this plant.");return false;}
    return true;
  }
  String details(const ABPlant::Profile &p){
    const ABPlant::Preset &r=ABPlant::Presets[p.preset];String out="{\"preset\":"+q(r.id)+",\"name\":"+q(r.name)+",\"stageIndex\":"+String(p.stage)+",\"stage\":"+q(ABPlant::stageName(p.stage));
    out+=",\"medium\":"+String(p.medium)+",\"event\":"+String(p.event)+",\"exposure\":"+String(p.exposure)+",\"potL\":"+num(p.potL,1)+",\"ageKnown\":"+yes(p.ageKnown)+",\"revision\":"+String(p.revision)+",\"cycle\":"+String(p.cycle);
    out+=",\"minPotL\":"+num(r.minPotL,1)+",\"repot\":"+yes(ABPlant::needsRepot(p))+",\"nurseryOnly\":"+yes(r.minPotL>20)+",\"customLimits\":"+yes(p.customLimits)+",\"note\":"+q(r.note)+",\"limits\":"+limits(p.limits)+"}";return out;
  }
  void catalogue(){
    String out;out.reserve(18000);out="{\"version\":1,\"count\":"+String(ABPlant::Count)+",\"presets\":[";
    for(uint8_t i=0;i<ABPlant::Count;i++){const ABPlant::Preset &p=ABPlant::Presets[i];if(i)out+=',';out+="{\"id\":"+q(p.id)+",\"name\":"+q(p.name)+",\"scientific\":"+q(p.scientific)+",\"group\":"+q(p.group)+",\"note\":"+q(p.note)+",\"minPotL\":"+num(p.minPotL,1)+",\"stageMask\":"+String(p.stages)+"}";}
    send(200,out+"]}");
  }
  void preview(){if(!allowed())return;ABPlant::Profile p;if(!draft(p))return;send(200,"{\"ok\":true,\"profile\":"+details(p)+"}");}
  void apply(){
    if(!allowed())return;ABPlant::Profile p;if(!draft(p))return;
    if(s_.arg("reviewed")!="1"){error(400,"Review the proposed settings before applying.");return;}
    if(ABPlant::needsRepot(p)&&s_.arg("repotAcknowledged")!="1"){error(400,"Acknowledge the repotting guidance first.");return;}
    if(a_.pump.on()||a_.pumpPending||a_.calibration.active){error(409,"Finish watering or calibration before changing the plant profile.");return;}
    if(!a_.saveGarden(p)){error(500,"Profile could not be saved. Previous settings are retained.");return;}
    send(200,"{\"ok\":true,\"profile\":"+details(a_.garden)+"}");
  }
  String prediction(const ABVision::Result &r,bool stage){return "{\"label\":"+q(stage?ABPlant::stageName(r.label):ABVision::conditionName(r.label))+",\"classesReady\":"+String(r.classes)+",\"neighborVotes\":"+String(r.agreement)+",\"distance\":"+String(r.nearest)+"}";}
  void observe(){
    if(!allowed()||!revision())return;
    if(!a_.gardenSet){error(409,"Choose a plant profile first.");return;}
    uint32_t crop;if(!integer("crop",0,2,crop)){error(400,"Invalid camera crop.");return;}
    if(a_.visionHasFrame&&AutobotanyApp::now()-a_.visionSeenMs<1000){error(429,"Wait a moment before analyzing another frame.");return;}
    bool photo=s_.arg("source")=="photo";
    if(!photo&&(!a_.cameraReady||!a_.cameraSeenMs||AutobotanyApp::now()-a_.cameraSeenMs>45000)){error(409,"Camera is offline. Reconnect, or analyse a photo file instead.");return;}
    if(a_.visionModel.count&&a_.visionModel.crop!=crop){error(409,"Clear the examples before changing the analysis crop.");return;}
    ABVision::Observation observation;String pixels=s_.arg("pixels");
    if(!ABVision::extract(pixels.c_str(),pixels.length(),observation)){error(400,"Expected a 24 by 24 RGB565 snapshot.");return;}
    a_.visionObservation=observation;a_.visionSeenMs=AutobotanyApp::now();a_.visionCrop=crop;a_.visionHasFrame=true;
    if(++a_.visionFrame==0)++a_.visionFrame;
    if(!a_.visionModel.count)a_.visionModel.crop=crop;
    send(200,"{\"ok\":true,\"vision\":"+visionJson()+"}");
  }
  // Condition 0-3 or 255 (skip); stage 1-6 allowed for this preset or 255; not both skipped.
  bool labelsOk(uint32_t condition,uint32_t stage){
    return !(((condition>=ABVision::Conditions)&&condition!=255)||((stage==0||stage>=ABPlant::StageCount)&&stage!=255)||(stage!=255&&!(ABPlant::Presets[a_.garden.preset].stages&(1<<stage)))||(stage==255&&condition==255));
  }
  bool duplicate(const ABVision::Model &m,const ABVision::Vector &x){for(uint8_t i=0;i<m.count;i++)if(!memcmp(&m.rows[i].x,&x,sizeof(ABVision::Vector)))return true;return false;}
  void teach(){
    if(!allowed()||!revision())return;uint32_t frame,condition,stage;
    if(!a_.gardenSet||!integer("frame",1,0xffffffffUL,frame)||!integer("condition",0,255,condition)||!integer("stage",0,255,stage)||!labelsOk(condition,stage)){error(400,"Choose a valid observation label.");return;}
    if(!a_.gardenSet||!a_.visionHasFrame||frame!=a_.visionFrame||frame==a_.visionLabelledFrame||AutobotanyApp::now()-a_.visionSeenMs>300000){error(409,"This frame changed, expired or was already labelled. Analyze a new snapshot.");return;}
    if(a_.visionObservation.quality){error(409,"Improve the framing and light before teaching this image.");return;}
    if(a_.visionModel.count>=ABVision::Capacity){error(409,"The 64-example model is full. Export it before clearing and retraining.");return;}
    if(duplicate(a_.visionModel,a_.visionObservation.x)){error(409,"This crop matches an existing example. Collect a different real snapshot.");return;}
    ABVision::Model next=a_.visionModel;ABVision::Example &row=next.rows[next.count++];row.x=a_.visionObservation.x;row.condition=condition;row.stage=stage;
    if(!a_.saveVision(next)){error(500,"Example could not be saved.");return;}
    a_.visionLabelledFrame=frame;send(200,"{\"ok\":true,\"vision\":"+visionJson()+"}");
  }
  // Teach without the camera: one labelled photo per request (mode=photo), or many
  // exported feature rows at once (mode=features, "condition,stage,f0..f15;...").
  void importExamples(){
    if(!allowed()||!revision())return;
    if(!a_.gardenSet){error(409,"Choose a plant profile first.");return;}
    uint32_t crop;if(!integer("crop",0,2,crop)){error(400,"Invalid analysis crop.");return;}
    if(a_.visionModel.count&&a_.visionModel.crop!=crop){error(409,"These examples use a different crop. Choose the model's crop or clear the examples first.");return;}
    ABVision::Model next=a_.visionModel;if(!next.count)next.crop=crop;
    String mode=s_.arg("mode");uint16_t added=0,skipped=0;
    if(mode=="photo"){
      uint32_t condition,stage;if(!integer("condition",0,255,condition)||!integer("stage",0,255,stage)||!labelsOk(condition,stage)){error(400,"Choose a valid label for this photo.");return;}
      ABVision::Observation o;String pixels=s_.arg("pixels");
      if(!ABVision::extract(pixels.c_str(),pixels.length(),o)){error(400,"Expected a 24 by 24 RGB565 photo crop.");return;}
      if(o.quality){error(422,(String("Photo skipped: ")+ABVision::qualityName(o.quality)+". Use a clear, well-lit view of the plant.").c_str());return;}
      if(duplicate(next,o.x)){error(409,"This photo matches an existing example.");return;}
      if(next.count>=ABVision::Capacity){error(409,"The 64-example model is full. Export it before clearing and retraining.");return;}
      ABVision::Example &row=next.rows[next.count++];row.x=o.x;row.condition=condition;row.stage=stage;added=1;
    }else if(mode=="features"){
      String preset=s_.arg("preset");
      if(preset.length()&&preset!=ABPlant::Presets[a_.garden.preset].id){error(409,"These examples were taught for a different plant.");return;}
      String rows=s_.arg("rows");size_t pos=0;
      if(!rows.length()||rows.length()>12000){error(400,"No examples were found in that file.");return;}
      while(pos<rows.length()){
        int end=rows.indexOf(';',pos);if(end<0)end=rows.length();
        uint32_t v[2+ABVision::Features];int n=0;bool ok=true;size_t i=pos;
        while(ok&&i<static_cast<size_t>(end)){uint32_t x=0;size_t digits=0;while(i<static_cast<size_t>(end)&&rows[i]>='0'&&rows[i]<='9'&&digits<5){x=x*10+(rows[i]-'0');++i;++digits;}
          if(!digits||n>=2+ABVision::Features){ok=false;break;}v[n++]=x;if(i<static_cast<size_t>(end)){if(rows[i]!=','){ok=false;break;}++i;}}
        pos=end+1;
        if(!ok||n!=2+ABVision::Features||!labelsOk(v[0],v[1])){++skipped;continue;}
        ABVision::Example row;row.condition=v[0];row.stage=v[1];bool range=true;for(int k=0;k<ABVision::Features;k++){if(v[2+k]>1000)range=false;row.x.v[k]=v[2+k];}
        if(!range||duplicate(next,row.x)||next.count>=ABVision::Capacity){++skipped;continue;}
        next.rows[next.count++]=row;++added;
      }
      if(!added){error(409,"No new examples could be imported (duplicates, other plant stages, or the model is full).");return;}
    }else{error(400,"Unknown import mode.");return;}
    if(!a_.saveVision(next)){error(500,"Examples could not be saved. Previous examples are retained.");return;}
    send(200,"{\"ok\":true,\"added\":"+String(added)+",\"skipped\":"+String(skipped)+",\"vision\":"+visionJson()+"}");
  }
  void clear(){if(!allowed()||!revision())return;uint32_t crop;if(!integer("crop",0,2,crop)||s_.arg("confirmed")!="1"){error(400,"Confirm clearing the examples and choose a crop.");return;}ABVision::Model model;model.cycle=a_.garden.cycle;model.crop=crop;if(!a_.saveVision(model)){error(500,"Examples could not be cleared.");return;}a_.visionHasFrame=false;send(200,"{\"ok\":true}");}
  void exportModel(){
    String out="{\"format\":\"autobotany-feature-examples-v1\",\"cycle\":"+String(a_.visionModel.cycle)+",\"crop\":"+String(a_.visionModel.crop)+",\"preset\":"+(a_.gardenSet?q(ABPlant::Presets[a_.garden.preset].id):String("null"))+",\"examples\":[";
    for(uint8_t i=0;i<a_.visionModel.count;i++){const auto &r=a_.visionModel.rows[i];if(i)out+=',';out+="{\"condition\":"+q(ABVision::conditionName(r.condition))+",\"stage\":"+q(ABPlant::stageName(r.stage))+",\"conditionIndex\":"+String(r.condition)+",\"stageIndex\":"+String(r.stage)+",\"features\":[";for(int k=0;k<ABVision::Features;k++){if(k)out+=',';out+=String(r.x.v[k]);}out+="]}";}
    s_.sendHeader("Content-Disposition","attachment; filename=autobotany-vision-examples.json");send(200,out+"]}");
  }
public:
  AutobotanyPlantWeb(AutobotanyApp &a,WebServer &s):a_(a),s_(s){}
  String profileJson(){return a_.gardenSet?details(a_.garden):String("null");}
  String visionJson(){
    const auto &o=a_.visionObservation;bool current=a_.visionHasFrame&&AutobotanyApp::now()-a_.visionSeenMs<=120000;
    ABVision::Result condition,stage;if(current&&!o.quality){condition=ABVision::predict(a_.visionModel,o.x,false);stage=ABVision::predict(a_.visionModel,o.x,true);}
    String out="{\"engine\":\"local-knn-v1\",\"examples\":"+String(a_.visionModel.count)+",\"capacity\":64,\"crop\":"+String(a_.visionModel.crop)+",\"frame\":"+String(a_.visionFrame)+",\"current\":"+yes(current)+",\"labelled\":"+yes(a_.visionFrame==a_.visionLabelledFrame)+",\"ageMs\":"+num(a_.visionHasFrame?double(AutobotanyApp::now()-a_.visionSeenMs):NAN);
    out+=",\"quality\":"+q(a_.visionHasFrame?ABVision::qualityName(o.quality):"no-frame")+",\"green\":"+num(o.green/10.0,1)+",\"yellow\":"+num(o.yellow/10.0,1)+",\"brown\":"+num(o.brown/10.0,1)+",\"coverage\":"+num(o.coverage/10.0,1)+",\"condition\":"+prediction(condition,false)+",\"stage\":"+prediction(stage,true);
    out+=",\"conditionCounts\":[";for(uint8_t i=0;i<ABVision::Conditions;i++){if(i)out+=',';out+=String(ABVision::examplesFor(a_.visionModel,false,i));}out+="],\"stageCounts\":[";for(uint8_t i=0;i<ABPlant::StageCount;i++){if(i)out+=',';out+=String(ABVision::examplesFor(a_.visionModel,true,i));}return out+"]}";
  }
  void begin(){
    s_.on("/api/plants",HTTP_GET,[this](){catalogue();});
    s_.on("/api/plants/preview",HTTP_POST,[this](){preview();});
    s_.on("/api/plants/apply",HTTP_POST,[this](){apply();});
    s_.on("/api/vision/observe",HTTP_POST,[this](){observe();});
    s_.on("/api/vision/teach",HTTP_POST,[this](){teach();});
    s_.on("/api/vision/clear",HTTP_POST,[this](){clear();});
    s_.on("/api/vision/import",HTTP_POST,[this](){importExamples();});
    s_.on("/api/vision/export",HTTP_GET,[this](){exportModel();});
  }
};
