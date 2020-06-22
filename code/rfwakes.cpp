/* rfwakes
 * g++ rfwakes.cpp rfm69bios.o -o rfwakes -lwiringPi
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
#include <iostream>
#include <fstream>
#include <vector>
// TIME HEADER
#include <chrono>
#include <ctime>


#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif

#define LogDIR "/home/pi/Desktop/log/"

struct Target {
	char rem[30];	
	unsigned char remrfid[IDSIZE];
};

void readConfig(char const *fileName, char clist[2][30])
{
	FILE *file = fopen(fileName, "r");
	int c;

	if (file == NULL) {
		fprintf(stderr, "Config file cannot find in /home/pi/myConfig.");
		fprintf(stderr, "Usage: rfwake [calledRFID] opt.[localRFID] [GPIO]\n");
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
	fclose(file);
}

std::vector <Target> readTargets(){
	FILE *file = fopen("/home/pi/targetlist", "r");
	std::vector <Target> tlist;
	char item[30] = {};
	int c;
	size_t n = 0;
	while ((c = fgetc(file)) != EOF)
	{
		if (c == '\n') {
			item[n] = '\0';
			//fprintf(stdout, "%s\n", item);
			Target t;
			strcpy(t.rem, item);
			tlist.push_back(t);
			memset(&item[0], 0, sizeof(item));
			n = 0;
		}
		else
			item[n++] = (char) c;
	}

	return tlist;
	//fprintf(stdout, "%s\n", tt);
	fclose(file);
}

void printrfid(unsigned char rfid[]) {
	for (int i = 0; i < IDSIZE; i++) {
		if(i != 0) fprintf(stdout,":");
		fprintf(stdout, "%02x", rfid[i]);
	}
}

char* toTime(std::chrono::system_clock::time_point target) {
	time_t temp = std::chrono::system_clock::to_time_t(target);
	char* result = ctime(&temp);
	return result;
}

std::string read_gps() {
	std::fstream fgps;
	std::string dir = LogDIR, lat, lng, result;
	fgps.open(dir + "gps.log", std::fstream::in);
	fgps >> lat >> lng;
	result = lat + " " + lng;
	fgps.close();
	return result;
}

int main(int argc, char* argv[]) {
	int fd, gpio, i, mode, res, nbr=1, gotyou=0;
	char *ap, *a1p;
	unsigned char locrfid[IDSIZE], remrfid2[IDSIZE], recrfid[IDSIZE];
	std::vector <Target> Targetlist;

	// *** Config ***
	char config[2][30];

	if (argc == 1) {	// case: rfwakes
		readConfig("/home/pi/myConfig", config); 
		Targetlist = readTargets();
	} else {  		// other case
		fprintf(stderr, "Usage: rfwakes\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0, ap = config[0]; i < IDSIZE; i++, ap++) {
		locrfid[i] = strtoul(ap,&ap,16);
	}
	gpio = atoi(config[1]);
	//-----------------------------------------------------------------------
	// *** Debug ***
	unsigned char remrfdeb[IDSIZE];
	for (i = 0, ap = Targetlist[0].rem; i < IDSIZE; i++, ap++) {
		remrfdeb[i] = strtoul(ap,&ap,16);
	}
	//fprintf(stdout, "%u\n", remrfdeb);
	//-----------------------------------------------------------------------

	fprintf(stdout, "Local RFID:%s, GPIO:%d\n",config[0] , gpio); 

	//for (int i = 0, a1p = Targetlist[0].rem; i < IDSIZE; i++, a1p++) {
	//	t.remrfid[i] = strtoul(a1p,&a1p,16);
	//}
	int counter = 0;
	for(auto it = Targetlist.begin(); it != Targetlist.end(); it++) {
		for (i = 0, ap = it->rem; i < IDSIZE; i++, ap++) {
			//remrfdeb[i] = strtoul(ap,&ap,16);
			it->remrfid[i] = strtoul(ap,&ap,16);
		}
		//memcpy(&it->remrfid, &remrfdeb, sizeof remrfdeb);
		//----------------------------------------------------------------------
		// *** Debug Print ***
		//fprintf(stdout, "Wake Remote RFID[%d]:%s ", ++counter, it->rem);
		fprintf(stdout, "Wake Remote RFID[%d]: ", ++counter);
		printrfid(it->remrfid);
		//----------------------------------------------------------------------
		fprintf(stdout, "\n");
	}


	// *** Setup ***
	if (wiringPiSetupSys() < 0) {
		fprintf(stderr, "Failed to setup wiringPi\n");
		exit(EXIT_FAILURE);
	}
	fd = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
	if (fd < 0) {
		fprintf(stderr, "Failed to open SPI device\n");
		exit(EXIT_FAILURE);
	}

	do {
		for (auto it = Targetlist.begin(); it != Targetlist.end();) {
			unsigned char remrfid[IDSIZE];
			memcpy(&remrfid, &it->remrfid, sizeof it->remrfid);
			// *** Transmission ***
			// prepare for TX
			if (rfm69startTxMode(remrfid)) {
				fprintf(stderr, "Failed to enter TX Mode\n");
				exit(EXIT_FAILURE);
			}
			// write Tx data
			rfm69txdata(&remrfid[IDSIZE-1],1);
			rfm69txdata(locrfid,IDSIZE);
			// wait for HW interrupt(s) and check for TX_Sent state, takes approx. 853.3µs
			do {
				if(waitForInterrupt(gpio, 1) <= 0) { // wait for GPIO_xx
					fprintf(stderr, "Failed to wait on TX interrupt\n");
					exit(EXIT_FAILURE);
				}
				mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
			} while ((mode & 0x08) == 0);
			fprintf(stdout, "%d. Wake-Telegram sent to %s.\n", nbr++, it->rem);

			// switch back to STDBY Mode
			if (rfm69STDBYMode()) {
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}

			// *** Reception ***
			// prepare for RX
			if (rfm69startRxMode(locrfid)) {
				fprintf(stderr, "Failed to enter RX Mode\n");
				exit(EXIT_FAILURE);
			}
			// wait for HW interrupt(s) and check for CRC_Ok state
			res = waitForInterrupt(gpio, 84); // wait for GPIO_xx
			if (res < 0) {
				fprintf(stderr, "Failed to wait on RX interrupt\n");
				exit(EXIT_FAILURE);
			}
			else if (res > 0) { // in case of reception ...
				mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
				if ((mode & 0x02) == 0x02) { // ... and CrcOk ...
					// read remote RF ID from FIFO
					rfm69rxdata(recrfid, 1); // skip last byte of called RF ID
					rfm69rxdata(recrfid, IDSIZE); // read complete remote RF ID
					// check received vs. called remote RF ID
					for (i = 0, gotyou = 1; i < IDSIZE; i++) // ... and RF ID equal ...
						if (remrfid[i] != recrfid[i]) gotyou = 0; // ... then done
					//if (!gotyou) delay(85); // wait long enough if wrong RF ID received
				}
			}
			// switch back to STDBY Mode
			if (rfm69STDBYMode()) {
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}

			// if not receive ACK
			if (gotyou == 0) {
				++it;
			}
			else {
				it = Targetlist.erase(it);
				// output of remote RF ID
				fprintf(stdout, "ACK received from called Station RF ID ");
				printrfid(recrfid);
				fprintf(stdout,"\n");
				
				// write into log file
				auto now = std::chrono::system_clock::now();
				std::fstream flog;
				std::string logfname(it->rem);
				flog.open (LogDIR + logfname + ".log", std::fstream::in | std::fstream::out | std::fstream::app);
				flog << "ACK receive at:\t\t\t" << read_gps() << "\t" << toTime(now);
				flog.close();
				// recover `gotyou` switch
				gotyou = 0;
			}
		}
	} while(!Targetlist.empty());


	close(fd);

	exit(EXIT_SUCCESS);
}
