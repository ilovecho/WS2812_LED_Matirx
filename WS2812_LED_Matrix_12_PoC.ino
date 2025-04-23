#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#include "config.h"             // System Configuration 
#include "AnHiveWS2812Class.h"  // Matrix Control with condition
#include "LittleFS.h"           // File system
#include <ArduinoJson.h>

SystemConfig CONFIG;
std::vector<uint8_t> postBuffer;  // Web Communication Buffer to set display
AsyncWebServer server(80);

// 파일 확장자에 따른 MIME 타입 자동 판별 함수
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css"))  return "text/css";
  if (filename.endsWith(".js"))   return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png"))  return "image/png";
  if (filename.endsWith(".jpg"))  return "image/jpeg";
  if (filename.endsWith(".ico"))  return "image/x-icon";
  if (filename.endsWith(".svg"))  return "image/svg+xml";
  if (filename.endsWith(".gz"))   return "application/x-gzip";
  return "text/plain";
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  CONFIG = loadSystemConfig();

  AnHiveWS2812.setup();
  AnHiveWS2812.loadImageFromLittleFS();
  AnHiveWS2812.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(CONFIG.wifiSSID, CONFIG.wifiPASS);

  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect. Starting AP mode...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(CONFIG.apSSID, CONFIG.apPASS);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }

  // Update and control the display context
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    // 완료 응답은 여기에서 처리됨
  }, nullptr,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
      postBuffer.clear();
      postBuffer.reserve(total);
    }

    postBuffer.insert(postBuffer.end(), data, data + len);
    if (index + len == total) {

      String msg = "";
      uint8_t first = postBuffer[0];
      Serial.printf("First byte [%x], 'B' expected", first);
      switch (first) {
        case 'B':
          msg = AnHiveWS2812.show(postBuffer);
          AnHiveWS2812.saveImageToLittleFS();
          CONFIG.autoModul[0] = first;
          saveSystemConfig(CONFIG);
          break;
        case 'M':
          msg = AnHiveWS2812.showMessage(postBuffer);
          AnHiveWS2812.saveTextToLittleFS();
          CONFIG.autoModul[0] = first;
          saveSystemConfig(CONFIG);
          break;
        case 'T':
          msg = AnHiveWS2812.showTik(postBuffer);
          AnHiveWS2812.saveTikToLittleFS();
          CONFIG.autoModul[0] = first;
          saveSystemConfig(CONFIG);
          break;
        default:
          msg = "Default case for process";
      }
      request->send(200, "text/plain", msg);
    }
  });

  // system control configuration
  server.on("/ctl", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static std::vector<uint8_t> sysBuffer;

    if (index == 0) sysBuffer.clear();
    sysBuffer.insert(sysBuffer.end(), data, data + len);

    if (index + len == total) {
      sysBuffer.push_back('\0'); // null-terminate
      String payload((char*)sysBuffer.data());

      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, payload);
      if (err) {
        Serial.printf("JSON 파싱 실패: %s\n",err.f_str());
        return;
      }

      if (!doc.containsKey("func")) {
        request->send(400, "text/plain", "Invalid format with [func]");
        return;
      }

      String func = doc["func"].as<String>();
      doc.remove("func");

      bool status = false;
      String msg = "["+func+"] is not available";
      status = (func == "setcfg")? setcfg(doc, msg):
               (func == "getcfg")? getcfg(msg):false;
      Serial.println(msg);

      if (status) request->send(200, "text/plain", msg);
      else        request->send(400, "text/plain", msg);

      return;
    }
  });

  // 모든 나머지 파일에 대응 (JS, CSS, 이미지 등)
  server.onNotFound([](AsyncWebServerRequest *request) {
    String path = request->url();
    if (path == "/") path = "/index.html";
    if (LittleFS.exists(path)) {
      request->send(LittleFS, path, getContentType(path));
    } else {
      request->send(404, "text/plain", "Not Found");
    }
  });

  server.begin();

  switch (CONFIG.autoModul[0]) {
    case 'M':
      AnHiveWS2812.loadTextFromLittleFS();
      AnHiveWS2812.showMessage();
      break;
    case 'T':
      AnHiveWS2812.loadTikFromLittleFS();
      AnHiveWS2812.showTik();
      break;
    default:
      break;
  }

}

void loop() {
  AnHiveWS2812.loop();
  delay(AnHiveWS2812.interval());
}