Raspberry Pi Wake on Radio and RTC
==================================

# Purpose

An add-on board attached to the Raspberry Pi's P1 header controls the 5V power
supply in a way that the RasPi is booted upon

* power return after power loss
* pressing the switch
* an pre-set alarm in the RTC chip
* a radio message with a pre-defined RFID received by RFM69 transceiver

Approx. 35 seconds after the Raspberry Pi shut down normally the add-on board disconnects
it from its 5V power supply - ready to boot it up again via above triggers.

# Synopsis

## rtc-power  <I>i2cbus  alarmtime</I>

`rtc-power` programs an alarm at `alarmtime` in RTC chip on I2C Bus numbered by `i2cbus`.

`i2cbus` is an integer value in the range of 0 .. 1 and appoints to the Raspberry
Pi's I2C bus the RTC chip as attached to.

Hint: the I2C bus number actually may depend on the used Raspberry Pi model even if it
uses the same pins on P1.

`alarmtime` defines when the RTC shall trigger a wakeup. It is an integer
number of seconds since the UNIX epoche, i.e. since 1. Januar 1970 00:00 Uhr UTC.
If `alarmtime` lies in the past then it is silently ignored and no alarm will be
programmed. If it lies within the next 120 second future then it silently will be set
to +120 seconds from now.

## rfwait <I>localRFID GPIO</I>

`rfwait` puts the transceiver on RFM69 into DRX mode. In that mode the hardware
signal goes up upon reception of `localRFID` and thus generates a hardware
wakeup event.

`GPIO` is the number of the GPIO in Broadcom SoC nomenclature routed to the
corresponding P1 pin the RFM69's wakeup interrupt hardware signal is connected to.

Example: a value 25 is associated to GPIO25 on SoC which is routed to pin 22 on P1
and Raspberry Pi and thus pin 22 on RPi-GPIO connector on add-on board.

## rfrespond <I>localRFID GPIO</I>

Upon start of daemon `rfrespond` it checks whether a previously received telegram
is available in the transceiver - e.g. received earlier in DRX mode.
If so, it acknowledges that telegram immediately. Subsequently it waits for
further telegrams matching the `localRFID` and acknowledges it.

`GPIO` is same as above.

## rfwake <I>localRFID calledRFID GPIO</I>

`rfwake` sends periodically telegrams with `localRFID` as SOURCE RFID and
`calledRFID` as DESTINATION RFID until it receives an acknowledge telegram.

`GPIO` is same as above.

Hint: before `rfwake` is started first the `GPIO` on Raspberry Pi needs to be
configured as interrupt source. This can be done by
`gpio edge <GPIO> rising`

Any `RFID` is a 64 bit value entered as a colon separated sequence of 8 bytes
<B><I>b0:b1:b2:b3:b4:b5:b6:b7</I></B> in hex nomenclature.

# Documents

* The add-on board's schematic can be found in
[raspi_wake_on_radio_rtc.pdf](./Documents/raspi_wake_on_radio_rtc.pdf).

* Radio telegram definition as well as transmission frame and DRX timing is shown in
[radio_diagrams.odg](./Documents/radio_diagrams.odg).

* RFM69(HCW) register settings are given in an overview in
[RFM69Register.ods](./Documents/RFM69Register.ods).