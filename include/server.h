#ifndef SERVER_H_
#define SERVER_H_

#define TCP_PORT_NO 				50660
#define TCP_BUFFER_SIZE 			256
#define MAX_COMMAND_ARGUMENTS 		256
#define MAX_COMMAND_ARGUMENT_LENGTH	50

int startServer(const unsigned int addressOfDevice);


#endif
