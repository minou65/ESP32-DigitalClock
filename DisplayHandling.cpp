
#define LV_CONF_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE

#include <ESP_Panel_Library.h>
#include <lvgl.h>

#include "lv_conf.h"
#include "lvgl_port_v8.h"
#include "lv_font_robertomonobold_128.h"
#include "lv_font_montserrat_32_umlaut.h"
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

// letters for the Berndeutsch word clock
const char* letters[10][11] = {
    {"E", "S", "K", "I", "S", "C", "H", "A", "F", "Ü", "F"},
    {"V", "I", "E", "R", "T", "U", "B", "F", "Z", "Ä", "Ä"},
    {"Z", "W", "Ä", "N", "Z", "G", "S", "I", "V", "O", "R"},
    {"A", "B", "O", "H", "A", "U", "B", "I", "E", "G", "E"},
    {"E", "I", "S", "Z", "W", "Ö", "I", "S", "D", "R", "Ü"},
    {"V", "I", "E", "R", "I", "F", "Ü", "F", "I", "Q", "T"},
    {"S", "Ä", "C", "H", "S", "I", "S", "I", "B", "N", "I"},
    {"A", "C", "H", "T", "I", "N", "Ü", "N", "I", "E", "L"},
    {"Z", "Ä", "N", "I", "E", "R", "B", "E", "U", "F", "I"},
    {"Z", "W", "Ö", "U", "F", "I", "N", "A", "U", "H", "R"}
};

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

class WordClock {
public:
    WordClock(lv_obj_t* parent) : _parent(parent) {
        // Create a matrix of letters
        createLetterMatrix();
		createDotMatrix();

        // Create a timer to update the time every second
        lv_timer_t* timer_ = lv_timer_create(setdatetime, 1000, this);
    }

    void updateDateTime() {
        // Get the current time
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);

        // Update the letter matrix to highlight the current time
        updateLetterMatrix(timeinfo_);
		updateDotMatrix(timeinfo_);

    }

private:
    lv_obj_t* _parent;
    lv_obj_t* _letterMatrix[10][11]; // Adjust the size according to your display
    lv_obj_t* _dotMatrix[4]; // Array to hold the dots
    lv_color_t _lightGray = lv_color_hex(0xAAAAAA);
    lv_color_t _transparentGray = lv_color_mix(_lightGray, lv_color_black(), LV_OPA_40); // 30% opacity

    static void setdatetime(lv_timer_t* timer) {
        WordClock* clock = static_cast<WordClock*>(timer->user_data);
        clock->updateDateTime();
    }

    void createLetterMatrix() {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 11; ++j) {
                _letterMatrix[i][j] = lv_label_create(_parent);
                lv_label_set_text(_letterMatrix[i][j], letters[i][j]);
                lv_obj_align(_letterMatrix[i][j], LV_ALIGN_CENTER, j * 40 - 200, i * 40 - 200); // Adjust the position
                lv_obj_set_style_text_color(_letterMatrix[i][j], _transparentGray, 0);
                lv_obj_set_style_text_font(_letterMatrix[i][j], &lv_font_montserrat_32_umlaut, 0);
                //lv_obj_set_style_text_font(_letterMatrix[i][j], &lv_font_montserrat_32, 0);
            }
        }
    }


    void createDotMatrix() {
        // Create a dot in each corner of the screen
        _dotMatrix[0] = lv_obj_create(_parent);
        lv_obj_set_size(_dotMatrix[0], 10, 10);
        lv_obj_align(_dotMatrix[0], LV_ALIGN_TOP_LEFT, 20, 20); 
        lv_obj_set_style_bg_color(_dotMatrix[0], lv_color_white(), 0);
        lv_obj_add_flag(_dotMatrix[0], LV_OBJ_FLAG_HIDDEN);

        _dotMatrix[1] = lv_obj_create(_parent);
        lv_obj_set_size(_dotMatrix[1], 10, 10);
        lv_obj_align(_dotMatrix[1], LV_ALIGN_TOP_RIGHT, -20, 20); 
        lv_obj_set_style_bg_color(_dotMatrix[1], lv_color_white(), 0);
        lv_obj_add_flag(_dotMatrix[1], LV_OBJ_FLAG_HIDDEN);

        _dotMatrix[2] = lv_obj_create(_parent);
        lv_obj_set_size(_dotMatrix[2], 10, 10);
        lv_obj_align(_dotMatrix[2], LV_ALIGN_BOTTOM_LEFT, 20, -20); 
        lv_obj_set_style_bg_color(_dotMatrix[2], lv_color_white(), 0);
        lv_obj_add_flag(_dotMatrix[2], LV_OBJ_FLAG_HIDDEN);

        _dotMatrix[3] = lv_obj_create(_parent);
        lv_obj_set_size(_dotMatrix[3], 10, 10);
        lv_obj_align(_dotMatrix[3], LV_ALIGN_BOTTOM_RIGHT, -20, -20); 
        lv_obj_set_style_bg_color(_dotMatrix[3], lv_color_white(), 0);
        lv_obj_add_flag(_dotMatrix[3], LV_OBJ_FLAG_HIDDEN);
    }

    void updateDotMatrix(struct tm* timeinfo) {
        int minute_ = timeinfo->tm_min;

        // Hide all dots initially
        for (int i = 0; i < 4; ++i) {
            lv_obj_add_flag(_dotMatrix[i], LV_OBJ_FLAG_HIDDEN);
        }

        // Determine which dots to show based on the current minute
        int dotsToShow = minute_ % 5;
        for (int i = 0; i < dotsToShow; ++i) {
            lv_obj_clear_flag(_dotMatrix[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    void updateLetterMatrix(struct tm* timeinfo) {
        // Reset all letters to light gray
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 11; ++j) {
                lv_obj_set_style_text_color(_letterMatrix[i][j], _transparentGray, 0);
            }
        }

        int hour_ = timeinfo->tm_hour;
        int minute_ = timeinfo->tm_min;

        // Convert 24-hour format to 12-hour format
        if (hour_ == 0) {
            hour_ = 12; // Midnight case
        }
        else if (hour_ > 12) {
            hour_ -= 12; // Convert to PM
        }

        if (minute_ >= 25) {
            hour_ = (hour_ % 12) + 1;
        }

        // Highlight the letters for Es isch in white
        highlightLetters({ {0,0}, {0,1}, {0,3}, {0,4}, {0,5}, {0,6} });

        // Highlight the letters for the current minutes in white
        if (minute_ == 0) {
            // No additional letters to highlight for "o'clock"
        }
        else if (minute_ >= 5 && minute_ < 10) {
            highlightLetters({ {0,8}, {0,9}, {0,10}, {3,0}, {3,1} });
        }
        else if (minute_ >= 10 && minute_ < 15) {
            highlightLetters({ {1,8}, {1,9}, {1,10}, {3,0}, {3,1} });
        }
        else if (minute_ >= 15 && minute_ < 20) {
            highlightLetters({ {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {3,0}, {3,1} });
        }
        else if (minute_ >= 20 && minute_ < 25) {
            highlightLetters({ {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {3,0}, {3,1} });
        }
        else if (minute_ >= 25 && minute_ < 30) {
            highlightLetters({ {0,8}, {0,9}, {0,10}, {2,8}, {2,9}, {2,10}, { 3,3 }, {3,4}, {3,5}, {3,6}, {3,7} });
        }
        else if (minute_ >= 30 && minute_ < 35) {
            highlightLetters({ {3,3}, {3,4}, {3,5}, {3,6}, {3,7} });
        }
        else if (minute_ >= 35 && minute_ < 40) {
            highlightLetters({ {0,8}, {0,9}, {0,10}, {3,0}, {3,1}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7} });
        }
        else if (minute_ >= 40 && minute_ < 45) {
            highlightLetters({ {2,0}, {2,1}, {2,2}, { 2,3 }, {2,4}, {2,5}, {2,8}, {2,9}, {2,10} });
        }
        else if (minute_ >= 45 && minute_ < 50) {
            highlightLetters({ {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {2,8}, {2,9}, {2,10} });
        }
        else if (minute_ >= 50 && minute_ < 55) {
            highlightLetters({ {1,8}, {1,9}, {1,10}, {2,8}, {2,9}, {2,10} });
        }
        else if (minute_ >= 55 && minute_ < 60) {
            highlightLetters({ {0,8}, {0,9}, {0,10}, {2,8}, {2,9}, {2,10} });
        }

        // Highlight the letters for the current hour in white
        if (hour_ == 0 || hour_ == 12) {
            highlightLetters({ {9,0}, {9,1}, {9,2}, {9,3}, {9,4}, {9,5} });
        }
        else if (hour_ == 1) {
            highlightLetters({ {4,0}, {4,1}, {4,2} });
        }
        else if (hour_ == 2) {
            highlightLetters({ {4,3}, {4,4}, {4,5}, {4,6} });
        }
        else if (hour_ == 3) {
            highlightLetters({ {4,8}, {4,9}, {4,10} });
        }
        else if (hour_ == 4) {
            highlightLetters({ {5,0}, {5,1}, {5,2}, {5,3}, {5,4} });
        }
        else if (hour_ == 5) {
            highlightLetters({ {5,5}, {5,6}, {5,7}, {5,8} });
        }
        else if (hour_ == 6) {
            highlightLetters({ {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, {6,5} });
        }
        else if (hour_ == 7) {
            highlightLetters({ {6,6}, {6,7}, {6,8}, {6,9}, {6,10} });
        }
        else if (hour_ == 8) {
            highlightLetters({ {7,0}, {7,1}, {7,2}, {7,3}, {7,4} });
        }
        else if (hour_ == 9) {
            highlightLetters({ {7,5}, {7,6}, {7,7}, {7,8} });
        }
        else if (hour_ == 10) {
            highlightLetters({ {8,0}, {8,1}, {8,2}, {8,3} });
        }
        else if (hour_ == 11) {
            highlightLetters({ {8,7}, {8,8}, {8,9}, {8,10} });
        }
    }

    void highlightLetters(const std::vector<std::pair<int, int>>& positions) {
        for (const auto& pos : positions) {
            lv_obj_set_style_text_color(_letterMatrix[pos.first][pos.second], lv_color_white(), 0);
        }
    }
};

class BinaryClock {
public:
    BinaryClock(lv_obj_t* parent) : _parent(parent) {
        createBinaryMatrix();

        // Create a timer to update the time every second
        lv_timer_t* timer_ = lv_timer_create(setdatetime, 1000, this);
    }

    void updateDateTime() {
        time_t now_ = time(nullptr);
        struct tm* timeinfo_ = localtime(&now_);

        int hour_ = timeinfo_->tm_hour;
        int minute_ = timeinfo_->tm_min;
        int second_ = timeinfo_->tm_sec;

        updateBinaryMatrix(hour_, minute_, second_);
    }

private:
    lv_obj_t* _parent;
    lv_obj_t* _binaryMatrix[3][6]; // 3 rows for hours, minutes, and seconds, 6 columns for binary digits (32, 16, 8, 4, 2, 1)

    static void setdatetime(lv_timer_t* timer) {
        BinaryClock* clock = static_cast<BinaryClock*>(timer->user_data);
        clock->updateDateTime();
    }

    void createBinaryMatrix() {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 6; ++j) {
                _binaryMatrix[i][j] = lv_obj_create(_parent);
                lv_obj_set_size(_binaryMatrix[i][j], 20, 20);
                lv_obj_align(_binaryMatrix[i][j], LV_ALIGN_CENTER, j * 30 - 75, i * 30 - 45); // Adjust the position
                lv_obj_set_style_bg_color(_binaryMatrix[i][j], lv_color_black(), 0);
            }
        }
    }

    void updateBinaryMatrix(int hour, int minute, int second) {
        int values[3] = { hour, minute, second };

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 6; ++j) {
                if (values[i] & (1 << (5 - j))) {
                    lv_obj_set_style_bg_color(_binaryMatrix[i][j], lv_color_white(), 0);
                }
                else {
                    lv_obj_set_style_bg_color(_binaryMatrix[i][j], lv_color_black(), 0);
                }
            }
        }
    }
};

//class AnalogClock {
//public:
//    AnalogClock(int w = 300, int h = 300) : _meter(nullptr) {
//        _meter = lv_meter_create(_parent);
//        lv_obj_set_size(_meter, w, h);
//        lv_obj_center(_meter);
//
//        lv_meter_scale_t* scale_min_ = lv_meter_add_scale(_meter);
//        lv_meter_set_scale_ticks(_meter, scale_min_, 61, 1, 10, lv_palette_main(LV_PALETTE_GREY));
//        lv_meter_set_scale_range(_meter, scale_min_, 0, 60, 360, 270);
//
//        lv_meter_scale_t* scale_hour_ = lv_meter_add_scale(_meter);
//        lv_meter_set_scale_ticks(_meter, scale_hour_, 12, 0, 0, lv_palette_main(LV_PALETTE_GREY));
//        lv_meter_set_scale_major_ticks(_meter, scale_hour_, 1, 2, 20, lv_color_black(), 10);
//        lv_meter_set_scale_range(_meter, scale_hour_, 1, 12, 330, 300);
//
//        LV_IMG_DECLARE(img_hand);
//
//        lv_meter_indicator_t* indic_min_ = lv_meter_add_needle_img(_meter, scale_min_, &img_hand, 5, 5);
//        lv_meter_indicator_t* indic_hour_ = lv_meter_add_needle_img(_meter, scale_hour_, &img_hand, 5, 5);
//
//        lv_anim_t a_;
//        lv_anim_init(&a_);
//        lv_anim_set_exec_cb(&a_, set_value);
//        lv_anim_set_values(&a_, 0, 60);
//        lv_anim_set_repeat_count(&a_, LV_ANIM_REPEAT_INFINITE);
//        lv_anim_set_time(&a_, 2000);
//        lv_anim_set_var(&a_, indic_min_);
//        lv_anim_start(&a_);
//
//        lv_anim_set_var(&a_, indic_hour_);
//        lv_anim_set_time(&a_, 24000);
//        lv_anim_set_values(&a_, 0, 12);
//        lv_anim_start(&a_);
//    }
//
//    void align(lv_align_t align, int x_ofs, int y_ofs) {
//        if (_meter != nullptr) {
//            lv_obj_align(_meter, align, x_ofs, y_ofs);
//        }
//    }
//
//private:
//    lv_obj_t* _meter;
//
//    static void set_value(void* indic, int32_t v) {
//        lv_meter_set_indicator_end_value(static_cast<lv_obj_t*>(indic), v);
//    }
//};

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
    static_cast<ESP_PanelBus_RGB*>(lcd_bus_)->configRgbBounceBufferSize(ESP_PANEL_LCD_WIDTH * 40);
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
    //DigitalClock* clock = new DigitalClock(screen_);
	WordClock* clock = new WordClock(screen_);
	//BinaryClock* clock = new BinaryClock(screen_);

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