AM_CPPFLAGS = -I$(top_srcdir)/src/pace -I$(top_builddir)/src/pace

libpace_la_SOURCES = sm.c scutil.c pace.c pace_lib.c
libpace_la_LIBADD = $(OPENSC_LIBS) $(OPENSSL_LIBS)
libpace_la_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)

pace_tool_SOURCES = pace-tool.c binutil.c
pace_tool_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS) libpace.la
pace_tool_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)
noinst_HEADERS = \
		 binutil.h

flaw:
	flawfinder -SQD *.h *.c

bin_PROGRAMS = pace-tool

lib_LTLIBRARIES = libpace.la

nobase_include_HEADERS = \
	pace/sm.h \
	pace/pace.h \
	pace/scutil.h \
	pace/pace_lib.h
