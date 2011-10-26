EXTRA_DIST = $(shell find $(top_srcdir)/src/opensc -path '*/.svn' -prune -o -type f -a -name '*.h' -print)

AM_CPPFLAGS = -I$(top_srcdir)/src/npa -I$(top_srcdir)/src/opensc/src

libnpa_la_SOURCES = sm.c scutil.c npa.c sslutil.c
libnpa_la_LIBADD = $(OPENSSL_LIBS) $(OPENSC_LIBS)
libnpa_la_CFLAGS = $(OPENSSL_CFLAGS)

npa_tool_SOURCES = npa-tool.c binutil.c
npa_tool_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libnpa.la
npa_tool_CFLAGS = $(OPENSSL_CFLAGS)

example_SOURCES = example.c
example_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libnpa.la
example_CFLAGS = $(OPENSSL_CFLAGS)


bin_PROGRAMS = npa-tool example

lib_LTLIBRARIES = libnpa.la

noinst_HEADERS = \
		 sslutil.h \
		 binutil.h

nobase_include_HEADERS = \
	npa/sm.h \
	npa/npa.h \
	npa/scutil.h
