2025.04.23
적용라이브러리
- Adafruit_NeoPixel
- ArduinoJson
- EEPROM
- ESP8266WiFi
- ESPAsyncTCP-master
- Hash
- LittleFS

#실행파일 업로드
python3 -I upload.py --chip esp8266 --port "COM5" --baud "921600" ""  --before default_reset --after hard_reset write_flash 0x0 ---.ino.bin

#파일업로드 준비
mklittlefs.exe -c data -p 256 -b 8192 -s 14655488 -.littlefs.bin
#파일 업로드
upload.py --chip esp8266 --port COM5 --baud 921600 write_flash 2097152 -.littlefs.bin
