
#define LV_CONF_INCLUDE_SIMPLE

#include <ESP_Panel_Library.h>
#include <lvgl.h>

#include "lv_conf.h"
#include "lvgl_port_v8.h"
#include "lv_font_robertomonobold_128.h"
#include "DisplayHandling.h"
#include "neotimer.h"

// Extend IO Pin define
#define TP_RST 1      // Touch screen reset pin
#define LCD_BL 2      // LCD backlight pinout
#define LCD_RST 3     // LCD reset pin
#define SD_CS 4       // SD card select pin
#define USB_SEL 5     // USB select pin
#define PIN_ADC2_CH2 6

// I2C pin definitions
#define I2C_MASTER_NUM I2C_NUM_0    // I2C Master Number
#define I2C_MASTER_SDA_IO 8         // I2C data line pins
#define I2C_MASTER_SCL_IO 9         // I2C clock line pin

ESP_PanelBacklight* backlight;
ESP_Panel* panel;
ESP_IOExpander* expander;

// Array mit deutschen Wochentagen 
const char* weekdays[] = { "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag" };

class DigitalClock {
public:
    DigitalClock(lv_obj_t* parent) : _parent(parent) {
        // create a label for the time
        _timelabel = lv_label_create(_parent);
        lv_label_set_text(_timelabel, "HH:mm:ss");
        lv_obj_align(_timelabel, LV_ALIGN_CENTER, 0, -20); // Adjust the y-offset to make space for the date label
        lv_obj_set_style_text_color(_timelabel, lv_color_white(), 0);
        lv_obj_set_style_text_font(_timelabel, &lv_font_robertomonobold_128, 0);

        // create a label for the date
        _datelabel = lv_label_create(_parent);
        lv_label_set_text(_datelabel, "Weekday, DD.MM.YYYY");
        lv_obj_align(_datelabel, LV_ALIGN_CENTER, 0, 100); // Adjust the y-offset to position below the time label
        lv_obj_set_style_text_color(_datelabel, lv_color_white(), 0);
        lv_obj_set_style_text_font(_datelabel, &lv_font_montserrat_48, 0);

        // Create a timer to update the time every second
        lv_timer_t* timer_ = lv_timer_create(setdatetime, 1000, this);
    }

    void updateDateTime() {
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);

        char timeStr_[9];
        strftime(timeStr_, sizeof(timeStr_), "%H:%M:%S", timeinfo_);

        char dateStr_[30];
        strftime(dateStr_, sizeof(dateStr_), "%d.%m.%Y", timeinfo_);

        // Get the weekday from the timeinfo struct and use it to index into the weekdays array
        const char* weekdayStr_ = weekdays[timeinfo_->tm_wday];

        String fullDateStr_ = String(weekdayStr_) + ", " + dateStr_;

        // Set the texts of the labels
        lv_label_set_text(_timelabel, timeStr_);
        lv_label_set_text(_datelabel, dateStr_);
    }

private:
    lv_obj_t* _parent;
    lv_obj_t* _timelabel;
    lv_obj_t* _datelabel;

    static void setdatetime(lv_timer_t* timer) {
        DigitalClock* clock = static_cast<DigitalClock*>(timer->user_data);
        clock->updateDateTime();
    }
};

class Screensaver {
public:
    Screensaver(lv_obj_t* parent, int analogPin, ESP_IOExpander* expander) : _analogPin(analogPin), _expander(expander) {
        setOnTime("08:00");
        setOffTime("20:00");
        setBrightnessThreshold(1000);
        setBacklightOffTime(1);

        // Initialisiere den Timer
        lv_timer_create(timerCallback, 10000, this);

        createTransparentPanel(parent);

        // Setze den Callback für Touch-Events
        lv_obj_add_event_cb(_transparentPanel, touch_event_callback, LV_EVENT_PRESSED, this);


    }

    void setOnTime(const char* onTime) {
        _onTime = parseTime(onTime);
    }

    void setOffTime(const char* offTime) {
        _offTime = parseTime(offTime);
    }

    void setBrightnessThreshold(int threshold) {
        _brightnessThreshold = threshold;
    }

    void setBacklightOffTime(int minutes) {
        _backlightOffTimer.start(minutes * 60 * 1000);
    }

    bool isBrightnessAboveThreshold() {
        int maxADCValue = 4095;
        int currentBrightness_ = maxADCValue - analogRead(_analogPin);
        //Serial.print("Brightness: "); Serial.println(currentBrightness_);
        //Serial.print("Threshold: "); Serial.println(_brightnessThreshold);

        if (_brightnessAboveThreshold) {
            if (currentBrightness_ < (_brightnessThreshold - 100)) {
                _brightnessAboveThreshold = false;
            }
        }
        else {
            if (currentBrightness_ > _brightnessThreshold) {
                _brightnessAboveThreshold = true;
            }
        }
        return _brightnessAboveThreshold;
    }

    bool isSchedulerActive() {
        time_t now_ = time(nullptr);
        struct tm* currentTime_ = localtime(&now_);
        time_t currentSeconds_ = currentTime_->tm_hour * 3600 + currentTime_->tm_min * 60 + currentTime_->tm_sec;
        time_t onTimeSeconds_ = _onTime % 86400; // Sekunden seit Mitternacht
        time_t offTimeSeconds_ = _offTime % 86400; // Sekunden seit Mitternacht

        //// Hilfsfunktion zum Formatieren der Zeit
        //auto formatTime = [](time_t seconds) {
        //    int hours = seconds / 3600;
        //    int minutes = (seconds % 3600) / 60;
        //    char buffer[6];
        //    snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
        //    return String(buffer);
        //    };

        //Serial.print("Current time: "); Serial.println(formatTime(currentSeconds_));
        //Serial.print("On time: "); Serial.println(formatTime(onTimeSeconds_));
        //Serial.print("Off time: "); Serial.println(formatTime(offTimeSeconds_));

        if (onTimeSeconds_ < offTimeSeconds_) {
            return currentSeconds_ >= onTimeSeconds_ && currentSeconds_ < offTimeSeconds_;
        }
        else {
            return currentSeconds_ >= onTimeSeconds_ || currentSeconds_ < offTimeSeconds_;
        }
    }

    bool isTouchTimerActive() {
        return !_backlightOffTimer.done();
    }

private:
    int _analogPin;
    time_t _onTime;
    time_t _offTime;
    int _brightnessThreshold;
    bool _brightnessAboveThreshold = false;
    Neotimer _backlightOffTimer = Neotimer(60 * 1000); // minutes in milliseconds
    ESP_IOExpander* _expander;
    lv_obj_t* _transparentPanel;

    static void timerCallback(lv_timer_t* timer) {
        Screensaver* handler_ = static_cast<Screensaver*>(timer->user_data);
        bool result_ = false;
        if (handler_->isBrightnessAboveThreshold()) {
            result_ = true;
        }
        else if (handler_->isSchedulerActive()) {
            result_ = true;
        }
        else if (handler_->isTouchTimerActive()) {
            result_ = true;
        }

        if (result_) {
            handler_->backlightOn();
        }
        else {
            handler_->backlightOff();
        }
    }

    static void touch_event_callback(lv_event_t* event) {
        Screensaver* handler_ = static_cast<Screensaver*>(event->user_data);
        handler_->_backlightOffTimer.start();
        handler_->backlightOn();
    }

    void backlightOn() {
        lv_obj_add_flag(_transparentPanel, LV_OBJ_FLAG_HIDDEN);
        _expander->digitalWrite(LCD_BL, HIGH);
    }

    void backlightOff() {
        _expander->digitalWrite(LCD_BL, LOW);
        lv_obj_clear_flag(_transparentPanel, LV_OBJ_FLAG_HIDDEN);
    }

    time_t parseTime(const char* timeStr) {
        struct tm timeinfo;
        if (sscanf(timeStr, "%2d:%2d", &timeinfo.tm_hour, &timeinfo.tm_min) != 2) {
            Serial.println("Invalid time format");
            return 0;
        }
        //Serial.print("Parsed time: "); Serial.println(timeinfo.tm_hour);
        //Serial.print("Parsed time: "); Serial.println(timeinfo.tm_min);

        timeinfo.tm_sec = 0;
        timeinfo.tm_year = 70; // 1970
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 1;

        return timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60;
    }

    void createTransparentPanel(lv_obj_t* parent) {
        _transparentPanel = lv_obj_create(parent);
        lv_obj_set_size(_transparentPanel, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_opa(_transparentPanel, LV_OPA_50, 0); // 50% Transparenz
        lv_obj_set_style_bg_color(_transparentPanel, lv_color_black(), 0);
        lv_obj_add_flag(_transparentPanel, LV_OBJ_FLAG_HIDDEN); // Standardmäßig versteckt
    }
};
Screensaver* screensaver;

void initDisplay() {
    String LVGL_Arduino_ = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino_);

    panel = new ESP_Panel();
    panel->init();
#if LVGL_PORT_AVOID_TEAR
    // When avoid tearing function is enabled, configure the bus according to the LVGL configuration
    ESP_PanelBus* lcd_bus_ = panel->getLcd()->getBus();
#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
    static_cast<ESP_PanelBus_RGB*>(lcd_bus_)->configRgbBounceBufferSize(ESP_PANEL_LCD_WIDTH * 20);
    static_cast<ESP_PanelBus_RGB*>(lcd_bus_)->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#elif ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI
    static_cast<ESP_PanelBus_DSI*>(lcd_bus_)->configDpiFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
#endif
    panel->begin();

    expander = panel->getExpander();

    Serial.println("Initialize LVGL");
    lvgl_port_init(panel->getLcd(), panel->getTouch());

    lvgl_port_lock(-1);

	Serial.println("Create the screen");
    lv_obj_t* screen_ = lv_scr_act();

    // Set background color to black
    lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);

	Serial.println("Create the screensaver");
    screensaver = new Screensaver(screen_, PIN_ADC2_CH2, expander);

	Serial.println("Create the clock");
    DigitalClock* clock = new DigitalClock(screen_);

    /* Release the mutex */
    lvgl_port_unlock();

};


void setScreensaverScheduler(const char* onTime, const char* offTime) {
    screensaver->setOnTime(onTime);
    screensaver->setOffTime(offTime);
}

void setScreensaverThreshold(int threshold) {
    screensaver->setBrightnessThreshold(threshold);
}