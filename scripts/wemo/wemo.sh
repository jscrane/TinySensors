#!/bin/bash

PATH=/usr/local/bin:$PATH
WEMO=wemo

. wemo_functions.sh

case $# in
0)
  wemo_get $WEMO
  ;;
1)
  mosquitto_pub -h iot -t cmnd/wemo/power -m $1
  ;;
*)
  echo "Usage: wemo [0|1|]"
;;
esac
