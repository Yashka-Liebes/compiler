#ifndef LABEL_H_
#define LABEL_H_



#define NAMELENGTH 31



typedef enum {data, code} Mark;

typedef struct {
	char name[NAMELENGTH];
	int address;
	Mark mark;
} Label;



extern const Label EMPTYLABEL;



Label initlabel();
int emptylabel(Label l);



#endif /* LABEL_H_ */
