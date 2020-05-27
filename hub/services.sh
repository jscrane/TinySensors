#!/bin/bash

SEN=/home/pi/TinySensors/hub
[ -x $SEN/sensorhub ] && $SEN/sensorhub
[ -x $SEN/mux ] && $SEN/mux localhost:5555 sheeva:5555 < $SEN/nodes.txt
[ -x $SEN/lcd ] && $SEN/lcd -l localhost
[ -x $SEN/status ] && $SEN/status
[ -x $SEN/mqtt ] && $SEN/mqtt -q mqtt.opensensors.io -m localhost:5678 -r /users/jscrane/stat -u jscrane -p lRWhtqio -c 5693
[ -x $SEN/mqtt ] && $SEN/mqtt -z -q localhost:1883 -m localhost:5678 -r domoticz/in
