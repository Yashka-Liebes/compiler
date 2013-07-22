#ifndef WORD_H_
#define WORD_H_



typedef enum {absolute = 'a', relocatable = 'r', external = 'e'} Link;

typedef struct {
	int bits;
	Link link;
} Word;



extern const Word EMPTY;



Word initword();
int setbits(int val, int pos);



#endif /* WORD_H_ */


