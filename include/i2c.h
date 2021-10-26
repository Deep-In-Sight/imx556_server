#ifndef _I2C_H_
#define _I2C_H_

#include "stdint.h"

/**************************************************************************
* Function Declarations                                                   *
**************************************************************************/

struct i2cReg {
    uint16_t addr;
    uint8_t val;
};

int i2c (unsigned int dev_addr, char op, unsigned int reg_addr, unsigned int n_bytes_data, unsigned char** p_data);

#endif
