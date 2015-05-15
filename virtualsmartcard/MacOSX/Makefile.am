EXTRA_DIST = MacOSX_install/postinstall MacOSX_uninstall/postinstall

all-local:
	@echo Use \`$(MAKE) osx\` to compile package for Mac OS X



OSX_TOOLS_DIR = "$(shell xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs"
OSX_TOOL_DIR ?= "$(OSX_TOOLS_DIR)/$(shell ls -1 $(OSX_TOOLS_DIR) | sort -n -k2 -t. -r | head -1)/usr"
OSX_TARGETDIR = $(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx
osx: $(OPENPACE_MAKEFILE)
	@echo Compiling OpenPACE
	$(MAKE) -C $(OPENPACE) osx
	rm -rf $(OSX_TARGETDIR) dmg $(top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx.dmg
	@echo Configuring virtualsmartcard for IFD bundle
	cd $(top_builddir) && ./configure --prefix=/Library/VirtualSmartCard \
		CFLAGS="-arch i386 -arch x86_64" --enable-infoplist
	@echo Compiling virtualsmartcard
	make -C $(top_builddir) clean install DESTDIR=$(OSX_TARGETDIR)
	@echo Configuring virtualsmartcard for builtin libpcsclite
	cd $(top_builddir) && ./configure --prefix=/Library/VirtualSmartCard \
		CFLAGS="-arch i386 -arch x86_64" --enable-libpcsclite
	@echo Compiling virtualsmartcard
	make -C $(top_builddir) clean install DESTDIR=$(OSX_TARGETDIR)
	pkgbuild --root $(OSX_TARGETDIR) --scripts MacOSX_install --identifier com.vsmartcard.virtualsmartcard.mac --version $(PACKAGE_VERSION) --install-location / VirtualSmartCard-$(PACKAGE_VERSION)_install.pkg
	pkgbuild --nopayload --identifier com.vsmartcard.virtualsmartcard.mac.uninstall --scripts MacOSX_uninstall Uninstall_VirtualSmartCard.pkg
	mkdir -p dmg
	cp *.pkg dmg
	cp $(OPENPACE)/cross/*.pkg dmg
	hdiutil create -srcfolder dmg -volname "$(PACKAGE_STRING) for Mac OS X" $(top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_osx.dmg



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
