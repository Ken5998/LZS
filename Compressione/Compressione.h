#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#define SLIDING_WINDOW 2048

//STRUTTURE
struct bit_field
{
	unsigned x9 : 9; //  9 bits
	unsigned x11 : 11; // 11 bits
	unsigned x13 : 13; // 13 bits
};

//VARIABILI

FILE *fileIn;
FILE *fileOut;
unsigned char *holdingBuffer;
unsigned char *historyBuffer;
int count = 0;
long fileLen; //lunghezza file in byte
long index;
int avanzo = 1;
int resto = 7;
unsigned char temp = NULL;
unsigned char rest = NULL;

int head = 0;
int tail = -1;   // valore a -1 indica che la finestra è vuota
unsigned int outBuffer[8];
int outIndex = 0;

//PROTOTIPI

/*void encode();
unsigned * outputToken(unsigned char * buffer, int index, int count, FILE *output);
unsigned offset(int index, int count);
unsigned outputLength(unsigned char * buffer, FILE *output);
void writeByteChar(FILE *fileout, unsigned int value, int type);*/

void sequenceLength(int length);
void encoding(int offset, int length, unsigned char value);
void loadBuffer(int slide);
void lzs_encode(FILE *inBuffer);
void lzs_decode(FILE *outBuffer);
void integerToBinary(unsigned int input, int dimension, int *buffer);
char writeBit(unsigned int value);
int matchingPattern(unsigned char * bufferChar, int index);

void flush(); // end file 110000000

//unsigned len(unsigned char * s);
bool getBit(int byte, int position);
