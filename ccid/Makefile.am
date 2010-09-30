ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src m4 patches

include doxygen.am

MOSTLYCLEANFILES = $(DX_CLEANFILES)

EXTRA_DIST = $(DX_CONFIG)

if WITH_PACE
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpace.pc
endif
