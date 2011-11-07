ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 doc

EXTRA_DIST = libnpa.pc.in apdus

do_subst = sed \
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

#do_export = sed -e 's,-export-symbols "[$$](srcdir)/libopensc.exports",,g' \
			#-e 's,libopensc.exports,,g' \
		   	#< src/libopensc/Makefile.am \
			#> src/libopensc/Makefile.am
do_export = cp $(abs_top_srcdir)/Makefile.am.opensc src/libopensc/Makefile.am

fixup_la = find "$(srcdir)/src/opensc/win32/build/image" -name '*.la' | while read f; do \
		   top="$$(dirname "$$(dirname "$$(echo "$${f}")")" | sed 's\#//\#/\#g')" \
		   sed 's\#//\#'"$${top}"'/\#g' "$${f}" > "$${f}.tmp"; \
		   mv "$${f}.tmp" "$${f}"; \
		   done
restore_la = find "$(srcdir)/src/opensc/win32/build/image" -name '*.la' | while read f; do \
			 x="$$(echo "$(srcdir)/src/opensc/win32/build/image" | sed 's\#/$$\#\#')" \
			 sed 's\#'"$${x}"'/[^/]*/lib\#//lib\#g' "$${f}" > "$${f}.tmp"; \
			 mv "$${f}.tmp" "$${f}"; \
			 done

src/opensc/win32/build/image/opensc/lib/libopensc.la:
	cd $(top_srcdir)/src/opensc && \
		./bootstrap && \
		./configure --disable-doc && \
		EXTRA_OPENSC_CONFIG="`$(do_export)`" OPENSSL_VERSION="1.0.0e" OPENSSL_URL="https://www2.informatik.hu-berlin.de/~morgner/files/openssl-1.0.0e.tar.gz" EXTRA_OPENSSL_CONFIG="experimental-pace" win32/installer_from_build.sh

src/opensc/win32/build/image/opensc/lib/libnpa.la: src/opensc/win32/build/image/opensc/lib/libopensc.la
	$(fixup_la)
	export ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes; ./configure \
		--sbindir=/bin \
		--prefix=/ \
		--host=i586-mingw32msvc \
		--build=i686-pc-linux-gnu \
		--program-prefix='' \
		OPENSSL_CFLAGS="-I$(abs_top_srcdir)/src/opensc/win32/build/image/opensc/include" \
		OPENSSL_LIBS="-L$(abs_top_srcdir)/src/opensc/win32/build/image/opensc/lib -lcrypto -lws2_32 -lgdi32 -lcrypt32" \
		OPENSC_LIBS="-L$(abs_top_srcdir)/src/opensc/win32/build/image/opensc/lib -lopensc"
	make install DESTDIR=$(abs_top_srcdir)/src/opensc/win32/build/image/opensc
	$(restore_la)

win-opensc: src/opensc/win32/build/image/opensc/lib/libopensc.la

win: src/opensc/win32/build/image/opensc/lib/libnpa.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnpa.pc
libnpa.pc:
	$(do_subst) < $(srcdir)/libnpa.pc.in > libnpa.pc

clean-local:
	rm -f libnpa.pc
