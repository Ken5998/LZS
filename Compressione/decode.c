#include "Compressione.h"

void readBit(unsigned char value) {
	int len, i;
	len = 8;

	//legge la codifica del byte in 8 bit
	for (i = 0; i < len; i++) {
		if ((value >> (len - i - 1)) % 2 == 0) {
			writeLiteral(0);
		}
		else {
			extract(1);
		}
	}
}

void writeLiteral()