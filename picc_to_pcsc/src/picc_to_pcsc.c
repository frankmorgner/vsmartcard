#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <winscard.h>
#include <string.h>
#include <errno.h>

#include "picc_to_pcsc.h"
#include "pcscutil.h"

static FILE *fd; /*filehandle used for PICCDEV*/


static sighandler_t register_sig(int signo, sighandler_t sighandler) {
   struct sigaction new_sig, old_sig;

   new_sig.sa_handler = sighandler;
   sigemptyset (&new_sig.sa_mask);
   new_sig.sa_flags = SA_RESTART;
   if (sigaction (signo, &new_sig, &old_sig) < 0)
      return SIG_ERR;
   return old_sig.sa_handler;

}

void checkSig(int signo){
	if (signo == SIGINT)
		printf("Received SIGINT, exiting.\n");
	cleanup();
}

void cleanup(void) {
	/*Close the device*/
	fclose(fd); 
	/*Die*/
	exit(0);
}

int picc_decode(char *inbuf, size_t inlen,
        unsigned char **outbuf, size_t *outlen)
{
	size_t pos;
	unsigned char buf[0xffff];
        char *end, *p;
        unsigned long int b, length;

        if (inbuf == NULL || inlen == 0 || inbuf[0] == '\0') {
            //Ignore empty lines and 'RESET' lines
            *outlen = 0;
            return 0;
        }

        errno = 0;
        length = strtoul(inbuf, &end, 16);
        if (errno)
            goto err;

        /* check for ':' right behind the length */
        if (inbuf+inlen < end+1 || end[0] != ':')
            goto err;
        end++;

        p = realloc(*outbuf, length);
        if (!p)
            goto err;
        *outbuf = p;
        *outlen = length;
        
        pos = 0;
        while(inbuf+inlen > end && length > pos) {
            b = strtoul(end, &end, 16);
            if (errno || b > 0xff)
                goto err;

            (*outbuf)[pos++] = b;
        }

	return 0;

err:
        *outlen = 0;
        return -1;
}

void write_to_device(unsigned char *buffer, unsigned int msgLength) {
	char* foo;
	foo = (char*) malloc(msgLength*3+5);
	//fprintf(fd, "%04X:", msgLength); 
	sprintf(foo,"%04X:",msgLength);
	int i;
	for(i=0; i<msgLength; i++) {
		sprintf(&foo[3*i+5]," %02X",buffer[i]);
		//fprintf(fd, " %02X", buffer[i]);
	}
	printf("%s",foo);
	fprintf(fd,"%s", foo);
	fprintf(fd, "\r\n");
	fflush(fd);
	free(foo);
}

int main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    unsigned char *inputBuffer = NULL;
    size_t inputLength;
    BYTE outputBuffer[MAX_BUFFER_SIZE];
    DWORD outputLength;

    LPSTR readers = NULL;
    LONG r;
    DWORD ctl, protocol;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;

    char *read = NULL;
    size_t readlen = 0;
    ssize_t linelen;

    unsigned int readernum = 0;


    if (argc > 1) {
        if (argc > 2) {
            fprintf(stderr, "Usage:  "
                    "%s [reader number] [PIN]\n", argv[0]);
        }
        if (sscanf(argv[1], "%u", &readernum) != 1) {
            fprintf(stderr, "Could not get number of reader\n");
            exit(2);
        }
    }


    /*Register signal handlers*/
    register_sig(SIGINT,checkSig);

    /*Open the device*/
    fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
    if (fd == NULL) {
        fprintf(stderr,"Failed to open %s. Exiting.\n", PICCDEV);
        exit(-1);
    } else
        printf("Initialisation completed\n");

    r = pcsc_connect(readernum, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_ANY,
            &hContext, &readers, &hCard, &protocol);
    if (r != SCARD_S_SUCCESS)
        goto err;

    while(1) {
        linelen = getline(&read, &readlen, fd);
        if (linelen < 0) {
            if (linelen < 0) {
                fprintf(stderr,"Error reading from %s\n", PICCDEV);
                goto err;
            }
        }
        if (linelen == 0)
            continue;
        fflush(fd);

        printf("%s\n", read);

        if (picc_decode(read, linelen, &inputBuffer, &inputLength) < 0)
            continue;

        printb("C-APDU:\n", inputBuffer, inputLength);

        outputLength = MAX_BUFFER_SIZE;
        r = pcsc_transmit(protocol, hCard, inputBuffer, inputLength, outputBuffer, &outputLength);
        if (r != SCARD_S_SUCCESS)
            goto err;

        printb("R-APDU:\n", outputBuffer, outputLength);

        write_to_device(outputBuffer, outputLength);
        //fprintf(fd,"0002: 90 00\0");
    }
    cleanup();

err:
    stringify_error(r);
    pcsc_disconnect(hContext, hCard, readers);

    exit(r == SCARD_S_SUCCESS ? 0 : 1);
}
