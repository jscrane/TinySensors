### Using uC-Makefile and avrdude
```
$ make -I ~/src/uC-Makefile write-fuses write-flash
$ avrdude -v -p t84 -P /dev/ttyUSB0 -b 19200 -c avrisp -U eeprom:w:1.eep
```
### Avrdude only
```
$ avrdude -v -p t84 -P /dev/ttyUSB0 -b 19200 -c avrisp -U flash:w:fw.bin -U eeprom:w:1.eep -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xfe:m
```

This sketch now requires the damellis core for
[attiny](https://github.com/damellis/attiny) and my
[SPI](https://github.com/jscrane/SPI) library.
