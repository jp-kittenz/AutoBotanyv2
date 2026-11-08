#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include <esp_arduino_version.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <atomic>
#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Select AI Thinker ESP32-CAM for the OV2640 camera module"
#endif

const char *AP_NAME="Autobotany Group 1";
const char *AP_PASSWORD="CSFJ-GRADE11";
const char *REGISTER_URL="http://192.168.4.1/api/camera";
const char *BOUNDARY="autobotanyframe";
const char *HELP_AP_NAME="Autobotany Camera Help";
const uint64_t WIFI_ATTEMPT_MS=30000,HELP_AFTER_MS=45000;
struct CameraProfile {framesize_t size;int width,height,quality,maxFps;};
const CameraProfile profiles[]={{FRAMESIZE_QVGA,320,240,20,20},{FRAMESIZE_VGA,640,480,16,12},{FRAMESIZE_SVGA,800,600,12,7}};
httpd_handle_t captureServer=nullptr,streamServer=nullptr;
SemaphoreHandle_t cameraMutex=nullptr;
Preferences cameraPreferences;
std::atomic<bool> cameraReady{false},heartbeatRequested{false};
std::atomic<int> activeProfile{0};
std::atomic<uint32_t> frameBytes{0},fpsTenths{0},lastFrameMs{0};
std::atomic<esp_err_t> cameraError{ESP_OK};
std::atomic<bool> serversReady{false},helpApReady{false},retryRequested{false},restorePending{false},registrationSeen{false};
std::atomic<int> savedProfile{0},bufferProfile{0},lastWifiReason{0},heartbeatCode{0};
std::atomic<uint32_t> wifiAttempts{0},wifiDisconnects{0},registeredMs{0};
bool preferencesReady=false;
int bootResetReason=0;
uint64_t nextReconnect=0,nextHeartbeat=0,nextCameraRetry=0,nextServerRetry=0,nextHelpRetry=0,wifiSince=0,failedSince=0;

uint64_t cameraNow() {return static_cast<uint64_t>(esp_timer_get_time()/1000);}
float measuredFps() {return static_cast<uint32_t>(cameraNow())-lastFrameMs.load()<3500 ? fpsTenths.load()/10.0f : 0.0f;}

void wifiEvent(WiFiEvent_t event,WiFiEventInfo_t info) {
  if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED){lastWifiReason.store(info.wifi_sta_disconnected.reason);wifiDisconnects.fetch_add(1);}
}

void connectToMain() {
  nextReconnect=cameraNow()+WIFI_ATTEMPT_MS;
  wifiAttempts.fetch_add(1);
  WiFi.begin(AP_NAME,AP_PASSWORD);
}

bool configureSensor(int profile) {
  sensor_t *sensor=esp_camera_sensor_get();
  if(!sensor)return false;
  return sensor->set_framesize(sensor,profiles[profile].size)==0 && sensor->set_quality(sensor,profiles[profile].quality)==0;
}

bool initializeCameraProfile(int profile) {
  if(profile<0 || profile>2)return false;
  if(!psramFound())profile=0;
  camera_config_t config={};
  config.ledc_channel=LEDC_CHANNEL_0;config.ledc_timer=LEDC_TIMER_0;
  config.pin_d0=5;config.pin_d1=18;config.pin_d2=19;config.pin_d3=21;
  config.pin_d4=36;config.pin_d5=39;config.pin_d6=34;config.pin_d7=35;
  config.pin_xclk=0;config.pin_pclk=22;config.pin_vsync=25;config.pin_href=23;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  config.pin_sccb_sda=26;config.pin_sccb_scl=27;
#else
  config.pin_sscb_sda=26;config.pin_sscb_scl=27;
#endif
  config.pin_pwdn=32;config.pin_reset=-1;config.xclk_freq_hz=20000000;
  config.pixel_format=PIXFORMAT_JPEG;
  config.frame_size=profiles[profile].size;
  config.jpeg_quality=profiles[profile].quality;config.fb_count=psramFound() ? 2 : 1;
  config.fb_location=psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
  config.grab_mode=CAMERA_GRAB_LATEST;
  esp_err_t result=esp_camera_init(&config);cameraError.store(result);
  if(result!=ESP_OK){Serial.printf("Camera initialization failed: 0x%x\n",static_cast<unsigned>(result));esp_camera_deinit();return false;}
  if(!configureSensor(profile)){cameraError.store(ESP_FAIL);esp_camera_deinit();return false;}
  activeProfile.store(profile);bufferProfile.store(profile);
  Serial.printf("Camera ready: %dx%d, JPEG quality %d\n",profiles[profile].width,profiles[profile].height,profiles[profile].quality);
  return true;
}

bool initializeCamera() {return initializeCameraProfile(0);}

bool applyProfile(int next) {
  int previous=activeProfile.load();bool ready=cameraReady.load(),ok=false;
  if(ready && next<=bufferProfile.load()){
    ok=configureSensor(next);
    if(ok)activeProfile.store(next);
    else if(!configureSensor(previous)){cameraReady.store(false);esp_camera_deinit();}
  }else{
    cameraReady.store(false);
    if(ready)esp_camera_deinit();
    ok=initializeCameraProfile(next);
    bool restored=ok;
    if(!restored)restored=initializeCameraProfile(previous);
    if(!restored && previous!=0)restored=initializeCameraProfile(0);
    cameraReady.store(restored);
  }
  fpsTenths.store(0);frameBytes.store(0);lastFrameMs.store(0);
  return ok;
}

esp_err_t unavailable(httpd_req_t *req,const char *message) {
  httpd_resp_set_status(req,"503 Service Unavailable");
  httpd_resp_set_type(req,"application/json");
  return httpd_resp_sendstr(req,message);
}

esp_err_t captureHandler(httpd_req_t *req) {
  httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
  if(!cameraReady.load())return unavailable(req,"{\"ok\":false,\"error\":\"Camera is initializing\"}");
  if(xSemaphoreTake(cameraMutex,pdMS_TO_TICKS(3000))!=pdTRUE)return unavailable(req,"{\"ok\":false,\"error\":\"Camera is busy\"}");
  if(!cameraReady.load()){xSemaphoreGive(cameraMutex);return unavailable(req,"{\"ok\":false,\"error\":\"Camera is initializing\"}");}
  camera_fb_t *frame=esp_camera_fb_get();
  if(!frame){xSemaphoreGive(cameraMutex);return unavailable(req,"{\"ok\":false,\"error\":\"Frame unavailable\"}");}
  httpd_resp_set_type(req,"image/jpeg");httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"Content-Disposition","inline; filename=autobotany.jpg");
  frameBytes.store(frame->len);
  esp_err_t result=httpd_resp_send(req,reinterpret_cast<const char *>(frame->buf),frame->len);
  esp_camera_fb_return(frame);xSemaphoreGive(cameraMutex);return result;
}

esp_err_t healthHandler(httpd_req_t *req) {
  int profile=activeProfile.load(),status=WiFi.status();char body[900];
  snprintf(body,sizeof(body),"{\"camera\":\"AutobotanyCamera\",\"version\":\"3.0.2\",\"ready\":%s,\"rssi\":%d,\"fps\":%.1f,\"profile\":%d,\"width\":%d,\"height\":%d,\"frameBytes\":%u,\"psram\":%s,\"freeHeap\":%u,\"error\":%d,\"wifiConnected\":%s,\"wifiStatus\":%d,\"ip\":\"%s\",\"wifiAttempts\":%u,\"disconnects\":%u,\"lastWifiReason\":%d,\"registrationHTTP\":%d,\"registrationAgeMs\":%ld,\"helpAP\":%s,\"serversReady\":%s,\"savedProfile\":%d,\"restorePending\":%s,\"uptimeSeconds\":%lu,\"resetReason\":%d,\"brownoutReset\":%s}",cameraReady.load() ? "true" : "false",status==WL_CONNECTED ? WiFi.RSSI() : 0,measuredFps(),profile,profiles[profile].width,profiles[profile].height,frameBytes.load(),psramFound() ? "true" : "false",ESP.getFreeHeap(),static_cast<int>(cameraError.load()),status==WL_CONNECTED ? "true" : "false",status,WiFi.localIP().toString().c_str(),wifiAttempts.load(),wifiDisconnects.load(),lastWifiReason.load(),heartbeatCode.load(),registrationSeen.load() ? static_cast<long>(static_cast<uint32_t>(cameraNow())-registeredMs.load()) : -1L,helpApReady.load() ? "true" : "false",serversReady.load() ? "true" : "false",savedProfile.load(),restorePending.load() ? "true" : "false",static_cast<unsigned long>(cameraNow()/1000),bootResetReason,bootResetReason==ESP_RST_BROWNOUT ? "true" : "false");
  httpd_resp_set_type(req,"application/json");httpd_resp_set_hdr(req,"Cache-Control","no-store");httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
  return httpd_resp_sendstr(req,body);
}

const char DIAGNOSTIC_HTML[] PROGMEM=R"ABHTML(<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Autobotany Camera Help</title><style>body{font:16px system-ui,sans-serif;background:#112118;color:#e8f0e6;max-width:680px;margin:auto;padding:24px;line-height:1.5}h1{font-size:25px;color:#c9ed98}section{background:#1a2e21;border:1px solid #3c5140;border-radius:12px;padding:18px;margin:16px 0}button,a{font:inherit;color:#132112;background:#c9ed98;border:0;border-radius:8px;padding:12px;margin:5px 8px 5px 0;display:inline-block}pre{font-size:13px;white-space:pre-wrap;overflow-wrap:anywhere}small{color:#b1c3b0}#note{min-height:24px}</style></head><body><h1>Autobotany Camera Help</h1><p id="summary">Reading camera status...</p><section><p id="wifi"></p><p id="main"></p><p id="sensor"></p><p id="reset"></p><button id="retry">Retry connection</button><button id="fast">Use Fast mode</button><a href="/capture" target="_blank" rel="noopener">Open photo</a><p id="note" role="status"></p></section><small>The camera keeps trying Autobotany Group 1. After registration succeeds, reconnect your phone to that network and open http://192.168.4.1.</small><details><summary>Diagnostic details</summary><pre id="details"></pre></details><script>
var busy=false;function put(id,text){document.getElementById(id).textContent=text;}async function refresh(){try{var response=await fetch('/health',{cache:'no-store'});if(!response.ok)throw Error();var s=await response.json();put('summary','Camera firmware '+s.version+' | Uptime '+s.uptimeSeconds+' seconds');put('wifi',s.wifiConnected?'WiFi connected: '+s.ip+' | '+s.rssi+' dBm':'WiFi waiting for Autobotany Group 1 | Last reason '+s.lastWifiReason);put('main',s.registrationHTTP===200?'S3 registration acknowledged | '+Math.floor(s.registrationAgeMs/1000)+' seconds ago':'S3 registration: '+(s.registrationHTTP===0?'not attempted yet':'HTTP/result '+s.registrationHTTP));put('sensor',s.ready?'Camera ready: '+s.width+' x '+s.height:s.wifiConnected?'Camera starting or retrying | Error '+s.error:'Camera waits for the main network before starting');put('reset',s.brownoutReset?'Last restart was a brownout. Supply voltage dropped.':'Reset reason: '+s.resetReason+' | PSRAM '+(s.psram?'detected':'not detected'));put('details',JSON.stringify(s,null,2));}catch(error){put('summary','Camera page unreachable. Check whether your phone stayed connected to its WiFi.');}}async function request(url,body){if(busy)return;busy=true;put('note','Working...');try{var r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body});var s=await r.json();put('note',s.ok?'Request applied.':s.error||'Request failed.');}catch(error){put('note','Connection interrupted. Refresh after WiFi reconnects.');}finally{busy=false;refresh();}}document.getElementById('retry').onclick=function(){request('/retry','retry=1');};document.getElementById('fast').onclick=function(){request('/settings','profile=0');};refresh();setInterval(refresh,3000);
</script></body></html>)ABHTML";

esp_err_t diagnosticHandler(httpd_req_t *req) {
  httpd_resp_set_type(req,"text/html; charset=utf-8");httpd_resp_set_hdr(req,"Cache-Control","no-store");
  return httpd_resp_sendstr(req,DIAGNOSTIC_HTML);
}

esp_err_t retryHandler(httpd_req_t *req) {
  retryRequested.store(true);httpd_resp_set_type(req,"application/json");
  return httpd_resp_sendstr(req,"{\"ok\":true}");
}

esp_err_t settingsHandler(httpd_req_t *req) {
  httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");httpd_resp_set_hdr(req,"Cache-Control","no-store");httpd_resp_set_type(req,"application/json");
  if(req->content_len<1 || req->content_len>80){httpd_resp_set_status(req,"400 Bad Request");return httpd_resp_sendstr(req,"{\"ok\":false,\"error\":\"Invalid settings\"}");}
  char body[81]={};int total=0;
  while(total<req->content_len){int got=httpd_req_recv(req,body+total,req->content_len-total);if(got<=0)return ESP_FAIL;total+=got;}
  char value[8]={};
  if(httpd_query_key_value(body,"profile",value,sizeof(value))!=ESP_OK || value[0]<'0' || value[0]>'2' || value[1]!='\0'){
    httpd_resp_set_status(req,"400 Bad Request");return httpd_resp_sendstr(req,"{\"ok\":false,\"error\":\"Choose profile 0, 1, or 2\"}");
  }
  int next=value[0]-'0';
  if(next>0 && !psramFound()){httpd_resp_set_status(req,"409 Conflict");return httpd_resp_sendstr(req,"{\"ok\":false,\"error\":\"This profile needs working PSRAM\"}");}
  if(!cameraMutex)return unavailable(req,"{\"ok\":false,\"error\":\"Camera memory unavailable\"}");
  if(xSemaphoreTake(cameraMutex,pdMS_TO_TICKS(3000))!=pdTRUE)return unavailable(req,"{\"ok\":false,\"error\":\"Pause video and retry\"}");
  restorePending.store(false);bool ok=applyProfile(next);
  xSemaphoreGive(cameraMutex);
  if(!ok)return unavailable(req,"{\"ok\":false,\"error\":\"Sensor rejected the profile\"}");
  bool saved=preferencesReady && cameraPreferences.putBytes("profile",&next,sizeof(next))==sizeof(next);
  if(saved)savedProfile.store(next);
  heartbeatRequested.store(true);
  return httpd_resp_sendstr(req,saved ? "{\"ok\":true,\"saved\":true}" : "{\"ok\":true,\"saved\":false}");
}

esp_err_t streamHandler(httpd_req_t *req) {
  if(!cameraReady.load())return unavailable(req,"{\"ok\":false,\"error\":\"Camera is initializing\"}");
  httpd_resp_set_type(req,"multipart/x-mixed-replace;boundary=autobotanyframe");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");httpd_resp_set_hdr(req,"Access-Control-Allow-Origin","*");
  uint64_t windowStart=cameraNow();uint32_t frames=0;
  while(WiFi.status()==WL_CONNECTED) {
    uint64_t started=cameraNow();
    if(xSemaphoreTake(cameraMutex,pdMS_TO_TICKS(3000))!=pdTRUE)return ESP_FAIL;
    if(!cameraReady.load()){xSemaphoreGive(cameraMutex);return ESP_FAIL;}
    camera_fb_t *frame=esp_camera_fb_get();
    if(!frame){xSemaphoreGive(cameraMutex);return ESP_FAIL;}
    char header[140];int length=snprintf(header,sizeof(header),"\r\n--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",BOUNDARY,static_cast<unsigned>(frame->len));
    esp_err_t result=httpd_resp_send_chunk(req,header,length);
    if(result==ESP_OK)result=httpd_resp_send_chunk(req,reinterpret_cast<const char *>(frame->buf),frame->len);
    frameBytes.store(frame->len);esp_camera_fb_return(frame);xSemaphoreGive(cameraMutex);
    if(result!=ESP_OK)return result;
    ++frames;uint64_t tick=cameraNow();lastFrameMs.store(static_cast<uint32_t>(tick));
    if(tick-windowStart>=2000){fpsTenths.store(static_cast<uint32_t>(frames*10000ULL/(tick-windowStart)));frames=0;windowStart=tick;}
    uint64_t budget=1000/profiles[activeProfile.load()].maxFps,elapsed=cameraNow()-started;
    delay(elapsed<budget ? budget-elapsed : 1);
  }
  return ESP_OK;
}

bool startCameraServers() {
  httpd_config_t config=HTTPD_DEFAULT_CONFIG();
  config.server_port=80;config.ctrl_port=32768;config.max_uri_handlers=5;config.max_open_sockets=3;config.lru_purge_enable=true;config.stack_size=6144;config.send_wait_timeout=3;config.recv_wait_timeout=3;
  if(!captureServer){
  if(httpd_start(&captureServer,&config)!=ESP_OK)return false;
  httpd_uri_t capture={};capture.uri="/capture";capture.method=HTTP_GET;capture.handler=captureHandler;
  httpd_uri_t health={};health.uri="/health";health.method=HTTP_GET;health.handler=healthHandler;
  httpd_uri_t settings={};settings.uri="/settings";settings.method=HTTP_POST;settings.handler=settingsHandler;
  httpd_uri_t diagnostic={};diagnostic.uri="/";diagnostic.method=HTTP_GET;diagnostic.handler=diagnosticHandler;
  httpd_uri_t retry={};retry.uri="/retry";retry.method=HTTP_POST;retry.handler=retryHandler;
  if(httpd_register_uri_handler(captureServer,&capture)!=ESP_OK || httpd_register_uri_handler(captureServer,&health)!=ESP_OK || httpd_register_uri_handler(captureServer,&settings)!=ESP_OK || httpd_register_uri_handler(captureServer,&diagnostic)!=ESP_OK || httpd_register_uri_handler(captureServer,&retry)!=ESP_OK){httpd_stop(captureServer);captureServer=nullptr;return false;}
  }
  if(streamServer)return true;
  config.server_port=81;config.ctrl_port=32769;config.max_uri_handlers=1;config.max_open_sockets=2;
  if(httpd_start(&streamServer,&config)!=ESP_OK){streamServer=nullptr;return false;}
  httpd_uri_t stream={};stream.uri="/stream";stream.method=HTTP_GET;stream.handler=streamHandler;
  if(httpd_register_uri_handler(streamServer,&stream)!=ESP_OK){httpd_stop(streamServer);streamServer=nullptr;return false;}
  return true;
}

void sendHeartbeat() {
  WiFiClient connection;HTTPClient http;
  http.setConnectTimeout(1500);http.setTimeout(2000);
  int result=-1;
  if(http.begin(connection,REGISTER_URL)){
    http.addHeader("Content-Type","application/x-www-form-urlencoded");http.addHeader("X-Autobotany-Request","1");
    char body[160];snprintf(body,sizeof(body),"device=AutobotanyCamera&protocol=3&ready=%d&rssi=%d&fps=%.1f&profile=%d&bytes=%u",cameraReady.load() && serversReady.load() ? 1 : 0,WiFi.RSSI(),measuredFps(),activeProfile.load(),frameBytes.load());
    result=http.POST(body);if(result!=200)Serial.printf("Camera registration: HTTP %d\n",result);
    http.end();
  }
  heartbeatCode.store(result);
  if(result==200){registeredMs.store(static_cast<uint32_t>(cameraNow()));registrationSeen.store(true);}
}

bool startHelpAp() {
  IPAddress address(192,168,5,1),mask(255,255,255,0);
  if(!WiFi.mode(WIFI_AP_STA))return false;
  bool ok=WiFi.softAPConfig(address,address,mask) && WiFi.softAP(HELP_AP_NAME,AP_PASSWORD,1,false,1);
  helpApReady.store(ok);
  if(ok)Serial.println("Camera help: connect to Autobotany Camera Help and open http://192.168.5.1");
  return ok;
}

void setup() {
  Serial.begin(115200);pinMode(4,OUTPUT);digitalWrite(4,LOW);
  Serial.println("AutobotanyCamera 3.0.2");
  bootResetReason=esp_reset_reason();
  cameraMutex=xSemaphoreCreateMutex();
  preferencesReady=cameraPreferences.begin("ab_camera",false);
  int profile=0;
  if(preferencesReady && cameraPreferences.getBytesLength("profile")==sizeof(profile) && cameraPreferences.getBytes("profile",&profile,sizeof(profile))==sizeof(profile) && profile>=0 && profile<=2)savedProfile.store(profile);
  restorePending.store(savedProfile.load()>0 && bootResetReason!=ESP_RST_BROWNOUT);
  WiFi.persistent(false);WiFi.setHostname("autobotany-camera");
  WiFi.onEvent(wifiEvent,ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.setAutoReconnect(false);
  failedSince=cameraNow();connectToMain();
  Serial.printf("PSRAM: %s. Connecting to %s\n",psramFound() ? "detected" : "not detected",AP_NAME);
}

void loop() {
  uint64_t tick=cameraNow();
  if(heartbeatRequested.exchange(false))nextHeartbeat=0;
  if(retryRequested.exchange(false)){nextReconnect=0;nextHeartbeat=0;}
  bool connected=WiFi.status()==WL_CONNECTED;
  bool registered=connected && registrationSeen.load() && static_cast<uint32_t>(tick)-registeredMs.load()<15000;
  if(registered)failedSince=tick;
  if(!helpApReady.load() && tick-failedSince>=HELP_AFTER_MS && tick>=nextHelpRetry){nextHelpRetry=tick+30000;startHelpAp();}
  if((connected || helpApReady.load()) && !serversReady.load() && tick>=nextServerRetry){serversReady.store(startCameraServers());nextServerRetry=cameraNow()+10000;}
  if(!connected){
    wifiSince=0;
    if(tick>=nextReconnect){WiFi.disconnect(false,false);connectToMain();Serial.println("Retrying the main S3 network.");}
  }else{
    nextReconnect=tick+WIFI_ATTEMPT_MS;
    if(!wifiSince){wifiSince=tick ? tick : 1;nextHeartbeat=0;Serial.printf("Camera IP: %s; signal: %d dBm\n",WiFi.localIP().toString().c_str(),WiFi.RSSI());}
    if(tick>=nextHeartbeat){sendHeartbeat();nextHeartbeat=cameraNow()+5000;}
    if(captureServer && cameraMutex && !cameraReady.load() && tick>=nextCameraRetry && tick>=wifiSince+1000 && xSemaphoreTake(cameraMutex,0)==pdTRUE){
      nextCameraRetry=tick+10000;
      if(!cameraReady.load())cameraReady.store(initializeCamera());
      xSemaphoreGive(cameraMutex);nextHeartbeat=0;
    }
    if(restorePending.load() && cameraReady.load() && registered && tick>=wifiSince+30000 && xSemaphoreTake(cameraMutex,0)==pdTRUE){
      if(restorePending.exchange(false))applyProfile(savedProfile.load());
      xSemaphoreGive(cameraMutex);nextHeartbeat=0;
    }
  }
  delay(5);
}
