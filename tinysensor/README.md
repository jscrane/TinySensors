Add the "tinysensor" board definition to the ATTinyCore:

```
$ cp boards.local.txt ~/.arduino15/packages/ATTinyCore/hardware/avr/1.4.1/
$ cp optiboot/optiboot_tinysensor_* ~/.arduino15/packages/ATTinyCore/hardware/avr/1.4.1/bootloaders/optiboot/
```

Build the "tinysensor" sketch:

```
$ make -I ~/src/uC-Makefile upload term
$ make -I ~/src/uC-Makefile NODE_ID=5 upload
```

To generate the bootloader file, apply "optiboot/optiboot.patch" to the optiboot 
source tree:

```
$ patch < optiboot.patch
$ ENV=arduino make attiny84 BAUD_RATE=19200 AVR_FREQ=8000000L
```

This will create a file `optiboot_attiny84.hex` for a chip clocked at 8MHz.

For a 1MHz part:

```
$ ENV=arduino make attiny84 BAUD_RATE=9600 AVR_FREQ=1000000L
```
