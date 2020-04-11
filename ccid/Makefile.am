ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . src doc

DIST_SUBDIRS = $(SUBDIRS) src/OpenSC

EXTRA_DIST = docs patches

all-local:
	cd src/OpenSC/src/common && $(MAKE) $(AM_MAKEFLAGS) all
	cd src/OpenSC/src/scconf && $(MAKE) $(AM_MAKEFLAGS) all
	cd src/OpenSC/src/pkcs15init && $(MAKE) $(AM_MAKEFLAGS) all
	cd src/OpenSC/src/sm && $(MAKE) $(AM_MAKEFLAGS) all
	cd src/OpenSC/src/libopensc && $(MAKE) $(AM_MAKEFLAGS) all

check-local:
	cd src/OpenSC/src/common && $(MAKE) $(AM_MAKEFLAGS) check
	cd src/OpenSC/src/scconf && $(MAKE) $(AM_MAKEFLAGS) check
	cd src/OpenSC/src/pkcs15init && $(MAKE) $(AM_MAKEFLAGS) check
	cd src/OpenSC/src/sm && $(MAKE) $(AM_MAKEFLAGS) check
	cd src/OpenSC/src/libopensc && $(MAKE) $(AM_MAKEFLAGS) check

clean-local:
	cd src/OpenSC/src/common && $(MAKE) $(AM_MAKEFLAGS) clean
	cd src/OpenSC/src/scconf && $(MAKE) $(AM_MAKEFLAGS) clean
	cd src/OpenSC/src/pkcs15init && $(MAKE) $(AM_MAKEFLAGS) clean
	cd src/OpenSC/src/sm && $(MAKE) $(AM_MAKEFLAGS) clean
	cd src/OpenSC/src/libopensc && $(MAKE) $(AM_MAKEFLAGS) clean
