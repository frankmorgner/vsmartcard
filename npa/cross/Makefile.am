EXTRA_DIST=opensc.conf_win32

all-local:
	@echo Use \`$(MAKE) win\` to cross compile for Windows
	@echo Use \`$(MAKE) android\` to cross compile for Android



OSX_TOOLS_DIR = "$(shell xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs"
OSX_TOOL_DIR ?= "$(OSX_TOOLS_DIR)/$(shell ls -1 $(OSX_TOOLS_DIR) | sort -n -k2 -t. -r | head -1)/usr"
OSX_TARGETDIR = $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx
osx: $(OPENPACE_MAKEFILE) $(OPENSC_CONFIGURE)
	@echo Compiling OpenPACE
	$(MAKE) -C $(OPENPACE) osx
	rm -rf $(OSX_TARGETDIR) dmg $(top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx.dmg
	cp -r $(OPENPACE)/*_osx $(OSX_TARGETDIR)
	@echo Configuring OpenSC
	grep _sc_card_add_ec_alg $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_card_add_ec_alg >> $(OPENSC)/src/libopensc/libopensc.exports
	grep _sc_card_add_rsa_alg $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_card_add_rsa_alg >> $(OPENSC)/src/libopensc/libopensc.exports
	grep _sc_match_atr $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_match_atr >> $(OPENSC)/src/libopensc/libopensc.exports
	grep sc_apdu_get_octets $(OPENSC)/src/libopensc/libopensc.exports || echo sc_apdu_get_octets >> $(OPENSC)/src/libopensc/libopensc.exports
	grep sc_pkcs15_pincache_add $(OPENSC)/src/libopensc/libopensc.exports || echo sc_pkcs15_pincache_add >> $(OPENSC)/src/libopensc/libopensc.exports
	@echo Compiling OpenSC
	cd $(OPENSC) && OPENSSL_LIBS="-L$(OSX_TARGETDIR)/Library/OpenPACE/lib -lcrypto" OPENSSL_CFLAGS="-I$(OSX_TARGETDIR)/Library/OpenPACE/include" MacOSX/build
	@echo Configuring libnpa
	cd $(top_builddir) && ./configure --prefix=/Library/nPA \
		PKG_CONFIG_SYSROOT_DIR=$(OSX_TARGETDIR) \
		PKG_CONFIG_PATH=$(OSX_TARGETDIR)/Library/OpenPACE/lib/pkgconfig \
		OPENSC_LIBS="-L$(OPENSC)/target/Library/OpenSC/lib -lopensc" \
		CFLAGS="-arch i386 -arch x86_64"
	@echo Compiling libnpa
	make -C $(top_builddir) install DESTDIR=$(OSX_TARGETDIR)
	mkdir -p $(OSX_TARGETDIR)/Library/nPA/etc
	cp $(top_builddir)/opensc.conf $(OSX_TARGETDIR)/Library/nPA/etc/opensc.conf
	rm -rf $(OSX_TARGETDIR)/Library/OpenPACE
	pkgbuild --root $(OSX_TARGETDIR) --scripts MacOSX_install --identifier com.vsmartcard.npa.mac --version $(PACKAGE_VERSION) --install-location / nPA-$(PACKAGE_VERSION)_install.pkg
	pkgbuild --nopayload --identifier com.vsmartcard.npa.mac.uninstall --scripts MacOSX_uninstall Uninstall_nPA.pkg
	mkdir -p dmg
	cp *.pkg dmg
	cp $(OPENPACE)/cross/*.pkg dmg
	cp $(OPENSC)/OpenSC-*.pkg dmg
	cp $(OPENSC)/Uninstall_OpenSC.pkg dmg
	hdiutil create -srcfolder dmg -volname "$(PACKAGE_STRING) for Mac OS X" $(top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx.dmg



WIN_TOOL ?= i686-w64-mingw32
WIN_TOOL_DIR ?= /usr/$(WIN_TOOL)
WIN_TARGETDIR = $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32

win:
	$(MAKE) cross \
		TOOL=$(WIN_TOOL) \
		TOOL_DIR=$(WIN_TOOL_DIR) \
		TARGETDIR=$(WIN_TARGETDIR) \
		LDFLAGS="-static-libgcc" \
		LIBEAC=$(WIN_TARGETDIR)/bin/libeac-0.dll \
		LIBNPA=$(WIN_TARGETDIR)/bin/libnpa-0.dll
	cp opensc.conf_win32 $(WIN_TARGETDIR)/etc/opensc.conf



ANDROID_ARCH ?= arm
ANDROID_TOOL ?= $(ANDROID_ARCH)-linux-androideabi
ANDROID_TOOL_DIR ?= $(abs_builddir)/ndk
ANDROID_TARGETDIR = $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_$(ANDROID_ARCH)
MAKE_STANDALONE_TOOLCHAIN ?= $${HOME}/.local/opt/android-ndk-r9/build/tools/make-standalone-toolchain.sh

android: $(VIRTUALSMARTCARD_CONFIGURE)
	test -d $(ANDROID_TOOL_DIR) \
		|| $(MAKE_STANDALONE_TOOLCHAIN) --install-dir=$(ANDROID_TOOL_DIR) --arch=$(ANDROID_ARCH)
	cd $(VIRTUALSMARTCARD) && env PATH=$(ANDROID_TOOL_DIR)/bin:$${PATH} ./configure \
		--prefix=/ \
		--host=$(ANDROID_TOOL) --target=$(ANDROID_TOOL) \
		--enable-libpcsclite --enable-vpcdhost=localhost \
		PKG_CONFIG_SYSROOT_DIR=$(ANDROID_TARGETDIR) \
		PKG_CONFIG_LIBDIR=$(ANDROID_TARGETDIR)/lib/pkgconfig \
		PKG_CONFIG_PATH=$(ANDROID_TARGETDIR)/lib/pkgconfig \
		CFLAGS=-I$(ANDROID_TOOL_DIR)/include \
		LDFLAGS=-L$(ANDROID_TOOL_DIR)/lib
	@echo Compiling virtual smartcard
	env PATH=$(ANDROID_TOOL_DIR)/bin:$${PATH} $(MAKE) -C $(VIRTUALSMARTCARD) install DESTDIR=$(ANDROID_TARGETDIR)
	env PATH=$(ANDROID_TOOL_DIR)/bin:$${PATH} $(MAKE) cross \
		TOOL=$(ANDROID_TOOL) \
		TOOL_DIR=$(ANDROID_TOOL_DIR) \
		TARGETDIR=$(ANDROID_TARGETDIR) \
		LIBEAC=$(ANDROID_TARGETDIR)/lib/libeac.so \
		LIBNPA=$(ANDROID_TARGETDIR)/lib/libnpa.so
	@echo Configuring virtual smartcard



clean-local:
	rm -rf $(TARGETDIR)



cross: $(LIBNPA)

$(LIBNPA): $(OPENPACE_MAKEFILE) $(OPENSC_CONFIGURE)
	@echo Compiling OpenPACE
	$(MAKE) -C $(OPENPACE)/cross cross \
		TOOL=$(TOOL) \
		TOOL_DIR=$(TOOL_DIR) \
		TARGETDIR=$(TARGETDIR) \
		LDFLAGS="$${LDFLAGS}" \
		LIBEAC=$(LIBEAC)
	@echo Configuring OpenSC
	grep _sc_card_add_ec_alg $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_card_add_ec_alg >> $(OPENSC)/src/libopensc/libopensc.exports
	grep _sc_card_add_rsa_alg $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_card_add_rsa_alg >> $(OPENSC)/src/libopensc/libopensc.exports
	grep _sc_match_atr $(OPENSC)/src/libopensc/libopensc.exports || echo _sc_match_atr >> $(OPENSC)/src/libopensc/libopensc.exports
	grep sc_apdu_get_octets $(OPENSC)/src/libopensc/libopensc.exports || echo sc_apdu_get_octets >> $(OPENSC)/src/libopensc/libopensc.exports
	grep sc_pkcs15_pincache_add $(OPENSC)/src/libopensc/libopensc.exports || echo sc_pkcs15_pincache_add >> $(OPENSC)/src/libopensc/libopensc.exports
	cd $(OPENSC) && ./configure \
		--prefix=/ \
		--host=$(TOOL) --target=$(TOOL) \
		--enable-sm \
		PKG_CONFIG_SYSROOT_DIR=$(TARGETDIR) \
		PKG_CONFIG_LIBDIR=$(TARGETDIR)/lib/pkgconfig \
		PKG_CONFIG_PATH=$(TARGETDIR)/lib/pkgconfig \
		CFLAGS="-I$(TOOL_DIR)/include -I$(TARGETDIR)/include" \
		LDFLAGS="$${LDFLAGS} -L$(TOOL_DIR)/lib"
	@echo Compiling OpenSC
	$(MAKE) -C $(OPENSC) install DESTDIR=$(TARGETDIR)
	@echo Configuring libnpa
	cd $(top_builddir) && env ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ./configure \
		--prefix=/ \
		--host=$(TOOL) --target=$(TOOL) \
		PKG_CONFIG_SYSROOT_DIR=$(TARGETDIR) \
		PKG_CONFIG_LIBDIR=$(TARGETDIR)/lib/pkgconfig \
		PKG_CONFIG_PATH=$(TARGETDIR)/lib/pkgconfig \
		OPENSC_LIBS="-L$(TARGETDIR)/lib -lopensc" \
		CFLAGS="-I$(TOOL_DIR)/include -I$(TARGETDIR)/include" \
		LDFLAGS="$${LDFLAGS} -L$(TOOL_DIR)/lib"
	@echo Compiling libnpa
	env CROSS_COMPILE=$(CROSS_COMPILE) \
		$(MAKE) -C $(top_builddir) install DESTDIR=$(TARGETDIR)



OPENPACE=$(abs_builddir)/openpace
OPENPACE_CONFIGURE_AC=$(OPENPACE)/configure.ac
OPENPACE_CONFIGURE=$(OPENPACE)/configure
OPENPACE_MAKEFILE=$(OPENPACE)/Makefile

$(OPENPACE_CONFIGURE_AC):
	git clone https://github.com/frankmorgner/openpace.git $(OPENPACE)

$(OPENPACE_CONFIGURE): $(OPENPACE_CONFIGURE_AC)
	cd $(OPENPACE) && autoreconf --verbose --install --symlink

$(OPENPACE_MAKEFILE): $(OPENPACE_CONFIGURE)
	cd $(OPENPACE) && ./configure --enable-openssl-install



OPENSC=$(abs_builddir)/opensc
OPENSC_CONFIGURE_AC=$(OPENSC)/configure.ac
OPENSC_CONFIGURE=$(OPENSC)/configure

$(OPENSC_CONFIGURE_AC):
	git clone git://github.com/OpenSC/OpenSC.git $(OPENSC)

$(OPENSC_CONFIGURE): $(OPENSC_CONFIGURE_AC)
	cd $(OPENSC) && autoreconf --verbose --install --symlink



VSMARTCARD=$(abs_builddir)/vsmartcard
VIRTUALSMARTCARD=$(VSMARTCARD)/virtualsmartcard
VIRTUALSMARTCARD_CONFIGURE_AC=$(VIRTUALSMARTCARD)/configure.ac
VIRTUALSMARTCARD_CONFIGURE=$(VIRTUALSMARTCARD)/configure

$(VIRTUALSMARTCARD_CONFIGURE_AC):
	git clone git://github.com/frankmorgner/vsmartcard.git $(VSMARTCARD)

$(VIRTUALSMARTCARD_CONFIGURE): $(VIRTUALSMARTCARD_CONFIGURE_AC)
	cd $(VIRTUALSMARTCARD) && autoreconf --verbose --install --symlink
