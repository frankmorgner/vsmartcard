IFDVPCD_LIB = $(LIB_PREFIX)ifdvpcd.$(DYN_LIB_EXT)

libifdvpcd_la_SOURCES = ifd-vpcd.c
libifdvpcd_la_LDFLAGS = -no-undefined
libifdvpcd_la_CPPFLAGS = $(PCSC_CFLAGS) -I$(srcdir)/../vpcd
libifdvpcd_la_LIBADD = $(top_builddir)/src/vpcd/libvpcd.la

noinst_HEADERS = ifd-vpcd.h

EXTRA_DIST = reader.conf.in


do_subst = $(SED) \
		   -e 's,[@]TARGET[@],$(serialdropdir)/$(IFDVPCD_LIB),g' \
		   -e 's,[@]VPCDHOST[@],$(vpcdhost),g'


if BUILD_LIBPCSCLITE

noinst_LTLIBRARIES = libifdvpcd.la

ifd-vpcd.c: pcsclite.h

.PHONY: pcsclite.h
pcsclite.h:
	$(MAKE) -C $(builddir)/../pcsclite-vpcd pcsclite.h

else

libifdvpcd_la_LIBADD += $(PCSC_LIBS)

lib_LTLIBRARIES = libifdvpcd.la

install: install_libifdvpcd

install_libifdvpcd: libifdvpcd.la reader.conf.in
	$(mkinstalldirs) $(DESTDIR)$(serialdropdir)
	$(LIBTOOL) --mode=install cp .libs/$(IFDVPCD_LIB) $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB).$(VERSION)
	(cd $(DESTDIR)$(serialdropdir) && \
		$(LN_S) -f $(IFDVPCD_LIB).$(VERSION) $(IFDVPCD_LIB))
	$(mkinstalldirs) $(DESTDIR)$(serialconfdir)
	$(do_subst) < $(srcdir)/reader.conf.in > $(DESTDIR)$(serialconfdir)/vpcd


uninstall: uninstall_libifdvpcd

uninstall_libifdvpcd:
	rm -f $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB).$(VERSION) \
	    $(DESTDIR)$(serialdropdir)/$(IFDVPCD_LIB) \
	    $(DESTDIR)$(serialconfdir)/vpcd

endif
