/* rfwait
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
   int fd, gpio, i;
   char *ap;
   unsigned char rfid[IDSIZE];

   if (argc == 3) {
      ap = argv[1];
      for (i = 0, ap = argv[1]; i < IDSIZE; i++,ap++) {
         rfid[i] = strtoul(ap,&ap,16);
      }
      gpio = atoi(argv[2]);
   }
   else {
      fprintf(stderr, "Usage: rfwait ll:oo:cc:aa:ll:xx:RF:ID GPIO#\n");
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
   // clear potentially pending HW interrupts
   if(waitForInterrupt(gpio, 1) < 0) { // wait for GPIO_25
      fprintf(stderr, "Failed to wait on HW interrupt\n");
      exit(EXIT_FAILURE);
   }

   // *** Reception ***
   // switch to Listen (DRX) Mode
   if (rfm69ListenMode(rfid)) {
      fprintf(stderr, "Failed to enter Listen Mode\n");
      exit(EXIT_FAILURE);
   }

   close(fd);
   exit(EXIT_SUCCESS);
}
