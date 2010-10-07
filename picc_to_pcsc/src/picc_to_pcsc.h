#ifndef PICC_TO_PCSC
#define PICC_TO_PCSC

#define PICCDEV "/dev/ttyACM0"

typedef void (*sighandler_t)(int);

static sighandler_t register_sig(int signo, sighandler_t sighandler);
void checkSig(int signo);
unsigned int read_from_device(unsigned char *inputBuffer);
void write_to_device(unsigned char *buffer, unsigned int msglength);
void setup(void);
void cleanup(void);
void printHex(char *title, unsigned char *msg, unsigned int length); 

#endif
