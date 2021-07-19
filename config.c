#include <getPhaseMapAccel.h>
#include "config.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


/// \addtogroup configuration
/// @{

/*!
 Initializes the configuration.
 @param deviceAddress i2c address of the chip (default 0x20)
 @returns On success, 0, on error -1
 */
int configInit(int deviceAddress) {
	int status;

	status = sony_loadRegisters("imx556_standard.cfg", deviceAddress);
	if (status < 0)
		return -1;

	status = accelInit(deviceAddress);
	if (status < 0)
		return -1;

	accelSetMode(MODE_PHASE);
	accelSetOffset(0);
	accelEnableAmplitudeScale(0);

	return 0;
}

int sony_loadRegisters(char* filename, const int deviceAddress) {
	FILE *configFile;
	char line[16];
	int regs = 0;
	uint16_t a;
	uint8_t v[0];
	uint8_t *pv = v;

	configFile = fopen(filename, "r");
	if (!configFile) {
		printf("Cannot open %s to read config\n", filename);
		return -1;
	}

	while (fgets(line, sizeof(line), configFile)) {
		char *pch;
		pch = strtok(line, " ");
		if (!pch)
			return -1;
		a = strtol(pch, NULL, 16);
		pch = strtok(NULL, " ");
		if (!pch)
			return -1;
		v[0] = strtol(pch, NULL, 16);
		if (i2c(deviceAddress, 'w', a, 1, &pv) < 0)
			return -1;
		regs++;
	}
	printf("Configured %d register\n", regs);
	return 0;
}
