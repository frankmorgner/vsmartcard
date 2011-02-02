EXTRA_DIST = opensc

AM_CPPFLAGS = -I$(top_srcdir)/src/npa -I$(top_srcdir)/src/opensc/src

libnpa_la_SOURCES = sm.c scutil.c npa.c npa_lib.c
libnpa_la_LIBADD = $(OPENSSL_LIBS) $(OPENSC_LIBS)
libnpa_la_CFLAGS = $(OPENSSL_CFLAGS)

npa_tool_SOURCES = npa-tool.c binutil.c
npa_tool_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libnpa.la
npa_tool_CFLAGS = $(OPENSSL_CFLAGS)


bin_PROGRAMS = npa-tool

lib_LTLIBRARIES = libnpa.la

noinst_HEADERS = \
		 binutil.h

nobase_include_HEADERS = \
	npa/sm.h \
	npa/npa.h \
	npa/scutil.h \
	npa/npa_lib.h
