$ avrdude -v -p t84 -P /dev/ttyUSB0 -b 19200 -c avrisp -U flash:w:fw.bin -U eeprom:w:1.eep -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xfe:m

Note that the sketch doesn't work with the current attiny core. Probably
the pin assignments have changed. Need to check against the circuit.
