TinySensors
===========
See [blog articles](http://programmablehardware.blogspot.ie/search/label/tinysensor).

* hub: lightweight Raspberry Pi server-side for TinySensor
* sensing: Database viewer
* eagle: TinySensor Eagle project
* [sensortest](https://gist.github.com/jscrane/8434935): 
sketch to test and provision (i.e., set node-id) a TinySensor
* [tinysensor](https://gist.github.com/jscrane/8434851): 
wireless sensor sketch
* [tinyrelay](https://gist.github.com/jscrane/9113801): wireless relay sketch

After cloning this repo, pull the sketches into the correct place with:
````	
% git submodule init
% git submodule update
````

Software
========
* Arduino
  - IDE version [1.5.4](http://arduino.cc/en/Main/Software)
  - [ATtiny core](https://github.com/jscrane/attiny)
  - SPI library with support for [ATtiny](https://github.com/jscrane/SPI)
  - Forked [RF24](https://github.com/jscrane/RF24) and 
[RF24Network](https://github.com/jscrane/RF24Network) libraries
  - [DHT22](https://github.com/jscrane/DHT22) library, slimmed-down for ATtiny
  - [JeeLib](https://github.com/jcw/jeelib)

* Raspberry-PI
  - [RF24-rpi](https://github.com/jscrane/RF24-rpi)

* Viewer
  - [Leiningen](http://leiningen.org) v2.0, or
  - [IntelliJ](http://www.jetbrains.com/idea) with the
[Cursive](https://cursiveclojure.com) plugin
