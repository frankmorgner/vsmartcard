CV_CERTS = at_cert_19a.cvcert at_cert_19a_ecdh.cvcert cvca-eid.cv dv_cert_19.cvcert dv_cert_19_ecdh.cvcert
X509_CERTS = csca-germany_013_self_signed_cer.cer

EXTRA_DIST = $(CV_CERTS) $(X509_CERTS)

install-data-local:
	for cert in $(X509_CERTS); do $(INSTALL_DATA) $(srcdir)/$${cert} "$(X509DIR)"; done
	for cert in $(CV_CERTS);   do $(INSTALL_DATA) $(srcdir)/$${cert} "$(CVCDIR)";  done
