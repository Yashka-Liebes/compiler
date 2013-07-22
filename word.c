#include "word.h"



/* function prototype */
static int power(int x, int y);



const Word EMPTY = {
		0
};



Word initword() {
	return EMPTY;
}

int setbits(int val, int pos) {
	return val * (int) power(2, pos);
}

static int power(int x, int y) {
	if (y == 0)
		return 1;

	return x * power(x, y - 1);
}



