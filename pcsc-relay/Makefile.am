SUBDIRS=src doc


LIBUSB_WIN32_BIN_VERSION=1.2.6.0
LIBUSB_WIN32_BIN_DIR=libusb-win32-bin-$(LIBUSB_WIN32_BIN_VERSION)
LIBUSB_DLL=$(abs_builddir)/$(LIBUSB_WIN32_BIN_DIR)/bin/x86/libusb0_x86.dll
LIBUSB_WIN32_BIN_ARCHIVE=$(LIBUSB_WIN32_BIN_DIR).zip
$(LIBUSB_WIN32_BIN_ARCHIVE):
	wget http://freefr.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/$(LIBUSB_WIN32_BIN_VERSION)/$(LIBUSB_WIN32_BIN_ARCHIVE)
$(LIBUSB_DLL): $(LIBUSB_WIN32_BIN_ARCHIVE)
	unzip $<


LIBNFC_DIR=$(abs_builddir)/libnfc
LIBNFC_DLL=$(LIBNFC_DIR)/libnfc.dll
$(LIBNFC_DIR):
	svn co http://libnfc.googlecode.com/svn/trunk $@
$(LIBNFC_DLL): $(LIBNFC_DIR) $(LIBUSB_DLL)
	cd $< && wget http://www2.informatik.hu-berlin.de/~morgner/libnfc.dll


MINGW=i686-w64-mingw32
MINGW_DIR=/usr/$(MINGW)
PCSC_RELAY_EXE=$(abs_builddir)/src/pcsc-relay.exe
$(PCSC_RELAY_EXE): $(LIBNFC_DIR) $(LIBNFC_DLL)
	./configure --target=$(MINGW) --host=$(MINGW) \
		PKG_CONFIG_LIBDIR=$(MINGW_DIR)/lib/pkgconfig \
		CFLAGS=-I$(MINGW_DIR)/include \
		LDFLAGS=-L$(MINGW_DIR)/lib \
		PCSC_LIBS="-lwinscard" \
		LIBNFC_CFLAGS="-I$(LIBNFC_DIR)/include" \
		LIBNFC_LIBS="$(LIBNFC_DLL)"
	make


WIN32_DIR=$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32
win: src/pcsc-relay.exe
	mkdir -p $(WIN32_DIR)
	for i in \
		$(LIBNFC_DLL) \
		$(PCSC_RELAY_EXE) \
		; do cp $${i} $(WIN32_DIR); done
	cp $(LIBUSB_DLL) $(WIN32_DIR)/libusb0.dll
