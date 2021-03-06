#include "Compressione.h"

int main(int argc, char **argv)
{
	clock_t start = clock();

	printf("tempo inizio = %d\n", start); //stampa tempo iniziale

	if (argc >= 2){// verifica il numero di argomenti passati al programma
		fileIn = fopen(argv[1], "r");
	} else {
		fileIn = fopen("C:\\Temp\\scrittura.encode.txt", "rb");
		//fileIn = fopen("C:\\Temp\\alice.txt", "rb+");
	}
	
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
		//lzs_encode(fileIn); // Avvia la codifica del file
		lzs_decode(fileIn); // Avvia la decodifica del file
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

	encodedBuffer = calloc(fileLen + 1, sizeof(*encodedBuffer));
	unsigned char carattere;
	
	//legge byte per byte il file ed inserisce il valore 
	//nell'array (per costruire il encodedBuffer)
	for (index = 0; index < fileLen; index++) {
		fread(encodedBuffer + index, 1, 1, inBuffer);
		carattere = *(encodedBuffer + index);
		encodedBuffer[index] = carattere;
		/*for (int i = 7; i >= 0; i--) {
			printf("%d", getBit(encodedBuffer[index], i));
		}
		printf("\n");*/
	}

	// dopo che ho l'array riempito lo ripercorro cercando eventuali matching pattern
	for (int i = 0; i < fileLen; i++) {
		int slide = matchingPattern(encodedBuffer, i);
		//printf("historyBuffer[i]: %d\n", historyBuffer[i]);
		//printf("slide: %d\n", slide);
		i += slide;
		loadBuffer(slide);
	}

	flush(); // end file

	fclose(fileOut);
	printf("\nCompressione riuscita!!\n");
}

void lzs_decode(FILE *outBuffer) {
	
	// apro/creo il file di output
	fileOut = fopen("C:\\Temp\\scrittura.decode.txt", "wb+");

	decodedBuffer = calloc(fileLen + 1, sizeof *decodedBuffer);
	unsigned char value;

	//legge byte per byte il file compresso ed inserisce 
	//il valore nell'array (per costruire il decodedBuffer)
	for (index = 0; index < fileLen; index++) {
		fread(decodedBuffer + index, 1, 1, outBuffer);
		value = *(decodedBuffer + index);
		decodedBuffer[index] = value;
	}

	readBit(decodedBuffer);

	fseek(fileOut, 0, SEEK_END); //leggo dall'inizio alla fine del file
	fileLen = ftell(fileOut); // leggo la lunghezza del file
	rewind(fileOut); // setto la posizione dello stream all'inizio del file

	printf("filelen decoded = %d\n", fileLen);

	fclose(fileOut);
	printf("\nDecompressione riuscita!!\n");
}