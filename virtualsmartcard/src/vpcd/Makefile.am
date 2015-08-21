libvpcd_la_SOURCES = vpcd.c lock.c
libvpcd_la_LDFLAGS = -no-undefined

noinst_HEADERS = vpcd.h lock.h

noinst_LTLIBRARIES = libvpcd.la

if WIN32

libvpcd_la_LDFLAGS += -lws2_32

endif
