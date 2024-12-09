// 
// 
// 

#include "ntp.h"
#include "neotimer.h"
#include "ESP32Time.h"

#define NTPUpdatePeriode 43200000 // 12h

String NTPServer = "ntp.metas.ch"; // "pool.ntp.org";
String TimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
int32_t TimeOffset_sec = 0;

Neotimer NTPTimer(NTPUpdatePeriode);
ESP32Time rtc;

bool NTPSync = true;

void NTPInit() {
    Serial.println(F("ntp initializing..."));

    // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    configTzTime(TimeZone.c_str(), NTPServer.c_str());
    NTPSync = true;

    Serial.println(F("OK"));
}

void NTPloop() {
    if (NTPTimer.repeat() || NTPSync) {

        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            //rtc.setTimeStruct(timeinfo);
            NTPSync = false;
            Serial.println(F("NTP time successfull set"));
            char s[51];
            strftime(s, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo);
            Serial.println(s);
        }
    }
}

