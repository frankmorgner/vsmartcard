EXTRA_DIST = virtualsmartcard.in

vpiccdir = $(pythondir)

vpicc_PYTHON = CardGenerator.py SEutils.py TLVutils.py ConstantDefinitions.py \
	       SmartcardFilesystem.py utils.py CryptoUtils.py SmartcardSAM.py \
	       SWutils.py VirtualSmartcard.py

all-local:
	perl -ne "s|PYTHONDIR|$(pythondir)| ; print" $(srcdir)/virtualsmartcard.in > tmp
	perl -ne "s|PYTHONBIN|$(PYTHON)| ; print" tmp > virtualsmartcard
	rm -f tmp

install-exec-local: all-local
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) virtualsmartcard $(DESTDIR)$(bindir)/virtualsmartcard

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/virtualsmartcard

clean-local:
	rm -f virtualsmartcard
