#ifndef PRU_H_
#define PRU_H_

#include "config.h"
#include <stdint.h>

int pruInit(const unsigned int addressOfDevice);
int pruGetImage(uint16_t **data);
int pruRelease();

#endif
