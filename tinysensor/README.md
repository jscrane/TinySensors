### Build with uC-Makefile
```
$ make -I ~/src/uC-Makefile NODE_ID=1 clean write-fuses write-flash
```

This sketch now requires the my clone of the damellis core for
[attiny](https://github.com/jscrane/attiny) which contains my
[SPI](https://github.com/jscrane/SPI) library for attiny.
