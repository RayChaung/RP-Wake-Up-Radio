#define RFM69BIOS_H

#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define SPI_SPEED 10000000
#define SPI_DEVICE 0
#define IDSIZE 8

int rfm69getState(void);
int rfm69txdata(const unsigned char*, unsigned int);
int rfm69rxdata(unsigned char*, unsigned int);
int rfm69STDBYMode(void);
int rfm69startTxMode(const unsigned char*);
int rfm69startRxMode(const unsigned char*);
int rfm69restartRx(void);
int rfm69ListenMode(const unsigned char*);
