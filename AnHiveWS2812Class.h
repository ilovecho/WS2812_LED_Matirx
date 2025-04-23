#ifndef AnHiveWS2812Class_H
#define AnHiveWS2812Class_H
/*
    uint8_t width = 0;    // 0      모듈갯수 x 가로픽셀수 + 여유 : 최대 
    uint8_t height = 0;   // 1      모듈의 y축 갯수 - 정확하지 않으면 글짜가 깨짐
    uint8_t action = 0;   // 2      1:좌로1칸, 2:우로1칸, 3:좌로32칸, 4:우로32칸
    uint8_t bright = 16;  // 3      0-255 밝기 지정
    uint16_t wait =  2;  // 4, 2 => 200msec, 스크롤 시간지정
*/
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <vector>

#include "LittleFS.h"
#include "fonts.h"  // HTML 코드 포함된 헤더

#define FONT   ascii_5x7
#define LED_PIN    D4
#define WIDTH      240
#define HEIGHT     16
#define NUM_LEDS   (WIDTH * HEIGHT)

class AnHiveWS2812Class {

  private:
    Adafruit_NeoPixel* strip = nullptr;

    uint8_t dtype = 0;
    uint8_t headLength = 7;
    uint8_t width = 0;
    uint8_t height = 0;
    uint8_t letterspace = 1;
    uint8_t action = 0;
    uint8_t bright = 16;
    uint16_t wait = 200;
    uint8_t textColor_R  = 125;
    uint8_t textColor_G  = 255;
    uint8_t textColor_B  = 255;
    uint8_t backColor_R  = 0;
    uint8_t backColor_G  = 0;
    uint8_t backColor_B  = 0;
    uint16_t stringWidth = 0;
    String textMessage = "";

    std::vector<uint8_t> lastImage;
    std::vector<uint8_t> lastText;
    std::vector<uint8_t> lastTik;
    int scrollOffset = 0;

    uint16_t getPixelIndex(uint16_t x, uint16_t y, uint16_t height) {
      return (x % 2 == 0) ? (x * height + y) : (x * height + (height - 1 - y));
    }

    uint16_t getStringWidth(String data) {
      uint8_t height = pgm_read_byte(&FONT[0]);
      uint16_t offset = 1;

      stringWidth = 0;
      uint8_t charWidth = 0;

      for (uint8_t i = 0; i < data.length(); i++) {
        char ch = data.charAt(i);
        if (ch < 32 || ch > 126) return false;
        offset = (ch - 32) * 6 + 1;
        charWidth = pgm_read_byte(&FONT[offset]);
        stringWidth += charWidth + ( (stringWidth>0)?letterspace:0 );
        //Serial.printf("%c :%d / %d, ", ch, charWidth, stringWidth);
      }
      //Serial.println("");
      return stringWidth;
    }

    bool getCharBitmap(char ch, const uint8_t* font, uint8_t& width, uint8_t* bitmap) {
      if (ch < 32 || ch > 126) return false;
      uint8_t height = pgm_read_byte(&font[0]);
      uint16_t offset = 1;

      offset += (ch - 32) * 6;
      width = pgm_read_byte(&font[offset]);
      //Serial.printf("O:[%d]/W:[%d], ", offset, width);
      for (uint8_t i = 0; i < width; i++) {
        bitmap[i] = pgm_read_byte(&font[offset + 1 + i]);
      }
      return true;
    }
/*
    uint8_t getCharBitmap(char ch, const uint8_t* font, uint8_t& width, uint8_t* bitmap) {
      if (ch < 32 || ch > 126) return false 0;
      width = 6;
      for (uint8_t i = 0; i < width; i++) {
        bitmap[i] = font6x8_ascii[ch][i];
      }
      return true;
    }    
*/
    uint8_t drawChar(int startX, int startY, char ch, uint32_t color) {
      uint8_t bitmap[8];
      uint8_t charWidth = 0;
      uint8_t drawWidth = 0;
      if (!getCharBitmap(ch, FONT, charWidth, bitmap)) return drawWidth;

      uint8_t fontHeight = height;

      int posX = 0;
      int posXd = 0;
      int winX = 0;
      int ledX = 0;
      int ledY = 0;
      int maxLen = width>stringWidth?width:stringWidth;
      for (uint8_t x = 0; x < charWidth; x++) {
        
        if (stringWidth > width) {
          posX = ( startX + x);
          posXd = posX;
          if (posX < 0) posXd = posX + maxLen + 5;
          if (posX > maxLen) posXd = posX - maxLen - 5;
          ledX = posXd % width ;
          winX = posXd / width;
          if (winX != 0) {
            //Serial.printf("-[%2d] %2d/%2d/%2d/%2d(%c),", scrollOffset, posX, posXd, winX, ledX, ch);
            continue;
          }
        } else {
          posX = ( startX + x);
          if (posX<0) posX += width;
          ledX = posX % width ;
        }

        for (uint8_t y = 0; y < fontHeight; y++) {
          if (bitmap[x] & (1 << y)) {
            ledY = startY + y;
            //Serial.printf("%d:%d, %d,    ",ch, ledX, ledY);
            if (ledX < width && ledY < height) {
              strip->setPixelColor(getPixelIndex(ledX, ledY, height), color);
            }
          }
        }
        drawWidth++;
      }
      //Serial.printf("%c/%d\n", ch, drawWidth);
      return charWidth;
    }

  public:
    void setup() {
      LittleFS.begin();

      strip = new Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
      strip->begin();
      strip->clear();
      strip->show();
      delete strip;
      strip = nullptr;
    }
    
    void process(uint8_t dt) {
      String msg = (dt=='B')?show():(dt=='M')?showMessage():(dt=='T')?showTik():"";
    }

    void loop() {
      switch (action) {
        // for image processing
        case 1: scrollOffset -= 1; process(dtype); break;
        case 2: scrollOffset += 1; process(dtype); break;
        case 3: scrollOffset -= 6; process(dtype); break;
        case 4: scrollOffset += 6; process(dtype); break;
        case 5: case 6: case 7: flashlight(action - 4); break;
        case 8: showText("23.5°C", 0, 0, strip->Color(255, 100, 0)); break;
        default: return;
      }
    }

    long interval() {
      return wait;
    }

    String show() {    return show(lastImage);    }
    String show(std::vector<uint8_t> postBuffer) {
      if (postBuffer.size() < headLength) return "Header too short";
      if (!postBuffer.empty()) lastImage = postBuffer;

      dtype      = lastImage[0];
      headLength = lastImage[1];
      width      = lastImage[2];
      height     = lastImage[3];
      action     = lastImage[4];
      bright     = lastImage[5];
      wait       = lastImage[6] * 100;

      size_t totalSize = headLength + (width * height * 3);
      if (lastImage.size() != totalSize) return "Size mismatch";

      if (!strip) strip = new Adafruit_NeoPixel(width * height, LED_PIN, NEO_GRB + NEO_KHZ800);
      strip->begin();
      strip->setBrightness(bright);

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          int pixelIndex = y * width + x;
          int dataIndex = headLength + pixelIndex * 3;
          uint8_t r = lastImage[dataIndex];
          uint8_t g = lastImage[dataIndex + 1];
          uint8_t b = lastImage[dataIndex + 2];

          int led_x = (x + scrollOffset + width) % width;
          strip->setPixelColor(getPixelIndex(led_x, y, height), strip->Color(r, g, b));
        }
      }
      strip->show();
      return "";
    }

    void saveImageToLittleFS() {
      scrollOffset = 0;

      File file = LittleFS.open("/show.data", "w");
      if (!file) {
        Serial.println("❌ Failed to open show.data for writing");
        return;
      }

      size_t written = file.write(lastImage.data(), lastImage.size());
      file.close();

      Serial.printf("✅ Saved %d bytes to /show.data\n", written);
    }

    bool loadImageFromLittleFS() {
      File file = LittleFS.open("/show.data", "r");
      if (!file) {
        Serial.println("❌ show.data not found");
        return false;
      }

      size_t size = file.size();
      if (size < headLength) {
        Serial.println("❌ show.data too small");
        file.close();
        return false;
      }

      lastImage.clear();
      lastImage.reserve(size);

      while (file.available()) {
        lastImage.push_back(file.read());
      }
      file.close();

      dtype      = lastImage[0];
      headLength = lastImage[1];
      width      = lastImage[2];
      height     = lastImage[3];
      action     = lastImage[4];
      bright     = lastImage[5];
      wait       = lastImage[6] * 100;

      size_t expected = headLength + (width * height * 3);
      if (lastImage.size() != expected) {
        Serial.printf("❌ size mismatch: expected %d, got %d\n", expected, lastImage.size());
        return false;
      }

      Serial.printf("✅ Loaded %d bytes from /show.data\n", lastImage.size());
      return true;
    }    

    
    // Flash light
    void flashlight(uint8_t b) {
      uint32_t color;
      uint16_t s, e;

      strip->clear();

      color = strip->Color(255, 0, 0);
      s = 0; e = width*height/8/b*8;
      setFlash(s, e, color);  delay(50); // 0.2초 점등
      setFlash(s, e, 0);   delay(50); // 0.2초 소등      
      setFlash(s, e, color);  delay(50); // 0.2초 점등
      setFlash(s, e, 0);   delay(50); // 0.2초 소등      

      color = strip->Color(0, 0, 255);
      s = width*height*(b-1)/8/b*8; e = width*height;
      setFlash(s, e, color);  delay(50); // 0.2초 점등
      setFlash(s, e, 0);   delay(50); // 0.2초 소등      
      setFlash(s, e, color);  delay(50); // 0.2초 점등
      setFlash(s, e, 0);   delay(50); // 0.2초 소등      
    }

    void setFlash(uint16_t s, uint16_t e, uint32_t c) {
      for (int i = s; i < e; i++) {
        strip->setPixelColor(i, c);
      }
      strip->show();
    }

    void saveTextToLittleFS() {
      scrollOffset = 0;

      File file = LittleFS.open("/text.data", "w");
      if (!file) {
        Serial.println("❌ Failed to open show.data for writing");
        return;
      }
      size_t written = file.write(lastText.data(), lastText.size());
      file.close();

      Serial.printf("✅ Saved %d bytes to /text.data\n", written);
    }

    // Text 
    bool loadTextFromLittleFS() {
      File file = LittleFS.open("/text.data", "r");
      if (!file) {
        Serial.println("❌ text.data not found");
        return false;
      }

      size_t size = file.size();
      if (size < headLength) {
        Serial.println("❌ text.data too small");
        file.close();
        return false;
      }

      lastText.clear();
      lastText.reserve(size);

      while (file.available()) {
        lastText.push_back(file.read());
      }
      file.close();

      dtype      = lastText[0];
      headLength = lastText[1];
      width      = lastText[2];
      height     = lastText[3];
      letterspace= lastText[4];
      action     = lastText[5];
      bright     = lastText[6];
      wait       = lastText[7] * 100;
      textColor_R    = lastText[8];
      textColor_G    = lastText[9];
      textColor_B    = lastText[10];
      backColor_R    = lastText[11];
      backColor_G    = lastText[12];
      backColor_B    = lastText[13];

      if (lastText.size() <= headLength) {
        Serial.printf("❌ size mismatch: grater than %d, got %d\n", headLength, lastText.size());
        return false;
      }

      size_t length = lastText.size() - headLength;  // 남은 갯수
      textMessage = "";
      for (size_t i=headLength; i < lastText.size(); i++)
        textMessage += (char)lastText[i];

      Serial.printf("✅ Loaded %d bytes from /text.data\n", lastText.size());
      return true;
    }    

    String showMessage()  { return showMessage(lastText); }

    //void showText(String data, int x = 0, int y = 0, uint32_t color = strip->Color(255, 255, 255)) {
    String showMessage(std::vector<uint8_t> postBuffer) {
      if (postBuffer.size() < headLength) return "Header too short";
      if (!postBuffer.empty()) lastText = postBuffer;

      dtype      = lastText[0];
      headLength = lastText[1];
      width      = lastText[2];
      height     = lastText[3];
      letterspace= lastText[4];
      action     = lastText[5];
      bright     = lastText[6];
      wait       = lastText[7] * 100;
      textColor_R    = lastText[8];
      textColor_G    = lastText[9];
      textColor_B    = lastText[10];
      backColor_R    = lastText[11];
      backColor_G    = lastText[12];
      backColor_B    = lastText[13];

      size_t length = lastText.size() - headLength;  // 남은 갯수
      textMessage = "";
      for (size_t i=headLength; i < lastText.size(); i++)
        textMessage += (char)lastText[i];

      stringWidth = getStringWidth(textMessage);
      showText(textMessage, 0, 0, strip->Color(textColor_R,textColor_G,textColor_B));
      return "";
    }

    void showText(String data, int x, int y, uint32_t color) {
      if (!strip) return;

      strip->clear();
      scrollOffset %= (dtype=='B')?width:stringWidth;
      //Serial.printf("%s : %d\n", data, scrollOffset);

      int cursorX = x + scrollOffset;
      char ch;
      uint8_t charWidth;
      for (uint8_t i = 0; i < data.length(); i++) {
        ch = data.charAt(i);
        charWidth = drawChar(cursorX, y, ch, color);
        cursorX += charWidth + letterspace; // 글자 사이 여백
        //cursorX %= width;
          //Serial.printf(" - ch[%x], => cursorX[%d] += charWidth[%d] + letterspace[%d]\n", ch, cursorX, charWidth, letterspace);
      }

      strip->show();
    }

    
    String showTik()  { return showTik(lastTik); }
    String showTik(std::vector<uint8_t> postBuffer) {
      if (postBuffer.size() < 3) return "Header too short";
      if (!postBuffer.empty()) lastTik = postBuffer;

      dtype      = lastTik[0];
      headLength = lastTik[1];
      action     = lastTik[2];

      size_t length = lastTik.size() - 3;  // 남은 갯수
      textMessage = "";
      for (size_t i=headLength; i < lastTik.size(); i++)
        textMessage += (char)lastTik[i];
      stringWidth = getStringWidth(textMessage);
      showText(textMessage, 0, 0, strip->Color(textColor_R,textColor_G,textColor_B));
      return "";
    }

    void saveTikToLittleFS() {
      scrollOffset = 0;

      File file = LittleFS.open("/tik.data", "w");
      if (!file) {
        Serial.println("❌ Failed to open show.data for writing");
        return;
      }
      size_t written = file.write(lastTik.data(), lastTik.size());
      file.close();

      Serial.printf("✅ Saved %d bytes to /tik.data\n", written);
    }

    bool loadTikFromLittleFS() {
      loadTextFromLittleFS();

      File file = LittleFS.open("/tik.data", "r");
      if (!file) {
        Serial.println("❌ tik.data not found");
        return false;
      }

      size_t size = file.size();
      if (size < headLength) {
        Serial.println("❌ tik.data too small");
        file.close();
        return false;
      }

      lastTik.clear();
      lastTik.reserve(size);

      while (file.available()) {
        lastTik.push_back(file.read());
      }
      file.close();

      dtype      = lastTik[0];
      headLength = lastTik[1];
      action     = lastTik[2];

      if (lastTik.size() < headLength) {
        Serial.printf("❌ size mismatch: grater than %d, got %d\n", headLength, lastTik.size());
        return false;
      }

      size_t length = lastTik.size() - headLength;  // 남은 갯수
      textMessage = "";
      for (size_t i=headLength; i < lastTik.size(); i++)
        textMessage += (char)lastTik[i];

      Serial.printf("✅ Loaded %d bytes from /tik.data\n", lastTik.size());
      return true;
    } 
};

AnHiveWS2812Class AnHiveWS2812;

#endif // AnHiveWS2812Class_H
