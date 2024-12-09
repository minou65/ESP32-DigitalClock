// ntp.h

#ifndef _NTP_h
#define _NTP_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void NTPInit();
void NTPloop();

#endif

