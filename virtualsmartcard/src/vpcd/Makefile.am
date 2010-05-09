VPCD_LIB = libvpcd.$(DYN_LIB_EXT)

lib_LTLIBRARIES = libvpcd.la
libvpcd_la_SOURCES = ifd.c vpcd.c
#libvpcd_la_LIBADD = $(PCSC_LIBS)
libvpcd_la_CFLAGS = $(PCSC_CFLAGS)

EXTRA_DIST = reader.conf.in

install: install_libvpcd

install_libvpcd: libvpcd.la
	$(mkinstalldirs) $(DESTDIR)$(serialdropdir)
	cp .libs/$(VPCD_LIB) $(DESTDIR)$(serialdropdir)/$(VPCD_LIB).$(VERSION)
	ln -fs $(VPCD_LIB).$(VERSION) $(DESTDIR)$(serialdropdir)/$(VPCD_LIB)
	if [ -e $(DESTDIR)$(sysconfdir)/reader.conf ] ; \
	then \
		echo "Edit existing /etc/reader.conf" ; \
	else \
		$(mkinstalldirs) $(DESTDIR)$(sysconfdir) ; \
		perl -ne "s|TARGET|$(serialdropdir)/$(VPCD_LIB)| ; print" $(srcdir)/reader.conf.in > $(DESTDIR)$(sysconfdir)/reader.conf ; \
	fi

uninstall: uninstall_libvpcd

uninstall_libvpcd:
	rm -f $(DESTDIR)$(serialdropdir)/$(VPCD_LIB).$(VERSION)
