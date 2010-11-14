ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4

DISTCLEANFILES = Doxyfile libpace.pc

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

if DOC_ENABLED
clean-local:
	rm -rf doc

doc :
	$(do_subst) < $(srcdir)/Doxyfile.in > Doxyfile
	$(DOXYGEN) Doxyfile

.PHONY: doc
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpace.pc
libpace.pc :
	$(do_subst) < $(srcdir)/libpace.pc.in > libpace.pc
