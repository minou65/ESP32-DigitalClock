#define LV_CONF_INCLUDE_SIMPLE


#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <time.h> 

#include <lvgl.h>

#include "common.h"
#include "lv_conf.h"
#include "lvgl_port_v8.h"
#include "lv_font_robertomonobold_128.h"
#include "version.h"
#include "ESP32Time.h"
#include "webhandling.h"
#include "version.h"
#include "ntp.h"


// Extend IO Pin define
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5     // USB select pin

#define PIN_ADC2_CH2 6   //Define the pin macro 

lv_obj_t* text_label_time_value;
lv_obj_t* text_label_date_value;

static lv_style_t style_time_label;
static lv_style_t style_date_label;

char Version[] = VERSION_STR; // Manufacturer's Software version code

void lv_create_main_ui(void) {
    // Create a text label aligned center: https://docs.lvgl.io/master/widgets/label.html
    text_label_time_value = lv_label_create(lv_scr_act());
    lv_label_set_text(text_label_time_value, "HH:mm:ss");
    lv_obj_align(text_label_time_value, LV_ALIGN_CENTER, 0, -20); // Adjust the y-offset to make space for the date label

    // Set font type and font size. More information: https://docs.lvgl.io/master/overview/font.html
    lv_style_init(&style_time_label);
    lv_style_set_text_font(&style_time_label, &lv_font_robertomonobold_128);
    lv_style_set_text_color(&style_time_label, lv_color_white());
    lv_obj_add_style(text_label_time_value, &style_time_label, 0);

    // Create a date label below the time label
    text_label_date_value = lv_label_create(lv_scr_act());
    lv_label_set_text(text_label_date_value, "YYYY-MM-DD");
    lv_obj_align(text_label_date_value, LV_ALIGN_CENTER, 0, 100); // Adjust the y-offset to position below the time label

    // Set font type and font size for the date label
    lv_style_init(&style_date_label);
    lv_style_set_text_font(&style_date_label, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_date_label, lv_color_white());
    lv_obj_add_style(text_label_date_value, &style_date_label, 0);

    // Set background color to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lv_style_set_text_opa(&style_time_label, 150); // Setze die Helligkeit auf 100
    lv_style_set_text_opa(&style_date_label, 150); // Setze die Helligkeit auf 100
}

//void lv_create_analog_clock(void) {
//	// Create an analog clock: https://docs.lvgl.io/master/widgets/clock.html
//	lv_obj_t* clock = lv_clock_create(lv_scr_act(), NULL);
//	lv_obj_align(clock, LV_ALIGN_CENTER, 0, 0);
//}

void lv_label_set_datetime(lv_timer_t* timer) {

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

    char dateStr[30];
    strftime(dateStr, sizeof(dateStr), "%A, %d.%m.%Y", timeinfo);

    lv_label_set_text(text_label_time_value, timeStr);
    lv_label_set_text(text_label_date_value, dateStr);

    uint16_t adcValue_ = analogRead(PIN_ADC2_CH2);
    uint16_t brightness_ = map(adcValue_, 0, 4095, 255, 100);

	lv_style_set_text_opa(&style_time_label, brightness_); 
	lv_style_set_text_opa(&style_date_label, brightness_);
}

void setup(){
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println("Starting with Firmware " + String(VERSION));
    String LVGL_Arduino_ = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino_);

    wifiInit();
    NTPInit();

    /**
     * These development boards require the use of an IO expander to configure the screen,
     * so it needs to be initialized in advance and registered with the panel for use.
     */
    Serial.println("Initialize IO expander");
    
    /* Initialize IO expander */
    ESP_IOExpander_CH422G *expander = new ESP_IOExpander_CH422G((i2c_port_t)I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    expander->init();
    expander->begin();

    Serial.println("Set the IO0-7 pin to output mode.");
    expander->enableAllIO_Output();
    expander->digitalWrite(TP_RST , HIGH);
    expander->digitalWrite(LCD_RST , HIGH);
    expander->digitalWrite(LCD_BL , HIGH);
    delay(100);
    
    // GT911 initialization, must be added, otherwise the touch screen will not be recognized  
    // Initialization begin
    expander->digitalWrite(TP_RST , LOW);
    delay(100);
    digitalWrite(GPIO_INPUT_IO_4, LOW);
    delay(100);
    expander->digitalWrite(TP_RST , HIGH);
    delay(200);
    // Initialization end

    Serial.println("Initialize panel device");
    ESP_Panel *panel = new ESP_Panel();
    panel->init();
    
#if LVGL_PORT_AVOID_TEAR
    // When avoid tearing function is enabled, configure the RGB bus according to the LVGL configuration
    ESP_PanelBus_RGB *rgb_bus = static_cast<ESP_PanelBus_RGB *>(panel->getLcd()->getBus());
    rgb_bus->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
    rgb_bus->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
#endif

    panel->begin();

    Serial.println("Initialize LVGL");
    lvgl_port_init(panel->getLcd(), panel->getTouch());

    Serial.println("Create UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    lv_create_main_ui();

    lv_timer_t* timer_time_ = lv_timer_create(lv_label_set_datetime, 1000, NULL);
    lv_timer_ready(timer_time_);

    /* Release the mutex */
    lvgl_port_unlock();


    analogReadResolution(12);  //Set ADC resolution to 12 bits (0-4096)

    
	Serial.println("Setup done");
}

void updateDateTime() {
    // Hole die aktuelle Zeit
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    // Formatiere die Zeit als "HH:mm:ss"
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

    // Formatiere das Datum als "Wochentag, dd.mm.yyyy"
    char dateStr[30];
    strftime(dateStr, sizeof(dateStr), "%A, %d.%m.%Y", timeinfo);

    // Setze die Texte der Labels
    lv_label_set_text(text_label_time_value, timeStr);
    lv_label_set_text(text_label_date_value, dateStr);
}

void loop(){
 
	//updateDateTime();

    wifiLoop();
    if (iotWebConf.getState() == iotwebconf::OnLine) {
        NTPloop();
    }
    else {

    }

    if (ParamsChanged) {

    }

    ParamsChanged = false;




}
