OPENSCVERSION = 0.12.0

EXTRA_DIST = opensc-$(OPENSCVERSION)

AM_CPPFLAGS = -I$(top_srcdir)/src/pace -I$(top_srcdir)/src/opensc-$(OPENSCVERSION)/src

libpace_la_SOURCES = sm.c scutil.c pace.c pace_lib.c
libpace_la_LIBADD = $(OPENSSL_LIBS) libopensc.la
libpace_la_CFLAGS = $(OPENSSL_CFLAGS)

pace_tool_SOURCES = pace-tool.c binutil.c
pace_tool_LDADD = $(OPENSSL_LIBS) libpace.la libopensc.la
pace_tool_CFLAGS = $(OPENSSL_CFLAGS)

opensc:
	cp -r $(top_srcdir)/src/opensc-$(OPENSCVERSION) opensc
	cd opensc && ./configure OPENSSL_LIBS="$(OPENSSL_LIBS)" OPENSSL_CFLAGS="$(OPENSSL_CFLAGS)" $(OPENSC_CONFIGURE)

.PHONY:libopensc.la
libopensc.la: opensc
	$(MAKE) -C opensc
	cp opensc/src/libopensc/.libs/libopensc.* .

clean-local:
	rm -rf opensc libopensc.*

bin_PROGRAMS = pace-tool

lib_LTLIBRARIES = libpace.la

noinst_HEADERS = \
		 binutil.h

nobase_include_HEADERS = \
	pace/sm.h \
	pace/pace.h \
	pace/scutil.h \
	pace/pace_lib.h
