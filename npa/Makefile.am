ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 doc

EXTRA_DIST = libnpa.pc.in apdus

do_subst = $(SED) \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	   -e 's,[@]builddir[@],$(builddir),g' \
	   -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]includedir[@],$(includedir),g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' \
	   -e 's,[@]OPENSC_LIBS[@],$(OPENSC_LIBS),g' \
	   -e 's,[@]top_srcdir[@],$(top_srcdir),g'

do_export = cp $(abs_srcdir)/Makefile.am.opensc src/libopensc/Makefile.am

opensc_img = $(abs_srcdir)/src/opensc/win32/build/image
opensc_root = $(opensc_img)/opensc

fixup_la = find "$(opensc_img)" -name '*.la' | while read f; do \
		   top="$$(dirname "$$(dirname "$$(echo "$${f}")")" | $(SED) 's\#//\#/\#g')" \
		   $(SED) 's\#//\#'"$${top}"'/\#g' "$${f}" > "$${f}.tmp"; \
		   mv "$${f}.tmp" "$${f}"; \
		   done
restore_la = find "$(opensc_img)" -name '*.la' | while read f; do \
			 x="$$(echo "$(opensc_img)" | $(SED) 's\#/$$\#\#')" \
			 $(SED) 's\#'"$${x}"'/[^/]*/lib\#//lib\#g' "$${f}" > "$${f}.tmp"; \
			 mv "$${f}.tmp" "$${f}"; \
			 done

$(opensc_root)/lib/libopensc.la:
	cd $(top_srcdir)/src/opensc && \
		./bootstrap && \
		./configure --disable-doc && \
		EXTRA_OPENSC_CONFIG="`$(do_export)`" \
		ZLIB_VERSION="1.2.6" \
		OPENSSL_VERSION="1.0.1_with_openpace-0.7" \
		OPENSSL_URL="http://www2.informatik.hu-berlin.de/~morgner/openssl-1.0.1_with_openpace-0.7.tar.gz" \
		EXTRA_OPENSSL_CONFIG="experimental-pace" \
		win32/installer_from_build.sh

$(opensc_root)/lib/libnpa.la: $(opensc_root)/lib/libopensc.la
	$(fixup_la)
	export ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes; ./configure \
		--sbindir=/bin \
		--prefix=/ \
		--host=i586-mingw32msvc \
		--build=$(shell gcc -dumpmachine) \
		--program-prefix='' \
		LIBS="-lws2_32" \
		PKG_CONFIG_PATH="$(opensc_root)/lib/pkgconfig" \
		PKG_CONFIG_SYSROOT_DIR="$(opensc_root)" \
		OPENSC_LIBS="-L$(opensc_root)/lib -lopensc" || ($(restore_la) && false)
	make install DESTDIR="$(opensc_root)" || ($(restore_la) && false)
	$(restore_la)

win-opensc: $(opensc_root)/lib/libopensc.la

win: $(opensc_root)/lib/libnpa.la
	mkdir -p $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32
	for i in \
		$(opensc_root)/bin/*npa*.dll \
		$(opensc_root)/bin/*opensc*.dll \
		$(opensc_root)/bin/*eay*.dll \
		$(opensc_root)/bin/*ltdl*.dll \
		$(opensc_root)/bin/*zlib*.dll \
		$(opensc_root)/bin/npa-tool.exe \
		; do cp $${i} $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32; done
	mkdir -p $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/include
	for i in \
		$(opensc_root)/include/openssl \
		$(opensc_root)/include/npa \
		; do cp -r $${i} $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/include; done
	mkdir -p $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/include/libopensc
	cp $(srcdir)/src/opensc/src/libopensc/*.h $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/include/libopensc
	rm -f $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/*pkcs* $(PACKAGE_TARNAME)-$(PACKAGE_VERSION)_win32/include/libopensc/*internal*.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnpa.pc
libnpa.pc:
	$(do_subst) < $(srcdir)/libnpa.pc.in > libnpa.pc

clean-local:
	rm -f libnpa.pc

dist-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev || true
