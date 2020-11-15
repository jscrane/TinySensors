BOARD := tinysensor
BOARD_CLOCK := 8internal
SKETCH := tinysensor.ino
TERMINAL_SPEED := 9600
CPPFLAGS = -DNODE_ID=$(NODE_ID)
ifeq ($(DEBUG),1)
CPPFLAGS += -DDEBUG=$(DEBUG)
endif

-include tiny.mk

$(runtime.platform.path)/boards.local.txt: $(runtime.platform.path)
	install -c -m 0644 boards.local.mk $^

$(runtime.platform.path)/bootloaders/optiboot/%.hex: optiboot/%.nex
	install -c -m 0644 $^ $(dir $@)

init: $(runtime.platform.path)/boards.local.txt $(addprefix $(runtime.platform.path)/bootloaders/, $(wildcard optiboot/*.hex))
