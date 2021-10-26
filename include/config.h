#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stdint.h"

int configInit(int addressOfDevice);
/*read preset*/
int sony_loadRegisters(char* filename);
/*read fmodData.txt and store fmod*/
int initModFreq();
/*get the register addresses and values to configure a modulation frequency*/
int changeModFreq(uint8_t freq);
/*change the sensor integration time*/
int changeIntegration(uint32_t time_ns);
/*adjust the distance offset*/
void changeDistanceOffset(int offset_cm);

#endif
