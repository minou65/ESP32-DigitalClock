// webhandling.h

#ifndef _WEBHANDLING_h
#define _WEBHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <WiFi.h>


#include <IotWebConf.h>
#include <IotWebConfOptionalGroup.h>
#include "common.h"

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "123456789";

extern void wifiInit();
extern void wifiLoop();

static WiFiClient wifiClient;
extern IotWebConf iotWebConf;

struct NTPSettings_t {
    bool useNTPServer;
    const char* ntpServer;
    const char* timeZone;
};

struct ScreenSaverSettings_t {
	int twilightThreshold;
	const char* offTime;
	const char* onTime;
	int offTimeMinutes;
};

NTPSettings_t getNTPSettings();
ScreenSaverSettings_t getScreenSaverSettings();

#endif

