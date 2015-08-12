IFDVPCD_LIB = $(LIB_PREFIX)ifdvpcd.$(DYN_LIB_EXT)

libifdvpcd_la_SOURCES = ifd-vpcd.c
libifdvpcd_la_LDFLAGS = -no-undefined
libifdvpcd_la_CPPFLAGS = $(PCSC_CFLAGS) -I$(srcdir)/../vpcd
libifdvpcd_la_LIBADD = $(top_builddir)/src/vpcd/libvpcd.la

noinst_HEADERS = ifd-vpcd.h

EXTRA_DIST = reader.conf.in Info.plist.in


do_subst = $(SED) \
		   -e 's,[@]VERSION[@],$(VERSION),g' \
		   -e 's,[@]TARGET_LIB[@],$(IFDVPCD_LIB),g' \
		   -e 's,[@]TARGET[@],$(serialdropdir)/$(IFDVPCD_LIB),g' \
		   -e 's,[@]VPCDHOST[@],$(vpcdhost),g'


if BUILD_LIBPCSCLITE

noinst_LTLIBRARIES = libifdvpcd.la

else

lib_LTLIBRARIES = libifdvpcd.la

if BUILD_INFOPLIST

install: install_libifdvpcd install_infoplist

uninstall: uninstall_libifdvpcd uninstall_infoplist

else

install: install_libifdvpcd install_readerconf

uninstall: uninstall_libifdvpcd uninstall_readerconf

endif

endif

install_libifdvpcd: libifdvpcd.la
	$(mkinstalldirs) $(DESTDIR)$(serialdropdir)
	$(LIBTOOL) --mode=install cp .libs/$(IFDVPCD_LIB) $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB).$(VERSION)
	(cd $(DESTDIR)$(serialdropdir) && \
		$(LN_S) -f $(IFDVPCD_LIB).$(VERSION) $(IFDVPCD_LIB))

install_readerconf: reader.conf.in
	$(mkinstalldirs) $(DESTDIR)$(serialconfdir)
	$(do_subst) < $(srcdir)/reader.conf.in > $(DESTDIR)$(serialconfdir)/vpcd

install_infoplist: Info.plist.in
	$(mkinstalldirs) $(DESTDIR)$(serialconfdir)
	$(do_subst) < $(srcdir)/Info.plist.in > $(DESTDIR)$(serialconfdir)/Info.plist


uninstall_libifdvpcd:
	rm -f $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB).$(VERSION) \
	    $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB)

uninstall_readerconf:
	rm -f $(DESTDIR)$(serialconfdir)/vpcd

uninstall_infoplist:
	rm -f $(DESTDIR)$(serialconfdir)/Info.plist
