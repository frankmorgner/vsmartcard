do_subst = sed \
	   -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

BUILT_SOURCES = cmdline.h cmdline.c

EXTRA_DIST  = ccid-emulator.ggo ccid-emulator.ggo.in
EXTRA_DIST += $(shell find $(top_srcdir)/src/opensc -path '*/.svn' -prune -o -type f -a -name '*.h' -print)
EXTRA_DIST += $(shell find -L $(top_srcdir)/src/opensc-npa -path '*/.git' -prune -o -type f -a -name '*.h' -print)
DISTCLEANFILES = $(BUILT_SOURCES) ccid-emulator.ggo

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c sslutil.c $(BUILT_SOURCES)
ccid_emulator_LDADD =  $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS)


ccid-emulator.c: $(BUILT_SOURCES)

$(BUILT_SOURCES): ccid-emulator.ggo
	gengetopt --output-dir=$(srcdir) < $<

ccid-emulator.ggo: ccid-emulator.ggo.in
	$(do_subst) < $< > $(srcdir)/$@


bin_PROGRAMS =

noinst_HEADERS = \
		 pcscutil.h \
		 scutil.h \
		 ccid.h \
		 sslutil.h \
		 usbstring.h

if WITH_CCID
bin_PROGRAMS += ccid-emulator
endif


if WITH_PACE
AM_CPPFLAGS = -I$(top_srcdir)/src/opensc-npa/src

ccid_emulator_LDADD += $(LIBNPA_LIBS)
ccid_emulator_CFLAGS += $(LIBNPA_CFLAGS)
else
AM_CPPFLAGS = -I$(top_srcdir)/src/opensc/src

ccid_emulator_SOURCES += scutil.c
ccid_emulator_LDADD += $(OPENSC_LIBS)
endif
