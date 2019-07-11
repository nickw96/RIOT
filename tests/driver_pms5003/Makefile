include ../Makefile.tests_common

BOARD_INSUFFICIENT_MEMORY += arduino-duemilanove arduino-leonardo arduino-nano \
                             arduino-uno
BOARD ?= nucleo-f446re

USEMODULE += fmt
USEMODULE += pms5003
USEMODULE += saul_reg
USEMODULE += auto_init_saul
USEMODULE += auto_init
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include

ifeq ($(BOARD),nucleo-f446re)
  CFLAGS += '-DPMS5003_PARAM_SET=GPIO_PIN(PORT_D,2)'
  CFLAGS += '-DPMS5003_PARAM_RESET=GPIO_PIN(PORT_C,12)'
endif
