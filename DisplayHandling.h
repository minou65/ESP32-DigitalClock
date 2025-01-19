// DisplayHandling.h

#ifndef _DISPLAYHANDLING_h
#define _DISPLAYHANDLING_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif

void initDisplay();

void setScreensaverScheduler(const char* onTime, const char* offTime);
void setScreensaverThreshold(int threshold);
