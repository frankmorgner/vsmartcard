EXTRA_DIST = vicc.in

vpiccdir = $(pythondir)/virtualsmartcard

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

all-local:
	perl -ne "s|PYTHONBIN|$(PYTHON)| ; print" vicc.in > vicc
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) vicc $(DESTDIR)$(bindir)/vicc

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/vicc

clean-local:
	rm -f vicc
