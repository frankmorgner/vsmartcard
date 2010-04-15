MAKEFLAGS         += -rR --no-print-directory

# Directories
prefix		   = 
exec_prefix	   = $(prefix)
bindir		   = $(exec_prefix)/bin


# Compiler
CC                 = gcc
CFLAGS             = -Wall -g
EXTRA_CFLAGS       = -DNO_PACE
#LIBPCSCLITE_CFLAGS = `pkg-config --cflags --libs libpcsclite`
OPENSSL_CFLAGS 	   = `pkg-config --cflags --libs libssl`
OPENSC_CFLAGS 	   = `pkg-config --cflags --libs libopensc`
PTHREAD_CFLAGS     = -pthread

INSTALL		   = install
INSTALL_PROGRAM    = $(INSTALL)


TARGETS		   = ccid

CCID_SRC = ccid.h ccid.c \
	   sm.c sm.h \
	   pace.h pace.c pace_lib.c \
	   usbstring.c usbstring.h usb.c

# top-level rule
all: $(TARGETS)


ccid: $(CCID_SRC)
	$(CC) $(LIBPCSCLITE_CFLAGS) $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS) \
	    $(PTHREAD_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) \
	    $(filter %.c, $(CCID_SRC)) -o $@


install: $(TARGETS) installdirs
	$(INSTALL_PROGRAM) ccid $(DESTDIR)$(bindir)

.PHONY: installdirs
installdirs:
	$(INSTALL) -d $(DESTDIR)$(bindir)

.PHONY: install-strip
install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(bindir)/ccid

.PHONY: clean
clean:
	rm -f ccid
