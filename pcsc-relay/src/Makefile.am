do_subst = $(SED) \
	   -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_SUMMARY[@],$(PACKAGE_SUMMARY),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

BUILT_SOURCES = cmdline.h cmdline.c

EXTRA_DIST  = pcsc-relay.ggo pcsc-relay.ggo.in
MAINTAINERCLEANFILES = $(BUILT_SOURCES) npa-tool.ggo $(dist_man1_MANS)

dist_man1_MANS		= pcsc-relay.1


pcsc-relay.1:
	$(AM_V_GEN)$(HELP2MAN) \
		--output=$@ \
		--no-info \
		--no-discard-stderr \
		--source='$(PACKAGE_STRING)' \
		--help-option='--input=$< --set-package=$(PACKAGE) --set-version=$(VERSION) --show-help' \
		--version-string='$(PACKAGE) $(VERSION)' \
		$(GENGETOPT)


bin_PROGRAMS = pcsc-relay

pcsc_relay_SOURCES = cmdline.c pcsc-relay.c pcsc.c vpcd.c vpcd-driver.c opicc.c lnfc.c vicc.c lock.c
pcsc_relay_LDADD = $(PCSC_LIBS) $(LIBNFC_LIBS)
pcsc_relay_CFLAGS = $(PCSC_CFLAGS) $(LIBNFC_CFLAGS)

if WIN32
pcsc_relay_LDADD += -lws2_32
endif

noinst_HEADERS = cmdline.h pcsc-relay.h vpcd.h lock.h

$(BUILT_SOURCES): pcsc-relay.ggo
	$(AM_V_GEN)$(GENGETOPT) --output-dir=$(srcdir) < $<

pcsc-relay.ggo: pcsc-relay.ggo.in
	$(AM_V_GEN)$(do_subst) < $< > $(srcdir)/$@
