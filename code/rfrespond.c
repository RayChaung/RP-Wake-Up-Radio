/* rfrespond
Copyright (C) 2020  Ray, Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif
void readConfig(char *fileName, char clist[2][30])
{
    FILE *file = fopen(fileName, "r");
    int c;

    if (file == NULL) {
			fprintf(stderr, "Config file cannot find in /home/pi/myConfig.");
			fprintf(stderr, "Usage: rfrespond opt.[localRFID] [GPIO]\n");
			exit(EXIT_FAILURE);
	} //could not open file

    char item[30] = {};
	
	size_t n = 0;
    while ((c = fgetc(file)) != EOF)
    {
		if (c == ' ') {
			item[n] = '\0';
			strcpy(clist[0], item);
			memset(&item[0], 0, sizeof(item));
			n = 0;
		}
		else
			item[n++] = (char) c;
    }

    // terminate with the null character
    item[n] = '\0';
    strcpy(clist[1], item); 
}


int main(int argc, char* argv[]) {
	pid_t pid, sid;
	int fdspi, gpio, i, mode, res, nbr=1;
	FILE* fdlog;
	char *ap;
	unsigned char locrfid[IDSIZE], remrfid[IDSIZE];

	// *** Protect ***
	if(geteuid() != 0) {
		fprintf(stderr, "Hint: call me as root!\n");
		exit(EXIT_FAILURE);
	}

	// *** Config ***
	char config[2][30];
	
	if (argc != 3) readConfig("/home/pi/myConfig", config); 
	else {
		strcpy(config[0], argv[1]);
		strcpy(config[1], argv[2]);
	}
	
	ap = config[0];
	for (i = 0, ap = config[0]; i < IDSIZE; i++,ap++) {
		locrfid[i] = strtoul(ap,&ap,16);
	}
	gpio = atoi(config[1]);
	fprintf(stdout, "RFID:%s, GPIO:%d\n",config[0] , gpio); 

	// *** Setup ***
	if (wiringPiSetupSys() < 0) {
		fprintf(stderr, "Failed to setup wiringPi\n");
		exit(EXIT_FAILURE);
	}
	fdspi = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
	if (fdspi < 0) {
		fprintf(stderr, "Failed to open SPI device\n");
		exit(EXIT_FAILURE);
	}

	// *** Daemonize ***
	//pid = fork();
	//if (pid < 0) exit(EXIT_FAILURE);
	//if (pid > 0) exit(EXIT_SUCCESS);

	fdlog = fopen("/var/log/radio.log", "w");
	fprintf(fdlog, "starting <rfrespond> in daemon mode\nListening on RF ID: ");
	fprintf(stdout, "starting <rfrespond> in daemon mode\nListening on RF ID: ");
	for (i = 0; i < IDSIZE; i++) {
		if(i != 0) {
			fprintf(fdlog,":");
			fprintf(stdout,":");
		}
		fprintf(fdlog, "%02x", locrfid[i]);
		fprintf(stdout, "%02x", locrfid[i]);
	}
	fprintf(fdlog,"\n");
	fprintf(stdout,"\n");
	//sid = setsid();
	/*if (sid < 0) {
		fprintf(fdlog, "failed to daemonize - exiting\n");
		fprintf(stdout, "failed to daemonize - exiting\n");
		exit(EXIT_FAILURE);
	}
	*/
	fflush(fdlog);
	fflush(stdout);
	//fclose(stdin);
	//fclose(stdout);
	//fclose(stderr);

	// *** Check RFM69 Status ***
	// get mode
	mode = rfm69getState();
	if (mode < 0) {
		fprintf(fdlog, "Failed to read RFM69 Status\n");
		fprintf(stdout, "Failed to read RFM69 Status\n");
		exit(EXIT_FAILURE);
	}

	if (mode == 0x00048060) {
		// *** Send ACK ***
		// read remote RF ID from FIFO
		rfm69rxdata(remrfid, 1); // skip last byte of called RF ID
		rfm69rxdata(remrfid, IDSIZE); // read complete remote RF ID
		// prepare for TX
		if (rfm69startTxMode(remrfid)) {
			fprintf(fdlog, "Failed to enter TX Mode\n");
			fprintf(stdout, "Failed to enter TX Mode\n");
			exit(EXIT_FAILURE);
		}

		// write Tx data
		rfm69txdata(&remrfid[IDSIZE-1],1); // write last byte of remote RF ID
		rfm69txdata(locrfid,IDSIZE); // write complete local RF ID
		// wait for HW interrupt(s) and check for TX_Sent state, takes approx. 853.3µs
		do {
			if(waitForInterrupt(gpio, 1) <= 0) { // wait for GPIO_xx
				fprintf(fdlog, "Failed to wait for TX interrupt\n");
				fprintf(stdout, "Failed to wait for TX interrupt\n");
				exit(EXIT_FAILURE);
			}
			mode = rfm69getState();
			if (mode < 0) {
				fprintf(fdlog, "Failed to read RFM69 Status\n");
				fprintf(stdout, "Failed to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}   
		} while ((mode & 0x08) == 0);

		fprintf(fdlog,"ACKed Wake-Up Call from Station: ");
		fprintf(stdout,"ACKed Wake-Up Call from Station: ");
		for (i = 0; i < IDSIZE; i++) {
			if(i != 0) {
				fprintf(fdlog,":");
				fprintf(stdout,":");
			}
			fprintf(fdlog, "%02x", remrfid[i]);
			fprintf(stdout, "%02x", remrfid[i]);
		}
		fprintf(fdlog,"\n");
		fprintf(stdout,"\n");
		fflush(fdlog);
		fflush(stdout);
	}

	while(1) {
		// switch back to STDBY Mode
		if (rfm69STDBYMode()) {
			fprintf(fdlog, "Failed to enter STDBY Mode\n");
			fprintf(stdout, "Failed to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		// *** Reception ***
		// prepare for RX
		if (rfm69startRxMode(locrfid)) {
			fprintf(fdlog, "Failed to enter RX Mode\n");
			fprintf(stdout, "Failed to enter RX Mode\n");
			exit(EXIT_FAILURE);
		}
		// wait for HW interrupt(s) and check for CRC_Ok state
		do {
			res = waitForInterrupt(gpio, 86); // wait for GPIO_xx
			if (res < 0) {
				fprintf(fdlog, "Failed to wait for RX interrupt\n");
				fprintf(stdout, "Failed to wait for RX interrupt\n");
				exit(EXIT_FAILURE);
			}
			else if (res == 0) rfm69restartRx(); // in case of timeout
			mode = rfm69getState();
			if (mode < 0) {
				fprintf(fdlog, "Failed to read RFM69 Status\n");
				fprintf(stdout, "Failed to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while ((mode & 0x02) == 0);

		// switch back to STDBY Mode
		if (rfm69STDBYMode()) {
			fprintf(fdlog, "Failed to enter STDBY Mode\n");
			fprintf(stdout, "Failed to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		delay(20);

		// *** Send ACK ***
		// read remote RF ID from FIFO
		rfm69rxdata(remrfid, 1); // skip last byte of called RF ID
		rfm69rxdata(remrfid, IDSIZE); // read complete remote RF ID
		// prepare for TX
		if (rfm69startTxMode(remrfid)) {
			fprintf(fdlog, "Failed to enter TX Mode\n");
			fprintf(stdout, "Failed to enter TX Mode\n");
			exit(EXIT_FAILURE);
		}
		// write Tx data
		rfm69txdata(&remrfid[IDSIZE-1],1); // write last byte of remote RF ID
		rfm69txdata(locrfid,IDSIZE); // write complete local RF ID
		// wait for HW interrupt(s) and check for TX_Sent state, takes approx. 853.3µs
		do {
			if(waitForInterrupt(gpio, 1) <= 0) { // wait for GPIO_xx
				fprintf(fdlog, "Failed to wait on sent-interrupt\n");
				fprintf(stdout, "Failed to wait on sent-interrupt\n");
				exit(EXIT_FAILURE);
			}
			mode = rfm69getState();
			if (mode < 0) {
				fprintf(fdlog, "Failed to read RFM69 Status\n");
				fprintf(stdout, "Failed to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while ((mode & 0x08) == 0);
		fprintf(fdlog,"ACKed %d. Call from Station: ",nbr);
		fprintf(stdout,"ACKed %d. Call from Station: ",nbr++);
		for (i = 0; i < IDSIZE; i++) {
			if(i != 0) {
				fprintf(fdlog,":");
				fprintf(stdout,":");
			}
			fprintf(fdlog, "%02x", remrfid[i]);
			fprintf(stdout, "%02x", remrfid[i]);
		}
		fprintf(fdlog,"\n");
		fprintf(stdout,"\n");
		fflush(fdlog);
		fflush(stdout);
		// guarantee < 1% air time
		delay(85);
	}
	close(fdspi);
	fclose(fdlog);
	fclose(stdout);
	fclose(stderr);
	exit(EXIT_SUCCESS);
}
