BOARD := tinysensor
SKETCH := tinysensor.ino
TERMINAL_SPEED := 9600
CPPFLAGS = -DNODE_ID=$(NODE_ID)
ifeq ($(DEBUG),1)
CPPFLAGS += -DDEBUG=$(DEBUG)
endif
LOCAL_LIBRARY_PATH := ~/src/nRF24

-include tiny.mk
