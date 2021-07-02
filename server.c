
#include "server.h"
#include "api.h"
#include "pru.h"
#include "helper.h"

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
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


void handleRequest(int sock);
void signalHandler(int sig);
void serverSend(void* buffer, size_t size);

int sockfd, newsockfd, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

static unsigned int deviceAddress;
unsigned char* pData;
int gSock = 0;

/*!
 Starts TCP Server for communication with client application
 @param addressOfDevice i2c address of the chip (default 0x20)
 @return On error, -1 is returned.
 */
int startServer(const unsigned int addressOfDevice) {
	deviceAddress = addressOfDevice;
	signal(SIGQUIT, signalHandler); // add signal to signalHandler to close socket on crash or abort
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, signalHandler);
	signal(SIGSEGV, signalHandler);
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
			apiReadRegister(registerAddress, nBytes, values, deviceAddress);
			for (v = 0; v < nBytes; v++) {
				responseValues[v] = values[v];
			}
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
					values, deviceAddress);
		} else {
			answer = -1;
		}
		send(sock, &answer, sizeof(int16_t), MSG_NOSIGNAL);
		free(values);
	} else if (strcmp(stringArray[0], "getRawData") == 0 && !argumentCount) {
		uint16_t *pMem = NULL;
//		for (int i = 0; i < 300; i++) {
			int dataSize = 2 * pruGetImage(&pMem);
//			printf("%s start sending %d bytes\n", __FUNCTION__, dataSize);
			send(sock, pMem, dataSize, MSG_NOSIGNAL);
//			printf("%s finished sending %d bytes\n", __FUNCTION__, dataSize);
//		}
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
	printf("caught signal %i .... going to close application\n", sig);
	close(sockfd);
	close(newsockfd);
	exit(0);
}

void serverSend(void* buffer, size_t size) {
	send(gSock, buffer, size, MSG_NOSIGNAL);
}

