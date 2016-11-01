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
	$(HELP2MAN) \
		--output=$@ \
		--no-info \
		--no-discard-stderr \
		--source='$(PACKAGE_STRING)' \
		--version-string='$(PACKAGE) $(VERSION)' \
		$(builddir)/pcsc-relay


bin_PROGRAMS = pcsc-relay

pcsc_relay_SOURCES = pcsc-relay.c pcsc.c vpcd.c vpcd-driver.c opicc.c lnfc.c vicc.c lock.c $(BUILT_SOURCES)
pcsc_relay_LDADD = $(PCSC_LIBS) $(LIBNFC_LIBS)
pcsc_relay_CFLAGS = $(PCSC_CFLAGS) $(LIBNFC_CFLAGS)

if WIN32
pcsc_relay_LDADD += -lws2_32
endif

noinst_HEADERS = pcsc-relay.h vpcd.h lock.h

pcsc-relay.c: $(BUILT_SOURCES)

$(BUILT_SOURCES): pcsc-relay.ggo
	$(GENGETOPT) --output-dir=$(srcdir) < $<

pcsc-relay.ggo: pcsc-relay.ggo.in
	$(do_subst) < $< > $(srcdir)/$@
