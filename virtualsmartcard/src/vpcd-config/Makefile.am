EXTRA_DIST 	        = qransi.c

bin_PROGRAMS        = vpcd-config

vpcd_config_CFLAGS  = $(PCSC_CFLAGS) $(QRENCODE_CFLAGS) -I$(srcdir)/../vpcd
vpcd_config_SOURCES = vpcd-config.c local-ip.c
vpcd_config_LDADD   = $(QRENCODE_LIBS)
