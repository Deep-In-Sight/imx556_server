#ifndef PRU_H_
#define PRU_H_

#include "config.h"
#include <stdint.h>

enum IMAGE_MODE {
	MODE_RAW = 0,
	MODE_AMP,
	MODE_PHASE,
	MODE_NUM
};

int accelInit(const unsigned int addressOfDevice);
int accelSetMode(int mode);
void accelSetOffset(uint16_t offset);
void accelEnableAmplitudeScale(int scale_en);
int accelGetImage(uint16_t **data);
int accelRelease();

#endif
