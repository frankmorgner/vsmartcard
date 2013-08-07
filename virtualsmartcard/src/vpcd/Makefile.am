VPCD_LIB = $(LIB_PREFIX)vpcd.$(DYN_LIB_EXT)

libvpcd_la_SOURCES = ifd-vpcd.c vpcd.c
libvpcd_la_LDFLAGS = -no-undefined
libvpcd_la_CFLAGS = $(PCSC_CFLAGS)

noinst_HEADERS = vpcd.h

EXTRA_DIST = reader.conf.in


do_subst = $(SED) \
		   -e 's,[@]TARGET[@],$(serialdropdir)/$(VPCD_LIB),g' \
		   -e 's,[@]VPCDHOST[@],$(vpcdhost),g'


if BUILD_LIBPCSCLITE

noinst_LTLIBRARIES = libvpcd.la

ifd-vpcd.c: pcsclite.h

.PHONY: pcsclite.h
pcsclite.h:
	$(MAKE) -C $(builddir)/../libpcsclite pcsclite.h

else

libvpcd_la_LIBADD = $(PCSC_LIBS)

lib_LTLIBRARIES = libvpcd.la

install: install_libvpcd

install_libvpcd: libvpcd.la reader.conf.in
	$(mkinstalldirs) $(DESTDIR)$(serialdropdir)
	$(LIBTOOL) --mode=install cp .libs/$(VPCD_LIB) $(DESTDIR)$(serialdropdir)/$(VPCD_LIB).$(VERSION)
	(cd $(DESTDIR)$(serialdropdir) && \
		$(LN_S) -f $(VPCD_LIB).$(VERSION) $(VPCD_LIB))
	$(mkinstalldirs) $(DESTDIR)$(serialconfdir)
	$(do_subst) < $(srcdir)/reader.conf.in > $(DESTDIR)$(serialconfdir)/vpcd


uninstall: uninstall_libvpcd

uninstall_libvpcd:
	rm -f $(DESTDIR)$(serialdropdir)/$(VPCD_LIB).$(VERSION) \
	    $(DESTDIR)$(serialdropdir)/$(VPCD_LIB) \
	    $(DESTDIR)$(serialconfdir)/vpcd

endif
