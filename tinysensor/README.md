Add the "tinysensor" board definition and bootloaders to the ATTinyCore:

```
$ make init
```

Build the "tinysensor" sketch:

```
$ make upload term
$ make NODE_ID=5 upload
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
