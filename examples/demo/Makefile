APPLICATION = nanocoap_server
BOARD ?= bluepill-128kib
RIOTBASE ?= $(CURDIR)/../..

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_udp
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_icmpv6_echo
USEMODULE += nanocoap_sock
USEMODULE += fmt
USEMODULE += shell_commands
USEMODULE += shell
USEMODULE += cc1101
USEMODULE += dfplayer
USEMODULE += ws281x
USEMODULE += mpu9250
USEMODULE += saul_default

DEVELHELP ?= 1
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
include $(RIOTMAKE)/default-radio-settings.inc.mk

CFLAGS += '-DCC110X_PARAM_CS=GPIO_PIN(PORT_A, 4)'
CFLAGS += '-DCC110X_PARAM_GDO0=GPIO_PIN(PORT_B, 0)'
CFLAGS += '-DCC110X_PARAM_GDO2=GPIO_PIN(PORT_B, 1)'
CFLAGS += '-DCC110X_PARAM_PATABLE=(&cc110x_patable_868mhz)'
CFLAGS += '-DCC110X_PARAM_CONFIG=(&cc110x_config_433mhz_250kbps_300khz)'
CFLAGS += '-DCC110X_PARAM_CHANNELS=(&cc110x_chanmap_433mhz_300khz)'

CFLAGS += '-DWS281X_PARAM_PIN=GPIO_PIN(PORT_B, 5)'
