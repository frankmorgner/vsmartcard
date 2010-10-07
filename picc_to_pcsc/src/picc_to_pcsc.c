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

static FILE *picc_fd = 0; /*filehandle used for PICCDEV*/

static LPSTR readers = NULL;
static SCARDCONTEXT hContext = 0;
static SCARDHANDLE hCard = 0;


static sighandler_t register_sig(int signo, sighandler_t sighandler) {

}

void cleanup_exit(int signo){
    cleanup();
    exit(0);
}

void cleanup(void) {
    fclose(picc_fd); 
    pcsc_disconnect(hContext, hCard, readers);
}

size_t picc_decode_apdu(char *inbuf, size_t inlen, unsigned char **outbuf)
{
    size_t pos, length;
    unsigned char buf[0xffff];
    char *end, *p;
    unsigned long int b;

    if (inbuf == NULL || inlen == 0 || inbuf[0] == '\0') {
        //Ignore empty lines and 'RESET' lines
        goto noapdu;
    }

    length = strtoul(inbuf, &end, 16);

    /* check for ':' right behind the length */
    if (inbuf+inlen < end+1 || end[0] != ':')
        goto noapdu;
    end++;

    p = realloc(*outbuf, length);
    if (!p) {
        fprintf(stderr, "Error allocating memory for decoded C-APDU\n");
        goto noapdu;
    }
    *outbuf = p;

    pos = 0;
    while(inbuf+inlen > end && length > pos) {
        b = strtoul(end, &end, 16);
        if (b > 0xff) {
            fprintf(stderr, "%s:%u Error decoding C-APDU\n", __FILE__, __LINE__);
            goto noapdu;
        }

        (*outbuf)[pos++] = b;
    }

    return length;

noapdu:
    return 0;
}

size_t picc_encode_rapdu(unsigned char *inbuf, size_t inlen, char **outbuf)
{
    char *p;
    unsigned char *next;
    size_t length;

    if (!inbuf || inlen > 0xffff)
        goto err;

    length = inlen*3+5;
    p = realloc(*outbuf, length);
    if (!p) {
        fprintf(stderr, "Error allocating memory for encoded R-APDU\n");
        goto err;
    }
    *outbuf = p;

    sprintf(*outbuf, "%04X:", inlen);

    next = inbuf;
    /* let p point behind ':' */
    p += 5;
    while (inbuf+inlen > next) {
        sprintf(p," %02X",*next);
        next++;
        p += 3;
    }

    return length;

err:
    return 0;
}

int main (int argc, char **argv)
{
    /*printf("%s:%d\n", __FILE__, __LINE__);*/
    void *buf = NULL;
    size_t buflen;

    BYTE outputBuffer[MAX_BUFFER_SIZE];
    DWORD outputLength;

    LONG r = SCARD_S_SUCCESS;
    DWORD ctl, protocol;

    char *read = NULL;
    size_t readlen = 0;
    ssize_t linelen;

    unsigned int readernum = 0;
    int verbose = 0;

    struct sigaction new_sig, old_sig;


    if (argc > 1) {
        readernum = strtoul(argv[1], NULL, 10);
        if (argc > 2) {
            if (0 != strcmp(argv[2], "verbose") || argc > 3) {
                fprintf(stderr, "Usage:  "
                        "%s [reader number] [verbose]\n", argv[0]);
                exit(2);
            }
            verbose++;
        }
    }


    /* Register signal handlers */
    new_sig.sa_handler = cleanup_exit;
    sigemptyset(&new_sig.sa_mask);
    new_sig.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &new_sig, &old_sig) < 0)
        goto err;


    /* Open the device */
    picc_fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
    if (!picc_fd) {
        fprintf(stderr,"Error opening %s\n", PICCDEV);
        goto err;
    }
    printf("Connected to %s\n", PICCDEV);


    /* connect to reader and card */
    r = pcsc_connect(readernum, SCARD_SHARE_EXCLUSIVE, SCARD_PROTOCOL_ANY,
            &hContext, &readers, &hCard, &protocol);
    if (r != SCARD_S_SUCCESS)
        goto err;


    while(1) {
        /* read C-APDU */
        linelen = getline(&read, &readlen, picc_fd);
        if (linelen < 0) {
            if (linelen < 0) {
                fprintf(stderr,"Error reading from %s\n", PICCDEV);
                goto err;
            }
        }
        if (linelen == 0)
            continue;
        fflush(picc_fd);

        if (verbose)
            printf("%s\n", read);


        /* decode C-APDU */
        buflen = picc_decode_apdu(read, linelen, (unsigned char **) &buf);
        if (!buflen)
            continue;

        if (!verbose)
            printb("C-APDU: ===================================================\n", buf, buflen);


        /* transmit APDU to card */
        outputLength = MAX_BUFFER_SIZE;
        r = pcsc_transmit(protocol, hCard, buf, buflen, outputBuffer, &outputLength);
        if (r != SCARD_S_SUCCESS)
            goto err;

        if (!verbose)
            printb("R-APDU:\n", outputBuffer, outputLength);


        /* encode R-APDU */
        buflen = picc_encode_rapdu(outputBuffer, outputLength, (char **) &buf);


        /* write R-APDU */
        if (verbose)
            printf("INF: Writing R-APDU\n\n%s\n\n", buf);

        fprintf(picc_fd,"%s\r\n", buf);
        fflush(picc_fd);
    }

err:
    cleanup();
    stringify_error(r);
    pcsc_disconnect(hContext, hCard, readers);

    exit(r == SCARD_S_SUCCESS ? 0 : 1);
}
