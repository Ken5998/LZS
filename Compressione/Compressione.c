#define _CRT_SECURE_NO_WARNINGS
#include "Compressione.h"
#define OPTION 'C'

int main(int argc, char **argv)
{
	clock_t start = clock();

	printf("tempo inizio = %d\n", start); //stampa tempo iniziale

	if (OPTION == 'C') {
		goto Compression;
	}
	else if (OPTION == 'D') {
		goto Decompression;
	}

	Compression:
	if (argc >= 2){// verifica il numero di argomenti passati al programma
		fileIn = fopen(argv[1], "r");
	} else {
		fileIn = fopen("C:\\Temp\\alice.txt", "r");
	}
	goto Continue;

	Decompression:
	if (argc >= 2) {// verifica il numero di argomenti passati al programma
		fileIn = fopen(argv[1], "rb");
	}
	else {
		fileIn = fopen("C:\\Temp\\scrittura.txt", "rb");
	}
	goto Continue;
	
	Continue:
	printf("argc = %d\n", argc); //stampa numero di argomenti

	for (int i = 0; i < argc; ++i) //stampa tutti gli argomenti
		printf("argv[ %d ] = %s\n", i, argv[i]);

	fseek(fileIn, 0, SEEK_END); //leggo dall'inizio alla fine del file
	fileLen = ftell(fileIn); // leggo la lunghezza del file
	rewind(fileIn); // setto la posizione dello stream all'inizio del file

	printf("filelen = %d\n", fileLen);

	if (!fileIn) {
		printf("errore fopen");
		perror(fileIn);
	} else {
		lzs_encode(fileIn); // Avvia la codifica del file
	}
	
	clock_t end = clock();
	printf("tempo fine = %d\n", end); //stampa tempo fine
	printf("Tempo di esecuzione =  %f secondi \n", ((double)(end - start)) / CLOCKS_PER_SEC);

	fclose(fileIn);

	_getch();
}

void lzs_encode(FILE *inBuffer) {
	// apro/creo il file di output
	fileOut = fopen("C:\\Temp\\scrittura.encode.txt", "wb");

	historyBuffer = calloc(fileLen + 1, sizeof *historyBuffer);
	struct bit_field bits;

	//legge byte per byte il file ed inserisce valore 
	//nell'array (per costruire il historyBuffer)
	for (index = 0; index < fileLen; index++) {
		fread(historyBuffer + index, 1, 1, inBuffer);
		bits.x9 = *(historyBuffer + index);
		historyBuffer[index] = bits.x9;
	}

	// dopo che ho l'array riempito lo ripercorro cercando eventuali matching pattern
	for (int i = 0; i < fileLen; i++) {
		int slide = matchingPattern(historyBuffer, i);
		//printf("historyBuffer[i]: %d\n", historyBuffer[i]);
		//printf("slide: %d\n", slide);
		i += slide;
		loadBuffer(slide);
	}

	flush(); // end file
	
	fclose(fileOut);
	printf("Compressione riuscita!!\n");
}

void lzs_decode(FILE *outBuffer) {
	// apro/creo il file di output
	fileOut = fopen("C:\\Temp\\scrittura.decode.txt", "wb");

	historyBuffer = calloc(fileLen + 1, sizeof *historyBuffer);
	struct bit_field bits;

	//legge byte per byte il file ed inserisce valore 
	//nell'array (per costruire il historyBuffer)
	for (index = 0; index < fileLen; index++) {
		fread(historyBuffer + index, 1, 1, outBuffer);
		bits.x9 = *(historyBuffer + index);
		historyBuffer[index] = bits.x9;
	}

	// dopo che ho l'array riempito lo ripercorro cercando eventuali matching pattern
	for (int i = 0; i < fileLen; i++) {
		//int slide = matchingPattern(historyBuffer, i);
		//printf("historyBuffer[i]: %d\n", historyBuffer[i]);
		//printf("slide: %d\n", slide);
		readBit(historyBuffer[index]);
	}

	fclose(fileOut);
	printf("Compressione riuscita!!\n");
}