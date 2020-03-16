/* rfwake
Copyright (C) 2015  Werner Hein

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif

int main(int argc, char* argv[]) {
   int fd, gpio, i, mode, res, nbr=1, gotyou=0;
   char *a1p, *a2p;
   unsigned char locrfid[IDSIZE], remrfid[IDSIZE], recrfid[IDSIZE];

   if (argc == 4) {
      for (i = 0, a1p = argv[1], a2p = argv[2]; i < IDSIZE; i++, a1p++, a2p++) {
         locrfid[i] = strtoul(a1p,&a1p,16);
         remrfid[i] = strtoul(a2p,&a2p,16);
      }
      gpio = atoi(argv[3]);
   }
   else {
      fprintf(stderr, "Usage: rfwake ll:oo:cc:aa:ll:xx:RF:ID rr:ee:mm:oo:tt:ee:RF:ID GPIO#\n");
      exit(EXIT_FAILURE);
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
      fprintf(stdout, "%d. Wake-Telegram sent.\n", nbr++);

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
            if (!gotyou) delay(85); // wait long enough if wrong RF ID received
         }
      }
      // switch back to STDBY Mode
      if (rfm69STDBYMode()) {
         fprintf(stderr, "Failed to enter STDBY Mode\n");
         exit(EXIT_FAILURE);
      }
   } while(!gotyou);

   // output of remote RF ID
   fprintf(stdout, "ACK received from called Station RF ID ");
   for (i = 0; i < IDSIZE; i++) {
      if(i != 0) fprintf(stdout,":");
      fprintf(stdout, "%02x", recrfid[i]);
   }
   fprintf(stdout,"\n");

   close(fd);
   exit(EXIT_SUCCESS);
}
