OPENSCVERSION = 0.12.0

EXTRA_DIST = opensc-$(OPENSCVERSION)

AM_CPPFLAGS = -I$(top_srcdir)/src/opensc-$(OPENSCVERSION)/src

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c binutil.c
ccid_emulator_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS)

cats_test_SOURCES = cats-test.c pcscutil.c
cats_test_LDADD = $(PCSC_LIBS)
cats_test_CFLAGS = $(PCSC_CFLAGS)

bin_PROGRAMS =

noinst_HEADERS = \
		 binutil.h \
		 pcscutil.h \
		 scutil.h \
		 ccid.h \
		 usbstring.h

opensc: opensc-$(OPENSCVERSION)
	cd $(top_builddir)/src && cp -r opensc-$(OPENSCVERSION) opensc

libopensc.la: opensc
	cd $(top_builddir)/src/opensc && ./configure
	$(MAKE) -C opensc
	cp $(top_builddir)/src/opensc/src/libopensc/.libs/libopensc* $(top_builddir)/src

flaw:
	flawfinder -SQD *.h *.c

if WITH_CCID
bin_PROGRAMS += ccid-emulator
endif


if WITH_CATS
bin_PROGRAMS += cats-test
endif


if WITH_PACE
ccid_emulator_LDADD += $(LIBPACE_LIBS)
ccid_emulator_CFLAGS += $(LIBPACE_CFLAGS)
else
ccid_emulator_SOURCES += scutil.c
ccid_emulator_LDADD += libopensc.la
endif
