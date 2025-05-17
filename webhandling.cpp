// 
// 
// 

#define DEBUG_WIFI(m) SERIAL_DBG.print(m)

#include <Arduino.h>
#include <ArduinoOTA.h>

#include <time.h>
#include "ESP32Time.h"

#include<iostream>
#include <string.h>

#include <DNSServer.h>
#include <IotWebConf.h>
#include <IotWebConfAsyncClass.h>
#include <IotWebConfAsyncUpdateServer.h>
#include <IotWebRoot.h>
#include <IotWebConfOptionalGroup.h>
#include "common.h"
#include "webhandling.h"
#include "favicon.h"

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "1.0"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN  -1

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN -1
#if ESP32 
#define ON_LEVEL HIGH
#else
#define ON_LEVEL LOW
#endif

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "ESP32-Wanduhr";

// -- Method declarations.
void onProgress(size_t prg, size_t sz);
void handleRoot(AsyncWebServerRequest* request);
void handleData(AsyncWebServerRequest* request);
void convertParams();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool ParamsChanged = true;
bool ShouldReboot = false;

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebServerWrapper asyncWebServerWrapper(&server);
AsyncUpdateServer AsyncUpdater;

class CustomHtmlFormatProvider : public iotwebconf::OptionalGroupHtmlFormatProvider {
protected:
    virtual String getFormEnd() {
        String _s = OptionalGroupHtmlFormatProvider::getFormEnd();
        _s += F("</br><form action='/reboot' method='get'><button type='submit'>Reboot</button></form>");
        _s += F("</br>Return to <a href='/'>home page</a>.");
		return _s;
	}
};
CustomHtmlFormatProvider customHtmlFormatProvider;

IotWebConf iotWebConf(thingName, &dnsServer, &asyncWebServerWrapper, wifiInitialApPassword, CONFIG_VERSION);

class NTPSettings : public iotwebconf::ParameterGroup {
public:
    NTPSettings(const char* id = "TimeSource", const char* lable = "Time Source")
        : iotwebconf::ParameterGroup(id, lable) {
        snprintf(_useNTPServerID, STRING_LEN, "%s-UseNTP", this->getId());
        snprintf(_ntpServerID, STRING_LEN, "%s-server", this->getId());
        snprintf(_timeZoneID, STRING_LEN, "%s-tz", this->getId());

        this->addItem(&this->_useNTPServerParam);
        this->addItem(&this->_ntpServerAddressParam);
        this->addItem(&this->_gmtOffsetParam);
    }

    const bool useNTPServer() const {
        return strcmp(_useNTPServerValue, "selected") == 0;
    }

    const char* getNTPServer() const {
        return _ntpServerValue;
    }

    const char* getTimeZone() const {
        return _timeZoneValue;
    }

private:
    char _useNTPServerValue[10];
    char _ntpServerValue[30];
    char _timeZoneValue[30];
    char _useNTPServerID[STRING_LEN];
    char _ntpServerID[STRING_LEN];
    char _timeZoneID[STRING_LEN];

    iotwebconf::CheckboxParameter _useNTPServerParam = iotwebconf::CheckboxParameter("Use NTP server", _useNTPServerID, _useNTPServerValue, 10, true);
    iotwebconf::TextParameter _ntpServerAddressParam = iotwebconf::TextParameter("NTP server (FQDN or IP address)", _ntpServerID, _ntpServerValue, 30, "pool.ntp.org");
    iotwebconf::TextParameter _gmtOffsetParam = iotwebconf::TextParameter("POSIX timezones string", _timeZoneID, _timeZoneValue, 30, "CET-1CEST,M3.5.0,M10.5.0/3");
};
NTPSettings NTPSettingsGroup;

class ScreenSaver : public iotwebconf::ParameterGroup {
public:
    ScreenSaver(const char* id = "Screensaver", const char* lable = "Screensaver")
        : iotwebconf::ParameterGroup(id, lable) {
        snprintf(_offTimeID, STRING_LEN, "%s-offTime", this->getId());
        snprintf(_onTimeID, STRING_LEN, "%s-onTime", this->getId());
        snprintf(_offTimeMinutesID, STRING_LEN, "%s-offTimeMinutes", this->getId());
        snprintf(_thresholdID, STRING_LEN, "%s-threshold", this->getId());

        this->addItem(&this->_onTimeParam);
        this->addItem(&this->_offTimeParam);
        this->addItem(&this->_offTimeMinutesParam);
        this->addItem(&this->_thresholdParam);
    }

    const char* getOffTime() const {
        return _offTimeValue;
    }

    const char* getOnTime() const {
        return _onTimeValue;
    }

    const int getOffTimeMinutes() const {
        return atoi(_offTimeMinutesValue);
    }

    const int getThreshold() const {
        return atoi(_thresholdValue);
    }

private:
    char _offTimeValue[6];
    char _onTimeValue[6];
    char _offTimeMinutesValue[5];
    char _thresholdValue[5];
    char _offTimeID[STRING_LEN];
    char _onTimeID[STRING_LEN];
    char _offTimeMinutesID[STRING_LEN];
    char _thresholdID[STRING_LEN];

    iotwebconf::TimeParameter _offTimeParam = iotwebconf::TimeParameter("Off Time (HH:MM)", _offTimeID, _offTimeValue, 6, "22:00");
    iotwebconf::TimeParameter _onTimeParam = iotwebconf::TimeParameter("On Time (HH:MM)", _onTimeID, _onTimeValue, 6, "06:00");
    iotwebconf::NumberParameter _offTimeMinutesParam = iotwebconf::NumberParameter("Off Time (Minutes)", _offTimeMinutesID, _offTimeMinutesValue, 5, "5", "1..3600", "min='1' max='3600' step='1'");
    iotwebconf::NumberParameter _thresholdParam = iotwebconf::NumberParameter("Twilight Limit", _thresholdID, _thresholdValue, 5, "100", "100..2500", "min='100' max='2500' step='1'");
};
ScreenSaver ScreenSaverGroup;

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting webserver...");

    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);

    iotWebConf.addParameterGroup(&ScreenSaverGroup);
    iotWebConf.addParameterGroup(&NTPSettingsGroup);

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { AsyncUpdater.setup(&server, updatePath, onProgress); },
        [](const char* userName, char* password) { AsyncUpdater.updateCredentials(userName, password); });

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    convertParams();

    // -- Set up required URL handlers on the web server.
    // -- Set up required URL handlers on the web server.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { handleRoot(request); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "image/x-icon", favicon_ico, sizeof(favicon_ico));
        request->send(response);
        }
    );
    server.on("/config", HTTP_ANY, [](AsyncWebServerRequest* request) {
        Serial.println("Config page requested.");
        AsyncWebRequestWrapper asyncWebRequestWrapper(request, 4096);
        Serial.println("Handling config page.");
        iotWebConf.handleConfig(&asyncWebRequestWrapper);
        }
    );
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest* request) { handleData(request); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html",
            "<html>"
            "<head>"
            "<meta http-equiv=\"refresh\" content=\"15; url=/\">"
            "<title>Rebooting...</title>"
            "</head>"
            "<body>"
            "Please wait while the device is rebooting...<br>"
            "You will be redirected to the homepage shortly."
            "</body>"
            "</html>");
        request->client()->setNoDelay(true); // Disable Nagle's algorithm so the client gets the response immediately
        request->send(response);
        ShouldReboot = true;
        }
    );
    server.onNotFound([](AsyncWebServerRequest* request) {
        AsyncWebRequestWrapper asyncWebRequestWrapper(request);
        iotWebConf.handleNotFound(&asyncWebRequestWrapper);
        }
    );
    Serial.println("webserver is ready");
}

void wifiLoop() {
    // -- doLoop should be called as frequently as possible.
    iotWebConf.doLoop();
    ArduinoOTA.handle();

    if (ShouldReboot || AsyncUpdater.isFinished()) {
        delay(1000);
        ESP.restart();
    }
}

void wifiConnected(){
    ArduinoOTA.begin();
}

class MyHtmlRootFormatProvider : public HtmlRootFormatProvider {
public:
    String getHtmlTableRowClass(String name, String htmlclass, String id) {
        String s_ = F("<tr><td align=\"left\">{n}</td><td align=\"left\"><span id=\"{id}\" class=\"{c}\"></span></td></tr>\n");
        s_.replace("{n}", name);
        s_.replace("{c}", htmlclass);
        s_.replace("{id}", id);
        return s_;
    }

    String getHtmlTableRowVar(String name, String id) {
        String s_ = F("<tr><td align=\"left\">{n}</td><td align=\"left\" id=\"{id}\">no data</td></tr>\n");
        s_.replace("{n}", name);
        s_.replace("{id}", id);
        return s_;
    }

protected:
    virtual String getStyleInner() {
        String s_ = HtmlRootFormatProvider::getStyleInner();
        s_ += F(".led {display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }\n");
        s_ += F(".led.off {background-color: grey;}\n");
        s_ += F(".led.on {background-color: green;}\n");
        s_ += F(".led.delayedoff {background-color: orange;}\n");
        return s_;
    }

    virtual String getScriptInner() {
        String s_ = HtmlRootFormatProvider::getScriptInner();
        s_.replace("{millisecond}", "1000");
        s_ += F("function updateData(json) {\n");
        s_ += F("  document.getElementById('RSSIValue').innerHTML = json.RSSI + \"dBm\" \n");
        s_ += F("  document.getElementById('DateTimeValue').innerHTML = json.time\n");
        s_ += F("  document.getElementById('twilightValue').innerHTML = json.twilight\n");
        s_ += F("}\n");
        return s_;
    }
};

void onProgress(size_t prg, size_t sz) {
    static size_t lastPrinted = 0;
    size_t currentPercent = (prg * 100) / sz;

    if (currentPercent % 5 == 0 && currentPercent != lastPrinted) {
        Serial.printf("Progress: %d%%\n", currentPercent);
        lastPrinted = currentPercent;
    }
}

void handleData(AsyncWebServerRequest* request) {
    // Get the current time
    time_t now_ = time(nullptr);
    struct tm* timeinfo_ = localtime(&now_);
    char timeStr_[20];
    char dateStr_[20];
    strftime(timeStr_, sizeof(timeStr_), "%H:%M:%S", timeinfo_);
    strftime(dateStr_, sizeof(dateStr_), "%Y-%m-%d", timeinfo_);
    int currentBrightness_ = 4095 - analogRead(PIN_ADC2_CH2);

    // Create the JSON string
    String json_ = "{";
    json_ += "\"RSSI\":\"" + String(WiFi.RSSI()) + "\"";
    json_ += ",\"time\":\"" + String(timeStr_) + "\"";
    json_ += ",\"date\":\"" + String(dateStr_) + "\"";
    json_ += ",\"twilight\":\"" + String(currentBrightness_) + "\"";
    json_ += "}";

    // Send the JSON response
    request->send(200, "application/json", json_);
}

void handleRoot(AsyncWebServerRequest* request) {
    MyHtmlRootFormatProvider rootFormatProvider;

    String response_ = "";
    response_ += rootFormatProvider.getHtmlHead(iotWebConf.getThingName());
    response_ += rootFormatProvider.getHtmlStyle();
    response_ += rootFormatProvider.getHtmlHeadEnd();
    response_ += rootFormatProvider.getHtmlScript();

    response_ += rootFormatProvider.getHtmlTable();
    response_ += rootFormatProvider.getHtmlTableRow() + rootFormatProvider.getHtmlTableCol();

    response_ += F("<fieldset align=left style=\"border: 1px solid\">\n");
    response_ += F("<table border=\"0\" align=\"center\" width=\"100%\">\n");
    response_ += F("<tr><td align=\"left\"><span id=\"DateTimeValue\">not valid</span></td></td><td align=\"right\"><span id=\"RSSIValue\">-100</span></td></tr>\n");
    response_ += rootFormatProvider.getHtmlTableEnd();
    response_ += rootFormatProvider.getHtmlFieldsetEnd();

    response_ += rootFormatProvider.getHtmlFieldset("Twilight");
    response_ += rootFormatProvider.getHtmlTable();
    response_ += rootFormatProvider.getHtmlTableRowVar("Sensor value:", "twilightValue");
    response_ += rootFormatProvider.getHtmlTableEnd();
    response_ += rootFormatProvider.getHtmlFieldsetEnd();

    response_ += rootFormatProvider.getHtmlFieldset("Network");
    response_ += rootFormatProvider.getHtmlTable();
    response_ += rootFormatProvider.getHtmlTableRowText("MAC Address:", WiFi.macAddress());
    response_ += rootFormatProvider.getHtmlTableRowText("IP Address:", WiFi.localIP().toString().c_str());
    response_ += rootFormatProvider.getHtmlTableEnd();
    response_ += rootFormatProvider.getHtmlFieldsetEnd();

    response_ += rootFormatProvider.addNewLine(2);

    response_ += rootFormatProvider.getHtmlTable();
    //response_ += rootFormatProvider.getHtmlTableRowSpan("Time:", "not valid", "DateTimeValue");
    response_ += rootFormatProvider.getHtmlTableRowText("Go to <a href = 'config'>configure page</a> to change configuration.");
    response_ += rootFormatProvider.getHtmlTableRowText(rootFormatProvider.getHtmlVersion(Version));
    response_ += rootFormatProvider.getHtmlTableEnd();

    response_ += rootFormatProvider.getHtmlTableColEnd() + rootFormatProvider.getHtmlTableRowEnd();
    response_ += rootFormatProvider.getHtmlTableEnd();
    response_ += rootFormatProvider.getHtmlEnd();

    request->send(200, "text/html", response_);
}

void convertParams() {
}

void configSaved() {
    convertParams();
    ParamsChanged = true;
}

NTPSettings_t getNTPSettings() {
    NTPSettings_t ntpSettings_;
    ntpSettings_.useNTPServer = NTPSettingsGroup.useNTPServer();
    ntpSettings_.ntpServer = NTPSettingsGroup.getNTPServer();
    ntpSettings_.timeZone = NTPSettingsGroup.getTimeZone();
    return ntpSettings_;
}

ScreenSaverSettings_t getScreenSaverSettings() {
    ScreenSaverSettings_t screenSaverSettings_;
    screenSaverSettings_.twilightThreshold = ScreenSaverGroup.getThreshold();
    screenSaverSettings_.offTime = ScreenSaverGroup.getOffTime();
    screenSaverSettings_.onTime = ScreenSaverGroup.getOnTime();
    screenSaverSettings_.offTimeMinutes = ScreenSaverGroup.getOffTimeMinutes();
    return screenSaverSettings_;
}