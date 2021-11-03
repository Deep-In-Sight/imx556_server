#include "getPhaseMapAccel.h"
#include "config.h"
#include "i2c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

static int initialized = false;
static unsigned int deviceAddress;

static const int minFreq = 4;
static const int maxFreq = 100;
static const int numRegsPerFreq = 10;
static const int numFreq = (maxFreq - minFreq + 1);
static i2cReg modSettings[numFreq * numRegsPerFreq];
static uint8_t i2cVal[1];

static uint8_t currentFreq = 24;

/*!
 Initializes the configuration.
 @param deviceAddress i2c address of the chip
 @returns On success, 0, on error -1
 */
int configInit(int addressOfDevice) {
	int status;

	deviceAddress = addressOfDevice;

	initialized = 0;

	status = sony_loadRegisters("imx556_standard.cfg");
	if (status < 0)
		return -1;

	status = accelInit(deviceAddress);
	if (status < 0)
		return -1;

	accelSetMode(MODE_PHASE);
	accelSetPhaseOffset(0);
	accelEnableAmplitudeScale(0);

	status = initModFreq();
	if (status < 0)
		return -1;

	initialized = 1;
	return 0;
}

int configGetStatus() {
	return initialized? 0 : -1;
}

int sony_loadRegisters(char* filename) {
	FILE *configFile;
	char line[16];
	int regs = 0;
	uint16_t a;
	uint8_t v[0];
	uint8_t *pv = v;

	configFile = fopen(filename, "r");
	if (!configFile) {
		printf("Cannot open %s to read config\n", filename);
		char cwd[256];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			printf("Current working dir: %s\n", cwd);
		} else {
			perror("getcwd() error");
			return 1;
		}
		exit(-1);
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


int initModFreq() {
	char* md5sum = "3df3357d73396f421fbdf3fb8dac77a2";
	char md5sumRet[256];
	char* ptoken;
	FILE *fp;

	system("md5sum freqmod.txt > md5sum.txt");
	fp = fopen("md5sum.txt", "r");
	if (!fp) {
		printf("Cannot check md5sum\n");
		return -1;
	}
	fgets(md5sumRet, sizeof(md5sumRet), fp);
	ptoken = strtok(md5sumRet, " ");
	if (strcmp(ptoken, md5sum)) {
		printf("freqmod.txt is corrupted ;;");
		return -1;
	}
	fclose(fp);
	system("rm md5sum.txt");

	fp = fopen("freqmod.txt", "r");
	if (!fp) {
		printf("Cannot load modulation frequencies\n");
		return -1;
	}

	//in freqmod.txt, from top to bottom, F=max to min.
	for (int freqIdx = 0; freqIdx < numFreq; freqIdx++) {
		i2cReg* regs = &modSettings[freqIdx*numRegsPerFreq];
		for (int regIdx = 0; regIdx < numRegsPerFreq; regIdx++) {
			int a, v;
			fscanf(fp, "%d %d\n", &a, &v);
			regs[regIdx].addr = (uint16_t) a;
			regs[regIdx].val = (uint8_t) v;
		}
	}
	fclose(fp);

	return 0;
}

int changeModFreq(uint8_t freq) {
	i2cReg* regs;
	uint8_t* pVal = i2cVal;

	if (freq < minFreq || freq > maxFreq)
		return -1;
	else {
		currentFreq = freq;

		regs = &modSettings[(maxFreq-freq)*numRegsPerFreq];
		printf("changing modulation frequency:\n");
		for (int regIdx = 0; regIdx < numRegsPerFreq; regIdx++) {
			uint16_t a = regs[regIdx].addr;
			i2cVal[0] = regs[regIdx].val;
			printf("a=0x%04x v=0x%02x\n", a, i2cVal[0]);
			i2c(deviceAddress, 'w', a, 1, &pVal);
		}
		return 0;
	}
}

int changeIntegration(uint32_t time_ns) {
    int clk120;
    uint8_t* pVal = i2cVal;

    if (time_ns < 250 || time_ns > 1000000)
    	return -1;

    clk120 = (int)(time_ns/8.3);

    //4registers/phase x 4phases, starting from 0x2120
    printf("changing integration time:\n");
    for (int i = 0; i < 16; i++) {
        int byteshift = 3-(i%4);
        uint16_t a = 0x2120 + i;
        i2cVal[0] = (clk120 >> (byteshift*8)) & 0xFF;
        printf("a=0x%04x v=0x%02x\n", a, i2cVal[0]);
        i2c(deviceAddress, 'w', a, 1, &pVal);
    }
    return 0;
}

void changeDistanceOffset(int offset_cm) {
	float range_cm = 300.0/(2*currentFreq)*100;
	//phase is shifted from 0:2pi to 0:65535
	int offsetInt = (int) (offset_cm / range_cm * 65535);
	uint16_t phaseOffset = (uint16_t)(offsetInt & 0xFFFF);
	printf("changing phaseoffset to %d\n", phaseOffset);
	accelSetPhaseOffset(phaseOffset);
}
