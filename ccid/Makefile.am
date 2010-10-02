ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 patches

MOSTLYCLEANFILES = $(DX_CLEANFILES)

if DOC_ENABLED
clean-local:
	rm -rf doc

doc : Doxyfile
	@DOXYGEN@ $(builddir)/Doxyfile

.PHONY: doc
endif

if WITH_PACE
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpace.pc
endif
