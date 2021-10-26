#ifndef _API_H_
#define _API_H_

#include <stdint.h>

int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char **values, const int deviceAddress);
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char **values, const int deviceAddress);
int apiSetMode(int mode);
void apiSetPhaseOffset(uint16_t phaseOffset);
void apiEnableAmplitudeScale(int scale_en);
int apiChangeModFreq(int freq);
int apiChangeIntegration(int time_ns);

int16_t apiTest(int val);

#endif
