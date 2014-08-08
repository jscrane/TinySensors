TinySensors' Hub
================

A couple of programs, _sensorhub_ and _mux_, to connect TinySensors to the
Internet. Prototyped and run on a Raspberry Pi with an nrf24l01+ connected
to its GPIO port, as described [here](https://github.com/jscrane/RF24-rpi).

sensorhub
---------
- must be run as root (for access to /dev/watchdog and /dev/spidev)
- polls the RF24 device for messages from sensors
- writes sensor readings into a MySQL database

By default it,
- listens on port 5555 for TCP connections on which it
  echoes sensor readings (disable with -s)
- runs as a daemon (disable with -v, also enables verbose mode)
- will reboot the Pi via the watchdog timer if no sensor readings
  received for 5 minutes, it seems the radio stops responding every
  few weeks or so (disable with -w)

See [here](http://blog.ricardoarturocabral.com/2013/01/auto-reboot-hung-raspberry-pi-using-on.html) for more information on the Pi's watchdog timer.

mux
---
- connects to sensorhub's port 5555
- listens for up to 20 clients on port 5678 and relays sensor readings to them
- by default runs as a daemon (disable with -v, also enables verbose mode)
