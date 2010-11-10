EXTRA_DIST = picc.py

bin_PROGRAMS = pcsc-relay
bin_SCRIPTS = picc.py

pcsc_relay_SOURCES = pcsc-relay.c pcscutil.c opicc.c lnfc.c
pcsc_relay_LDADD = $(PCSC_LIBS) $(LIBNFC_LIBS)
pcsc_relay_CFLAGS = $(PCSC_CFLAGS) $(LIBNFC_CFLAGS)

noinst_HEADERS = pcscutil.h pcsc-relay.h
