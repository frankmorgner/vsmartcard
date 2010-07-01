EXTRA_DIST = picc.py

bin_PROGRAMS = picc_to_pcsc
bin_SCRIPTS = picc.py

picc_to_pcsc_SOURCES = picc_to_pcsc.c
picc_to_pcsc_LDADD = $(PCSC_LIBS)
picc_to_pcsc_CFLAGS = $(PCSC_CFLAGS)

noinst_HEADERS = picc_to_pcsc.h
