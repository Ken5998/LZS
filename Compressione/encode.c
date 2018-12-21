#include "Compressione.h"

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
	if (length > 7) goto xx;

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