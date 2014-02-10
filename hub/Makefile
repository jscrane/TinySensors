prefix := /usr/local

# The recommended compiler flags for the Raspberry Pi
CXXFLAGS=-Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -fno-strict-aliasing  -g
CPPFLAGS=-I$(prefix)/include -I/usr/include/mysql -DBIG_JOINS=1 
LDFLAGS=-L$(prefix)/lib -lrf24-bcm -lrf24-network -lmysqlclient -lpthread -lz -lm -lrt -ldl

# define all programs
PROGRAMS = sensorhub
SOURCES = ${PROGRAMS:=.cpp}
OBJECTS = ${PROGRAMS:=.o}

all: ${PROGRAMS}

${PROGRAMS}: ${OBJECTS}

clean:
	rm -rf $(PROGRAMS) $(OBJECTS)

