#include <getPhaseMapAccel.h>
#include "api.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/// \addtogroup api
/// @{

#define API_ENTER printf("+++Enter %s\n", __FUNCTION__)
#define API_EXIT printf("---Exit %s\n", __FUNCTION__)


/*!
 Reads Bytes from the registers over I2C
 @param registerAddress address of the register it starts to read from
 @param nBytes number of Bytes to read
 @param values pointer to the array where values will be stored to
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - reading ROI (6Bytes) -> readRegister 96 6   or   read 96 6   or   r 96 6
 - reading 1 Byte from Register AE -> read AE 1   or   read AE
 */
int16_t apiReadRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	API_ENTER;

	if (i2c(deviceAddress, 'r', registerAddress, nBytes, &values) > 0) {
		return nBytes;
	}
	API_EXIT;
	return -1;
}

/*!
 Writes Bytes into the registers over I2C
 @param registerAddress address of the register it starts to write from
 @param nBytes number of Bytes to write
 @param values pointer to the array where values are stored in
 @param deviceAddress I2C address of the chip (default 0x20)
 @returns On success, 0, on error -1
 @note
 usage with server:
 - writing default ROI (6Bytes) -> writeRegister 96 00 04 01 43 06 70    or   write 96 ...   or   w 96 6 ...
 - writing 1 Byte to Register AE -> write AE 1   or   write AE
 */
int16_t apiWriteRegister(const int registerAddress, const int nBytes, unsigned char *values, const int deviceAddress) {
	API_ENTER;

	i2c(deviceAddress, 'w', registerAddress, nBytes, &values);
	API_EXIT;
	return nBytes;
}

int apiSetMode(int mode) {
	API_ENTER;
	int rc = accelSetMode(mode);
	API_EXIT;
	return rc;
}

void apiSetPhaseOffset(uint16_t offset) {
	API_ENTER;
	accelSetOffset(offset);
	API_EXIT;
}

void apiEnableAmplitudeScale(int scale_en) {
	API_ENTER;
	accelEnableAmplitudeScale(scale_en);
	API_EXIT;
}

int16_t apiTest(int val){
	API_ENTER;


	printf("test... \n");

	return val;
}

/// @}

