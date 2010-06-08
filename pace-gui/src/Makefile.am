EXTRA_DIST = pinpad.in

bin_SCRIPTS = pinpad.py

uidir = $(datadir)/pinpad
ui_DATA = pinpad.glade

all-local:
	perl -ne "s|PYTHONDIR|$(pythondir)| ; print" $(srcdir)/pinpad.in > tmp
	perl -ne "s|PYTHONBIN|$(PYTHON)| ; print" tmp > pinpad
	rm -f tmp

install-exec-local: all-local
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) pinpad $(DESTDIR)$(bindir)/pinpad

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/pinpad

clean-local:
	rm -f pinpad
