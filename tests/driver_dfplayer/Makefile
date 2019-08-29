include ../Makefile.tests_common

BUSY_PIN ?= GPIO_PIN(0,8)

USEMODULE += dfplayer
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include

CFLAGS += '-DDFPLAYER_PARAM_BUSY_PIN=$(BUSY_PIN)'
