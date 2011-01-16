ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4

EXTRA_DIST = README.dox libpace.pc.in Doxyfile.in

do_subst = sed \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	   -e 's,[@]builddir[@],$(builddir),g' \
	   -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]includedir[@],$(includedir),g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' \
	   -e 's,[@]top_srcdir[@],$(top_srcdir),g'

opensc-0.11.13.tar.gz:
	wget http://www.opensc-project.org/files/opensc/opensc-0.11.13.tar.gz

opensc-0.11.13: opensc-0.11.13.tar.gz
	tar xzf opensc-0.11.13.tar.gz
	patch -p1 -d opensc-0.11.13 < opensc/le0.patch
	patch -p0 -d opensc-0.11.13 < opensc/hex_dump_align.2.patch

win: opensc-0.11.13
	cd opensc-0.11.13 && \
	    ./bootstrap && \
	    ./configure --disable-doc && \
	    env OPENSSL_VERSION="1.0.0c_with_openpace-0.4.1" OPENSSL_URL="http://downloads.sourceforge.net/project/openpace/openssl-1.0.0c_with_openpace-0.4.1.tar.gz" EXTRA_OPENSSL_CONFIG="experimental-pace" ../opensc/installer_from_build.sh opensc/win32/build

if DOC_ENABLED
.PHONY: doc
doc :
	$(do_subst) < $(srcdir)/Doxyfile.in > Doxyfile
	$(DOXYGEN) Doxyfile
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpace.pc
libpace.pc:
	$(do_subst) < $(srcdir)/libpace.pc.in > libpace.pc

clean-local:
	rm -rf doc Doxyfile libpace.pc
