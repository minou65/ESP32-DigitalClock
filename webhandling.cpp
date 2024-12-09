// 
// 
// 

#define DEBUG_WIFI(m) SERIAL_DBG.print(m)

#include <Arduino.h>
#include <ArduinoOTA.h>

#include <time.h>
#include "ESP32Time.h"

#include <DNSServer.h>
#include<iostream>
#include <string.h>

#include <IotWebConf.h>
#include <IotWebConfESP32HTTPUpdateServer.h>
#include "common.h"
#include "webhandling.h"
#include "favicon.h"
#include <IotWebRoot.h>

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
void handleRoot();
void handleFavIcon();
void handleReboot();
void convertParams();

// -- Callback methods.
void configSaved();
void wifiConnected();

bool ParamsChanged = true;

DNSServer dnsServer;
WebServer server(80);

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

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

char vUseNTPServer[STRING_LEN];
char vNTPServer[STRING_LEN];
char vTimeZone[STRING_LEN];
iotwebconf::ParameterGroup TimeSourceGroup = iotwebconf::ParameterGroup("TimeSourceGroup", "Time Source");
iotwebconf::CheckboxParameter cUseNTPServer = iotwebconf::CheckboxParameter("Use NTP server", "UseNTPServerParam", vUseNTPServer, STRING_LEN, true);
iotwebconf::TextParameter cNTPServerAddress = iotwebconf::TextParameter("NTP server (FQDN or IP address)", "NTPServerParam", vNTPServer, STRING_LEN, "pool.ntp.org");
iotwebconf::TextParameter cgmtOffset_sec = iotwebconf::TextParameter("POSIX timezones string", "TimeOffsetParam", vTimeZone, STRING_LEN, "CET-1CEST,M3.5.0,M10.5.0/3");

HTTPUpdateServer httpUpdater;

void wifiInit() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("starting up...");

    iotWebConf.setConfigPin(CONFIG_PIN);

    iotWebConf.setHtmlFormatProvider(&customHtmlFormatProvider);

    TimeSourceGroup.addItem(&cUseNTPServer);
    TimeSourceGroup.addItem(&cNTPServerAddress);
    TimeSourceGroup.addItem(&cgmtOffset_sec);

    iotWebConf.addParameterGroup(&TimeSourceGroup);

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
        [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
        [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });

    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    iotWebConf.getApTimeoutParameter()->visible = true;

    // -- Initializing the configuration.
    iotWebConf.init();

    convertParams();

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.on("/favicon.ico", [] { handleFavIcon(); });
    server.on("/config", [] { iotWebConf.handleConfig(); });
    server.on("/reboot", HTTP_GET, []() { handleReboot(); });
    server.onNotFound([]() { iotWebConf.handleNotFound(); });

    Serial.println("Ready.");
}

void wifiLoop() {
    // -- doLoop should be called as frequently as possible.
    iotWebConf.doLoop();
    ArduinoOTA.handle();
}

void wifiConnected(){
    ArduinoOTA.begin();
}

void handleFavIcon() {
   server.send_P(200, "image/x-icon", favicon, sizeof(favicon));
}

void handleReboot() {
    String _message;

    // redirect to the root page after 15 seconds
    _message += "<HEAD><meta http-equiv=\"refresh\" content=\"15;url=/\"></HEAD>\n<BODY><p>\n";
    _message += "Rebooting...<br>\n";
    _message += "Redirected after 15 seconds...\n";
    _message += "</p></BODY>\n";

    server.send(200, "text/html", _message);
    ESP.restart();
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

        return s_;
    }
};

void handleRoot() {
    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal()){
        // -- Captive portal request were already served.
        return;
    }

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

    server.send(200, "text/html", response_);
}

void convertParams() {
}

void configSaved() {
    convertParams();
    ParamsChanged = true;
}