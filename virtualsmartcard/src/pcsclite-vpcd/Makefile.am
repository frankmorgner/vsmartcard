EXTRA_DIST 				= libpcsclite.pc.in

lib_LTLIBRARIES         = libpcsclite.la

libpcsclite_la_CPPFLAGS = $(PCSC_CFLAGS) -I$(srcdir)/../ifd-vpcd -I$(srcdir)/../vpcd
libpcsclite_la_LDFLAGS  = -no-undefined -version-info 1:0:0
libpcsclite_la_SOURCES  = winscard.c error.c
libpcsclite_la_LIBADD   = $(top_builddir)/src/ifd-vpcd/libifdvpcd.la

noinst_HEADERS = misc.h

nobase_include_HEADERS  = \
						  PCSC/pcsclite.h \
						  PCSC/winscard.h \
						  PCSC/ifdhandler.h \
						  PCSC/wintypes.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpcsclite.pc
libpcsclite.pc: libpcsclite.pc.in
	$(do_subst) < $(srcdir)/libpcsclite.pc.in > libpcsclite.pc

do_subst = $(SED) \
		   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
		   -e 's,[@]VERSION[@],$(PCSCVERSION),g' \
		   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
		   -e 's,[@]includedir[@],$(includedir),g' \
		   -e 's,[@]libdir[@],$(libdir),g' \
		   -e 's,[@]prefix[@],$(prefix),g'

clean-local:
	rm -f libpcsclite.pc
