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
#include <sys/syscall.h>
#include <pthread.h>
#include <stdint.h>

#include "xgetphasemap.h"
#include "profile.h"
#include "queue.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define DCS_NUM 4
#define DCS_SZ (640*480)
#define FRAME_SZ (640*480*2*5)
#define BUFFER_DEPTH 2

static XGetphasemap phaseAccel;
static enum IMAGE_MODE imageMode = MODE_RAW;
static int scaleMode = 0;

static int autoTrigger = 0;

/// \addtogroup pru
/// @{
//#define ACCEL_DBG
#ifdef ACCEL_DBG
#define ACCEL_ENTER printf("Enter %s\n", __FUNCTION__)
#define ACCEL_EXIT printf("Exit %s\n", __FUNCTION__)
#define ACCEL_LINE printf("%s at %d\n", __FUNCTION__, __LINE__)
#else
#define ACCEL_ENTER
#define ACCEL_EXIT
#define ACCEL_LINE
#endif

static int fd_mem;

static uint64_t ddr_map_base_addr;
static uint64_t ddr_map_size;
static unsigned int buffer_depth;
static uint16_t* pMem;
static uint32_t* pMem32;
static unsigned char* pData;

static unsigned int deviceAddress;

static uint32_t* pMonitor = NULL;

void printMonitor() {
	printf("Valid count: %d\n", pMonitor[0]);
	printf("Last count: %d\n", pMonitor[1]);
	printf("User count: %d\n", pMonitor[2]);
	printf("Ready count: %d\n", pMonitor[3]);
}


int accelInit(const uint8_t addressOfDevice) {

	pData = (unsigned char*) malloc(1 * sizeof(unsigned char));
	pData[0] = 0x01;

	FILE* fd_map;
	deviceAddress = addressOfDevice;
	unsigned int str_len = 19;
	char line[str_len];

	int Status;
	Status = XGetphasemap_Initialize(&phaseAccel, "getPhaseMap2");
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
	ddr_map_base_addr = strtoll(line, NULL, 0);

	fd_map = fopen("/sys/class/uio/uio0/maps/map1/size", "r");
	if (fd_map == NULL) {
		perror("Failed to open a file for dumping data\n");
		return -1;
	}
	fgets(line, str_len, fd_map);
	fflush(stdin);
	fclose(fd_map);
	ddr_map_size = strtoll(line, NULL, 0);
#ifdef BUFFER_DEPTH
	buffer_depth = BUFFER_DEPTH;
#else
	buffer_depth = ddr_map_size / FRAME_SZ;
#endif
	printf("Initialized tof accelerator with buffer depth: %d\n", buffer_depth);


	/* Open the file for the memory device: */
	fd_mem = open("/dev/mem", O_RDWR | O_SYNC); //slow
//			fd_mem = open("/dev/mem", O_RDWR); // horizontal bars in picture :(
	if (fd_mem < 0) {
		perror("\x1b[31m" "Failed to open /dev/mem (%s)\n" "\x1b[0m");
		return -1;
	}

	printf("ddr_map_base_addr = %llx, ddr_map_size = %llx\n", ddr_map_base_addr, ddr_map_size);
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

	/*
	pMonitor = (uint32_t*)mmap(0, 4*sizeof(uint32_t), PROT_WRITE | PROT_READ, MAP_SHARED, fd_mem,
			0x44a00000);
	if ((pMonitor) == MAP_FAILED) {
		perror("\x1b[31m" "Failed to map the device\n." "\x1b[0m");
		close(fd_mem);
		return -1;
	}

	if (mlock(pMonitor, 16) != 0) { // Makes sure that the used pages stay in the memory.
		perror("\x1b[31m" "mlock failed" "\x1b[0m");
		close(fd_mem);
		return -1;
	}*/

	printf("Memory mapped successfully from %llx to %llx\n", ddr_map_base_addr, (long long unsigned int)pMem);
	//printf("getPhaseMap monitor memory mapped successfully from %x to %x\n", (uint32_t)0x44a00000, (uint32_t)pMonitor);

	XGetphasemap_Set_frame02_offset(&phaseAccel, ddr_map_base_addr);
	XGetphasemap_Set_frame13_offset(&phaseAccel, ddr_map_base_addr);

//	XGetphasemap_Set_frame13_offset(&phaseAccel, ddr_map_base_addr + DCS_SZ*2*sizeof(uint16_t));
	XGetphasemap_InterruptGlobalEnable(&phaseAccel);
	XGetphasemap_InterruptEnable(&phaseAccel, XGETPHASEMAP_CONTROL_INTERRUPTS_DONE_MASK);


	//put sensor on streaming mode, need to start dma to consume the first frame
	XGetphasemap_Start(&phaseAccel);
	i2c(deviceAddress, 'w', 0x1001, 1, &pData);

	ACCEL_LINE;
	XGetphasemap_IsDonePoll(&phaseAccel, 50);

	//printMonitor();

	return 0;
}

void accelSetVideoMode(int isVideo) {
	autoTrigger = isVideo;

	if (isVideo) {
		pData[0] = (0x01 << 3);
	} else {
		pData[0] = 0;
	}

	i2c(deviceAddress, 'w', 0x2100, 1, &pData);
}

int accelSetMode(uint8_t mode) {
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

void accelSetPhaseOffset(uint16_t phaseOffset) {
	uint32_t regCtrl = XGetphasemap_Get_regCtrl(&phaseAccel);
	regCtrl &= 0x0000FFFF;
	regCtrl |= (((uint32_t)phaseOffset) << 16);
	XGetphasemap_Set_regCtrl(&phaseAccel, regCtrl);
}

void accelEnableAmplitudeScale(uint8_t scale_en) {
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

void getNextFrameSlot(uint16_t** virt, uint32_t* phys) {
	static int frameCnt = 0;

	uint64_t buffer_base_virt = (uint64_t) pMem;
	uint64_t buffer_base_phys = ddr_map_base_addr;

	buffer_base_virt = buffer_base_virt + FRAME_SZ * frameCnt;
	buffer_base_phys = buffer_base_phys + FRAME_SZ * frameCnt;
	frameCnt = (frameCnt + 1) % buffer_depth;

	*virt = (uint16_t*) buffer_base_virt;
	*phys = buffer_base_phys;
}

int accelGetImage(uint16_t **data) {
	uint16_t* frameVirtAddr = NULL;
	uint32_t framePhysAddr;
	//pid_t tid = syscall(__NR_gettid);
	//static int icount_last = 0;
	//int icount = 0;

	ACCEL_ENTER;

	__TIC__(GET_FRAME)
//	printMonitor();

	getNextFrameSlot(&frameVirtAddr, &framePhysAddr);

	XGetphasemap_Set_frame02_offset(&phaseAccel, framePhysAddr);
	XGetphasemap_Set_frame13_offset(&phaseAccel, framePhysAddr);

	//XGetphasemap_Set_frame13_offset(&phaseAccel, framePhysAddr + DCS_SZ*2*sizeof(uint16_t));
//	XGetphasemap_InterruptEnable(&phaseAccel, XGETPHASEMAP_CONTROL_INTERRUPTS_DONE_MASK);
//	XGetphasemap_InterruptClear(&phaseAccel, XGETPHASEMAP_CONTROL_INTERRUPTS_DONE_MASK);
//	usleep(1000);

	XGetphasemap_Start(&phaseAccel);
//	if (!autoTrigger) {
//		pData[0] = 0x01;
//		i2c(deviceAddress, 'w', 0x2100, 1, &pData);
//	}
	pData[0] = 0x01;
	i2c(deviceAddress, 'w', 0x2100, 1, &pData);

//	while(!XGetphasemap_IsDone(&phaseAccel)){
//		//wastefully spending time doing nothing here
//		printf("[TID %d] Sensor not done yet\n", tid);
//	}

//	ACCEL_LINE;

	XGetphasemap_IsDonePoll(&phaseAccel, 30000);

//	int err = read(phaseAccel.uio_fd, &icount, 4);
//	if (err != 4) {
//		printf("interrupt wait failed\n");
//	}
//	if (icount > icount_last) {
//		printf("Received %d interrupts\n", icount - icount_last);
//		icount_last = icount;
//	}

	int size, buffer_offset;

	if (imageMode == MODE_RAW) {
		size = 4 * DCS_SZ * sizeof(uint16_t);
		buffer_offset = 0;
	} else if (imageMode == MODE_AMP) {
		size = DCS_SZ * sizeof(uint8_t);
		buffer_offset = DCS_SZ*4;
	} else {
		size = DCS_SZ * sizeof(uint16_t);
		buffer_offset = DCS_SZ*3;
	}
	*data = frameVirtAddr + buffer_offset;

//	printMonitor();
	__TOC__(GET_FRAME)
	ACCEL_EXIT;
	return size;		//number of pixels
}

int accelGetBufferDepth() {
	return buffer_depth;
}

int accelSetAmplitudeThreshold(int threshold) {
	ACCEL_ENTER;
	XGetphasemap_Set_threshold(&phaseAccel, threshold);
	ACCEL_EXIT;
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
