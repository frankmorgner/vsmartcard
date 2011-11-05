SUBDIRS = api

do_subst = sed \
	   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	   -e 's,[@]PACKAGE_TARNAME[@],$(PACKAGE_TARNAME),g' \
	   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
	   -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
	   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	   -e 's,[@]builddir[@],$(builddir),g' \
	   -e 's,[@]prefix[@],$(prefix),g' \
	   -e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	   -e 's,[@]libdir[@],$(libdir),g' \
	   -e 's,[@]includedir[@],$(includedir),g' \
	   -e 's,[@]VERSION[@],$(VERSION),g' \
	   -e 's,[@]OPENSC_LIBS[@],$(OPENSC_LIBS),g' \
	   -e 's,[@]abs_top_srcdir[@],$(abs_top_srcdir),g' \
	   -e 's,[@]top_srcdir[@],$(top_srcdir),g'

EXTRA_DIST = README.rst.in README.rst autotools.rst.in questions.rst.in autotools.rst questions.rst

dist_noinst_SCRIPTS = generate_modules.py

doc: $(top_srcdir)/src/vpicc/virtualsmartcard/*.py $(top_srcdir)/src/vpicc/virtualsmartcard/cards/*.py generate_modules.py *.in
	$(do_subst) < autotools.rst.in > autotools.rst
	$(do_subst) < questions.rst.in > questions.rst
	$(do_subst) < README.rst.in > README.rst
	./generate_modules.py $(top_srcdir)/src/vpicc --dest-dir=api --suffix=rst --no-toc -1 -f
	touch doc

clean-local:
	rm -f doc
