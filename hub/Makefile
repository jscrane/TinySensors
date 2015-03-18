include ../auth

prefix := /usr/local

# The recommended compiler flags for the Raspberry Pi
CXXFLAGS=-Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -fno-strict-aliasing  -g
CPPFLAGS=-I$(prefix)/include -I/usr/include/mysql -I../tinysensor -DBIG_JOINS=1 -DUSER=\"${USER}\" -DPASS=\"${PASSWORD}\" -DMAX_SENSORS=64
LDFLAGS=-L$(prefix)/lib -lrf24-bcm -lrf24-network -lmysqlclient -lpthread -lrrd -lz -lm -lrt -ldl

PROGRAMS = sensorhub mux lcd mysql status rrd
SOURCES = ${wildcard *.cpp}
OBJECTS = ${SOURCES:.cpp=.o}

all: ${PROGRAMS}

sensorhub: sensorhub.o sensorlib.o

mux: mux.o sensorlib.o

lcd: lcd.o sensorlib.o

mysql: mysql.o sensorlib.o

status: status.o sensorlib.o

rrd: rrd.o sensorlib.o

sensorlib.o: sensorlib.h

clean:
	rm -rf $(PROGRAMS) $(OBJECTS)

