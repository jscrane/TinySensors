#!/bin/bash

PATH=/usr/local/bin:$PATH
WEMO=wemo
HOST=iot
CLIENT=wemo_mqtt
STAT=stat/wemo/power
CMND=cmnd/wemo/power
 
. wemo_functions.sh
    
mosquitto_pub -h $HOST -t $STAT -m $(wemo_get $WEMO)

mosquitto_sub -h $HOST -i $CLIENT -t $CMND | while read c; do
  echo $c
  wemo_set $WEMO $c
  mosquitto_pub -h $HOST -t $STAT -m $c
done
