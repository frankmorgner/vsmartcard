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

#include "picc_to_pcsc.h"

static sighandler_t register_sig(int signo, sighandler_t sighandler) {
   struct sigaction new_sig, old_sig;

   new_sig.sa_handler = sighandler;
   sigemptyset (&new_sig.sa_mask);
   new_sig.sa_flags = SA_RESTART;
   if (sigaction (signo, &new_sig, &old_sig) < 0)
      return SIG_ERR;
   return old_sig.sa_handler;

}

char* perform_initialization(int num)
{
    char *readers, *str;
    DWORD size;
    LONG r;

    r = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hcontext);
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }
    r = SCardListReaders(hcontext, NULL, NULL, &size);
    if (size == 0)
        r = SCARD_E_UNKNOWN_READER;
    if (r != SCARD_S_SUCCESS) {
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }

    /* get all readers */
    readers = (char*)malloc(size);
    if (readers == NULL) {
        fprintf(stderr, "pc/sc error: %s\n",
                pcsc_stringify_error(SCARD_E_NO_MEMORY));
        SCardReleaseContext(hcontext);
	return NULL;
    }
    r = SCardListReaders(hcontext, NULL, readers, &size);
    if (r != SCARD_S_SUCCESS) {
        free(readers);
        fprintf(stderr, "pc/sc error: %s\n", pcsc_stringify_error(r));
        SCardReleaseContext(hcontext);
	return NULL;
    }

    /* name of reader number num*/
    str = readers;
    for (size = 0; size < num; size++) {
        /* go to the next name */
        str += strlen(str) + 1;

        /* no more readers available? */
        if (strlen(str) == 0) {
            free(readers);
            fprintf(stderr, "pc/sc error: %s\n",
                    pcsc_stringify_error(SCARD_E_UNKNOWN_READER));
            SCardReleaseContext(hcontext);
            return NULL;
        }
    }
    strcpy(reader_name, str);
    free(readers);

    rstate.dwCurrentState = SCARD_STATE_UNAWARE;
    rstate.szReader       = reader_name;

    return reader_name;
}


void setup(void) {
	/*setup environment, open device, set global filehandle*/
	DWORD dwActiveProtocol;
   	LONG pcsc_result;

	perform_initialization(0); /*Right num for PICC?*/
	printf("Initialised reader: %s\n",reader_name);
	pcsc_result = SCardConnect(hcontext, reader_name,
            SCARD_SHARE_EXCLUSIVE, (SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1),
            &hcard, &dwActiveProtocol);
	if (pcsc_result == SCARD_S_SUCCESS) {
		pcsc_result = SCardGetStatusChange(hcontext, 1, &rstate, 1);
		if (pcsc_result != SCARD_S_SUCCESS) fprintf(stderr,"%s\n",pcsc_stringify_error(pcsc_result));
	} else {
		fprintf(stderr,"%s\n",pcsc_stringify_error(pcsc_result));
	}
	/*Register our signal handlers*/
	register_sig(SIGINT,checkSig);
	/*Open the device*/
	fd = fopen(PICCDEV, "a+"); /*O_NOCTTY ?*/
	if (fd == NULL) {
		fprintf(stderr,"Failed to open %s. Exiting.\n", PICCDEV);
		exit(-1);
	} else printf("Initialisation completed\n");
	
}

void checkSig(int signo){
	if (signo == SIGINT)
		printf("Received SIGINT, exiting.\n");
	cleanup();
}

void printHex(char *title, unsigned char *msg, unsigned int length) {
	int n = 0;
	printf("%s: \t",title);
	while (n<length) {
		printf("%x ",msg[n]);
		n++;
	}
	printf("\n");
}

void cleanup(void) {
	int releaseContext;	
	/*Power down card*/
	SCardDisconnect(hcard, SCARD_UNPOWER_CARD);
   	releaseContext = SCardReleaseContext(hcontext);
	if (releaseContext != SCARD_S_SUCCESS) {
		printf("%s\n",pcsc_stringify_error(releaseContext));
	}
	printf("Smartcard disconnected\n");
	/*Close the device*/
	fclose(fd); 
	/*Die*/
	exit(0);
}

unsigned int read_from_device(unsigned char *inputBuffer) {
	char *line;
	size_t linelen=0;
	if(getline(&line, &linelen, fd) != -1) {
		printf("%s\n",line);
		fflush(stdout);
	} else {line = NULL;}// cleanup();} //Should we quit when we encounter an EOL?
	
	unsigned int length=0;
	unsigned char *buf=NULL;
	if (line != NULL && line[0] == '0') { //Ignore empty lines and 'RESET' lines
		sscanf(line, "%x : ", &length  );
		buf = malloc(length);
		if(buf == NULL) { fprintf(stderr, "WAHH!\n"); return 0; }
		int pos=0;
		while(sscanf(line+(pos*3+5), " %x ", (unsigned int *) &(buf[pos])) == 1) {
			printf("[%02X]", buf[pos]);
			if(++pos >= length) break;
		}
		
		printf("Have %i bytes, should have %i bytes\n", pos, length);
		memcpy(inputBuffer,buf,length);
	}

	if(line != NULL) {
		free(line);
	}
	if(buf != NULL) {
		free(buf);
	}
	return length;
}

int transmit_to_pcsc(unsigned char *inputBuffer, unsigned int inputLength, unsigned char *outputBuffer, unsigned int *outputLength) {

	printHex("APDU", inputBuffer, inputLength); 
	unsigned int dwRecvLength = MAX_BUFFER_SIZE;
	int result;
	result = SCardTransmit(hcard, SCARD_PCI_T0, inputBuffer,
            inputLength, NULL, outputBuffer, (DWORD*) &dwRecvLength);

	/*Check response code */
	if (result != SCARD_S_SUCCESS) {
		fprintf(stderr,"SCardTransmit failed.\n");
		fprintf(stderr,"dwRecvLength = %d maximum is:%d\n",dwRecvLength,MAX_BUFFER_SIZE);
		fprintf(stderr,"%s\n",pcsc_stringify_error(result));
		return -1;
	} else {
		printf("Received %d Byte\n",dwRecvLength);
		printHex("RAPDU", outputBuffer, dwRecvLength);
		*outputLength = dwRecvLength;
	}
	return 0;
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

int main (int argv, char **args) {
	unsigned char inputBuffer[MAX_BUFFER_SIZE];
	unsigned char outputBuffer[MAX_BUFFER_SIZE];
	unsigned int inputLength, outputLength = MAX_BUFFER_SIZE;
	int status;	
	setup();

	while(1) {
		inputLength = read_from_device(inputBuffer);
		if (inputLength == 0) continue;		
		fflush(fd);
		status = transmit_to_pcsc(inputBuffer,inputLength,outputBuffer,&outputLength);
		if (!status) {
			write_to_device(outputBuffer, outputLength);
			//fprintf(fd,"0002: 90 00\0");
		} else {
			printf("Failed\n");
		}
	}
	cleanup();
	return 0;
}
