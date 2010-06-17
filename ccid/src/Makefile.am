IFDNFC_LIB = libifdnfc.$(DYN_LIB_EXT)

ccid_emulator_SOURCES = ccid.c usbstring.c usb.c binutil.c scutil.c
ccid_emulator_LDADD = $(OPENSC_LIBS) $(PTHREAD_LIBS)
ccid_emulator_CFLAGS = $(OPENSC_CFLAGS) $(PTHREAD_CFLAGS)

ccid_test_SOURCES = ccid-test.c
ccid_test_LDADD = $(PCSC_LIBS)
ccid_test_CFLAGS = $(PCSC_CFLAGS)

pace_tool_SOURCES = sm.c pace-tool.c binutil.c scutil.c pace.c pace_lib.c
pace_tool_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS)
pace_tool_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)

bin_PROGRAMS =
if WITH_CCID
bin_PROGRAMS += ccid-emulator ccid-test
endif
if WITH_PACE
bin_PROGRAMS += pace-tool

ccid_emulator_SOURCES += sm.c pace.c pace_lib.c
ccid_emulator_LDADD += $(OPENSSL_LIBS)
ccid_emulator_CFLAGS += $(OPENSSL_CFLAGS)
endif

noinst_HEADERS = binutil.h  ccid.h  pace.h  pace_lib.h  scutil.h  sm.h  usbstring.h
