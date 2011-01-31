EXTRA_DIST = opensc

AM_CPPFLAGS = -I$(top_srcdir)/src/opensc/src

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c binutil.c
ccid_emulator_LDADD =  $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS)

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
ccid_emulator_LDADD += $(OPENSC_LIBS)
endif
