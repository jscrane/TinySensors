#!/bin/bash

SEN=/usr/local/bin
MUX=localhost:5678
MQTT=localhost:1883

[ -x $SEN/sensorhub ] && $SEN/sensorhub
[ -x $SEN/mux ] && $SEN/mux localhost:5555 sheeva:5555 < /usr/local/etc/nodes.txt
[ -x $SEN/lcd ] && $SEN/lcd -l localhost -m $MUX -t 1200
[ -x $SEN/status ] && $SEN/status -m $MUX -t 600
[ -x $SEN/mqtt ] && $SEN/mqtt -c sensors-domoticz -z -q $MQTT -m $MUX -r domoticz/in
[ -x $SEN/mqtt ] && $SEN/mqtt -c sensors-json -j -q $MQTT -m $MUX
