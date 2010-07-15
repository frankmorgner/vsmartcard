AM_CPPFLAGS = -I$(top_srcdir)/src/pace -I$(top_builddir)/src/pace

libpace_la_SOURCES = sm.c scutil.c pace.c pace_lib.c
libpace_la_LIBADD = $(OPENSC_LIBS) $(OPENSSL_LIBS)
libpace_la_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c binutil.c
ccid_emulator_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS)

ccid_test_SOURCES = ccid-test.c
ccid_test_LDADD = $(PCSC_LIBS)
ccid_test_CFLAGS = $(PCSC_CFLAGS)

pace_tool_SOURCES = pace-tool.c binutil.c
pace_tool_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS) libpace.la
pace_tool_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)

bin_PROGRAMS =

noinst_HEADERS = \
		 binutil.h \
		 ccid.h \
		 usbstring.h

if WITH_CCID
bin_PROGRAMS += ccid-emulator ccid-test
endif


if WITH_PACE
bin_PROGRAMS += pace-tool
ccid_emulator_LDADD   += libpace.la

lib_LTLIBRARIES = libpace.la

nobase_include_HEADERS = \
	pace/sm.h \
	pace/pace.h \
	pace/scutil.h \
	pace/pace_lib.h

else
ccid_emulator_SOURCES += scutil.c sm.c

noinst_HEADERS += \
	pace/sm.h \
	pace/pace.h \
	pace/scutil.h \
	pace/pace_lib.h
endif
