MAKEFLAGS         += -rR --no-print-directory

# Directories
prefix		   = 
exec_prefix	   = $(prefix)
bindir		   = $(exec_prefix)/bin


# Compiler
CC                 = gcc
CFLAGS             = -Wall -g 
LIBPCSCLITE_CFLAGS = `pkg-config --cflags --libs libpcsclite`
PTHREAD_CFLAGS     = -pthread

INSTALL		   = install
INSTALL_PROGRAM    = $(INSTALL)


TARGETS		   = ccid

# top-level rule
all: $(TARGETS)


ccid: ccid.h ccid.c usbstring.c usbstring.h
	$(CC) $(LIBPCSCLITE_CFLAGS) $(PTHREAD_CFLAGS) $(CFLAGS) usbstring.c ccid.c -o $@


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
