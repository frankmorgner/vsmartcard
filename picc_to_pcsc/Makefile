MAKEFLAGS         += -rR --no-print-directory

# Compiler
CC      = gcc
CFLAGS  = -Wall -g
LIBPCSCLITE_CFLAGS = `pkg-config --cflags --libs libpcsclite`

TARGETS = picc_to_pcsc
            
all: $(TARGETS)

picc_to_pcsc: picc_to_pcsc.c picc_to_pcsc.h
	$(CC) $(CFLAGS) $(LIBPCSCLITE_CFLAGS) picc_to_pcsc.c -o $@

clean:
	rm -f picc_to_pcsc
