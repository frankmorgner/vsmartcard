bin_SCRIPTS = vicc
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = vicc.in
MAINTAINERCLEANFILES = $(dist_man1_MANS)

dist_man1_MANS		= vicc.1

vpiccdir = $(pythondir)/virtualsmartcard
vpicccardsdir = $(vpiccdir)/cards

vpicc_PYTHON = virtualsmartcard/CardGenerator.py \
	       virtualsmartcard/SEutils.py \
	       virtualsmartcard/TLVutils.py \
	       virtualsmartcard/ConstantDefinitions.py \
	       virtualsmartcard/SmartcardFilesystem.py \
	       virtualsmartcard/utils.py \
	       virtualsmartcard/CryptoUtils.py \
	       virtualsmartcard/SmartcardSAM.py \
	       virtualsmartcard/SWutils.py \
	       virtualsmartcard/VirtualSmartcard.py \
	       virtualsmartcard/__init__.py

vpicccards_PYTHON = virtualsmartcard/cards/__init__.py \
           virtualsmartcard/cards/ePass.py \
           virtualsmartcard/cards/nPA.py \
           virtualsmartcard/cards/Relay.py \
           virtualsmartcard/cards/cryptoflex.py \
           virtualsmartcard/cards/belpic.py

do_subst = $(SED) \
		   -e 's,[@]PYTHON[@],$(PYTHON),g' \
		   -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
		   -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
		   -e 's,[@]PACKAGE_SUMMARY[@],$(PACKAGE_SUMMARY),g' \
		   -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g'

$(builddir)/vicc: vicc.in Makefile
	$(do_subst) < $(srcdir)/vicc.in > $(builddir)/vicc
	chmod +x $(builddir)/vicc

vicc.1: vicc.in
	$(HELP2MAN) \
		--no-discard-stderr \
		--output=$@ \
		$(builddir)/vicc
