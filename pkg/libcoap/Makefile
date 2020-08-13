PKG_NAME=libcoap
PKG_URL=https://github.com/obgm/libcoap
PKG_VERSION=cbc3430ade825ec54f166d418b401e5f178f7be7
PKG_LICENSE=BSD-2-Clause

include $(RIOTBASE)/pkg/pkg.mk

TOOLCHAIN_FILE=$(PKG_SOURCE_DIR)/xcompile-toolchain.cmake
CFLAGS += $(INCLUDES)
CFLAGS += -Wno-missing-include-dirs
CFLAGS += -Wno-unused-parameter
CFLAGS += -D_DEFAULT_SOURCE
CFLAGS := $(filter-out -DCPU% -DBOARD% -DMCU% -DRIOT_APPLICATION%,$(CFLAGS))

all: $(TOOLCHAIN_FILE)
	cmake \
	    -B "$(PKG_BUILD_DIR)" \
	    -S "$(PKG_SOURCE_DIR)" \
	    -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_FILE)" \
	    -DCMAKE_C_COMPILER_WORKS=TRUE \
	    -DCMAKE_CXX_COMPILER_WORKS=TRUE \
	    -DENABLE_DTLS=OFF \
	    -DENABLE_TCP=OFF \
	    -DENABLE_DOCS=OFF \
	    -DENABLE_EXAMPLES=OFF \
	    -DCMAKE_BUILD_TYPE=None \
	    -DCMAKE_C_FLAGS="$(CFLAGS)"
	"$(MAKE)" -C "$(PKG_BUILD_DIR)" VERBOSE=1
	cp "$(PKG_BUILD_DIR)/libcoap-2.a" "$(BINDIR)/$(PKG_NAME).a"

$(TOOLCHAIN_FILE):
	$(RIOTTOOLS)/cmake/generate-xcompile-toolchain.sh > $(TOOLCHAIN_FILE)
