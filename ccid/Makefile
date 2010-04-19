MAKEFLAGS         += -rR --no-print-directory

# Directories
prefix		   = 
exec_prefix	   = $(prefix)
bindir		   = $(exec_prefix)/bin


# Compiler
CC                 = gcc
CFLAGS             = -Wall -g
EXTRA_CFLAGS       = -DNO_PACE

OPENSSL_CFLAGS 	   = `pkg-config --cflags --libs libssl`
OPENSC_CFLAGS 	   = `pkg-config --cflags --libs libopensc`
PTHREAD_CFLAGS     = -pthread
a_flags		   = $(CFLAGS) $(OPENSC_CFLAGS) $(OPENSSL_CFLAGS) \
		     $(PTHREAD_CFLAGS) $(EXTRA_CFLAGS)

INSTALL		   = install
INSTALL_PROGRAM    = $(INSTALL)


TARGETS		   = ccid

CCID_OBJ = ccid.o sm.o pace.o pace_lib.o usbstring.o usb.o

# top-level rule
all: $(TARGETS)


ccid: $(CCID_OBJ)
	$(CC) $^ -o $@    $(a_flags)
%.o: %.c %.h
	$(CC) $< -o $@ -c $(a_flags)
%.o: %.c
	$(CC) $< -o $@ -c $(a_flags)


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
	rm -f $(TARGETS) $(CCID_OBJ)
