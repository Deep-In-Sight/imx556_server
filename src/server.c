
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

#ifdef THREADSAFE_QUEUE
static SafeQueue<frame_desc_t>* q;
#endif

void handleRequest(int sock);
void signalHandler(int sig);
void serverSend(void* buffer, size_t size);

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

static unsigned int deviceAddress;
unsigned char* pData;
static int gSock = 0;

void* imagingThread(void*){
	uint16_t* frameAddr=NULL;
	gVideoStopped = 0;
	frame_desc_t desc;
	uint32_t dataSize = 0;

	while(gVideoRunning){
		__TIC_SUM__(GET_DATA)
			dataSize = 2 * accelGetImage(&frameAddr);
		__TOC_SUM__(GET_DATA)
		desc.addr = frameAddr;
		desc.size = dataSize;
#ifdef THREADSAFE_QUEUE
		q->enqueue(desc);
#else
		enqueue(desc);
#endif
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
#ifdef THREADSAFE_QUEUE
	delete q;
#else
	queueDestroy();
#endif
	printf("queued destroyed\n");
}
/*!
 Starts TCP Server for communication with client application
 @param addressOfDevice i2c address of the chip (default 0x20)
 @return On error, -1 is returned.
 */
int startServer(const unsigned int addressOfDevice) {
	deviceAddress = addressOfDevice;
	//signal(SIGQUIT, signalHandler); // add signal to signalHandler to close socket on crash or abort
	//signal(SIGINT, signalHandler);
	//signal(SIGPIPE, signalHandler);
	//signal(SIGSEGV, signalHandler);
	
	struct sigaction new_act, old_act;
	new_act.sa_handler = signalHandler;
	sigemptyset(&new_act.sa_mask);
	new_act.sa_flags = 0;

	sigaction(SIGINT, &new_act, NULL);
	sigaction(SIGQUIT, &new_act, NULL);
	sigaction(SIGPIPE, &new_act, NULL);
	sigaction(SIGSEGV, &new_act, NULL);

	pData = (unsigned char*)malloc(MAX_COMMAND_ARGUMENTS - 3 * sizeof(unsigned char));//free???
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

	while (1) { //?????
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			printf("couldn't accept\n");
			return -1;
		}
		handleRequest(newsockfd);
		close(newsockfd);
	}

	close(sockfd);
	return 0;
}

/*!
 Handles Requests and writes answer into TCP socket
 @param sock TCP-socket to write response.
 @return On error, -1 is returned.
 */
void handleRequest(int sock) {
//	bbbLEDEnable(3, 1);
	char buffer[TCP_BUFFER_SIZE];
	bzero(buffer, TCP_BUFFER_SIZE);
	int size = read(sock, buffer, TCP_BUFFER_SIZE);
	gSock = sock;
	if (buffer[size - 1] == '\n') {
		buffer[size - 1] = '\0';
	}
	char stringArray[MAX_COMMAND_ARGUMENTS][MAX_COMMAND_ARGUMENT_LENGTH];
	int argumentCount = helperParseCommand(buffer, stringArray);
	unsigned char response[TCP_BUFFER_SIZE];
	bzero(response, TCP_BUFFER_SIZE);
	int16_t answer;

	//COMMANDS
	if ((strcmp(stringArray[0], "readRegister") == 0
			|| strcmp(stringArray[0], "read") == 0
			|| strcmp(stringArray[0], "r") == 0) && argumentCount >= 1
			&& argumentCount < 3) {
		unsigned char *values;
		int v;
		int nBytes;

		if (argumentCount == 1) {
			nBytes = 1;
		} else {
			nBytes = helperStringToHex(stringArray[2]);
		}
		int registerAddress = helperStringToHex(stringArray[1]);

		if (nBytes > 0 && registerAddress >= 0) {
			values = (unsigned char*)malloc(nBytes * sizeof(unsigned char));
			int16_t responseValues[nBytes];
			apiReadRegister(registerAddress, nBytes, &values, deviceAddress);
			for (v = 0; v < nBytes; v++) {
				responseValues[v] = values[v];
			}
			printf("reg value: %d\n", responseValues[0]);
			send(sock, responseValues, nBytes * sizeof(int16_t), MSG_NOSIGNAL);
			free(values);
		} else {
			answer = -1;
			send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		}
	} else if ((strcmp(stringArray[0], "writeRegister") == 0
			|| strcmp(stringArray[0], "write") == 0
			|| strcmp(stringArray[0], "w") == 0) && argumentCount > 1) {
		unsigned char *values = (unsigned char*)malloc(
				(argumentCount - 1) * sizeof(unsigned char));
		int registerAddress = helperStringToHex(stringArray[1]);
		int i;
		int ok = 1;
		for (i = 0; i < argumentCount - 1; i++) {
			if (helperStringToHex(stringArray[i + 2]) >= 0) {
				values[i] = helperStringToHex(stringArray[i + 2]);
			} else {
				ok = 0;
			}
		}

		if (registerAddress > 0 && ok > 0) {
			answer = apiWriteRegister(registerAddress, argumentCount - 1,
					&values, deviceAddress);
		} else {
			answer = -1;
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		free(values);
	} else if (strcmp(stringArray[0], "getFrame") == 0 && !argumentCount) {
		frame_desc_t desc;
		uint16_t* pMem = NULL;

		if (gImagingMode == MODE_VIDEO) {
			if (gVideoRunning == 0) {
				gVideoRunning = 1;
				int buffer_depth = accelGetBufferDepth();
#ifdef THREADSAFE_QUEUE
				q = new SafeQueue<frame_desc_t>();
#else
				queueInit(buffer_depth);
#endif
				pthread_create(&imageThread, NULL, (void*(*)(void*))imagingThread, NULL);
			}

			//send 10 frames at a time ->> increase frame rate and reduce times on getFrame command
			for (int i = 0; i < 10; i++) {
			//block until there is data to send
#ifdef THREADSAFE_QUEUE
				desc = q->dequeue();
#else
				dequeue(&desc);
#endif
				__TIC_SUM__(SOCKET_SEND)
				send(sock, desc.addr, desc.size, MSG_NOSIGNAL);
				__TOC_SUM__(SOCKET_SEND)
			}
		} else {
			__TIC__(GET_DATA)
			int dataSize = 2 * accelGetImage(&pMem);
			__TOC__(GET_DATA)
			__TIC__(SOCKET_SEND)
			send(sock, pMem, dataSize, MSG_NOSIGNAL);
			__TOC__(SOCKET_SEND)
		}
	} else if (strcmp(stringArray[0], "setMode") == 0 && argumentCount == 1) {
		int mode = helperStringToHex(stringArray[1]);
		answer = apiSetMode(mode);
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
	} else if (strcmp(stringArray[0], "setPhaseOffset") == 0 && argumentCount == 1) {
		int offset = helperStringToInteger(stringArray[1]);
		printf("Setting offset to %d\n", offset);
		apiSetPhaseOffset(offset);
	} else if (strcmp(stringArray[0], "setAmplitudeScale") == 0 && argumentCount == 1) {
		int enable = helperStringToInteger(stringArray[1]);
		apiEnableAmplitudeScale(enable);
	} else if (strcmp(stringArray[0], "startVideo") == 0 && !argumentCount) {
		__TIC_GLOBAL__(VIDEO)
		gImagingMode = MODE_VIDEO;
		send(sock, &gImagingMode, sizeof(int16_t), MSG_NOSIGNAL);

	}else if (strcmp(stringArray[0], "stopVideo") == 0 && !argumentCount) {
		serverStopThread();
		gImagingMode = MODE_PICTURE;
		send(sock, &gImagingMode, sizeof(int16_t), MSG_NOSIGNAL);
		__TOC_GLOBAL__(VIDEO)
	}
	// unknown command
	else {
		printf("unknown command -> %s\n", stringArray[0]);
		int16_t response = -1;
		send(sock, &response, sizeof(int16_t), MSG_NOSIGNAL);
	}
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

void serverSend(void* buffer, size_t size) {
	send(gSock, buffer, size, MSG_NOSIGNAL);
}


