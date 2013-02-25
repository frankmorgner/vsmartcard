CV_CERTS = DECVCAeID00102 DECVCAEPASS00102 DECVCAeSign00102
X509_CERTS = ff3d20d2

EXTRA_DIST = $(CV_CERTS) $(X509_CERTS)

install-data-local:
	for cert in $(X509_CERTS); do $(INSTALL_DATA) $(srcdir)/$${cert} "$(X509DIR)"; done
	for cert in $(CV_CERTS);   do $(INSTALL_DATA) $(srcdir)/$${cert} "$(CVCDIR)";  done
