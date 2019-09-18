# name of your application
APPLICATION = rss

# If no BOARD is found in the environment, use this default:
BOARD ?= bluepill

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
USEMODULE += auto_init_multimedia
USEMODULE += dfplayer
USEMODULE += mpu9150
USEMODULE += neopixel
USEMODULE += ps
USEMODULE += saul_default
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include

CFLAGS += '-DNEOPIXEL_PARAM_PIN=GPIO_PIN(PORT_B, 5)'
CFLAGS += '-DDFPLAYER_PARAM_BUSY_PIN=GPIO_PIN(PORT_A, 8)'
CFLAGS += '-DDFPLAYER_PARAM_UART=UART_DEV(0)'
