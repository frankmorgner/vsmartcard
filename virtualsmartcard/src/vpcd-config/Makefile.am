EXTRA_DIST 	        = qransi.c inet_ntop.c

bin_PROGRAMS        = vpcd-config

vpcd_config_CFLAGS  = $(QRENCODE_CFLAGS) -I$(srcdir)/../vpcd
vpcd_config_SOURCES = vpcd-config.c local-ip.c
vpcd_config_LDADD   = $(QRENCODE_LIBS)

if WIN32

vpcd_config_CFLAGS += -D_WIN32_WINNT=0x0600
vpcd_config_LDADD  += -lws2_32

endif
