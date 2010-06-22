#ifndef PICC_TO_PCSC
#define PICC_TO_PCSC

#define PICCDEV "/dev/ttyACM0"

typedef void (*sighandler_t)(int);
/*Global variables*/
FILE *fd; /*filehandle used for PICCDEV*/
SCARDCONTEXT hcontext;
SCARDHANDLE hcard;
SCARD_READERSTATE rstate;

char reader_name[MAX_READERNAME];

static sighandler_t register_sig(int signo, sighandler_t sighandler);
void checkSig(int signo);
unsigned int read_from_device(unsigned char *inputBuffer);
int transmit_to_pcsc(unsigned char *inputBuffer, unsigned int inputLength, unsigned char *outputBuffer, unsigned int *outputLength);
void write_to_device(unsigned char *buffer, unsigned int msglength);
void setup(void);
void cleanup(void);
void printHex(char *title, unsigned char *msg, unsigned int length); 

#endif
