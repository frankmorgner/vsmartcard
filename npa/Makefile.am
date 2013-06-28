ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 doc win32 certs

EXTRA_DIST = libnpa.pc.in apdus test_vicc_with_npa-tool.sh

do_subst = $(SED) \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	   -e 's,[@]PACKAGE_SUMMARY[@],$(PACKAGE_SUMMARY),g' \
	   -e 's,[@]builddir[@],$(builddir),g' \
	   -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]includedir[@],$(includedir),g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' \
	   -e 's,[@]OPENSC_LIBS[@],$(OPENSC_LIBS),g' \
	   -e 's,[@]top_srcdir[@],$(top_srcdir),g'

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnpa.pc
libnpa.pc: libnpa.pc.in
	$(do_subst) < $(srcdir)/libnpa.pc.in > libnpa.pc

win:
	make -C win32 $@

clean-local:
	rm -f libnpa.pc

distcheck-hook:
	test -d .svn && \
		svn2cl --group-by-day --reparagraph --separate-daylogs --include-actions --include-rev \
		|| true
	test -d .git && \
		git2cl > ChangeLog \
		|| true
