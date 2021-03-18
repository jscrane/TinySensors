BOARD := tinysensor
BOARD_CLOCK := 8internal
SKETCH := tinysensor.ino
TERMINAL_SPEED := 9600
CPPFLAGS = -DNODE_ID=$(NODE_ID) -DRF24_POWERUP_DELAY=0
ifeq ($(DEBUG),1)
CPPFLAGS += -DDEBUG=$(DEBUG)
endif

LOCAL_LIBRARY_PATH := ~/src/nRF24

-include tiny.mk

$(runtime.platform.path)/boards.local.txt: boards.local.txt
	install -c -m 0644 boards.local.txt $(runtime.platform.path)

$(runtime.platform.path)/bootloaders/optiboot/%.hex:
	install -c -m 0644 optiboot/$(notdir $@) $(dir $@)

init: $(runtime.platform.path)/boards.local.txt $(addprefix $(runtime.platform.path)/bootloaders/, $(wildcard optiboot/*.hex))
