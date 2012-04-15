EXTRA_DIST = build_fix_zlib.patch build_npa.patch


all-local:
	echo "Use \`make win\` to cross compile for Windows"


OPENSC ?= $(abs_builddir)/opensc
$(OPENSC)/configure.ac:
	git clone git://github.com/frankmorgner/OpenSC.git $(OPENSC)
	true cp Makefile.am.opensc.libopensc $(OPENSC)/src/libopensc/Makefile.am

OPENSC_TAR_GZ=$(OPENSC)/opensc-0.12.3-pre1.tar.gz
$(OPENSC_TAR_GZ): $(OPENSC)/configure.ac
	cd $(OPENSC) && ./bootstrap && ./configure --disable-doc
	make -C $(OPENSC) dist

NPA_TAR_GZ=$(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz
$(NPA_TAR_GZ):
	make -C $(abs_top_builddir) dist

BUILD=$(abs_builddir)/build
$(BUILD)/build: $(OPENSC_TAR_GZ) $(NPA_TAR_GZ)
	svn co -r123 http://www.opensc-project.org/svn/build/trunk $(BUILD)
	patch -p1 -d $(BUILD) < build_fix_zlib.patch
	patch -p1 -d $(BUILD) < build_npa.patch
	mkdir -p $(BUILD)/sources
	cp $(OPENSC_TAR_GZ) $(BUILD)/sources
	cp $(NPA_TAR_GZ) $(BUILD)/sources


MINGW ?= i586-mingw32msvc
IMAGE=$(BUILD)/image/opensc
NPA_TOOL_EXE=$(IMAGE)/bin/npa-tool.exe
$(NPA_TOOL_EXE): $(BUILD)/build
	cd $(BUILD) && \
		export ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes; \
		CHOST=$(MINGW) CBUILD=`gcc -dumpmachine` \
		ZLIB_VERSION="1.2.6" \
		./build


WIN32_DIR=$(abs_top_builddir)/$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32
win: $(NPA_TOOL_EXE)
	mv $(IMAGE) $(WIN32_DIR)
	$(INSTALL) -d $(WIN32_DIR)/include/libopensc
	$(INSTALL_DATA) -t $(WIN32_DIR)/include/libopensc \
		$(OPENSC)/src/libopensc/*.h
	rm -f $(WIN32_DIR)/include/libopensc/*internal*.h

clean-local:
	rm -rf $(IMAGE) $(WIN32_DIR)

distclean-local:
	rm -rf $(OPENSC) $(BUILD)
