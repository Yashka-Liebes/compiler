#ifndef LINE_H_
#define LINE_H_



#include "string.h"


/* this struct will contain information that was extracted from the assembly file */
typedef struct {
	String line;		/* the current line */
	String filename;
	int pos;			/* current position in the line */
	int linec;			/* line counter */
} Fextr;



Fextr initfextr();



extern const Fextr EMPTY_FEXTR;



#endif /* LINE_H_ */
