EXTRA_DIST = opensc

AM_CPPFLAGS = -I$(top_srcdir)/src/pace -I$(top_srcdir)/src/opensc/src

libpace_la_SOURCES = sm.c scutil.c pace.c pace_lib.c
libpace_la_LIBADD = $(OPENSSL_LIBS) $(OPENSC_LIBS)
libpace_la_CFLAGS = $(OPENSSL_CFLAGS)

pace_tool_SOURCES = pace-tool.c binutil.c
pace_tool_LDADD = $(OPENSSL_LIBS) $(OPENSC_LIBS) libpace.la
pace_tool_CFLAGS = $(OPENSSL_CFLAGS)


bin_PROGRAMS = pace-tool

lib_LTLIBRARIES = libpace.la

noinst_HEADERS = \
		 binutil.h

nobase_include_HEADERS = \
	pace/sm.h \
	pace/pace.h \
	pace/scutil.h \
	pace/pace_lib.h
