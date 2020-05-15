include ../auth

prefix := /usr/local

# The recommended compiler flags for the Raspberry Pi
CXXFLAGS = -Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -fno-strict-aliasing -g
CPPFLAGS = -I$(prefix)/include -I../sketches -DMAX_SENSORS=64
LDFLAGS = -L$(prefix)/lib
DEPFLAGS = -MM -MP -MF
LD = g++

PROGRAMS = sensorhub mux lcd status mqtt
SOURCES = ${wildcard *.cpp}
OBJECTS = ${SOURCES:.cpp=.o}
DEPS = ${SOURCES:.cpp=.d}

.PHONY: all clean

all: ${DEPS} ${PROGRAMS}

%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(DEPFLAGS) $@ $<

sensorhub: LDLIBS = -lrf24-bcm -lrf24-network
sensorhub: sensorhub.o sensorlib.o

mux: mux.o sensorlib.o

lcd: lcd.o sensorlib.o

status: LDLIBS = -lrf24-bcm
status: status.o sensorlib.o

mqtt: LDLIBS = -lmosquitto
mqtt: mqtt.o sensorlib.o

sensorlib.o: sensorlib.h

clean:
	rm -rf $(PROGRAMS) $(OBJECTS) $(DEPS)

-include $(DEPS)
