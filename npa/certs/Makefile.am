CV_CERTS = DECVCAeID00102 DECVCAEPASS00102 DECVCAeSign00102
X509_CERTS = ff3d20d2

EXTRA_DIST = $(CV_CERTS) $(X509_CERTS)

install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(X509DIR)"
	for cert in $(X509_CERTS); do $(INSTALL_DATA) $(srcdir)/$${cert} "$(DESTDIR)$(X509DIR)"; done
	$(MKDIR_P) "$(DESTDIR)$(CVCDIR)"
	for cert in $(CV_CERTS);   do $(INSTALL_DATA) $(srcdir)/$${cert} "$(DESTDIR)$(CVCDIR)";  done

uninstall-local:
	for cert in $(X509_CERTS); do rm -f "$(DESTDIR)$(X509DIR)/$${cert}"; done
	for cert in $(CV_CERTS);   do rm -f "$(DESTDIR)$(CVCDIR)/$${cert}";  done
