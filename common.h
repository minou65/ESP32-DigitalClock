// common.h

#ifndef _COMMON_h
#define _COMMON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "neotimer.h"

#define STRING_LEN 50
#define NUMBER_LEN 6

#define PIN_ADC2_CH2 6

extern char Version[];

extern bool ParamsChanged;

extern bool UseNTPServer;
extern String NTPServer;
extern String TimeZone;


extern char vBacklightOffTime[6];
extern char vBacklightOnTime[6];

#endif

