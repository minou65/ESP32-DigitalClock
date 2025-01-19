


#include <Arduino.h>

#include "common.h"
#include "version.h"
#include "webhandling.h"
#include "DisplayHandling.h"
#include "version.h"
#include "ntp.h"


NTPClient ntpClient(3);

char Version[] = VERSION_STR; // Manufacturer's Software version code


//void lv_create_analog_clock(void) {
//	// Create an analog clock: https://docs.lvgl.io/master/widgets/clock.html
//	lv_obj_t* clock = lv_clock_create(lv_scr_act(), NULL);
//	lv_obj_align(clock, LV_ALIGN_CENTER, 0, 0);
//}

void configDisplay() {
    Serial.println("Configure the display");
    NTPSettings_t ntpSettings_ = getNTPSettings();
    if (ntpSettings_.useNTPServer) {
        ntpClient.begin(ntpSettings_.ntpServer, ntpSettings_.timeZone, 0);
    }

    ScreenSaverSettings_t screenSaverSettings_ = getScreenSaverSettings();
    setScreensaverThreshold(screenSaverSettings_.twilightThreshold);
    setScreensaverScheduler(screenSaverSettings_.onTime, screenSaverSettings_.offTime);

    Serial.println("Display configured");
}





void setup(){
	Serial.begin(115200);
	Serial.println("Start setup");
    
    wifiInit();
    initDisplay();
    configDisplay();

	Serial.println("Setup done");
}

void loop(){
    wifiLoop();

    if (iotWebConf.getState() == iotwebconf::OnLine) {

        if (ntpClient.isInitialized()) {
            ntpClient.process();
        }
    }

    if (ParamsChanged) {
        ParamsChanged = false;
        configDisplay();
    }
}
