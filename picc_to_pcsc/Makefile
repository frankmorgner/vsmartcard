SUBDIRS = ccid picc_to_pcsc virtualsmartcard

default: all

%:
	for d in $(SUBDIRS); do $(MAKE) $@ -C $$d; done
