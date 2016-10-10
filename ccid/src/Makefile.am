do_subst = $(SED) \
	   -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_SUMMARY[@],$(PACKAGE_SUMMARY),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

BUILT_SOURCES = cmdline.h cmdline.c

EXTRA_DIST  = ccid-emulator.ggo ccid-emulator.ggo.in
EXTRA_DIST += $(shell find $(top_srcdir)/src/OpenSC/src -path '*/.git' -prune -o -type f -a \( -name '*.h' -o -name 'apdu.c' -o -name 'card.c' \) -print)
MAINTAINERCLEANFILES = $(BUILT_SOURCES) ccid-emulator.ggo $(dist_man1_MANS)

dist_man1_MANS		= ccid-emulator.1

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c scutil.c $(BUILT_SOURCES)
ccid_emulator_LDADD =  $(OPENSC_LIBS) $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS) -I$(top_srcdir)/src/OpenSC/src


ccid-emulator.c: $(BUILT_SOURCES)

$(BUILT_SOURCES): ccid-emulator.ggo
	$(GENGETOPT) --output-dir=$(srcdir) < $<

ccid-emulator.ggo: ccid-emulator.ggo.in
	$(do_subst) < $< > $(srcdir)/$@


ccid-emulator.1:
	$(HELP2MAN) \
		--output=$@ \
		--no-info \
		--source='$(PACKAGE_STRING)' \
		--version-string='$(PACKAGE) $(VERSION)' \
		$(builddir)/ccid-emulator


bin_PROGRAMS = ccid-emulator

noinst_HEADERS = \
		 ccid.h \
		 ccid-types.h \
		 scutil.h \
		 usbstring.h
