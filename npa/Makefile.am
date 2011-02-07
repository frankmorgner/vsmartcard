ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4

EXTRA_DIST = README.dox libnpa.pc.in Doxyfile.in

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
	    env OPENSSL_VERSION="1" OPENSSL_URL="https://www2.informatik.hu-berlin.de/~morgner/files/openssl-1.tar.gz" EXTRA_OPENSSL_CONFIG="experimental-pace" win32/installer_from_build.sh

if DOC_ENABLED
.PHONY: doc
doc :
	$(do_subst) < $(srcdir)/Doxyfile.in > Doxyfile
	$(DOXYGEN) Doxyfile
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnpa.pc
libnpa.pc:
	$(do_subst) < $(srcdir)/libnpa.pc.in > libnpa.pc

clean-local:
	rm -rf doc Doxyfile libnpa.pc
