#include "config.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	printf("IMX556 demo server\n");
	int deviceAddress = 0x57;
	
	configInit(deviceAddress);

	printf("Starting server...\n");
	startServer(deviceAddress);
	return 0;
}
