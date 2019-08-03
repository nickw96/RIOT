BOARD ?= nucleo-f446re
include ../Makefile.tests_common

ifeq (arduino-due,$(BOARD))
  # Connect to lower row of the ISCP header: left=gnd, middle=PA26, right=5V
  PIN ?= GPIO_PIN(PA,26)
else
  PIN ?= GPIO_PIN(0,0)
endif

USEMODULE += neopixel

include $(RIOTBASE)/Makefile.include

CFLAGS += '-DNEOPIXEL_PARAM_PIN=$(PIN)'
