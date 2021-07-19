#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <getPhaseMapAccel.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include "xgetphasemap.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define DCS_NUM 4
#define DCS_SZ (640*480)

static XGetphasemap phaseAccel;
enum IMAGE_MODE imageMode = MODE_RAW;
int scaleMode = 0;
int frameIdx = 0;

/// \addtogroup pru
/// @{
#define ACCEL_DBG
#ifdef ACCEL_DBG
#define ACCEL_ENTER printf("Enter %s\n", __FUNCTION__)
#define ACCEL_EXIT printf("Exit %s\n", __FUNCTION__)
#else
#define ACCEL_ENTER
#define ACCEL_EXIT
#endif

static int fd_mem;
static unsigned int ddr_map_base_addr;
static unsigned ddr_map_size;
static uint16_t* pMem;
static uint32_t* pMem32;
static unsigned char* pData;

static unsigned int deviceAddress;

int accelInit(const unsigned int addressOfDevice) {

	pData = (unsigned char*) malloc(1 * sizeof(unsigned char));
	pData[0] = 0x01;

	FILE* fd_map;
	deviceAddress = addressOfDevice;
	unsigned int str_len = 11;
	char line[str_len];

	int Status;
	Status = XGetphasemap_Initialize(&phaseAccel, "dma");
	if (Status != XST_SUCCESS) {
		printf("Dma initialize failed\n");
		return -1;
	}

	/* Load start address and size of memory mapping: */
	fd_map = fopen("/sys/class/uio/uio0/maps/map1/addr", "r");
	if (fd_map == NULL) {
		perror("Failed to open a file for dumping data\n");
		return -1;
	}
	fgets(line, str_len, fd_map);
	fflush(stdin);
	fclose(fd_map);
	ddr_map_base_addr = (unsigned int) strtoll(line, NULL, 0);

	fd_map = fopen("/sys/class/uio/uio0/maps/map1/size", "r");
	if (fd_map == NULL) {
		perror("Failed to open a file for dumping data\n");
		return -1;
	}
	fgets(line, str_len, fd_map);
	fflush(stdin);
	fclose(fd_map);
	ddr_map_size = (unsigned int) strtoll(line, NULL, 0);

	/* Open the file for the memory device: */
	fd_mem = open("/dev/mem", O_RDWR | O_SYNC); //slow
//			fd_mem = open("/dev/mem", O_RDWR); // horizontal bars in picture :(
	if (fd_mem < 0) {
		perror("\x1b[31m" "Failed to open /dev/mem (%s)\n" "\x1b[0m");
		return -1;
	}

	pMem = (uint16_t*)mmap(0, ddr_map_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd_mem,
			ddr_map_base_addr);
	pMem32 = (uint32_t*) pMem;

	if ((pMem) == MAP_FAILED) {
		perror("\x1b[31m" "Failed to map the device\n." "\x1b[0m");
		close(fd_mem);
		return -1;
	}

	if (mlock(pMem, ddr_map_size) != 0) { // Makes sure that the used pages stay in the memory.
		perror("\x1b[31m" "mlock failed" "\x1b[0m");
		close(fd_mem);
		return -1;
	}

	printf("Memory mapped successfully from %x to %x\n", (uint32_t)ddr_map_base_addr, (uint32_t)pMem);

	XGetphasemap_Set_frame02_offset(&phaseAccel, ddr_map_base_addr);
	XGetphasemap_Set_frame13_offset(&phaseAccel, ddr_map_base_addr + DCS_SZ*2*sizeof(uint16_t));

	//put sensor on streaming mode, need to start dma to consume the first frame
	XGetphasemap_Start(&phaseAccel);
	i2c(deviceAddress, 'w', 0x1001, 1, &pData);

	return 0;
}

int accelSetMode(int mode) {
	if (mode > MODE_NUM) {
		return -1;
	} else {
		imageMode = (IMAGE_MODE)mode;

		uint32_t regCtrl = XGetphasemap_Get_regCtrl(&phaseAccel);
		regCtrl &= 0xFFFFFFFC;
		regCtrl |= mode;
		XGetphasemap_Set_regCtrl(&phaseAccel, regCtrl);
		return 0;
	}
}

void accelSetOffset(uint16_t offset) {
	uint32_t regCtrl = XGetphasemap_Get_regCtrl(&phaseAccel);
	regCtrl &= 0x0000FFFF;
	regCtrl |= (((uint32_t)offset) << 16);
	XGetphasemap_Set_regCtrl(&phaseAccel, regCtrl);
}

void accelEnableAmplitudeScale(int scale_en) {
	scaleMode = scale_en;
	uint32_t SCALE_EN_BIT = (0x01 << 2);
	uint32_t regCtrl = XGetphasemap_Get_regCtrl(&phaseAccel);
	if (scale_en) {
		regCtrl |= SCALE_EN_BIT;
	} else {
		regCtrl &= ~SCALE_EN_BIT;
	}
	XGetphasemap_Set_regCtrl(&phaseAccel, regCtrl);
}

int accelGetImage(uint16_t **data) {
	//double elapsedTime;
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);

	ACCEL_ENTER;

	XGetphasemap_Start(&phaseAccel);
	i2c(deviceAddress, 'w', 0x2100, 1, &pData);

//	while(!XGetphasemap_IsDone(&phaseAccel)){
//		//wastefully spending time doing nothing here
//	}


	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("seconds elapsed in ms = %2.4f\n", elapsedTime *1000.0);

	int size, offset;

	if (imageMode == MODE_RAW) {
		size = 4 * DCS_SZ;
		offset = 0;
	} else if (imageMode == MODE_AMP) {
		size = DCS_SZ;
		if (scaleMode) {
			offset = DCS_SZ*4*sizeof(uint16_t);
		} else {
			offset = DCS_SZ*3*sizeof(uint16_t);
		}
	} else if (imageMode == MODE_PHASE) {
		size = DCS_SZ;
		offset = DCS_SZ*3*sizeof(uint16_t);
	}
	*data = (uint16_t*)(((unsigned int)pMem) + offset);

	ACCEL_EXIT;
	return size;		//number of pixels
}

/*!
 Releases data used by the PRU
 @return On success, 0 is returned. On error, -1 is returned.
 */
int accelRelease() {	//free???
	free(pData);

	if (munlock(pMem, ddr_map_size) != 0) {
		perror("\x1b[31m" "Unlocking memory failed" "\x1b[0m");
		return -1;
	}
	if (munmap(pMem, ddr_map_size) < 0) {
		perror("\x1b[31m" "Unmapping memory failed." "\x1b[0m");
		return -1;
	}
	if (close(fd_mem) != 0) {
		printf("\x1b[31m" "Closing fd_mem failed." "\x1b[0m");
		return -1;
	}

	XGetphasemap_Release(&phaseAccel);

	return 0;
}

/// @}
