View this project on [CADLAB.io](https://cadlab.io/project/1612). 

TinySensors
===========
See [blog articles](http://programmablehardware.blogspot.ie/search/label/tinysensor).

* hub: lightweight Raspberry Pi server-side for TinySensor
* sensing: Database viewer
* eagle: TinySensor Eagle project

Software
========
* Arduino
  - IDE version [1.8.9](http://arduino.cc/en/Main/Software) or [uC-Makefile](https://github.com/jscrane/uC-Makefile)
  - [ATtiny core](https://github.com/SpenceKonde/ATTinyCore)
  - [nRF24](https://github.com/nRF24/RF24) library
  - [DHT22](https://github.com/jscrane/DHT22) library, slimmed-down for ATtiny

* Raspberry-Pi
  - Tested on Raspberry Pi 2 and Raspberry Pi 3 Model B
  - [nRF24](https://github.com/nRF24/RF24) library
  - [BCM2835](http://www.airspayce.com/mikem/bcm2835) library
  - libmosquitto-dev
  - lcdproc

* Viewer
  - [Leiningen](http://leiningen.org) v2.0, or
  - [IntelliJ](http://www.jetbrains.com/idea) with the
[Cursive](https://cursiveclojure.com) plugin
