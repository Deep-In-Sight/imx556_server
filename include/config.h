#ifndef _CONFIG_H_
#define _CONFIG_H_

int configInit(int addressOfDevice);
/*read preset*/
int sony_loadRegisters(char* filename);
/*read fmodData.txt and store fmod*/
int initModFreq();
/*get the register addresses and values to configure a modulation frequency*/
int changeModFreq(int freq);
/*change the sensor integration time*/
int changeIntegration(int time_ns);

#endif
