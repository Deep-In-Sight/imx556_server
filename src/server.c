
#include "server.h"
#include "api.h"
#include "helper.h"

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <getPhaseMapAccel.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/reboot.h>


#include "queue.h"
#include "profile.h"

auto start_time = std::chrono::steady_clock::now();
auto end_time = std::chrono::steady_clock::now();

typedef enum {
	MODE_VIDEO = 0,
	MODE_PICTURE
} IMAGING_MODE;

static pthread_t imageThread;
static IMAGING_MODE gImagingMode = MODE_PICTURE;
static int gVideoRunning = 0;
static int gVideoStopped = 1;


size_t handleRequest(char* cmdline, char** pData);
void signalHandler(int sig);
void serverSend(void* buffer, size_t size);

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

static unsigned int deviceAddress;
unsigned char* pData;

void* imagingThread(void*){
	uint16_t* frameAddr=NULL;
	gVideoStopped = 0;
	frame_desc_t desc;
	uint32_t dataSize = 0;

	while(gVideoRunning){
		__TIC_SUM__(GET_DATA)
			dataSize = accelGetImage(&frameAddr);
		__TOC_SUM__(GET_DATA)
		desc.addr = frameAddr;
		desc.size = dataSize;
		enqueue(desc);
	};

	gVideoStopped = 1;
	printf("imagingThread: gVideoStopped = %d\n", gVideoStopped);	//TODO remove
}

void serverStopThread() {
	gVideoRunning = 0;
	frame_desc_t e;

	//in case thread is still waiting to enqueue lasst frame
	if (isQueueFull()){
		dequeue(&e);
	}
	printf("thread join start\n");
	pthread_join(imageThread, NULL);
	printf("thread joined\n");
	queueDestroy();
	printf("queued destroyed\n");
}
/*!
 Starts TCP Server for communication with client application
 @param addressOfDevice i2c address of the chip (default 0x20)
 @return On error, -1 is returned.
 */
int startServer(const unsigned int addressOfDevice) {

	char rxBuffer[TCP_BUFFER_SIZE];
	char* txBuffer;

	deviceAddress = addressOfDevice;
	
	struct sigaction new_act;
	new_act.sa_handler = signalHandler;
	sigemptyset(&new_act.sa_mask);
	new_act.sa_flags = 0;

	sigaction(SIGINT, &new_act, NULL);
	sigaction(SIGQUIT, &new_act, NULL);
	sigaction(SIGPIPE, &new_act, NULL);
	sigaction(SIGSEGV, &new_act, NULL);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	int optval = 1;
	// set SO_REUSEADDR on a socket to true
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval)
			< 0) {
		printf("Error setting socket option\n");
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(TCP_PORT_NO);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {// bind server address to socket
		printf(
				"Binding error: please try again in 20sec and make sure no other instance is running\n");
		return -1;
	}
	printf("Socket successfully opened\n");
	listen(sockfd, 5);	// tell socket that we want to listen for communication
	clilen = sizeof(cli_addr);

	// handle single connection
	printf("waiting for requests\n");
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) {
		printf("couldn't accept\n");
		return -1;
	}

	while (1) { //?????
		//read request
		bzero(rxBuffer, TCP_BUFFER_SIZE);
		int ret = read(newsockfd, rxBuffer, TCP_BUFFER_SIZE);

		//reconnect if client disconnect
		if (!ret) {
			close(newsockfd);
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if (newsockfd < 0) {
				printf("couldn't accept\n");
				return -1;
			}
			continue;
		}

		//handle request
		size_t retSize = handleRequest(rxBuffer, &txBuffer);

		//write data/return
		if(retSize) {
			__TIC__(SEND_DATA);
			if (retSize == 640*480*2*4) {
			//hack the raw frame
				char* Idata = txBuffer + 640*480*2;
			  char* Qdata = txBuffer + 640*480*2*3;
			  size_t mapSize = 640*480*2;
			  send(newsockfd, Idata, mapSize, MSG_NOSIGNAL);
				send(newsockfd, Qdata, mapSize, MSG_NOSIGNAL);
			} else {
				send(newsockfd, txBuffer, retSize, MSG_NOSIGNAL);
			}
			__TOC__(SEND_DATA);
		}
	}

	close(newsockfd);
	close(sockfd);
	return 0;
}

/*!
 Handles Requests
 Args:
 	 cmd: request to be handled
 	 pData: data to send to client
 Return:
 	 size of data to be sent from pData
 */
size_t handleRequest(char* cmdline, char** pData) {
	//store the return code for short command
	static int16_t answer;
	//store the frame data for getFrame command
	uint16_t* pMem = NULL;
	//size of the data to write to socket
	size_t answerSize;
	//arguments list
	char arguments[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH];
	int argumentCount = helperParseCommand(cmdline, arguments);
	char* cmd = arguments[0];

	//COMMANDS
	if ((strcmp(cmd, "readRegister") == 0
			|| strcmp(cmd, "read") == 0
			|| strcmp(cmd, "r") == 0) && argumentCount == 1
			&& argumentCount < 3) {
		uint8_t readVal[1];
		uint8_t *values = readVal;

		uint16_t registerAddress = (uint16_t) helperStringToHex(arguments[1]);
		apiReadRegister(registerAddress, 1, &values, deviceAddress);
		answer = (int16_t)readVal[0];
		answerSize = 2;
	} else if ((strcmp(cmd, "writeRegister") == 0
			|| strcmp(cmd, "write") == 0
			|| strcmp(cmd, "w") == 0) && argumentCount == 2) {
		uint16_t registerAddress = (uint16_t) helperStringToHex(arguments[1]);
		uint8_t registerValues[1];
		uint8_t* pVal = registerValues;
		registerValues[0] =  (uint8_t) helperStringToHex(arguments[2]);

		answer = apiWriteRegister(registerAddress, 1, &pVal, deviceAddress);
		answerSize = 2;
	} else if (strcmp(cmd, "checkStatus") == 0 && !argumentCount) {
		answer = configGetStatus();
		answerSize = 2;
		printf("Checking status: %d\n", answer);
	} else if (strcmp(cmd, "reboot") == 0 && !argumentCount) {
		reboot(RB_AUTOBOOT);
	} else if (strcmp(cmd, "getFrame") == 0 && !argumentCount) {
		frame_desc_t desc;

		if (gImagingMode == MODE_VIDEO) {
			if (gVideoRunning == 0) {
				gVideoRunning = 1;
				int buffer_depth = accelGetBufferDepth();
				queueInit(buffer_depth);
				pthread_create(&imageThread, NULL, (void*(*)(void*))imagingThread, NULL);
			}

			//send 10 frames at a time ->> increase frame rate and reduce times on getFrame command
//			for (int i = 0; i < 10; i++) {
//			//block until there is data to send
//				dequeue(&desc);
//				pMem = desc.addr;
//				answerSize = desc.size;
//			}
			dequeue(&desc);
			pMem = desc.addr;
			answerSize = desc.size;
		} else {
			__TIC__(GET_DATA)
			answerSize = accelGetImage(&pMem);
			__TOC__(GET_DATA)
		}
	} else if (strcmp(cmd, "setMode") == 0 && argumentCount == 1) {
		int mode = helperStringToInteger(arguments[1]);
		answer = apiSetMode(mode);
		answerSize = 2;
	} else if (strcmp(cmd, "changeDistanceOffset") == 0 && argumentCount == 1) {
		int offset_cm = helperStringToInteger(arguments[1]);
		printf("Setting offset to %d cm\n", offset_cm);
		apiSetDistanceOffset(offset_cm);
		answer = 0;
		answerSize = 2;
	} else if (strcmp(cmd, "setAmplitudeScale") == 0 && argumentCount == 1) {
		int enable = helperStringToInteger(arguments[1]);
		apiEnableAmplitudeScale(enable);
		answer = 0;
		answerSize = 2;
	} else if (strcmp(cmd, "setAmplitudeThreshold") == 0 && argumentCount == 1) {
		int threshold = helperStringToInteger(arguments[1]);
		apiSetAmplitudeThreshold(threshold);
		answer = 0;
		answerSize = 2;
	} else if (strcmp(cmd, "startVideo") == 0 && !argumentCount) {
		__TIC_GLOBAL__(VIDEO);
		if (gImagingMode != MODE_VIDEO) {
			gImagingMode = MODE_VIDEO;
			answer = 0;
		} else {
			answer = -1;
		}
		answerSize = 2;
	} else if (strcmp(cmd, "stopVideo") == 0 && !argumentCount) {
		if (gImagingMode == MODE_VIDEO) {
			serverStopThread();
			gImagingMode = MODE_PICTURE;
			answer = 0;
		} else {
			answer = -1;
		}
		answerSize = 2;
		__TOC_GLOBAL__(VIDEO)
	} else if (strcmp(cmd, "changeModFreq") == 0 && argumentCount == 1) {
		int freq = helperStringToInteger(arguments[1]);
		answer = apiChangeModFreq(freq);
		answerSize = 2;
	} else if (strcmp(cmd, "changeIntegration") == 0 && argumentCount == 1) {
		int time_ns = helperStringToInteger(arguments[1]);
		answer = apiChangeIntegration(time_ns);
		answerSize = 2;
	}
	// unknown command
	else {
		printf("unknown command -> %s\n", cmd);
		answer = -1;
		answerSize = 2;
	}

	//short commands
	if (answerSize == 2) {
		*pData = (char*) (&answer);
	} else {
		*pData = (char*) pMem;
	}

	return answerSize;
}

/*!
 Handles signals
 @param sig signal ID
 */
void signalHandler(int sig) {
	//this signal can go f itself I dont know why it happen here
	//if (sig==33)
	//	return;
	printf("caught signal %i .... going to close application\n", sig);
	close(sockfd);
	close(newsockfd);
	exit(0);
}

