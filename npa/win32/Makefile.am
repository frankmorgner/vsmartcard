MINGW ?= i686-w64-mingw32
MINGW_DIR ?= /usr/$(MINGW)
WIN32_DIR ?= $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32

OPENPACE=$(abs_builddir)/openpace
OPENPACE_CONFIGURE_AC=$(OPENPACE)/configure.ac
OPENPACE_CONFIGURE=$(OPENPACE)/configure
OPENPACE_MAKEFILE=$(OPENPACE)/Makefile
LIBEAC=$(WIN32_DIR)/bin/libeac-0.dll

OPENSC=$(abs_builddir)/opensc
OPENSC_CONFIGURE_AC=$(OPENSC)/configure.ac
OPENSC_CONFIGURE=$(OPENSC)/configure
LIBOPENSC=$(WIN32_DIR)/bin/libopensc-3.dll

NPA_TOOL=$(WIN32_DIR)/bin/npa-tool.exe



$(OPENPACE_CONFIGURE_AC):
	git clone git://openpace.git.sourceforge.net/gitroot/openpace/openpace $(OPENPACE)

$(OPENPACE_CONFIGURE): $(OPENPACE_CONFIGURE_AC)
	cd $(OPENPACE) && autoreconf --verbose --install --symlink

$(OPENPACE_MAKEFILE): $(OPENPACE_CONFIGURE)
	cd $(OPENPACE) && ./configure

$(LIBEAC): $(OPENPACE_MAKEFILE)
	make -C $(OPENPACE) win MINGW=$(MINGW) WIN32_DIR=$(WIN32_DIR)



$(OPENSC_CONFIGURE_AC):
	git clone git://github.com/OpenSC/OpenSC.git $(OPENSC)

$(OPENSC_CONFIGURE): $(OPENSC_CONFIGURE_AC)
	cd $(OPENSC) && autoreconf --verbose --install --symlink

$(LIBOPENSC): $(OPENSC_CONFIGURE) $(LIBEAC)
	cd $(OPENSC) && ./configure \
		--prefix=/ \
		--target=$(MINGW) --host=$(MINGW) \
		PKG_CONFIG_SYSROOT_DIR=$(WIN32_DIR) \
		PKG_CONFIG_LIBDIR=$(WIN32_DIR)/lib/pkgconfig \
		CFLAGS=-I$(MINGW_DIR)/include \
		LDFLAGS=-L$(MINGW_DIR)/lib
	$(MAKE) -C $(OPENSC) install DESTDIR=$(WIN32_DIR)



$(NPA_TOOL): $(LIBEAC) $(LIBOPENSC)
	cd $(abs_top_builddir) \
		&& export ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes \
		&& ./configure \
		--prefix=/ \
		--host=$(MINGW) --target=$(MINGW) \
		PKG_CONFIG_SYSROOT_DIR=$(WIN32_DIR) \
		PKG_CONFIG_LIBDIR=$(WIN32_DIR)/lib/pkgconfig \
		CFLAGS=-I$(MINGW_DIR)/include \
		LDFLAGS="-L$(MINGW_DIR)/lib " \
		OPENSC_LIBS="-L$(WIN32_DIR)/lib -lopensc"
	$(MAKE) -C $(abs_top_builddir) install DESTDIR=$(WIN32_DIR)



all-local:
	@echo Use \`$(MAKE) win\` to cross compile for Windows

win: $(NPA_TOOL)
	$(INSTALL) -d $(WIN32_DIR)/include/libopensc
	$(INSTALL_DATA) -t $(WIN32_DIR)/include/libopensc \
		$(OPENSC)/src/libopensc/*.h
	rm -f $(WIN32_DIR)/include/libopensc/*internal*.h

clean-local:
	rm -rf $(IMAGE) $(WIN32_DIR)

distclean-local:
	rm -rf $(OPENSC) $(BUILD)
