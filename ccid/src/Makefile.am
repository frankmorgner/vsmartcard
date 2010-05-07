IFDNFC_LIB = libifdnfc.$(DYN_LIB_EXT)

bin_PROGRAMS =
if WITH_CCID
bin_PROGRAMS += ccid
endif
if WITH_PACE
bin_PROGRAMS += pace-tool
endif

ccid_SOURCES = ccid.c usbstring.c usb.c binutil.c scutil.c
ccid_LDADD = $(OPENSC_LIBS) $(PTHREAD_LIBS)
ccid_CFLAGS = $(OPENSC_CFLAGS) $(PTHREAD_CFLAGS)

pace_tool_SOURCES = sm.c pace-tool.c binutil.c scutil.c pace.c pace_lib.c
pace_tool_LDADD = $(OPENSC_LIBS) $(OPENSSL_LIBS)
pace_tool_CFLAGS = $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS)
