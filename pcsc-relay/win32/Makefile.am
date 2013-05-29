EXTRA_DIST =


all-local:
	@echo "Use \`make win\` to cross compile for Windows"


WIN32_DIR = $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32


MINGW ?= i686-w64-mingw32
MINGW_DIR=/usr/$(MINGW)


LIBNFC_DIR=$(abs_builddir)/libnfc
LIBNFC_DLL=$(WIN32_DIR)/bin/libnfc-4.dll
$(LIBNFC_DIR)/configure.ac:
	git clone git://github.com/frankmorgner/libnfc $(LIBNFC_DIR)
$(LIBNFC_DLL): $(LIBNFC_DIR)/configure.ac
	cd $(LIBNFC_DIR) && env MINGW=$(MINGW) sh mingw-cross-configure.sh --prefix=/
	make -C $(LIBNFC_DIR) install DESTDIR=$(WIN32_DIR)


PCSC_RELAY_EXE=$(WIN32_DIR)/bin/pcsc-relay.exe
$(PCSC_RELAY_EXE): $(LIBNFC_DLL)
	cd $(top_builddir) && ./configure \
		--prefix=/ \
		--host=$(MINGW) --target=$(MINGW) \
		PKG_CONFIG_SYSROOT_DIR=$(WIN32_DIR) \
		PKG_CONFIG_LIBDIR=$(WIN32_DIR)/lib/pkgconfig \
		CFLAGS=-I$(MINGW_DIR)/include \
		LDFLAGS=-L$(MINGW_DIR)/lib \
		PCSC_LIBS="-lwinscard"
	make -C $(top_builddir) install DESTDIR=$(WIN32_DIR)


LIBUSB_DLL=$(LIBNFC_DIR)/libusb-win32-bin-*/bin/x86/libusb0_x86.dll
win: $(PCSC_RELAY_EXE)
	$(INSTALL_PROGRAM) $(LIBUSB_DLL) $(WIN32_DIR)/bin/libusb0.dll

clean-local:
	rm -rf $(WIN32_DIR)
	test ! -d $(LIBNFC_DIR) || \
		make -C $(LIBNFC_DIR) clean

distclean-local:
	rm -rf $(LIBNFC_DIR)
