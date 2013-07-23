#ifndef FEXTRACT_H_
#define FEXTRACT_H_



#include "string.h"



typedef struct {
	String line;
	String filename;
	int linec;
	int pos;
} Fextr;



extern const Fextr EMPTY_FXTR;



Fextr initfextr();



#endif /* FEXTRACT_H_ */
