
# ESP32 Wanduhr

## WaveShare
- [ESP32-S3-Touch-LCD-7](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7)
- [Creating lv_conf.h](https://docs.lvgl.io/master/intro/add-lvgl-to-your-project/configuration.html#lv-conf-h)
- [LVGL: Tips and Tricks](https://esphome.io/cookbook/lvgl.html)


- [ESP32 TFT with LVGL: Digital Clock with Time and Date](https://randomnerdtutorials.com/esp32-tft-lvgl-digital-clock/)
- [How to create a clock?](https://docs.lvgl.io/v8/en/html/overview/objects.html#clock)
- [A Step-by-Step Guide to Using LVGL with ESP32](https://www.tech-sparks.com/a-step-by-step-guide-to-using-lvgl-with-esp32/#:~:text=Go%20to%20the%20%E2%80%9CTools%E2%80%9D%20menu%20and%20select%20%E2%80%9CManage,the%20right.%20Wait%20for%20the%20installation%20to%20complete.)
- https://www.elecrow.com/wiki/lesson05-introduction-to-the-5-categories-of-lvgl-gui-library-widgets.html

## vMicro Boardsettings
[vMicro-Boardsettings.png](/img/esp32-wanduhr/vMicro-Boardsettings.png)]

## esp-idf
https://forum.arduino.cc/t/esp-memory-utils-h-missing-in-arduino-ide/1260898/5
The dependence on esp-idf 5.x (and thus "esp32" boards platform version 3.x) was introduced in the 0.1.0 release of the "ESP32_Display_Panel" library. So if you would like to use the library with esp32 boards platform 2.x, you should install version 0.0.2 of the library which is the last version that is compatible with esp-idf 4.x.

I'll provide instructions you can follow to do that:

Select Sketch > Include Library > Manage Libraries... from the Arduino IDE menus to open the "Library Manager" view in the left side panel.
- Type ESP32_Display_Panel in the "Filter your search..." field.
- Scroll down through the list of libraries until you see the "ESP32_Display_Panel" entry.
- You will see a drop-down version menu at the bottom of the entry. Select "0.0.2" from the menu.
- Click the "INSTALL" button at the bottom of the entry.
- An "Install library dependencies" dialog will open.
- Click the "INSTALL ALL" button in the "Install library dependencies" dialog.
- The dialog will close.
- Wait for the installation process to finish, as indicated by a notification at the bottom right corner of the Arduino IDE window:

## LVGL
- [Arduino](https://docs.lvgl.io/master/details/integration/framework/arduino.html#configure-lvgl)

- https://github.com/lvgl/lvgl/issues/6778
- Zwischen Version 8 und 9 gibt es einen breaking change. Die Beispiele und bereitgestellenten librarys von Waveshare sind für Version 8.x


### Font
- [Fontconverter](https://lvgl.io/tools/fontconverter)
- [Monserrat](https://fonts.google.com/specimen/Montserrat?query=monts)
- [Font (lv_font)](https://docs.lvgl.io/master/details/main-components/font.html)
- 
### lv_conf.h
Um die lokale Konfiguration verwednen zu können muss die Datei `lv_conf.h` im Projektordner vorhanden sein. Diese Datei wird von der Bibliothek `lvgl` benötigt. Die Datei `lv_conf.h` kann aus dem Ordner `lvgl` kopiert werden.

Danch müssen die  notwendigen Einstellungen für build_flags vorgenommen werden. Für VisualMicro muss dazu unter Projekt Eigenschaften -> Extra Flags folgendes eingetragen werden:

```
-D ESP_PANEL_CONF_INCLUDE_SIMPLE -D LV_CONF_INCLUDE_SIMPLE
```

## board.txt
In der Datei `board.txt` müssen die notwendigen Einstellungen für die Bibliothek `lvgl` vorgenommen werden.

'''
board_build.flash_size = 8MB
board_build.psram_type = opi
'''

