Add the "tinysensor" board definition to the ATTinyCore:

```
$ cp boards.local.txt ~/.arduino15/packages/ATTinyCore/hardware/avr/1.2.4/
$ cp optiboot_tinysensor.hex ~/.arduino15/packages/ATTinyCore/hardware/avr/1.2.4/bootloaders/optiboot/
```

Build either the "sensortest" (default) or the "tinysensor" sketch:

```
$ make -I ~/src/uC-Makefile upload term
$ make -I ~/src/uC-Makefile NODE_ID=5 SKETCH=tinysensor.ino upload
```

To generate the bootloader file, apply "optiboot.patch" to the optiboot 
source tree:

```
$ patch < optiboot.patch
$ ENV=arduino make attiny84 BAUD_RATE=19200
```
