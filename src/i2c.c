#include "i2c.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

int i2c(unsigned int dev_addr, char op, unsigned int reg_addr, unsigned int n_bytes_data, unsigned char** p_data) {

	int fd_i2c;
#ifdef CABIN2
	char *filename = "/dev/i2c-0";
#elif ULTRA96
	char *filename = "/dev/i2c-4";
#else
	char *filename = "/dev/i2c-0";
#endif
	unsigned char buf_rd[1];
	unsigned char buf_wr[3];
	int rd = 0, wr = 0;

	if (n_bytes_data != 1) {
		printf("Sony apps only support writing/reading 1 register a time\n");
		return -1;
	}

	if ((fd_i2c = open(filename, O_RDWR)) < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		perror("Failed to open the i2c bus.");
		return -1;
	}

	if (ioctl(fd_i2c, I2C_SLAVE, dev_addr) < 0) {
		printf("0 2\nFailed to acquire bus access and/or talk to slave.\n");
		return -1;
	}

	buf_wr[0] = (reg_addr & 0xFF00) >> 8;
	buf_wr[1] = reg_addr & 0xFF;
	
	switch (op) {
	case 'r':
		wr = write(fd_i2c, buf_wr, 2);
		rd = read(fd_i2c, buf_rd, 1);
		if (wr != 2 || rd != 1) {
			printf("Read register failed\n");
			return -1;
		}
		**p_data = buf_rd[0];
		break;
	case 'w':
		buf_wr[2] = **p_data;
		wr = write(fd_i2c, buf_wr, 3);
		if (wr != 3) {
			printf("Write register failed\n");
			return -1;
		}
		break;
	default:
		printf("Not supported i2c operation\n");
	}

	close(fd_i2c);
		

	return 0;

}

