/*
 * Copyright (C) 2010 Dominik Oepen, Frank Morgner
 *
 * This file is part of picc_to_pcsc.
 *
 * picc_to_pcsc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * picc_to_pcsc is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * picc_to_pcsc.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PICC_TO_PCSC
#define PICC_TO_PCSC

#define PICCDEV "/dev/ttyACM0"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sighandler_t)(int);

static sighandler_t register_sig(int signo, sighandler_t sighandler);
void checkSig(int signo);
unsigned int read_from_device(unsigned char *inputBuffer);
void write_to_device(unsigned char *buffer, unsigned int msglength);
void setup(void);
void cleanup(void);
void printHex(char *title, unsigned char *msg, unsigned int length); 

#ifdef  __cplusplus
}
#endif
#endif
