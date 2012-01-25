EXTRA_DIST  = $(shell find $(top_srcdir)/src/opensc -path '*/.svn' -prune -o -type f -a -name '*.h' -print)
EXTRA_DIST += $(shell find $(top_srcdir)/src/opensc-npa -path '*/.svn' -prune -o -type f -a -name '*.h' -print)

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c binutil.c sslutil.c
ccid_emulator_LDADD =  $(OPENSSL_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSSL_CFLAGS) $(PTHREAD_CFLAGS)


bin_PROGRAMS =

noinst_HEADERS = \
		 binutil.h \
		 pcscutil.h \
		 scutil.h \
		 ccid.h \
		 sslutil.h \
		 usbstring.h

if WITH_CCID
bin_PROGRAMS += ccid-emulator
endif


if WITH_PACE
AM_CPPFLAGS = -I$(top_srcdir)/src/opensc-npa/src

ccid_emulator_LDADD += $(LIBNPA_LIBS)
ccid_emulator_CFLAGS += $(LIBNPA_CFLAGS)
else
AM_CPPFLAGS = -I$(top_srcdir)/src/opensc/src

ccid_emulator_SOURCES += scutil.c
ccid_emulator_LDADD += $(OPENSC_LIBS)
endif
