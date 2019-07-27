BOARD ?= nucleo-f767zi
include ../Makefile.tests_common

# This test will rely on a human interacting with the shell, so we better add
# the shell and some commands
USEMODULE += shell
USEMODULE += shell_commands
# checking current state of network threads can be useful for testing/debuggig
USEMODULE += ps
# Add the driver to test
USEMODULE += mfrc522

include $(RIOTBASE)/Makefile.include
