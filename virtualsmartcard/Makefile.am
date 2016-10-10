ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc npa-example-data MacOSX

EXTRA_DIST = test_vicc_with_npa-tool.sh test_vicc_with_handler_test.sh docs
DISTCHECK_CONFIGURE_FLAGS = --enable-libpcsclite

osx:
	$(MAKE) -C MacOSX $@
