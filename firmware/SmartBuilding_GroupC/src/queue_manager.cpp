#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ctype.h>
#include <atomic>
#include "queue_manager.h"
static SemaphoreHandle_t fsLock=nullptr;
static std::atomic<bool> healthy{false};
// Classroom safety limit; do not delete unacknowledged records to make space.
static constexpr size_t MAX_QUEUE_FILES = 2000;
static bool validId(const String& s) {
 if(s.length()!=36)return false;
 for(int i=0;i<36;i++){char c=s[i];if(i==8||i==13||i==18||i==23){if(c!='-')return false;}else if(!isxdigit((unsigned char)c))return false;}
 return s[14]=='4' && strchr("89abAB",s[19])!=nullptr;
}
static bool validPayload(const String& path,const String& payload) {
 JsonDocument d;if(deserializeJson(d,payload))return false;
 if(!d["type"].is<const char*>()||!d["recorded_at"].is<const char*>())return false;
 String type=d["type"].as<String>();
 const char* idKey=type=="group_c"?"record_id":nullptr;
 if(!idKey||!d[idKey].is<const char*>())return false;
 String id=d[idKey].as<String>();
 return validId(id) && (path=="/queue/"+id+".json" || path=="/queue/"+id+".json.tmp");
}
static bool atomicWrite(const String& path,const String& payload) {
 const String temp=path+".tmp";
 if(LittleFS.exists(temp)){Serial.println("STORAGE_ERROR: pending temp collision "+temp);return false;}
 File f=LittleFS.open(temp,"w");if(!f)return false;
 size_t written=f.print(payload);f.flush();f.close();
 if(written!=payload.length())return false;
 File r=LittleFS.open(temp,"r");if(!r)return false;
 bool same=(r.size()==payload.length()&&r.readString()==payload);r.close();
 if(!same)return false;
 if(LittleFS.exists(path))return false;
 return LittleFS.rename(temp,path);
}
bool queueInit(){
 fsLock=xSemaphoreCreateMutex();if(!fsLock)return false;
 if(!LittleFS.begin(false))return false; // NEVER auto-format
 healthy=LittleFS.exists("/queue")||LittleFS.mkdir("/queue");
 if(healthy)healthy=LittleFS.exists("/quarantine")||LittleFS.mkdir("/quarantine");
 return healthy;
}
bool queueHealthy(){return healthy;}
void queueRecover(){
 if(!healthy)return;
 if(xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return;
 File dir=LittleFS.open("/queue");if(!dir){healthy=false;xSemaphoreGive(fsLock);return;}
 File item=dir.openNextFile();
 while(item){
  String path=item.path();bool isDir=item.isDirectory();String payload=isDir?"":item.readString();item.close();
  if(!isDir && path.endsWith(".tmp")){
   String target=path.substring(0,path.length()-4);
   if(validPayload(path,payload)&&!LittleFS.exists(target)){
    if(!LittleFS.rename(path,target)){healthy=false;Serial.println("STORAGE_ERROR: recover rename failed");}
    else Serial.println("RECOVERED_TMP "+target);
   }else{
    String quarantine="/quarantine/"+path.substring(path.lastIndexOf('/')+1);
    if(!LittleFS.rename(path,quarantine)){healthy=false;Serial.println("STORAGE_ERROR: quarantine failed");}
    else Serial.println("QUARANTINED "+quarantine);
   }
  }else if(!isDir && path.endsWith(".json") && !validPayload(path,payload)){
   String quarantine="/quarantine/"+path.substring(path.lastIndexOf('/')+1);
   if(!LittleFS.rename(path,quarantine)){healthy=false;Serial.println("STORAGE_ERROR: quarantine failed");}
   else Serial.println("QUARANTINED "+quarantine);
  }
  item=dir.openNextFile();
 }
 dir.close();xSemaphoreGive(fsLock);
}
bool queueSave(const String& id,const String& payload){
 if(!healthy||!validId(id))return false;
 String path="/queue/"+id+".json";
 if(!validPayload(path,payload))return false;
 if(xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return false;
 bool ok=false;
 if(LittleFS.exists(path)){File f=LittleFS.open(path,"r");ok=f&&f.readString()==payload;if(f)f.close();if(!ok)Serial.println("UUID_COLLISION_DIFFERENT_PAYLOAD");}
 else {
  size_t pending=0;
  File dir=LittleFS.open("/queue");
  if (!dir) { healthy=false; Serial.println("STORAGE_ERROR: queue directory unavailable"); }
  else {
   File item=dir.openNextFile();
   while(item) {
    if(!item.isDirectory() && String(item.path()).endsWith(".json")) ++pending;
    item.close(); item=dir.openNextFile();
   }
   dir.close();
   if(pending>=MAX_QUEUE_FILES) {
    Serial.println("QUEUE_FULL: record not saved; existing unacknowledged records preserved");
    xSemaphoreGive(fsLock);
    return false;
   }
   ok=atomicWrite(path,payload);
  }
 }
 if(!ok)healthy=false; // fail closed; do not silently lose research data
 xSemaphoreGive(fsLock);return ok;
}
bool queuePeek(String& path,String& payload){
 if(!healthy||xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return false;
 File dir=LittleFS.open("/queue");if(!dir){healthy=false;xSemaphoreGive(fsLock);return false;}
 File item=dir.openNextFile();bool found=false;
 while(item){String p=item.path();if(!item.isDirectory()&&p.endsWith(".json")){path=p;payload=item.readString();found=true;item.close();break;}item.close();item=dir.openNextFile();}
 dir.close();xSemaphoreGive(fsLock);return found;
}
bool queueRemove(const String& path){
 if(!healthy||!path.startsWith("/queue/")||!path.endsWith(".json"))return false;
 if(xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return false;
 bool ok=LittleFS.remove(path);if(!ok)healthy=false;
 xSemaphoreGive(fsLock);return ok;
}
size_t queueCount(){
 if(!healthy||xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return 0;
 File dir=LittleFS.open("/queue");File item=dir.openNextFile();size_t n=0;
 while(item){if(!item.isDirectory()&&String(item.path()).endsWith(".json"))n++;item.close();item=dir.openNextFile();}
 dir.close();xSemaphoreGive(fsLock);return n;
}

bool queueReject(const String& path) {
 if(!healthy || !path.startsWith("/queue/") || !path.endsWith(".json"))return false;
 if(xSemaphoreTake(fsLock,portMAX_DELAY)!=pdTRUE)return false;
 String target="/quarantine/"+path.substring(path.lastIndexOf('/')+1);
 bool ok=!LittleFS.exists(target) && LittleFS.rename(path,target);
 if(!ok)healthy=false;
 xSemaphoreGive(fsLock);return ok;
}
