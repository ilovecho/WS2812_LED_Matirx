"C:\Users\ilove\AppData\Local\Arduino15\packages\esp8266\tools\python3\3.7.2-post1/python3" -I "C:\Users\ilove\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2/tools/upload.py" --chip esp8266 --port "COM5" --baud "921600" ""  --before default_reset --after hard_reset write_flash 0x0 "C:\Users\ilove\AppData\Local\arduino\sketches\1C174A18E8450F17E5C2DBD73E57995A/Text_Web_Class_Json_12_PoC.ino.bin"


C:\Users\ilove\AppData\Local\Arduino15\packages\esp8266\tools\mklittlefs\3.1.0-gcc10.3-e5f9fec\mklittlefs.exe -c D:\source\WS2812_PoC\Text_Web_Class_Json_12_PoC\data -p 256 -b 8192 -s 14655488 C:\Users\ilove\AppData\Local\Temp\tmp-9168-iWj23u5ecd4O-.littlefs.bin

C:\Users\ilove\AppData\Local\Arduino15\packages\esp8266\tools\python3\3.7.2-post1\python3.exe C:\Users\ilove\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\tools\upload.py --chip esp8266 --port COM5 --baud 921600 write_flash 2097152 C:\Users\ilove\AppData\Local\Temp\tmp-9168-iWj23u5ecd4O-.littlefs.bin
