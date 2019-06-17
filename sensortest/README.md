Add the "tinysensor" board definition to the ATTinyCore:

```
$ cp boards.local.txt ~/.arduino15/packages/ATTinyCore/hardware/avr/1.2.4/
$ cp optiboot_tinysensor.hex ~/.arduino15/packages/ATTinyCore/hardware/avr/1.2.4/bootloaders/optiboot/
```

To generate the bootloader file, apply "optiboot.patch" to the optiboot 
source tree:

```
$ patch < optiboot.patch
$ make attiny84 BAUD_RATE=19200
```
