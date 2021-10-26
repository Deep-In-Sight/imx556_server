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

int accelInit(const uint8_t addressOfDevice);
int accelGetBufferDepth();
int accelSetMode(uint8_t mode);
void accelSetPhaseOffset(uint16_t phaseOffset);
void accelEnableAmplitudeScale(uint8_t scale_en);
int accelGetImage(uint16_t **data);
int accelRelease();

#endif
