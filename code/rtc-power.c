/* rtc-power
Copyright (C) 2013  Werner Hein

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
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <time.h>

#define MCP7941X        0x6f
#define CTRLREG         0x07
#define ALARM0REG       0x0a
#define ALARMREGSETLEN  6
#define ALARMMC         0x70
#define RTCOUT1         0x80
#define ALARM0EN        0x10

#define MINTIMEGAP      120

char int2bcd(char val) {
  return (val%10+16*(val/10));
}

int main(int argc, char *argv[]) {
  int file;
  int i2cbus; // I2C bus number
  char i2cdevicename[11];
  char cbuffer[2]; // I2C Control Buffer
  char buffer[ALARMREGSETLEN+1]; // I2C Alarm0 Buffer
  char *end;
  time_t atime; // Alarm time in seconds since epoch
  time_t now = time(NULL);
  struct tm *alarm; // Alarm in broken down time

  /* get command line parameters */
  if (argc != 3) exit(1);
  i2cbus = strtol(argv[1], &end, 0);
  if (*end || i2cbus < 0 || i2cbus > 1) exit(2);
  atime = strtol(argv[2], &end, 0);
  if (*end || atime < 0) exit(3);

  /* set RTC control and ALARM0 accordingly */
  if (atime < now) { // in case alarm time in the past then ...
    int i;
    cbuffer[1] = RTCOUT1; // ... simply switch off ...
    for (i=1; i<=ALARMREGSETLEN; i++) buffer[i]=0; // ... and clear alarm time
  }
  else { // in case of an alarm in future ....
    cbuffer[1] = ALARM0EN; // ... enable the alarm ...
    if (atime - now < MINTIMEGAP) atime = now + MINTIMEGAP; // ... make alarm causal ...
    alarm = gmtime(&atime);
    if (alarm->tm_sec > 59) alarm->tm_sec = 59; // ... correct any leap seconds and write ...
    buffer[1] = int2bcd(alarm->tm_sec); // seconds
    buffer[2] = int2bcd(alarm->tm_min); // minutes
    buffer[3] = int2bcd(alarm->tm_hour); // hour
    buffer[4] = int2bcd(alarm->tm_wday+1); // day of week
    buffer[4] |= ALARMMC; // add alarm match condition, write
    buffer[5] = int2bcd(alarm->tm_mday); // day of month
    buffer[6] = int2bcd(alarm->tm_mon+1); // month
  }

  /* open i2c device */
  snprintf(i2cdevicename, 11, "/dev/i2c-%d", i2cbus);
  file = open(i2cdevicename, O_WRONLY);
  if (file < 0) exit(4);
  if (ioctl(file, I2C_SLAVE_FORCE, MCP7941X) < 0) exit(5);

  /* write alarm0 to RTC */
  buffer[0] = ALARM0REG; // set ALARM0 register address
  if (write(file, buffer, ALARMREGSETLEN+1) != ALARMREGSETLEN+1) exit(6);

  /* activate alarm and deactivate power rail */
  cbuffer[0] = CTRLREG; // set RTC control register address
  if (write(file, cbuffer, 2) != 2) exit(7);

  exit(0);
}
