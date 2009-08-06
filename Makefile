SUBDIRS = ccid virtualsmartcard

default: all

%:
	for d in $(SUBDIRS); do $(MAKE) $@ -C $$d; done
