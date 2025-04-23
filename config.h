#include <EEPROM.h>
#include "LittleFS.h"
#include <ArduinoJson.h>

#define EEPROM_SIZE 256
#define EEPROM_ADDR_SYS 0  // 0번지부터 시작

struct SystemConfig {
  char wifiSSID[16];
  char wifiPASS[16];
  char apSSID[16];
  char apPASS[16];
  int width;
  int height;
  int fontsize;
  char account[16];
  char passwd[16];
  char passkey[16];
  char autoModul[1];
};

SystemConfig CurrentConfig;

void saveSystemConfig(const SystemConfig& config) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(EEPROM_ADDR_SYS, config);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("✅ System config saved to EEPROM.");
}

bool loadConfigFromLittleFS(SystemConfig& config) {

  Dir dir = LittleFS.openDir("/");
  while(dir.next()) {
      Serial.print("File: ");
      Serial.println("/" + dir.fileName());
  }

  // 1) 파일 열기
  File file = LittleFS.open("/config.setup", "r");
  if (!file) return false;
  Serial.println(F("❌ config.setup not found"));


  // 2) JSON 파싱 (Stream 기반 overload)
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();

  if (err) {
    Serial.print(F("JSON 파싱 실패: "));
    Serial.println(err.f_str());
    return false;
  }

  // 3) 구조체에 복사
  // 문자열 최대 길이에 맞춰 strlcpy 사용
  strlcpy(config.wifiSSID, doc["wifiSSID"]   | "", sizeof(config.wifiSSID));
  strlcpy(config.wifiPASS, doc["wifiPASS"]   | "", sizeof(config.wifiPASS));
  strlcpy(config.apSSID,   doc["apSSID"]     | "", sizeof(config.apSSID));
  strlcpy(config.apPASS,   doc["apPASS"]     | "", sizeof(config.apPASS));
  config.width  = doc["width"].as<int>();
  config.height = doc["height"].as<int>();
  config.fontsize = doc["fontsize"].as<int>();
  strlcpy(config.account,  doc["account"]    | "", sizeof(config.account));
  strlcpy(config.passwd,   doc["passwd"]     | "", sizeof(config.passwd));
  strlcpy(config.passkey,  doc["passkey"]    | "", sizeof(config.passkey));

  LittleFS.remove("/config.setup"); 
  return true;
}

SystemConfig loadSystemConfig() {
  SystemConfig config;
  /*
  "wifiSSID": "test24",
  "wifiPASS": "20622065",
  "apSSID":   "WS2812_AP",
  "apPASS":   "",
  "width":    80,
  "height":   16,
  "fontsize":   10,
  "account": "admin",
  "passwd": "1234",
  "passkey": "fyVz3yqOgjWqb4K"  
  */
  if (loadConfigFromLittleFS(config) ) saveSystemConfig(config);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR_SYS, config);
  EEPROM.end();

  CurrentConfig = config;

  Serial.println("✅ System config loaded from EEPROM.");
  return config;
}

void serialPrintConfig(SystemConfig& config) {
    Serial.printf("WiFi SSID: %s\n", config.wifiSSID);
    Serial.printf("WiFi PASS: %s\n", config.wifiPASS);
    Serial.printf("AP SSID:   %s\n", config.apSSID);
    Serial.printf("AP PASS:   %s\n", config.apPASS);
    Serial.printf("Width:     %d\n", config.width);
    Serial.printf("Height:    %d\n", config.height);
}

bool setcfg(StaticJsonDocument<256> doc, String& msg) {
    // 파싱: json with doc

    SystemConfig config;
    config = CurrentConfig;

    if (doc.containsKey("wifiSSID")) strlcpy(config.wifiSSID, doc["wifiSSID"], sizeof(config.wifiSSID));
    if (doc.containsKey("wifiPASS")) strlcpy(config.wifiPASS, doc["wifiPASS"], sizeof(config.wifiPASS));
    if (doc.containsKey("apSSID"))   strlcpy(config.apSSID,   doc["apSSID"],   sizeof(config.apSSID));
    if (doc.containsKey("apPASS"))   strlcpy(config.apPASS,   doc["apPASS"],   sizeof(config.apPASS));
    if (doc.containsKey("width"))    config.width    = doc["width"].as<int>();
    if (doc.containsKey("height"))   config.height   = doc["height"].as<int>();
    if (doc.containsKey("fontsize")) config.fontsize = doc["fontsize"].as<int>();
    if (doc.containsKey("account"))  strlcpy(config.account,  doc["account"],  sizeof(config.account));
    if (doc.containsKey("passwd"))   strlcpy(config.passwd,   doc["passwd"],   sizeof(config.passwd));
    if (doc.containsKey("passkey"))  strlcpy(config.passkey,  doc["passkey"],  sizeof(config.passkey));

    serialPrintConfig(config);
    saveSystemConfig(config);

    Serial.println("✅ New system config received and saved.");
    serialPrintConfig(config);

    msg = "System config saved";
    return true;
}

bool getcfg(String& json) {
  // JSON 형태로 반환
  SystemConfig config = loadSystemConfig();
  
  Serial.print("config.width: ");
  Serial.print(config.width);
  Serial.print(", config.height: ");
  Serial.print(config.height);
  Serial.print(", config.fontsize: ");
  Serial.println(config.fontsize);

  json = "{";
  json += "\"wifiSSID\":\"" + String(config.wifiSSID) + "\",";
  json += "\"wifiPASS\":\"" + String(config.wifiPASS) + "\",";
  json += "\"apSSID\":\"" + String(config.apSSID) + "\",";
  json += "\"apPASS\":\"" + String(config.apPASS) + "\",";
  json += "\"width\":" + String(config.width) + ",";
  json += "\"height\":" + String(config.height) + ",";
  json += "\"fontsize\":" + String(config.fontsize) + "";
  json += "}";
  
  Serial.println(json);
  return true;
}

