include ../auth

prefix := /usr/local

# The recommended compiler flags for the Raspberry Pi
CXXFLAGS = -Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -fno-strict-aliasing -g
CPPFLAGS = -I$(prefix)/include -I../tinysensor -DMAX_SENSORS=64
CC = g++
LDFLAGS = -L$(prefix)/lib
DEPFLAGS = -MM -MP -MF

PROGRAMS = sensorhub mux lcd status mqtt
SOURCES = ${wildcard *.cpp}
OBJECTS = ${SOURCES:.cpp=.o}
DEPS = ${SOURCES:.cpp=.d}

.PHONY: all clean install

all: ${DEPS} ${PROGRAMS}

%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $@ $<

sensorhub: LDLIBS = -lrf24
sensorhub: sensorhub.o sensorlib.o

mux: mux.o sensorlib.o

lcd: lcd.o sensorlib.o

status: LDLIBS = -lbcm2835
status: status.o sensorlib.o

mqtt: LDLIBS = -lmosquitto
mqtt: mqtt.o sensorlib.o

sensorlib.o: sensorlib.h

clean:
	rm -rf $(PROGRAMS) $(OBJECTS) $(DEPS)

install:
	install -s $(PROGRAMS) $(prefix)/bin
	install services.sh $(prefix)/bin
	install nodes.txt $(prefix)/etc

-include $(DEPS)
