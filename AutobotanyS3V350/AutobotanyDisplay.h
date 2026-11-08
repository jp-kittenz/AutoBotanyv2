#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "AutobotanyApp.h"
#include "AutobotanyTFTAssets.h"
#include "AutobotanyTFTVisuals.h"

// Draws into the shared canvas memory with a different width/height (card 14 pane).
class AutobotanyPaneBuffer:public Adafruit_GFX {
  uint16_t *b_;int16_t w_,h_;
public:
  AutobotanyPaneBuffer(uint16_t *buffer,int16_t w,int16_t h):Adafruit_GFX(w,h),b_(buffer),w_(w),h_(h){}
  void drawPixel(int16_t x,int16_t y,uint16_t c)override{if(x>=0&&y>=0&&x<w_&&y<h_)b_[y*w_+x]=c;}
  void drawFastHLine(int16_t x,int16_t y,int16_t w,uint16_t c)override{if(y<0||y>=h_)return;if(x<0){w+=x;x=0;}if(x+w>w_)w=w_-x;for(int16_t i=0;i<w;++i)b_[y*w_+x+i]=c;}
  void drawFastVLine(int16_t x,int16_t y,int16_t h,uint16_t c)override{if(x<0||x>=w_)return;if(y<0){h+=y;y=0;}if(y+h>h_)h=h_-y;for(int16_t i=0;i<h;++i)b_[(y+i)*w_+x]=c;}
  void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c)override{for(int16_t j=0;j<h;++j)drawFastHLine(x,y+j,w,c);}
};

// Cooperative animation: one bounded SPI transfer per tick, no delays or tasks.
// A 128x48 canvas is shared by six dirty regions and all transition frames.
class AutobotanyDisplay {
  AutobotanyApp &a_;
  Adafruit_ST7735 tft_{&SPI,AB::TFT_CS,AB::TFT_DC,AB::TFT_RST};
  GFXcanvas16 canvas_{128,48};
  AB::DebouncedButton next_,previous_,select_;
  enum class Scene:uint8_t { Waiting,Intro,ToWhite,FromWhite,Menu,Sleep };
  Scene scene_=Scene::Waiting;
  int page_=0,shownMeter_=-1;
  uint64_t nextModel_=0,toastUntil_=0,lastInput_=0,sceneAt_=0,nextFrame_=0,frameAt_=0,releaseAt_=0;
  uint8_t dirty_=63,fade_=0,motion_=0,paintCursor_=0;
  uint64_t pageAt_=0,iconAt_=0,toastAt_=0;
  bool iconDirty_=true,effectActive_=false;
  uint64_t effectAt_=0;
  bool effectBefore_=false;
  int infoTicker_=-1;
  bool ready_=false,haveView_=false,releaseGuard_=true,releaseTiming_=false;
  String toast_;
  static constexpr uint16_t BG=0x0863,PANEL=0x1125,LIME=0x9FB3,TEXT=0xF7BE,DIM=0x9535,LINE=0x2A69,AMBER=0xFE6D,BLUE=0x65FF;
  struct View {
    String value,unit,condition,detail,extra,toast,date;
    uint8_t total=0;
    uint16_t tone=DIM;
    int meter=-1;
    uint8_t links=0;
    bool activity=false;
    ABTFT::Visual icon;
    uint8_t tip=0;
    ABTFT::PlantVisual plant;
    String info; // card 14 right column, one line per '\n', first char = style
  } view_;
  const char *title(int page) {
    static const char *titles[]={"RESERVOIR","SOIL MOISTURE","IS RAINING","TEMPERATURE","HUMIDITY","SUNLIGHT","COOLING FAN","HEAT INDEX","SUN BLOCKER","RAIN BLOCKER","PUMP STATUS","BOARD TEMPERATURE","WEBSITE / SOFTAP","YOUR PLANT"};
    return titles[page];
  }
  bool selectable() {return page_==0 || page_==1 || page_==6 || page_==8 || page_==9 || page_==10;}
  bool apRunning()const{return a_.apReady&&(WiFi.getMode()&WIFI_AP);}
  static constexpr int PlantPage=13,InfoX=55;
  uint32_t enterAge()const {uint64_t now=AutobotanyApp::now();return now>=pageAt_ ? static_cast<uint32_t>(now-pageAt_) : 0;}
  static constexpr uint32_t EnterMs=360;
  uint32_t reactionAge()const {uint64_t now=AutobotanyApp::now();return effectActive_&&now>=effectAt_&&now-effectAt_<ABTFT::ReactionMs ? static_cast<uint32_t>(now-effectAt_) : ABTFT::NoReaction;}
  static String ticker(const String &value,uint64_t now,size_t width){
    if(value.length()<=width)return value;
    size_t start=(now/400)%(value.length()+5);String out;
    for(size_t n=0;n<width;++n){size_t i=(start+n)%(value.length()+5);out+=i<value.length() ? value[i] : ' ';}return out;
  }
  static void label(Adafruit_GFX &g,int x,int y,const String &value,uint16_t color,uint8_t size=1) {
    g.setTextSize(size);g.setTextColor(color);g.setCursor(x,y);g.print(value);
  }
  static void centered(Adafruit_GFX &g,int y,const String &value,uint16_t color,uint8_t maxSize=3) {
    uint8_t size=maxSize;
    while(size>1 && value.length()*6*size>112)--size;
    // Long driver errors/labels remain inside the display, even at size 1.
    String fitted=value;
    if(fitted.length()>20){fitted="";for(size_t i=0;i<17;++i)fitted+=value[i];fitted+="...";}
    int x=(128-static_cast<int>(fitted.length())*6*size)/2;
    label(g,x<4 ? 4 : x,y,fitted,color,size);
  }
  static String runTime(double seconds) {
    uint32_t s=isfinite(seconds) && seconds>0 ? static_cast<uint32_t>(seconds) : 0;
    char buffer[20];snprintf(buffer,sizeof(buffer),"%02lu:%02lu:%02lu",static_cast<unsigned long>(s/3600),static_cast<unsigned long>(s/60%60),static_cast<unsigned long>(s%60));return String(buffer);
  }
  static String age(uint64_t ms) {
    uint64_t s=ms/1000;
    if(s<60)return String(static_cast<unsigned long>(s))+"s";
    if(s<3600)return String(static_cast<unsigned long>(s/60))+"m";
    if(s<86400)return String(static_cast<unsigned long>(s/3600))+"h";
    return String(static_cast<unsigned long>(s/86400))+"d";
  }
  static uint16_t mix(uint16_t from,uint16_t to,uint8_t amount) {
    uint32_t r=(((from>>11)&31)*(255-amount)+((to>>11)&31)*amount+127)/255;
    uint32_t g=(((from>>5)&63)*(255-amount)+((to>>5)&63)*amount+127)/255;
    uint32_t b=((from&31)*(255-amount)+(to&31)*amount+127)/255;
    return static_cast<uint16_t>((r<<11)|(g<<5)|b);
  }
  static uint8_t eased(uint64_t elapsed,uint32_t duration) {
    if(elapsed>=duration)return 255;
    uint32_t t=static_cast<uint32_t>(elapsed*255/duration);
    return static_cast<uint8_t>((t*t*(765-2*t)+32512)/65025);
  }
  void toast(const String &value) {toast_=value;toastAt_=AutobotanyApp::now();uint64_t duration=((value.length()+39)/40)*1800;toastUntil_=toastAt_+(duration>3500 ? duration : 3500);nextModel_=0;}
  void doSelect() {
    String error;
    if(page_==6)effectBefore_=a_.fan.on();
    if(page_==8)effectBefore_=a_.sunBlocker.covering;
    if(page_==9)effectBefore_=a_.rainBlocker.covering;
    if(page_==10)effectBefore_=a_.pump.on();
    if(page_==0)error=a_.probeCommand("water",!a_.water.active);
    else if(page_==1)error=a_.probeCommand("moisture",!a_.moisture.active);
    else if(page_==6)error=a_.fanCommand("manual",!a_.fan.on());
    else if(page_==8)error=a_.blockerCommand("sunBlocker","manual",!a_.sunBlocker.covering);
    else if(page_==9)error=a_.blockerCommand("rainBlocker","manual",!a_.rainBlocker.covering);
    else if(page_==10)error=a_.pumpCommand(!(a_.pump.on() || a_.pumpPending));
    else {if(ABTFT::playfulPage(page_)){effectAt_=AutobotanyApp::now();effectActive_=true;iconDirty_=true;}return;}
    // A successful command also plays the card's transition so the change reads smoothly.
    if(!error.length()){effectAt_=AutobotanyApp::now();effectActive_=true;iconDirty_=true;}
    toast(error.length() ? error : String(page_==0 || page_==1 ? "Sensor command sent" : "Control applied"));
  }
  static ABTFT::Reading reading(const AB::Sample &s,uint64_t now) {
    ABTFT::Reading r;r.value=s.value;r.current=s.current(now);r.calibrated=s.calibrated;return r;
  }
  ABTFT::Visual visual(uint64_t now,bool networkRunning,uint8_t stations) {
    ABTFT::VisualInput i;
    i.water=reading(a_.water,now);i.soil=reading(a_.moisture,now);i.rain=reading(a_.rain,now);
    i.light=reading(a_.sunlight,now);i.temperature=reading(a_.temperature,now);i.humidity=reading(a_.humidity,now);
    i.heat=reading(a_.heat,now);i.chip=reading(a_.boardTemperature,now);i.luxPresent=a_.luxPresent;
    i.clockValid=a_.rtcPresent&&AutobotanyApp::clockValid();
    if(i.clockValid){time_t stamp=AutobotanyApp::epoch();struct tm local;localtime_r(&stamp,&local);i.night=ABTFT::isNightHour(local.tm_hour);}
    i.fanOn=a_.fan.on();i.pumpOn=a_.pump.on();i.sunKnown=a_.sunBlocker.known;i.sunCovered=a_.sunBlocker.covering;
    i.rainKnown=a_.rainBlocker.known;i.rainCovered=a_.rainBlocker.covering;i.rainAutomatic=a_.rainBlocker.automatic;
    i.waterMin=a_.limits.get("waterMin");i.fanHot=a_.limits.get("fanOnC");
    i.apRunning=networkRunning;i.stations=stations;i.cameraOnline=ABTFT::cameraFresh(networkRunning,stations,a_.cameraSeenMs,now);
    ABTFT::Visual v=ABTFT::chooseVisual(page_,i);v.before=effectBefore_;return v;
  }
  static String upper(const char *text){String out;for(const char *c=text;*c;++c)out+=static_cast<char>(*c>='a'&&*c<='z' ? *c-32 : *c=='-' ? ' ' : *c);return out;}
  static String whole(float v){return String(static_cast<long>(lroundf(v)));}
  static String kilo(float v){if(v<1000)return whole(v);float k=v/1000;return fabsf(k-lroundf(k))<.05f ? String(static_cast<long>(lroundf(k)))+"K" : String(k,1)+"K";}
  // Card 14: plant visual and the info column, both derived from the saved profile.
  void plantView(View &out,uint64_t now) {
    ABTFT::PlantVisual &p=out.plant;
    bool night=false;if(a_.rtcPresent&&AutobotanyApp::clockValid()){time_t stamp=AutobotanyApp::epoch();struct tm local;localtime_r(&stamp,&local);night=ABTFT::isNightHour(local.tm_hour);}
    p.night=night;
    if(!a_.gardenSet){out.info="\x01NO PLANT YET\n\n\x02SET IT UP AT\n\x02"+WiFi.softAPIP().toString()+"\n\x02PLANT SETUP\n\n\x03""34 PRESETS\n\x03""FOR SMALL TO\n\x03MEDIUM POTS";return;}
    const ABPlant::Profile &g=a_.garden;const ABPlant::Preset &r=ABPlant::Presets[g.preset];
    double age=a_.plantAge(now);
    p.preset=g.preset;p.stage=g.stage;p.stageMask=r.stages;p.ageDays=isfinite(age) ? static_cast<int16_t>(fmin(age,30000.0)) : -1;p.potL=g.potL;
    p.mood=a_.moisture.current(now)&&a_.moisture.calibrated&&a_.moisture.value<a_.limits.get("moistureMin") ? 1 : 0;
    static const char *media[]={"POTTING MIX","COCO BLEND","SOIL+COMPOST"},*light[]={"MORNING SUN","FULL SUN","BRIGHT SHADE"},*events[]={"ESTABLISHED","JUST SOWN","TRANSPLANTED","CUTTING"};
    const AB::Thresholds &t=a_.limits;bool repot=ABPlant::needsRepot(g);
    String info="\x01"+upper(r.name)+"\n\x02"+upper(ABPlant::stageName(g.stage));
    info+="\n\x04""AGE "+(isfinite(age) ? String(static_cast<long>(age))+" D" : String("UNKNOWN"));
    info+=String("\n")+(repot ? "\x05" : "\x04")+"POT "+String(g.potL,1)+" L"+(repot ? " !" : "");
    info+="\n\x02"+String(media[g.medium<3 ? g.medium : 0])+"\n\x02"+light[g.exposure<3 ? g.exposure : 0]+"\n\x02"+events[g.event<4 ? g.event : 0];
    info+="\n\x04SOIL "+whole(t.values[0])+"-"+whole(t.values[1])+"%\n\x04LUX "+kilo(t.values[3])+"-"+kilo(t.values[4]);
    info+="\n\x04""AIR "+whole(t.values[7])+"-"+whole(t.values[8])+"C\n\x04RH "+whole(t.values[5])+"-"+whole(t.values[6])+"%";
    out.info=info;
  }
  void drawInfo(Adafruit_GFX &g,int shift) {
    // Eleven 10-pixel rows from y=34; a divider separates profile from targets.
    int row=0;size_t start=0;const String &s=view_.info;
    while(start<=s.length()&&row<11){
      int end=s.indexOf('\n',start);if(end<0)end=s.length();
      String line=s.substring(start,end);start=end+1;int y=34+row*10-shift;++row;
      if(!line.length())continue;
      char style=line[0];line=line.substring(1);
      if(style==1){label(g,InfoX,y,ticker(line,AutobotanyApp::now(),12),LIME);continue;}
      if(style==4||style==5){int gap=line.indexOf(' ');label(g,InfoX,y,line.substring(0,gap),DIM);label(g,InfoX+(gap+1)*6,y,line.substring(gap+1),style==5 ? AMBER : TEXT);continue;}
      label(g,InfoX,y,line,style==3 ? DIM : TEXT);
    }
    if(view_.plant.preset!=255)g.drawFastHLine(InfoX,103-shift,70,LINE);
  }
  View readView(uint64_t now) {
    const bool networkRunning=apRunning();const uint8_t stations=networkRunning ? WiFi.softAPgetStationNum() : 0;
    String value="--",unit,condition="NO CURRENT DATA",detail,extra;
    uint16_t tone=DIM;double meter=NAN;
    const AB::Sample *sample=nullptr;
    if(page_==0)sample=&a_.water;
    if(page_==1)sample=&a_.moisture;
    if(page_==2)sample=&a_.rain;
    if(page_==3)sample=&a_.temperature;
    if(page_==4)sample=&a_.humidity;
    if(page_==5)sample=&a_.sunlight;
    if(page_==7)sample=&a_.heat;
    if(page_==11)sample=&a_.boardTemperature;
    if(sample) {
      if(sample->valid) value=page_==2 ? (sample->value>0 ? "YES" : "NO") : String(sample->value,page_==5 ? 0 : 1);
      if(page_==0 || page_==1 || page_==4) unit="%";
      if(page_==3 || page_==7 || page_==11)unit="C";
      if(page_==5)unit="lux";
      if(sample->current(now)) {
        condition="CURRENT";tone=LIME;
        if(page_==0) {condition=sample->value<a_.limits.get("waterMin") ? "REFILL SOON" : "AVAILABLE";meter=sample->value;}
        if(page_==1) {condition=sample->value<a_.limits.get("moistureMin") ? "DRY SOIL" : sample->value>a_.limits.get("moistureMax") ? "VERY WET" : "HEALTHY RANGE";meter=sample->value;}
        if(page_==2)condition=sample->value>0 ? "RAIN DETECTED" : "DRY CONDITIONS";
        if(page_==3)condition=sample->value<a_.limits.get("tempMin") ? "COOL" : sample->value>a_.limits.get("tempMax") ? "WARM" : "MODERATE";
        if(page_==4){condition=sample->value<a_.limits.get("humidityMin") ? "DRY AIR" : sample->value>a_.limits.get("humidityMax") ? "HUMID AIR" : "BALANCED";meter=sample->value;}
        if(page_==5){condition=sample->value<a_.limits.get("sunlightMin") ? "LOW LIGHT" : sample->value>a_.limits.get("sunlightMax") ? "INTENSE LIGHT" : "MODERATE LIGHT";meter=sample->value/a_.limits.get("sunlightMax")*100;}
        if(page_==7)condition=sample->value>a_.limits.get("heatMax") ? "ELEVATED" : "MODERATE";
        if(page_==11)condition=sample->value>=a_.limits.get("fanOnC") ? "COOLING ADVISED" : "MODERATE";
        if(condition=="REFILL SOON" || condition=="DRY SOIL" || condition=="VERY WET" || condition=="WARM" || condition=="ELEVATED" || condition=="INTENSE LIGHT")tone=AMBER;
        if(!sample->calibrated){condition="CALIBRATE SENSOR";tone=AMBER;}
      } else if(sample->valid){condition="OUTDATED VALUES";tone=AMBER;}
      if(sample->active && sample->scheduled)condition="TAKING A READING";
      if(sample->valid && now>=sample->acquiredMs)detail="READ "+age(now-sample->acquiredMs)+" AGO";
      else detail="WAITING FOR SAMPLE";
      if(sample->scheduled){extra="RAW "+String(sample->raw)+"  PWR "+String(sample->active ? "ON" : "OFF");}
      else extra="AMBIENT SENSOR";
      if(page_==5)extra="BH1750 / LUX";
      if(page_==11)extra="ESP32-S3 CHIP";
    }
    if(page_==6){bool on=a_.fan.on();value=on ? "ON" : "OFF";condition=a_.fanAutomatic ? "AUTOMATIC" : "MANUAL / 2 MIN";tone=on ? LIME : DIM;unit="4010 / 5V";detail="RUN "+runTime(a_.fan.runSeconds(now));extra=a_.fanAutomatic ? "CHIP TEMP CONTROL" : "LEFT "+runTime(a_.fan.remainingMs(now)/1000.0);}
    if(page_==8 || page_==9){AutobotanyBlocker &b=page_==8 ? a_.sunBlocker : a_.rainBlocker;value=b.known ? (b.covering ? "COVERED" : "OPEN") : "UNKNOWN";condition=b.known ? (b.covering ? "COVERING" : "NOT COVERING") : "DRIVER UNAVAILABLE";tone=b.covering ? BLUE : LIME;unit=b.moving ? "MOVING" : "MG90S";detail=b.automatic ? "AUTOMATIC MODE" : "MANUAL MODE";extra="COMMANDED POSITION";}
    if(page_==10){value=a_.pumpPending ? "WAIT" : a_.pump.on() ? "ON" : "OFF";condition=a_.water.calibrated ? (a_.gardenSet?"TIMED / 5 SEC":"TIMED / 2 MIN") : "CALIBRATE WATER";tone=a_.pump.on() ? BLUE : DIM;detail="RUN "+runTime(a_.pump.runSeconds(now));extra=a_.pump.on() ? "LEFT "+runTime(a_.pump.remainingMs(now)/1000.0) : a_.pumpPending ? "CHECKING RESERVOIR" : "DRY RUN PROTECTION";}
    if(a_.calibration.active && ((page_==0 && a_.calibration.target==1) || (page_==1 && a_.calibration.target==0) || (page_==2 && a_.calibration.target==2))) {
      value="CAL";unit="REFERENCE "+String(a_.calibration.point+1);tone=AMBER;
      condition=a_.calibration.running ? "SAMPLING" : "NEXT REFERENCE";
      detail="USE WEB DASHBOARD";extra=a_.calibration.running ? "PROBE POWERED" : "PROBE OFF";meter=NAN;
    }
    if(page_==12){bool cam=ABTFT::cameraFresh(networkRunning,stations,a_.cameraSeenMs,now);value=String(ABTFT::userDevices(networkRunning,stations,cam));unit=networkRunning ? WiFi.softAPIP().toString() : String("--");condition=networkRunning ? "ONLINE" : "OFFLINE";detail=AB::AP_NAME;extra=AB::AP_PASSWORD;tone=networkRunning ? LIME : AMBER;}
    View out;if(page_==PlantPage){plantView(out,now);value="";condition="";}
    out.total=stations;out.date=a_.rtcPresent&&AutobotanyApp::clockValid() ? AutobotanyApp::localDate(AutobotanyApp::epoch()) : String("RTC NOT SET");out.icon=visual(now,networkRunning,stations);out.tip=ABTFT::tipIndex(page_,now-pageAt_);out.links=(networkRunning ? 1 : 0) | (a_.sdReady ? 2 : 0) | (a_.rtcPresent && AutobotanyApp::clockValid() ? 4 : 0);
    out.activity=(sample && sample->active && sample->scheduled) || (page_==6 && a_.fan.on()) || (page_==10 && (a_.pump.on() || a_.pumpPending)) || ((page_==8 || page_==9) && (page_==8 ? a_.sunBlocker.moving : a_.rainBlocker.moving));
    if(sample && !sample->current(now))meter=NAN;
    if(!sample && page_==10 && a_.pump.on())meter=100.0*a_.pump.remainingMs(now)/AB::MANUAL_RUN_MS;
    if(!sample && page_==6 && a_.fan.on() && !a_.fanAutomatic)meter=100.0*a_.fan.remainingMs(now)/AB::MANUAL_RUN_MS;
    out.value=value;out.unit=unit;out.condition=condition;out.detail=detail;out.extra=extra;out.tone=tone;
    out.meter=isfinite(meter) ? constrain(static_cast<int>(meter*1.1),0,110) : -1;
    if(now<toastUntil_)out.toast=toast_;
    return out;
  }
  void updateView(uint64_t now) {
    View next=readView(now);
    if(!haveView_){dirty_=63;shownMeter_=next.meter;}
    else {
      if(next.links!=view_.links)dirty_|=1;
      if(page_==12){if(next.value!=view_.value||next.unit!=view_.unit)dirty_|=4;if(next.total!=view_.total||next.icon.camera!=view_.icon.camera||next.date!=view_.date)dirty_|=16;}
      if(next.icon!=view_.icon)iconDirty_=true;
      if(page_==PlantPage){if(next.info!=view_.info)dirty_|=30;if(next.plant!=view_.plant)iconDirty_=true;}
      if(next.tip!=view_.tip)dirty_|=16;
      if(next.value!=view_.value || next.unit!=view_.unit)dirty_|=2;
      if(next.condition!=view_.condition || next.tone!=view_.tone || next.activity!=view_.activity)dirty_|=4;
      if(next.meter!=view_.meter && (next.meter<0 || shownMeter_<0)){shownMeter_=next.meter;dirty_|=4;}
      if(next.detail!=view_.detail || next.extra!=view_.extra)dirty_|=8;
      if(next.toast!=view_.toast)dirty_|=8;
    }
    view_=next;haveView_=true;nextModel_=now+AB::TFT_POLL_MS;
  }
  void drawToast(Adafruit_GFX &g,int shift) {
    // A temporary message uses the details band only; the value and tip stay visible.
    size_t page=((AutobotanyApp::now()-toastAt_)/1800)%((view_.toast.length()+39)/40);
    size_t pos=page*40;
    g.fillRect(0,98-shift,128,22,PANEL);
    for(int row=0;row<2;++row){String text;for(int j=0;j<20&&pos<view_.toast.length();++j){char c=view_.toast[pos++];text+=c=='\n'||c=='\r'||c=='\t' ? ' ' : c;}centered(g,100+row*10-shift,text,AMBER,1);}
  }
  void cardText(Adafruit_GFX &g,int y,const String &text,uint16_t color,uint8_t maxSize,int shift) {
    uint8_t size=maxSize;while(size>1 && text.length()*6*size>70)--size;
    // All ordinary values fit. A long unit uses the full card width on its own line.
    int x=52+(72-static_cast<int>(text.length())*6*size)/2;
    if(x<52)x=52;
    label(g,x,y-shift,text,color,size);
  }
  void drawRegion(Adafruit_GFX &g,uint8_t region,int shift) {
    if(region==0) {
      g.fillRect(0,-shift,128,12,PANEL);
      label(g,4,2-shift,"AUTOBOTANY",TEXT);label(g,94,2-shift,String(page_+1)+"/"+String(AB::TFT_PAGE_COUNT),DIM);
      const char *names[]={"AP","SD","RTC"};
      for(int i=0;i<3;++i){int x=12+39*i;g.fillRect(x,15-shift,3,3,(view_.links&(1<<i)) ? LIME : AMBER);label(g,x+6,13-shift,names[i],DIM);}
      centered(g,24-shift,title(page_),LIME,1);
    } else if(page_==PlantPage && region>=1 && region<=4) {
      ABTFT::drawPlantPane(g,0,32-shift,view_.plant,iconAt_,PANEL,BG,reactionAge(),enterAge());
      drawInfo(g,shift);
    } else if(region==1) {
      g.fillRoundRect(3,32-shift,122,44,5,PANEL);
      // While a card is entering, the art box is left to region 6, which fades it in.
      bool art=!(scene_==Scene::Menu&&enterAge()<EnterMs&&canvas_.getBuffer());
      if(page_==12){if(art){ABTFT::drawVisual(g,16,34-shift,view_.icon,iconAt_,PANEL,reactionAge());ABTFT::drawClients(g,72,34-shift,view_.icon,iconAt_,PANEL,reactionAge());}}
      else{if(art)ABTFT::drawVisual(g,5,34-shift,view_.icon,iconAt_,PANEL,reactionAge());cardText(g,40,view_.value,TEXT,2,shift);cardText(g,62,view_.unit,DIM,1,shift);}
    } else if(region==2) {
      if(page_==12){label(g,5,78-shift,view_.condition+"  USERS "+view_.value,view_.tone);label(g,5,89-shift,"IP "+view_.unit,TEXT);return;}
      g.fillRoundRect(9,78-shift,110,3,1,LINE);
      if(shownMeter_>=0 && shownMeter_)g.fillRoundRect(9,78-shift,shownMeter_,3,1,view_.tone);
      if(view_.activity){int x=9+static_cast<int>(motion_)*102/31;g.fillRect(x,78-shift,8,3,TEXT);}
      centered(g,87-shift,view_.condition,view_.tone,1);
    } else if(region==3) {
      if(page_==12){label(g,5,100-shift,"WI-FI NAME",DIM);label(g,5,110-shift,ticker(view_.detail,iconAt_,20),TEXT);return;}
      centered(g,100-shift,view_.detail,DIM,1);centered(g,111-shift,view_.extra,DIM,1);
      if(view_.toast.length())drawToast(g,shift);
    } else if(region==4) {
      if(page_==12){label(g,5,121-shift,"PASS",DIM);label(g,35,121-shift,ticker(view_.extra,iconAt_,15),TEXT);label(g,5,131-shift,String(view_.icon.camera ? "CAM ON" : "CAM OFF")+" / ALL "+String(view_.total),view_.icon.camera ? BLUE : DIM);label(g,5,141-shift,view_.date,DIM);return;}
      const ABTFT::Tip &tip=ABTFT::Tips[view_.tip];
      g.fillRoundRect(3,120-shift,122,28,4,PANEL);
      label(g,7,122-shift,"TIP "+String(view_.tip%3+1)+"/3",LIME);
      // Page dots share the tip header; neither touches the two text lines.
      for(int i=0;i<AB::TFT_PAGE_COUNT;++i)g.fillRect(70+i*4,125-shift,2,2,i==page_ ? LIME : LINE);
      label(g,7,131-shift,tip.line1,TEXT);label(g,7,139-shift,tip.line2,TEXT);
    } else centered(g,152-shift,selectable() ? "< PREV  SEL  NEXT >" : "< PREV PLAY NEXT >",DIM,1);
  }
  void drawIntro(Adafruit_GFX &g,int shift) {
    // Freeze at the completed reveal while fading through white.
    ABTFT::drawIntroFrame(g,shift,scene_==Scene::ToWhite ? AB::TFT_INTRO_MS : frameAt_-sceneAt_);
  }
  void drawSleep(Adafruit_GFX &g,int shift) {
    // Everything outside these three softly rising letters stays RGB565 black.
    for(int i=0;i<3;++i){
      uint32_t phase=static_cast<uint32_t>((frameAt_-sceneAt_+i*700)%2400);
      uint8_t light=phase<1200 ? phase*150/1200 : (2400-phase)*150/1200;
      label(g,41+i*16,(i==2 ? 58 : 64)-static_cast<int>(phase*20/2400)-shift,"Z",mix(0,DIM,light),i==2 ? 2 : 1);
    }
  }
  void enter(Scene next,uint64_t now) {
    scene_=next;sceneAt_=now;nextFrame_=now;frameAt_=now;dirty_=63;fade_=0;
    if(next==Scene::Menu){lastInput_=now;haveView_=false;nextModel_=0;iconDirty_=true;iconAt_=now;}
    if(next==Scene::Sleep){effectActive_=false;toastUntil_=0;toast_="";}
  }
  void guardRelease() {releaseGuard_=true;releaseTiming_=false;}
  void buttons(uint64_t now) {
    bool rn=digitalRead(AB::NEXT_PIN)==LOW,rp=digitalRead(AB::PREVIOUS_PIN)==LOW,rs=digitalRead(AB::SELECT_PIN)==LOW;
    bool n=next_.update(rn,now),p=previous_.update(rp,now),s=select_.update(rs,now);
    bool any=rn || rp || rs;
    if(releaseGuard_) {
      // Consume the complete gesture, including other held buttons and bounce.
      if(any){releaseTiming_=false;if(scene_==Scene::Menu)lastInput_=now;}
      else if(!releaseTiming_){releaseTiming_=true;releaseAt_=now;}
      else if(now-releaseAt_>=35 && !next_.stable && !previous_.stable && !select_.stable)releaseGuard_=false;
      return;
    }
    if(scene_==Scene::Sleep) {
      if(n || p || s){enter(Scene::Menu,now);guardRelease();}
      return;
    }
    if(scene_!=Scene::Menu)return;
    if(any || n || p || s)lastInput_=now;
    if(n || p){page_=AB::wrapPage(page_+(n ? 1 : -1));pageAt_=now;effectActive_=false;iconDirty_=true;haveView_=false;dirty_=63;nextModel_=0;toastUntil_=0;}
    if(s&&!n&&!p)doSelect();
  }
  void animate(uint64_t now) {
    // Frame parameters only change after all bands of a transition are sent.
    if(scene_==Scene::Intro && !dirty_ && now-sceneAt_>=AB::TFT_INTRO_MS){enter(Scene::ToWhite,now);}
    else if(scene_==Scene::ToWhite && !dirty_ && fade_==255){enter(Scene::FromWhite,now);updateView(now);fade_=255;guardRelease();}
    else if(scene_==Scene::FromWhite && !dirty_ && fade_==0){enter(Scene::Menu,now);guardRelease();}
    if(scene_==Scene::Menu) {
      if(now-lastInput_>=AB::TFT_SLEEP_MS){enter(Scene::Sleep,now);}
      else {
        if(now>=nextModel_)updateView(now);
        if(now>=nextFrame_){
          nextFrame_=now+AB::TFT_FRAME_MS;
          iconAt_=now;if(page_==PlantPage||ABTFT::animated(view_.icon)||effectActive_||enterAge()<EnterMs+80)iconDirty_=true;
          if(page_==PlantPage){int tick=static_cast<int>(now/400);if(tick!=infoTicker_){infoTicker_=tick;if(view_.info.indexOf('\n')>13)dirty_|=2;}}
          if(effectActive_&&now-effectAt_>=ABTFT::ReactionMs)effectActive_=false;
          if(page_==12){if(view_.detail.length()>20)dirty_|=8;if(view_.extra.length()>15)dirty_|=16;}
          if(view_.toast.length()>40)dirty_|=8;
          if(view_.meter>=0 && shownMeter_>=0 && shownMeter_!=view_.meter){
            int delta=view_.meter-shownMeter_;int step=delta/4;
            if(!step)step=delta>0 ? 1 : -1;
            shownMeter_+=step;dirty_|=4;
          }
          uint8_t phase=(now/AB::TFT_FRAME_MS)%32;
          if(view_.activity && motion_!=phase){motion_=phase;dirty_|=4;}
        }
        return;
      }
    }
    if(!dirty_ && now>=nextFrame_) {
      frameAt_=now;nextFrame_=now+AB::TFT_FRAME_MS;
      if(scene_==Scene::Intro)dirty_=63; // One coherent time sample across the complete animated reveal.
      if(scene_==Scene::Sleep)dirty_=2; // ZZZ fits in the 32..75 band.
      if(scene_==Scene::ToWhite || scene_==Scene::FromWhite)dirty_=63;
    }
    if(dirty_==63 && (scene_==Scene::ToWhite || scene_==Scene::FromWhite)) {
      frameAt_=now;nextFrame_=now+AB::TFT_FRAME_MS;
      uint8_t amount=eased(now-sceneAt_,AB::TFT_FADE_MS);
      fade_=scene_==Scene::ToWhite ? amount : 255-amount;
    }
  }
  void paintOneRegion() {
    if(!dirty_ && !(scene_==Scene::Menu&&iconDirty_))return;
    static const uint8_t top[]={0,32,76,98,120,148},height[]={32,44,22,22,28,12};
    uint8_t region=0;
    if(scene_==Scene::Menu){
      // Round-robin prevents fast-changing artwork/meters from starving details/tips.
      for(int n=0;n<7;++n){region=(paintCursor_+n)%7;if(region==6 ? iconDirty_ : (dirty_&(1U<<region)))break;}
      paintCursor_=(region+1)%7;
    }else while(!(dirty_&(1U<<region)))++region;
    if(region==6){
      if(page_==PlantPage){
        // 52x116 pane = 6,032 pixels, inside the 6,144-pixel shared canvas.
        if(canvas_.getBuffer()){AutobotanyPaneBuffer pane(canvas_.getBuffer(),ABTFT::PlantPaneW,ABTFT::PlantPaneH);ABTFT::drawPlantPane(pane,0,0,view_.plant,iconAt_,PANEL,BG,reactionAge(),enterAge());tft_.drawRGBBitmap(0,32,canvas_.getBuffer(),ABTFT::PlantPaneW,ABTFT::PlantPaneH);}
        else ABTFT::drawPlantPane(tft_,0,32,view_.plant,iconAt_,PANEL,BG,reactionAge(),enterAge());
        iconDirty_=false;return;
      }
      const int width=page_==12 ? 96 : 40,x=page_==12 ? 16 : 5;
      if(canvas_.getBuffer()){
        // Card entrance: the art rises 6 px and fades in from the panel colour.
        uint32_t enter=enterAge();uint8_t shown=enter<EnterMs ? eased(enter,EnterMs) : 255;int rise=(255-shown)*6/255;
        canvas_.fillScreen(PANEL);ABTFT::drawVisual(canvas_,0,rise,view_.icon,iconAt_,PANEL,reactionAge());
        if(page_==12)ABTFT::drawClients(canvas_,56,rise,view_.icon,iconAt_,PANEL,reactionAge());
        uint16_t *pixels=canvas_.getBuffer();
        for(int row=1;row<40;++row)memmove(pixels+row*width,pixels+row*128,width*sizeof(uint16_t));
        if(shown<255)for(int i=0;i<width*40;++i)pixels[i]=mix(PANEL,pixels[i],shown);
        tft_.drawRGBBitmap(x,34,pixels,width,40);
      }else{tft_.fillRect(x,34,width,40,PANEL);ABTFT::drawVisual(tft_,x,34,view_.icon,iconAt_,PANEL,reactionAge());if(page_==12)ABTFT::drawClients(tft_,72,34,view_.icon,iconAt_,PANEL,reactionAge());}
      iconDirty_=false;return;
    }
    if(canvas_.getBuffer()) {
      canvas_.fillScreen(scene_==Scene::Sleep ? 0 : BG);
      if(scene_==Scene::Intro || scene_==Scene::ToWhite)drawIntro(canvas_,top[region]);
      else if(scene_==Scene::Sleep)drawSleep(canvas_,top[region]);
      else drawRegion(canvas_,region,top[region]);
      if((scene_==Scene::ToWhite || scene_==Scene::FromWhite) && fade_){
        uint16_t *pixels=canvas_.getBuffer();
        for(unsigned i=0;i<128U*height[region];++i)pixels[i]=mix(pixels[i],0xFFFF,fade_);
      }
      if(scene_==Scene::Menu && page_==PlantPage && region>=1 && region<=4){
        // The pane columns belong to region 6; bands only send the info column.
        const int left=ABTFT::PlantPaneW,width=128-left;uint16_t *pixels=canvas_.getBuffer();
        for(int row=0;row<height[region];++row)memmove(pixels+row*width,pixels+row*128+left,width*sizeof(uint16_t));
        tft_.drawRGBBitmap(left,top[region],pixels,width,height[region]);
      }else tft_.drawRGBBitmap(0,top[region],canvas_.getBuffer(),128,height[region]);
    } else {
      // Memory-pressure fallback: readable static screens; skip pixel blending.
      tft_.fillRect(0,top[region],128,height[region],scene_==Scene::Sleep ? 0 : BG);
      if(scene_==Scene::Intro || scene_==Scene::ToWhite)drawIntro(tft_,0);
      else if(scene_==Scene::Sleep)drawSleep(tft_,0);
      else drawRegion(tft_,region,0);
    }
    dirty_&=static_cast<uint8_t>(~(1U<<region));
    if(region==1 && !(page_==PlantPage && canvas_.getBuffer() && scene_==Scene::Menu))iconDirty_=false;
  }
public:
  explicit AutobotanyDisplay(AutobotanyApp &app):a_(app){}
  void begin() {
    pinMode(AB::SELECT_PIN,INPUT_PULLUP);pinMode(AB::NEXT_PIN,INPUT_PULLUP);pinMode(AB::PREVIOUS_PIN,INPUT_PULLUP);
    next_=AB::DebouncedButton{};previous_=AB::DebouncedButton{};select_=AB::DebouncedButton{};
    tft_.initR(AB::TFT_TAB);tft_.setSPISpeed(AB::TFT_SPI_HZ);tft_.setRotation(AB::TFT_ROTATION);tft_.invertDisplay(AB::TFT_INVERT);tft_.setTextWrap(false);canvas_.setTextWrap(false);tft_.fillScreen(BG);
    centered(tft_,56,"AUTOBOTANY",LIME,2);centered(tft_,82,"GROWING SYSTEM",DIM,1);
    ready_=true;haveView_=false;dirty_=63;nextModel_=0;scene_=Scene::Waiting;pageAt_=AutobotanyApp::now();guardRelease();
  }
  void startupStage(const char *stage){tft_.fillScreen(BG);centered(tft_,22,"AUTOBOTANY",LIME,2);centered(tft_,49,"STARTUP / 3.6.0",DIM,1);centered(tft_,77,stage,TEXT,1);centered(tft_,110,"192.168.4.1/startup",DIM,1);haveView_=false;dirty_=63;nextModel_=0;}
  void tick() {
    if(!ready_)return;
    uint64_t now=AutobotanyApp::now();
    if(scene_==Scene::Waiting)enter(Scene::Intro,now);
    buttons(now);animate(now);paintOneRegion();
  }
};
