do_subst = sed \
	   -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

BUILT_SOURCES = cmdline.h cmdline.c

EXTRA_DIST  = npa-tool.ggo npa-tool.ggo.in
EXTRA_DIST += $(shell find -L $(top_srcdir)/src/opensc -path '*/.git' -prune -o -type f -a -name '*.h' -print)
DISTCLEANFILES = $(BUILT_SOURCES) npa-tool.ggo

AM_CPPFLAGS = -I$(top_srcdir)/src/npa -I$(top_srcdir)/src/opensc/src

libnpa_la_SOURCES = sm.c scutil.c npa.c sslutil.c
libnpa_la_LIBADD = $(OPENSSL_LIBS) $(OPENSC_LIBS)
libnpa_la_CFLAGS = $(OPENSSL_CFLAGS)
libnpa_la_LDFLAGS = -no-undefined

npa_tool_SOURCES = npa-tool.c $(BUILT_SOURCES)
npa_tool_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libnpa.la
npa_tool_CFLAGS = $(OPENSSL_CFLAGS)

example_SOURCES = example.c
example_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libnpa.la
example_CFLAGS = $(OPENSSL_CFLAGS)


npa-tool.c: $(BUILT_SOURCES)

$(BUILT_SOURCES): npa-tool.ggo
	gengetopt --output-dir=$(srcdir) < $<

npa-tool.ggo: npa-tool.ggo.in
	$(do_subst) < $< > $(srcdir)/$@


bin_PROGRAMS = npa-tool
noinst_PROGRAMS = example

lib_LTLIBRARIES = libnpa.la

noinst_HEADERS = \
		 sslutil.h

nobase_include_HEADERS = \
	npa/sm.h \
	npa/npa.h \
	npa/scutil.h
