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

win:
	cd $(top_srcdir)/src/opensc && \
		./bootstrap && \
		./configure --disable-doc && \
		env OPENSSL_VERSION="1.0.0e" OPENSSL_URL="https://www2.informatik.hu-berlin.de/~morgner/files/openssl-1.0.0e.tar.gz" EXTRA_OPENSSL_CONFIG="experimental-pace" win32/installer_from_build.sh
	env CHOST=i586-mingw32msvc CBUILD=i686-pc-linux-gnu \
		./configure --sbindir=/bin \
		OPENSSL_CFLAGS="-I`pwd`/src/opensc/win32/build/image/opensc/include" \
		OPENSSL_LIBS="-L`pwd`/src/opensc/win32/build/image/opensc/lib -lcrypto" \
		OPENSC_LIBS="-L`pwd`/src/opensc/win32/build/image/opensc/lib -lopensc"
	make install DESTDIR=`pwd`/src/opensc/win32/build/image/opensc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnpa.pc
libnpa.pc:
	$(do_subst) < $(srcdir)/libnpa.pc.in > libnpa.pc

clean-local:
	rm -f libnpa.pc
