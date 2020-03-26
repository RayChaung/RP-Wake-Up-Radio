/* rfm69bios
Copyright (C) 2020  Ray, Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 */

#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif

const unsigned char initvec[] = {
		0x01 | 0x80, // Address + write cmd bit
	//	 0/8   1/9   2/A   3/B   4/C   5/D   6/E   7/F
		      0x24, 0x02, 0x00, 0x6B, 0x04, 0xC9, 0xD9,	// 00
		0x13, 0x33, 0x80, 0x00, 0x02, 0xAA, 0xC6, 0x16,	// 08
		0x23, 0x7C, 0x09, 0x1A, 0x40, 0xB0, 0x7B, 0x9B,	// 10
		0x00, 0x48, 0x80, 0x40, 0x80, 0x06, 0x0C, 0x00,	// 18
		0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x07, 0x80,	// 20
		0x00, 0xA0, 0x00, 0x00, 0x00, 0x0E, 0xB0, 0x00,	// 28
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12,	// 30
		0x09, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00,	// 38
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 40
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,	// 48
};

const unsigned char inittst1[] = {
		0x58 | 0x80, // Address + write cmd bit
	//	 0/8   1/9   2/A   3/B   4/C   5/D   6/E   7/F
		0x1B, 0x09,					// 58
};
const unsigned char inittst2[] = {
		0x5F | 0x80, // Address + write cmd bit
	//	 0/8   1/9   2/A   3/B   4/C   5/D   6/E   7/F
							  0x08,	// 58
};
const unsigned char inittst3[] = {
		0x6F | 0x80, // Address + write cmd bit
	//	 0/8   1/9   2/A   3/B   4/C   5/D   6/E   7/F
							  0x30,	// 68
};
const unsigned char inittst4[] = {
		0x71 | 0x80, // Address + write cmd bit
	//	 0/8   1/9   2/A   3/B   4/C   5/D   6/E   7/F
		      0x00,					// 70
};

int rfm69init(unsigned char* spibuffer, const unsigned char* rfid) {
	int i;
	for (i = 0; i < sizeof(initvec); i++)
		spibuffer[i] = initvec[i];
	for (i = 0; i < IDSIZE-1; i++)
		spibuffer[0x2F+i] = rfid[i];
	spibuffer[0x39] = rfid[IDSIZE-1];
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, sizeof(initvec)) < 0) exit(1);

	for (i = 0; i < sizeof(inittst3); i++)
		spibuffer[i] = inittst3[i];
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, sizeof(inittst3)) < 0) exit(2);
	return 0;
}

int rfm69getState() {
	unsigned char spibuffer[3];
	int status = 0;
	int statusP = 0;
	spibuffer[0] = 0x01 & 0x7F; // Address + read cmd bit
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(-1);
	status 	= spibuffer[1] << 16;
	// debug printf
	statusP = spibuffer[1];
	//printf("OpMode(0x01)\t:%02X\n", statusP);

	spibuffer[0] = 0x27 & 0x7F; // Address + read cmd bit
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 3) < 0) exit(-2);	
	status |= spibuffer[1] << 8;
	status |= spibuffer[2];
	// debug printf
	statusP = 0; statusP |= spibuffer[1];
	//printf("IrqFlag1(0x27)\t:%02X\n", statusP);
	statusP = 0; statusP |= spibuffer[2];
	//printf("IrqFlag2(0x28)\t:%02X\n", statusP);
	
	
	return status;
}

int rfm69txdata(const unsigned char* data, unsigned int size) {
	unsigned char spibuffer[IDSIZE+1];
	int i;
	if (size > IDSIZE) exit (1);
	spibuffer[0] = 0x00 | 0x80; // Address + write cmd bit
	for (i = 0; i < size; i++) spibuffer[1+i] = data[i];
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, size+1) < 0) exit(2);
	return 0;
}

int rfm69rxdata(unsigned char* data, unsigned int size) {
	unsigned char spibuffer[IDSIZE+1];
	int i;
	if (size > IDSIZE) exit (1);
	spibuffer[0] = 0x00 & 0x7F; // Address + write cmd bit
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, size+1) < 0) exit(2);
	for (i = 0; i < size; i++) data[i] = spibuffer[1+i];
	return 0;
}

int rfm69STDBYMode() {
	unsigned char spibuffer[2];
	spibuffer[0] = 0x01 | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x24; // STDBY Mode (+ terminate Listen Mode)
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, sizeof(spibuffer)) < 0) exit(1);
	delayMicroseconds(100);
	return 0;
}

int rfm69startTxMode(const unsigned char* rfid) {
	unsigned char spibuffer[sizeof(initvec)];
	if (rfm69init(spibuffer, rfid)) exit(1);
	// switch to TX Mode
	spibuffer[0] = 0x01 | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x0C; // TX Mode (+ terminate Listen Mode)
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(2);
	return 0;
}

int rfm69startRxMode(const unsigned char* rfid) {
	unsigned char spibuffer[sizeof(initvec)];
	if (rfm69init(spibuffer, rfid)) exit(1);
	// switch to RX Mode
	spibuffer[0] = 0x01 | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x10; // RX Mode
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(2);
	return 0;
}

int rfm69restartRx(void) {
	unsigned char spibuffer[2];
	spibuffer[0] = 0x3D | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x06; // STDBY Mode (+ terminate Listen Mode)
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, sizeof(spibuffer)) < 0) exit(1);
	return 0;
}

int rfm69ListenMode(const unsigned char* rfid) {
	unsigned char spibuffer[sizeof(initvec)];
	if (rfm69init(spibuffer, rfid)) exit(1);
	// switch to RX Mode
	spibuffer[0] = 0x01 | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x44; // Listen Mode (+ STDBY Mode)
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(2);
	return 0;
}

int rfm69getAllState() {
    printf("ST | 08, 19, 2A, 3B, 4C, 5D, 6E, 7F, | EN\n");
    printf("-----------------------------------------\n");
    unsigned char spibuffer[2];
    printf("00 | ");
    for(int i = 0; i < 80; i++) {
        int status = 0;
        spibuffer[0] = i & 0x7F; // Address + read cmd bit
	    if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(-1);
        status = spibuffer[1];
        printf("%02X, ", status);
        if((i + 1)% 8 == 0) {
            printf("| %02X\n", i);
            if (i == 79) break;
            printf("%02X | ", i+1);
		}
    }
    printf("\n");
    return 0;
}
