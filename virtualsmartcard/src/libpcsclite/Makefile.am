EXTRA_DIST 				= PCSC/pcsclite.h.in libpcsclite.pc.in

lib_LTLIBRARIES         = libpcsclite.la

libpcsclite_la_CPPFLAGS = $(PCSC_CFLAGS) -I$(srcdir)/../vpcd
libpcsclite_la_LDFLAGS  = -no-undefined
libpcsclite_la_SOURCES  = winscard.c $(top_builddir)/src/vpcd/libvpcd.la

noinst_HEADERS = pcsclite.h

nobase_include_HEADERS  = \
						  PCSC/pcsclite.h \
						  PCSC/winscard.h \
						  PCSC/wintypes.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpcsclite.pc
libpcsclite.pc: libpcsclite.pc.in
	$(do_subst) < $(srcdir)/libpcsclite.pc.in > libpcsclite.pc

winscard.c: $(builddir)/pcsclite.h

do_subst = $(SED) \
		   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
		   -e 's,[@]VERSION[@],$(PCSCVERSION),g' \
		   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
		   -e 's,[@]includedir[@],$(includedir),g' \
		   -e 's,[@]libdir[@],$(libdir),g' \
		   -e 's,[@]prefix[@],$(prefix),g'

pcsclite.h: PCSC/pcsclite.h.in
	$(do_subst) < $(srcdir)/PCSC/pcsclite.h.in > $(builddir)/pcsclite.h

install-data-local:
	$(INSTALL) -d $(DESTDIR)$(includedir)/PCSC
	$(INSTALL_DATA) $(builddir)/pcsclite.h $(DESTDIR)$(includedir)/PCSC/pcsclite.h

uninstall-local:
	rm -f $(DESTDIR)$(includedir)/PCSC/pcsclite.h

clean-local:
	rm -f libpcsclite.pc pcsclite.h
