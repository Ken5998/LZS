#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#define SLIDING_WINDOW 2048

//STRUTTURE
typedef enum {
	ENCODE,
	DECODE
} MODES;

//VARIABILI

int head = 0;
int tail = -1;   // valore a -1 indica che la finestra è vuota
unsigned int outBuffer[8];
int outIndex = 0;

int zeroVal = 0; // conta bit a zero durante la scrittura

FILE *fileIn;
FILE *fileOut;
unsigned char *encodedBuffer;
unsigned char *decodedBuffer;

unsigned long countPosition = 0; // Significa che in fase di decompressione verifica se ci sono ancora dei bit da scrivere o meno
long fileLen; //lunghezza file in byte
int index;

//PROTOTIPI

/*ENCODE*/
void sequenceLength(int length);
void encoding(int offset, int length, unsigned char value);
void loadBuffer(int slide);
void lzs_encode(FILE *inBuffer);
void integerToBinary(unsigned int input, int dimension, int *buffer);
char writeBit(unsigned int value);
int matchingPattern(unsigned char * bufferChar, int index);
void flush(); // end file 110000000

bool getBit(int byte, int position);

/*DECODE*/
void readBit(unsigned char *buffer);
void checkBit(unsigned char *buffer);
int bitsCounter(unsigned char *buffer, int number);
unsigned int offsetGet(unsigned char *buffer, int number);
unsigned int lengthGet(unsigned char *buffer, int number, int bits);
int checkZeroValues(unsigned char *buffer, int position, int offset);
void lzs_decode(FILE *outBuffer);

/*
  ______ _   _  _____ ____  _____ _____ _   _  _____
 |  ____| \ | |/ ____/ __ \|  __ \_   _| \ | |/ ____|
 | |__  |  \| | |   | |  | | |  | || | |  \| | |  __
 |  __| | . ` | |   | |  | | |  | || | | . ` | | |_ |
 | |____| |\  | |___| |__| | |__| || |_| |\  | |__| |
 |______|_| \_|\_____\____/|_____/_____|_| \_|\_____|
*/

void encoding(int offset, int length, unsigned char value) {

	// calcolo la dimensione del buffer e riempio in caso che value 
	// è minore di 128 il buffer sarà di 7bit altrimenti sarà di 11bit

	//printf("value: %d\noffset: %d\n", value, offset);

	if (offset == 0) { // non esistono ricorrenze quindi scrive il valore effettivo
		for (int i = 8; i >= 0; i--) {
			writeBit(getBit(value, i));
		}
	}
	else if (offset < 128) {
		// scrive i primi 2bit a 1, i restanti 7 sono l'offset
		// e dopo l'offset scrive la lunghezza della sequenza
		writeBit(1);
		writeBit(1);

		int tempBuff[7];
		integerToBinary(offset, 7, tempBuff);
		for (int i = 0; i < 7; i++) {
			writeBit(tempBuff[i]);
		}
		sequenceLength(length);
	}
	else {
		writeBit(1);
		writeBit(0);

		int tempBuff[11];
		integerToBinary(offset, 11, tempBuff);
		// riempio il buffer di scrittura
		for (int i = 0; i < 11; i++) {
			writeBit(tempBuff[i]);
		}
		sequenceLength(length);
	}
}

void sequenceLength(int length) {
	int n, i, j, x;

	// inserisco la codifica della lunghezza della sequenza
	// come trovato nelle varie guide
	if (length == 2) goto x2;
	if (length == 3) goto x3;
	if (length == 4) goto x4;
	if (length == 5) goto x5;
	if (length == 6) goto x6;
	if (length == 7) goto x7;
	if (length >  7) goto xx;

x2:
	writeBit(0);
	writeBit(0);
	goto fine;
x3:
	writeBit(0);
	writeBit(1);
	goto fine;
x4:
	writeBit(1);
	writeBit(0);
	goto fine;
x5:
	writeBit(1);
	writeBit(1);
	writeBit(0);
	writeBit(0);
	goto fine;
x6:
	writeBit(1);
	writeBit(1);
	writeBit(0);
	writeBit(1);
	goto fine;
x7:
	writeBit(1);
	writeBit(1);
	writeBit(1);
	writeBit(0);
	goto fine;
xx:
	// insersico gruppi di 4 (1111) ad ogni n
	n = (length + 7) / 15;
	for (i = 0; i < n; i++) {
		for (j = 0; j < 4; j++) {
			writeBit(1);
		}
	}
	// completo la sequenza con la codifica del valore
	x = length - ((n * 15) - 7);
	int buff[4];
	integerToBinary(x, 4, buff);
	for (i = 0; i < 4; i++) {
		writeBit(buff[i]);
	}
	goto fine;

fine:;
}

void integerToBinary(unsigned int input, int dim, int *buffer) {
	unsigned int temp = 1U << (dim - 1);
	int i;
	for (i = 0; i < dim; i++) {
		buffer[i] = (input & temp) ? 1 : 0;
		input <<= 1;
	}
}

void flush() {
	// aggiunge il 9° e 8° bit a 1
	writeBit(1);
	writeBit(1);
	// i restanti 7bit vanno a 0
	for (int i = 0; i < 7; i++) {
		writeBit(0);
	}
	// finche il buffer non viene svuotato, aggiungo n volte 0
	while (outIndex != 0) {
		writeBit(0);
	}
}

char writeBit(unsigned int value) { // scrive bit per bit fino al riempimento del byte poi scrive byte su file
	if (outIndex < 8) {
		outBuffer[outIndex] = value;
		outIndex++;
	}

	// se il buffer è pieno scrive su file e svuota il buffer
	unsigned int valueInt = 0;

	// calcola il valore binario nella potenza di due del nuovo char
	if (outIndex == 8) {
		int j = 7;
		for (int i = 0; i < 8; i++) {
			valueInt += outBuffer[i] * pow(2.0, j);
			j--;
		}
		unsigned char valueChar = valueInt;
		fputc(valueChar, fileOut);

		//printf("int value: %d hex: %x\n", valueInt, valueInt);

		outIndex = 0;
		return valueChar;
	}
	return -1;
}

int matchingPattern(unsigned char * bufferChar, int index) {
	int headCopy = head;
	int tailCopy = tail;

	int offset = 0;
	int length = 0;

	if (tailCopy - headCopy >= 0) {
		int countBack = 0;
		while (tailCopy >= headCopy) {
			countBack++;
			//printf("bufferChar[index] == bufferChar[index - countBack]: %d = %d\n", bufferChar[index], bufferChar[index - countBack]);
			if (bufferChar[index] == bufferChar[index - countBack] && index - countBack >= 0) {
				//ora cerca in avanti
				int countAhead = 0;
				//printf("Front: %d\nBack: %d\n", countFront, countBack);
				while (1) {
					countAhead++;
					//printf("Front nel while: %d\nIndex: %d\n\n", countFront, index);
					//printf("bufferChar[index+countFront] == bufferChar[index - countBack]: %d = %d\n", bufferChar[index+countFront], bufferChar[index - countBack]);
					if (bufferChar[index + countAhead] == bufferChar[(index - countBack) + countAhead] && countAhead < SLIDING_WINDOW) {
						//printf("Front nel if: %d\nBack: %d\n", countFront, countBack);
						continue;
					}
					else {
						break;
					}
				}
				if (countAhead > length && countAhead > 1) {
					offset = countBack;
					length = countAhead;
					//printf("offset: %d\nlength: %d\n", offset, length);
				}
			}
			tailCopy--;
		}
	}

	encoding(offset, length, bufferChar[index + length]);

	if (length != 0) {
		length--;
	}

	// si sposta in avanti
	return length;
}

void loadBuffer(int slide) {
	// finestra non piena
	if (tail - head < SLIDING_WINDOW) {
		if (tail == -1) {
			//inizializzazione
			head = 0;
			tail = 0;
		}
		else {
			if ((tail + slide) - head > SLIDING_WINDOW) {
				tail += slide + 1;
				head += slide + 1;
				//printf("head: %d\n tail: %d\n", head, tail);
			}
			else {
				tail += slide + 1;
				//printf("\ntail: %d\n", tail);
			}

		}
	}
	else {
		// finestra piena
		head += slide + 1;
		tail += slide + 1;
		//printf("head: %d\n tail: %d\n", head, tail);
	}
}

bool getBit(int byte, int position) // position in range 0-7 _ 0-9 _ 0-11 _ 0-13
{
	return (byte >> position) & 0x1;
}

/*
  _____  ______ _____ ____  _____ _____ _   _  _____
 |  __ \|  ____/ ____/ __ \|  __ \_   _| \ | |/ ____|
 | |  | | |__ | |   | |  | | |  | || | |  \| | |  __
 | |  | |  __|| |   | |  | | |  | || | | . ` | | |_ |
 | |__| | |___| |___| |__| | |__| || |_| |\  | |__| |
 |_____/|______\_____\____/|_____/_____|_| \_|\_____|
*/

void readBit(unsigned char *buffer)
{
	unsigned char * bufferToBinary;
	bufferToBinary = calloc((fileLen * 9) + 1, sizeof *bufferToBinary);

	int k = 0;

	for (int i = 0; i < fileLen; i++) {
		for (int j = 7; j >= 0; j--) {
			bufferToBinary[k] = getBit(buffer[i], j);
	//		printf("%d", getBit(buffer[i], j));
			k++;
		}
	//	printf("\n");
	}

	/*printf("conenuto di bufferToBinary:");
	for (int i = 0; i < fileLen *8; i++) {
		if (i % 8 == 0) printf("\n");
		printf("%d", bufferToBinary[i]);
	}*/

	checkBit(bufferToBinary);
}

void checkBit(unsigned char *buffer) 
{
	long numberOfBits = 0, val = 0, i = 0, temp = 0;
	unsigned long offset = 0, length = 0, finalLength = 0, k = 0;
	unsigned char * arrayLength, *historyArray;

	int numOfBitsForEndFile = checkEndFile(buffer);

	arrayLength = calloc((fileLen * 8) + 1, sizeof *arrayLength); // array che contiene il numero di bit per ogni byte codificato
	historyArray = calloc((fileLen * 12) + 1, sizeof *historyArray); // array che contiene i veri valori dei caratteri

	while(k <= numOfBitsForEndFile){
	//for (k = 0; k < fileLen * 12 + 1;) {
		if (buffer[k] == 1 && buffer[k + 1] == 1 && buffer[k + 2] == 0 && buffer[k + 3] == 0 && buffer[k + 4] == 0 && buffer[k + 5] == 0 && buffer[k + 6] == 0 && buffer[k + 7] == 0 && buffer[k + 8] == 0) {
			//printf("FINE\nk: %d\nfileLen * 16 + 1 = %d\n", k, fileLen * 16 + 1);
			break;
		}
		else {
			if (buffer[k] == 0) {
				k++;
				int temp = 0;
				for (int h = 0; h < 8; h++) {
					if (buffer[k + h] == 0) temp++;
				}
				if(temp != 8){
					//printf("\nunsigned char buffer: ");
					for (int j = 0; j < 8; j++) {
						writeBit(buffer[k + j]);
						historyArray[countPosition] = buffer[k + j];
						//printf("%d", buffer[k + j]);
						countPosition++;
					}
				}
				k = k + 8;
				arrayLength[i] = 9;
				//arrayLength[i] = 8;
				i++;
				//printf("\n");
			}
			else {
				offset = offsetGet(buffer, k);
				numberOfBits = bitsCounter(buffer, k);
				length = lengthGet(buffer, k, numberOfBits);
				temp = 0;

				//printf("\noffset: %d\nnumberOfBits: %d\nlength: %d\n", offset, numberOfBits, length);

				val = i;
				
				// calcolo il numero di bit che devo andare indietro partendo dall'offset trovato
				for (long x = offset; x >= 0; x--) {
					if (val >= 0) {
						temp = temp + arrayLength[val];
						val--;
					}
				}

				//for (int a = 0; a < i; a++) {
					//printf("\nval di arrayLength: %d\n", arrayLength[a]);
				//}

				finalLength = length;
				val = k + 1 - temp;
				//printf("k: %d\ntemp: %d\nval: %d\n", k, temp, val);
				
				//trovata la ricorrenza la scrive verificando se il primo bit del byte successivo
				//e' = 1 allora significa che deve riscrivere x volte lo stesso carattere
				while (finalLength > 0) {

					//if (checkZeroValues(historyArray, countPosition, offset) > 0) {
						//printf("count zero values: %d\nk value = %d\n", checkZeroValues(historyArray, countPosition, offset), k);
					//}

					for (int j = 0; j < 8; j++) {

						if (checkZeroValues(historyArray, countPosition, offset) > 0) {
							historyArray[countPosition] = historyArray[(countPosition + 8 * checkZeroValues(historyArray, countPosition, offset)) - 8 * offset];
							writeBit(historyArray[countPosition]);
							//printf("writeBit: %d\ncount position: %d\nval: %d\n", historyArray[(countPosition + 8 * checkZeroValues(historyArray, countPosition, offset)) - 8 * offset], countPosition, val);
							countPosition++;
						}
						else {
							historyArray[countPosition] = historyArray[countPosition - 8 * offset];
							writeBit(historyArray[countPosition]);
							//printf("writeBit: %d\ncount position: %d\nval: %d\n", historyArray[countPosition - 8 * offset + j], countPosition, val);
							countPosition++;
						}
					}
					//val = iniziale;
					//printf("val: %d\n", val);

					finalLength--;
					}

				val = k;

				if (length >= 2 && length <= 4) temp = 2;
				if (length >= 5 && length <= 7) temp = 4;
				if (length >= 8 && length <= 22) temp = 8;
				if (length >= 23 && length <= 37) temp = 12;
				if (length > 37) {
					while (buffer[val + 2 + numberOfBits + 1] == 1 && buffer[val + 2 + numberOfBits + 2] == 1 && buffer[val + 2 + numberOfBits + 3] == 1 && buffer[val + 2 + numberOfBits + 4] == 1) {
						temp = temp + 5;
						val = val + 5;
					}
					temp = temp + 4;
				}

				k = k + 2 + numberOfBits + temp;
				
				arrayLength[i] = 2 + numberOfBits + temp;
				i++;
			}
		}

	}

	/*
	for (int i = 0; i < countPosition; i++) {
		if (i % 8 == 0) printf("\n");
		printf("%d", historyArray[i]);
	}

	printf("\nFINE\nk: %d\nfileLen * 16 + 1 = %d\ntemp: %d\ncount position: %d\ncheck end file: %d\n", k, fileLen * 16 + 1, temp, countPosition, numOfBitsForEndFile);
	printf("\nMAX VALUE OF INT: %d\n", INT_MAX);
	printf("\nValue of i: %d\n", i);
	*/
}

int bitsCounter(unsigned char *buffer, int number) 
{
	// metodo che conta il numero di bits per capire se l'offset è inforiore a 128 oppure maggiore
	// 0b11 offset < 128 ... 0b10 offset >= 128
number = number + 1;
int value;

if (buffer[number] == 0) {
	value = 11;
	return value;
}
else {
	value = 7;
	return value;
}

}

unsigned int offsetGet(unsigned char *buffer, int number)
{
	// metodo che restituisce l'offset da dove parte la sequenza
	number = number + 1;
	unsigned int returnValue = 0;

	if (buffer[number] == 0) {
		number++;
		for (int i = 10; i >= 0; i--) {
			returnValue |= (buffer[number] << i);
			number++;
		}
		return returnValue;
	}
	else {
		number++;
		for (int i = 6; i >= 0; i--) {
			returnValue |= (buffer[number] << i);
			number++;
		}
		return returnValue;
	}
}

unsigned int lengthGet(unsigned char *buffer, int number, int bits)
{
	// metodo che restituisce la lunghezza della sequenza
	number = number + bits + 2;
	int value = 0, temp = 0;
	unsigned int length = 0;

	if (buffer[number] == 1 && buffer[number + 1] == 1) {
		number = number + 2;
		if (buffer[number] == 1 && buffer[number + 1] == 1) {
			number = number + 2;
			while (buffer[number] == 1 && buffer[number + 1] == 1 && buffer[number + 2] == 1 && buffer[number + 3] == 1) {
				value = value + 15;
				number = number + 4;
			}
			length = value + 8;
			for (int i = 3; i >= 0; i--) {
				temp |= (buffer[number] << i);
				number++;
			}
			length = length + temp;
		}
		else {
			if (buffer[number] == 0 && buffer[number + 1] == 0) length = 5;
			if (buffer[number] == 0 && buffer[number + 1] == 1) length = 6;
			if (buffer[number] == 1 && buffer[number + 1] == 0) length = 7;
		}
	}
	else {
		if (buffer[number] == 0 && buffer[number + 1] == 0) length = 2;
		if (buffer[number] == 0 && buffer[number + 1] == 1) length = 3;
		if (buffer[number] == 1 && buffer[number + 1] == 0) length = 4;

	}

	return length;
}

int checkZeroValues(unsigned char *buffer, int position, int offset) {

	zeroVal = 0;

	for (int i = 0; i < 64; i++) {
		if (buffer[(position + i) - (8 * offset)] == 0) {
			zeroVal++;
		}
		else break;
	}

	while (zeroVal % 8 != 0) {
		zeroVal--;
	}

	if (zeroVal >= 8) {
		return zeroVal / 8;
	}
	else return 0;
}

int checkEndFile(unsigned char *buffer) {
	int returnValue = 0, i;


	continueSearching:
	for (i = 0; i < fileLen * 8; i++) {
		if(buffer[i] == 1 && buffer[i + 1] == 1 && buffer[i + 2] == 0 && buffer[i + 3] == 0 && buffer[i + 4] == 0 && buffer[i + 5] == 0 && buffer[i + 6] == 0 && buffer[i + 7] == 0 && buffer[i + 8] == 0){
			break;
		}
		else {
			returnValue++;
		}
	}

	if (buffer[returnValue + 9] == 1 || buffer[returnValue + 10] == 1 || buffer[returnValue + 11] == 1 || buffer[returnValue + 12] == 1 || buffer[returnValue + 13] == 1 || buffer[returnValue + 14] == 1 || buffer[returnValue + 15] == 1 || buffer[returnValue + 16] == 1) goto continueSearching;

	return returnValue;
}