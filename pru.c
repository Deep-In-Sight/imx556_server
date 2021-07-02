#include "pru.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include "xdma.h"

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define DCS_NUM 4

static XDma dma;

/// \addtogroup pru
/// @{
#define PRU_DBG
#ifdef PRU_DBG
#define PRU_ENTER printf("Enter %s\n", __FUNCTION__)
#define PRU_EXIT printf("Exit %s\n", __FUNCTION__)
#else
#define PRU_ENTER
#define PRU_EXIT
#endif

static int fd_mem;
static unsigned int ddr_map_base_addr;
static unsigned ddr_map_size;
static uint16_t* pMem;
static uint32_t* pMem32;
static unsigned char* pData;

static unsigned int deviceAddress;

int pruInit(const unsigned int addressOfDevice) {

	pData = (unsigned char*) malloc(1 * sizeof(unsigned char));
	pData[0] = 0x01;

	FILE* fd_map;
	deviceAddress = addressOfDevice;
	unsigned int str_len = 11;
	char line[str_len];

	int Status;
	Status = XDma_Initialize(&dma, "dma");
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

	size_t framesize = DCS_NUM * FRAME_HEIGHT * FRAME_WIDTH * 2;
	XDma_Set_s2mm_offset(&dma, ddr_map_base_addr);
	XDma_Set_len(&dma, framesize);

	//put sensor on streaming mode, need to start dma to consume the first frame
	XDma_Start(&dma);
	i2c(deviceAddress, 'w', 0x1001, 1, &pData);

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

	return 0;
}


int pruGetImage(uint16_t **data) {
	//double elapsedTime;
	//struct timeval tv1, tv2;
	//gettimeofday(&tv1, NULL);

	PRU_ENTER;

	XDma_Start(&dma);
	i2c(deviceAddress, 'w', 0x2100, 1, &pData);
	usleep(10000);

//	FILE *fp;
//	fp = fopen("frame.gray", "w");
//	if (!fp) {
//		printf("Cannot save frame\n");
//	}
//	fwrite(pMem, nRowsPerHalf * nHalves * nCols * nDCS, 1, fp);
//	fclose(fp);
//	while(!XDma_IsDone(&dma)){
//		//wastefully spending time doing nothing here
//	}
	PRU_EXIT;

	//gettimeofday(&tv2, NULL);
	//elapsedTime = (double)(tv2.tv_sec - tv1.tv_sec) + (double)(tv2.tv_usec - tv1.tv_usec)/1000000.0;
	//printf("seconds elapsed in ms = %2.4f\n", elapsedTime *1000.0);

	*data = pMem;
	int size = FRAME_WIDTH * FRAME_HEIGHT * DCS_NUM;

	return size;		//number of pixels
}

/*!
 Releases data used by the PRU
 @return On success, 0 is returned. On error, -1 is returned.
 */
int pruRelease() {	//free???
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

	XDma_Release(&dma);

	return 0;
}

/// @}
