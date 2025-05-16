#ifndef _NTP_h
#define _NTP_h

#if defined(ARDUINO) && ARDUINO >= 100
    #include "arduino.h"
#else
    #include "WProgram.h"
#endif

#include "neotimer.h"
#include "ESP32Time.h"

class NTPClient {
public:
	// interval in hours
    NTPClient(int interval = 1) : _NTPTimer(interval * 3600 * 1000), _NTPSync(true), _initialized(false) {}

    void begin(const String& server, const String& timeZone, int32_t timeOffset) {
        _NTPServer = server;
        _TimeZone = timeZone;
        _TimeOffset_sec = timeOffset;

        Serial.println(F("ntp initializing..."));

        _NTPSync = true;
        _initialized = true;

        Serial.println(F("OK"));
    }

    void process() {
        if (_NTPTimer.repeat() || _NTPSync) {
            Serial.println(F("ntp processing..."));
            Serial.print(F("    Timezone: ")); Serial.println(_TimeZone);
            Serial.print(F("    NTP server: ")); Serial.println(_NTPServer);

            // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
            configTzTime(_TimeZone.c_str(), _NTPServer.c_str());

            struct tm timeinfo_;
            if (getLocalTime(&timeinfo_)) {
                // _rtc.setTimeStruct(timeinfo_);
                _NTPSync = false;
                Serial.println(F("NTP time successfully set"));
                char s_[51];
                strftime(s_, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo_);
                Serial.println(s_);
            }
        }
    }

    bool isInitialized() const {
        return _initialized;
    }

	void setInterval(int interval) {
		_NTPTimer.start(interval * 3600 * 1000);
	}

private:
    String _NTPServer;
    String _TimeZone;
    int32_t _TimeOffset_sec;
    Neotimer _NTPTimer;
    ESP32Time _rtc;
    bool _NTPSync;
    bool _initialized;
};

#endif
