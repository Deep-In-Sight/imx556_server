#ifndef _API_H_
#define _API_H_

#include <stdint.h>

int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress);
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress);
int16_t apiTest(int val);

#endif
