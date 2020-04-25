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
MAINTAINERCLEANFILES = $(BUILT_SOURCES) ccid-emulator.ggo $(dist_man1_MANS)

dist_man1_MANS		= ccid-emulator.1

ccid_emulator_SOURCES = cmdline.c ccid.c usbstring.c usb.c scutil.c
ccid_emulator_LDADD = OpenSC/src/libopensc/libopensc.la
ccid_emulator_LDFLAGS = $(OPENSSL_LIBS) $(PTHREAD_LIBS) -static
ccid_emulator_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS) -I$(top_srcdir)/src/OpenSC/src


$(BUILT_SOURCES): ccid-emulator.ggo
	$(AM_V_GEN)$(GENGETOPT) --output-dir=$(srcdir) < $<

ccid-emulator.ggo: ccid-emulator.ggo.in
	$(AM_V_GEN)$(do_subst) < $< > $(srcdir)/$@


ccid-emulator.1:
	$(AM_V_GEN)$(HELP2MAN) \
		--output=$@ \
		--no-info \
		--no-discard-stderr \
		--source='$(PACKAGE_STRING)' \
		--help-option='--input=$< --set-package=$(PACKAGE) --set-version=$(VERSION) --show-help' \
		--version-string='$(PACKAGE) $(VERSION)' \
		$(GENGETOPT)


bin_PROGRAMS = ccid-emulator

noinst_HEADERS = \
		 ccid.h \
		 cmdline.h \
		 scutil.h \
		 usbstring.h
