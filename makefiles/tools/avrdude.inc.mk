FLASHER = avrdude
AVARICE_PATH = $(RIOTTOOLS)/avarice
DEBUGSERVER_PORT = 4242
DEBUGSERVER = $(AVARICE_PATH)/debug_srv.sh
# Allow choosing debugger hardware via DEBUGDEVICE, default to AVR Dragon, which
# is compatible to all AVR devices and the least expensive module
DEBUGDEVICE ?= -g
DEBUGINTERFACE ?= usb
ifneq (,$(filter $(CPU),atmega328p))
  # Use debugWIRE as protocol for debugging (ATmega328P does not support JTAG)
  DEBUGPROTO := -w
else
  # Use JTAG as protocol for debugging
  DEBUGPROTO := -j $(DEBUGINTERFACE)
endif
DEBUGSERVER_FLAGS = "$(DEBUGDEVICE) $(DEBUGPROTO) :$(DEBUGSERVER_PORT)"
DEBUGGER_FLAGS = "-x $(AVARICE_PATH)/gdb.conf $(ELFFILE)"
DEBUGGER = "$(AVARICE_PATH)/debug.sh" $(DEBUGSERVER_FLAGS) $(AVARICE_PATH) $(DEBUGSERVER_PORT)

PROGRAMMER_FLAGS = -p $(subst atmega,m,$(CPU))

# Set flasher port only for programmers that require it
ifneq (,$(filter $(PROGRAMMER),arduino avr109 buspirate stk500v1 stk500v2 wiring))
  # make the flasher port configurable (e.g. with atmelice the port is usb)
  # defaults to terminal's serial port if not configured
  PROGRAMMER_FLAGS += -P $(PROG_DEV)
endif
PROGRAMMER_FLAGS += $(FFLAGS_EXTRA)

# don't force to flash HEXFILE, but set it as default
FLASHFILE ?= $(HEXFILE)
FFLAGS += -c $(PROGRAMMER) $(PROGRAMMER_FLAGS) -U flash:w:$(FLASHFILE)

ifeq (,$(filter $(PROGRAMMER),arduino avr109 stk500v1 stk500v2 wiring))
  # Use avrdude to trigger a reset, if programming is not done via UART and a
  # bootloader.
  RESET ?= $(FLASHER) -c $(PROGRAMMER) $(PROGRAMMER_FLAGS)
endif
