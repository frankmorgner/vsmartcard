MAKEFLAGS         += -rR --no-print-directory

# Directories
prefix		   = 
exec_prefix	   = $(prefix)
bindir		   = $(exec_prefix)/bin
serialdropdir	   = `pkg-config libpcsclite --variable=usbdropdir`/serial
sysconfdir	   = $(prefix)/etc
readerconfdir      = $(sysconfdir)/reader.conf.d
python_sitelib     = /usr/lib/python2.5/site-packages


# Compiler
CC                 = gcc
CFLAGS             = -Wall -g 
LIBPCSCLITE_CFLAGS = `pkg-config --cflags --libs libpcsclite`
# use -fPIC in case of problems
SO_CFLAGS          = -fpic

# Linker
LD                 = ld
LDFLAGS            = -G

INSTALL		   = install
INSTALL_PROGRAM    = $(INSTALL)
INSTALL_DATA       = $(INSTALL) -m 644

UPDATEREADERCONF   = update-reader.conf
RESTARTPCSCD       = /etc/init.d/pcscd restart


TARGETS		   = libvpcd.so configuration virtualsmartcard

# top-level rule
all: $(TARGETS)

libvpcd.so: vpcd/vpcd.c vpcd/vpcd.h
	$(CC) $(LIBPCSCLITE_CFLAGS) $(SO_CFLAGS) $(CFLAGS) -c $< -o $(@:%.so=%.o)
	$(LD) $(LDFLAGS) $(@:%.so=%.o) -o $@ -soname=$@.1 -lc
	rm -f $(@:%.so=%.o)

.PHONY: virtualsmartcard
virtualsmartcard:
	echo '#!/bin/sh' > virtualsmartcard
	echo 'cd "$(python_sitelib)"' \
	    >> virtualsmartcard
	echo 'python "$(python_sitelib)/VirtualSmartcard.py" "$$@"' \
	    >> virtualsmartcard

.PHONY: configuration
configuration:
	echo 'FRIENDLYNAME "Virtual PCD"' > vpcd.conf
	echo 'DEVICENAME   /dev/null' >> vpcd.conf
	echo "LIBPATH      $(serialdropdir)/libvpcd.so" >> vpcd.conf
	echo 'CHANNELID    0' >> vpcd.conf


install: $(TARGETS) installdirs
	$(INSTALL_PROGRAM) libvpcd.so $(DESTDIR)$(serialdropdir)
	$(INSTALL_PROGRAM) virtualsmartcard $(DESTDIR)$(bindir)
	$(INSTALL_DATA) vpcd.conf $(DESTDIR)$(readerconfdir)
	$(INSTALL_DATA) vpicc/ConstantDefinitions.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/CryptoUtils.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/SEutils.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/SmartcardFilesystem.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/SmartcardSAM.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/SWutils.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/TLVutils.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/utils.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/VirtualSmartcard.py $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/testconfig.sam $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/testconfig.mf $(DESTDIR)$(python_sitelib)
	$(INSTALL_DATA) vpicc/jp2.jpg $(DESTDIR)$(python_sitelib)

.PHONY: installdirs
installdirs:
	$(INSTALL) -d $(DESTDIR)$(serialdropdir)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -d $(DESTDIR)$(readerconfdir)
	$(INSTALL) -d $(DESTDIR)$(python_sitelib)

.PHONY: install-strip
install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(serialdropdir)/libvpcd.so
	rm -f $(DESTDIR)$(bindir)/virtualsmartcard
	rm -f $(DESTDIR)$(readerconfdir)/vpcd.conf
	rm -f $(DESTDIR)$(python_sitelib)/ConstantDefinitions.py
	rm -f $(DESTDIR)$(python_sitelib)/CryptoUtils.py
	rm -f $(DESTDIR)$(python_sitelib)/SEutils.py
	rm -f $(DESTDIR)$(python_sitelib)/SmartcardFilesystem.py
	rm -f $(DESTDIR)$(python_sitelib)/SmartcardSAM.py
	rm -f $(DESTDIR)$(python_sitelib)/SWutils.py
	rm -f $(DESTDIR)$(python_sitelib)/TLVutils.py
	rm -f $(DESTDIR)$(python_sitelib)/utils.py
	rm -f $(DESTDIR)$(python_sitelib)/VirtualSmartcard.py
	rm -f $(DESTDIR)$(python_sitelib)/testconfig.sam
	rm -f $(DESTDIR)$(python_sitelib)/testconfig.mf
	rm -f $(DESTDIR)$(python_sitelib)/jp2.jpg

.PHONY: clean
clean:
	rm -f libvpcd.so
	rm -f vpcd.conf
	rm -f virtualsmartcard
