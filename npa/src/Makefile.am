do_subst = $(SED) \
	   -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

BUILT_SOURCES = cmdline.h cmdline.c

EXTRA_DIST  = npa-tool.ggo npa-tool.ggo.in
EXTRA_DIST += $(shell find -L $(top_srcdir)/src/opensc/src -path '*/.git' -prune -o -type f -a -name '*.h' -print)
MAINTAINERCLEANFILES = $(BUILT_SOURCES) npa-tool.ggo $(dist_man1_MANS)

dist_man1_MANS		= npa-tool.1

AM_CPPFLAGS = -I$(top_srcdir)/src/opensc/src -DENABLE_SM

libnpa_la_SOURCES = iso-sm.c scutil.c npa.c
libnpa_la_LIBADD = $(OPENSSL_LIBS) $(OPENPACE_LIBS) $(OPENSC_LIBS)
libnpa_la_CFLAGS = $(OPENSSL_CFLAGS) $(OPENPACE_CFLAGS)
libnpa_la_LDFLAGS = -no-undefined

npa_tool_SOURCES = npa-tool.c $(BUILT_SOURCES)
npa_tool_LDADD = $(OPENSSL_LIBS) $(OPENPACE_LIBS) $(OPENSC_LIBS) libnpa.la
npa_tool_CFLAGS = $(OPENSSL_CFLAGS) $(OPENPACE_CFLAGS)

example_SOURCES = example.c
example_LDADD = $(OPENSSL_LIBS) $(OPENPACE_LIBS) $(OPENSC_LIBS) libnpa.la
example_CFLAGS = $(OPENSSL_CFLAGS) $(OPENPACE_CFLAGS)


npa-tool.c: $(BUILT_SOURCES)

$(BUILT_SOURCES): npa-tool.ggo
	$(GENGETOPT) --output-dir=$(srcdir) < $<

npa-tool.ggo: npa-tool.ggo.in
	$(do_subst) < $< > $(srcdir)/$@


npa-tool.1:
	$(HELP2MAN) \
		--output=$@ \
		--no-info \
		--source='$(PACKAGE_STRING)' \
		$(builddir)/npa-tool


bin_PROGRAMS = npa-tool
noinst_PROGRAMS = example

lib_LTLIBRARIES = libnpa.la

noinst_HEADERS = \
		 sslutil.h

nobase_include_HEADERS = \
	npa/iso-sm.h \
	npa/npa.h \
	npa/scutil.h
