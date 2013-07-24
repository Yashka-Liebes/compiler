#ifndef LINE_H_
#define LINE_H_



#include "string.h"



typedef struct {
	String line;
	String filename;
	int pos;
	int linec;
} Fextr;



Fextr initfextr();



extern const Fextr EMPTY_FEXTR;



#endif /* LINE_H_ */
