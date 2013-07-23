
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "word.h"
#include "label.h"
#include "string.h"
#include "fextract.h"



#define COMB    	0
#define DESTINATION 2
#define ORIGIN   	7
#define OPCODE   	12
#define TYPE	 	16
#define DBL		 	17
#define ADDR 	 	3
#define OPERAND  	5

#define IMADDR 		0
#define DIRECTADDR 	1
#define INDEXADDR 	2
#define REGADDR 	3

#define ENTRY	"entry"

#define NOT 	4
#define LEA 	6
#define RTS 	14

#define BEGINNING		100
#define MASK			524288
#define ASCIIZERO 		48
#define NFILES 			100
#define MAXLENGTH		2000
#define LABELMAXSIZE	30

#define NOTRELEVANT 0


#define REGISTERINSTR(f) (getreg(&(f).line.chars[(f).pos]) != -1)

#define REGISTER(c) (getreg(c) != -1)

#define EMPTY(f) ((f).line.chars[(f).pos] == ' ' || (f).line.chars[(f).pos] == '\t')

#define SETBITS(ic, val, pos) IC[ic].bits |= setbits(val, pos)


#define NODELIMITER(f) (					\
	(f).line.chars[(f).pos] != ' '			\
		&& (f).line.chars[(f).pos] != '\t'	\
		&& (f).line.chars[(f).pos] != ':'	\
		&& (f).line.chars[(f).pos] != '\n'	\
		&& (f).line.chars[(f).pos] != '{'	\
		&& (f).line.chars[(f).pos] != '}'	\
		&& (f).line.chars[(f).pos] != ',')


#define WHITESPACES(f)	\
	while (EMPTY(f))	\
		(f).pos++;


#define EXPECTCHAR(f, c)																		\
	WHITESPACES(f)																				\
																								\
	if ((f).line.chars[(f).pos++] != c)	{														\
		printf("%s:%d:%d assembler: missing %c\n", (f).filename.chars, (f).linec, (f).pos, c);	\
		exit(1);																				\
	}																							\
																								\
	WHITESPACES(f)


#define ONEORZERO(f)														\
	if ((f).line.chars[(f).pos] != '0' && (f).line.chars[(f).pos] != '1')	\
		EXIT(f, "wrong type-dbl-comb assignment")


#define SKIPFIRSTLABEL(f) 											\
	for (f.pos = 0; NODELIMITER(f); f.pos++) 						\
		;															\
	(f).pos = (f).line.chars[(f).pos] == ':' ? (f).pos + 1 : 0;		\
																	\
	WHITESPACES(f)

#define EXIT(f, s) {																\
	printf("%s:%d:%d assembler: %s\n", (f).filename.chars, (f).linec, (f).pos, s);	\
	exit(1);																		\
}



/* function prototype */
void compile(String fname);
void init();
void secondpass(FILE *asf, String fname, int dc);
void getentry(Fextr fxtr, FILE *entfp);
void setaddress(Fextr fxtr, int ic, int next, FILE *extfp);
void verifylabels();
Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark);
int firstpass(FILE *asf, String fname);
int getline(String *line, FILE *fp);
int getdirection(int *labelc, Fextr fxtr, int dc, int *extlabelc);
int getdata(int *labelc, Fextr fxtr, int dc);
int getstring(int *labelc, Fextr fxtr, int dc);
int getextern(int *labelc, Fextr fxtr, int dc, int *extlabelc);
int getinstruction(int *labelc, Fextr fxtr, int ic);
int getreg(char *line);
int getcomb(Fextr *fxtr);
int countlabelwords(int ic, Fextr *fxtr, int wordpos);
int countwords(Fextr *fxtr, int ic, int wordpos);
int setoperand(Fextr fxtr, int ic, int next, FILE *extfp);
int getnumber(Fextr *fxtr);



struct {
	const char *type;
	int (*func)();
} directionarr[] = {
		{"data", getdata},
		{"string", getstring},
		{"extern", getextern},
		{"notexist", NULL},
};

const char *opcode[] = {
		"mov",
		"cmp",
		"add",
		"sub",
		"not",
		"clr",
		"lea",
		"inc",
		"dec",
		"jmp",
		"bne",
		"red",
		"prn",
		"jsr",
		"rts",
		"stop",
		0
};



Word IC[MAXLENGTH];
Label ltbl[MAXLENGTH], extltbl[MAXLENGTH];
int DC[MAXLENGTH];
int linec;



int main(int argc, char *argv[]) {
	while (--argc) {
		compile(tostring(argv[argc]));
	}

	return 1;
}

void compile(String fname) {
	FILE *fp;
	String temp;

	init();
	temp = fname;

	fp = fopen(strcat(temp.chars, ".as"), "r");
	if (!fp) {
		printf("assembler: could not open ro file %s\n", temp.chars);
		exit(1);
	}

	secondpass(fp, fname, firstpass(fp, fname));
}

void init() {
	int i;
	
	for (i = 0; i < MAXLENGTH; i++) {
		IC[i] = initword();
		DC[i] = 0;
		ltbl[i] = extltbl[i] = initlabel();
	}
}

int firstpass(FILE *asf, String fname) {
	Fextr fxtr;
	int ic, dc, labelc, extlabelc, i;

	fxtr =  initfextr();
	fxtr.filename = fname;

	for (linec = extlabelc = labelc = ic = dc = 0; getline(&(fxtr.line), asf) != EOF; fxtr.line = initstring()/*, fxtr.linec++*/) {
		fxtr.pos =  0;
		WHITESPACES(fxtr)

		if (fxtr.line.chars[0] != ';' && fxtr.line.chars[fxtr.pos] != '\n') {
			SKIPFIRSTLABEL(fxtr)

			if (fxtr.line.chars[fxtr.pos] == '.')
				dc = getdirection(&labelc, fxtr, dc, &extlabelc);
			else
				ic = getinstruction(&labelc, fxtr, ic);

											printf("linec = %d, dc = %d, ic = %d\tline: ~~%s~~\n", fxtr.linec, dc, ic, fxtr.line.chars);
		}
	}

	for (i = 0; !emptylabel(ltbl[i]); i++)
		ltbl[i].address += (ic * !ltbl[i].mark) + BEGINNING;

	verifylabels();

							/*for (i = 0; i < ic; i++)
										printf("text %o\t%o\t%c\n", i + BEGINNING, IC[i].bits, IC[i].link);
							for (i = 0; i < dc; i++)
										printf("text %o\t%o\n", i + ic + BEGINNING, DC[i]);*/

	return dc;
}

void secondpass(FILE *asf, String fname, int dc) {
	Fextr fxtr;
	String obfname, entfname, extfname;
	FILE *obfp, *entfp, *extfp;
	int ic, i, next;

	fxtr = initfextr();
	fxtr.filename = obfname = entfname = extfname = fname;

	obfp = 	fopen(strcat(obfname.chars, ".ob"), "w");
	entfp = fopen(strcat(entfname.chars, ".ent"), "w");
	extfp = fopen(strcat(extfname.chars, ".ext"), "w");

	if (!obfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname.chars, "ob"));
		exit(1);
	}

	if (!entfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname.chars, "ent"));
		exit(1);
	}

	if (!extfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname.chars, "ext"));
		exit(1);
	}

	fseek(asf, 0L, SEEK_SET);

	for (ic = 0; getline(&fxtr.line, asf) != EOF; fxtr.line = initstring()/*, fxtr.linec++*/) {
		SKIPFIRSTLABEL(fxtr)
		
		if (fxtr.line.chars[fxtr.pos] == '\n' || fxtr.line.chars[0] == ';')
			continue;
		
		if (fxtr.line.chars[fxtr.pos] == '.') {
			getentry(fxtr, entfp);
			continue;
		}
		
		while (fxtr.line.chars[fxtr.pos++] != '/')
			;

		while (!isalpha(fxtr.line.chars[fxtr.pos]) && fxtr.line.chars[fxtr.pos] != '#' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		if (fxtr.line.chars[fxtr.pos] == '\n')
			continue;

		next = setoperand(fxtr, ic, 1, extfp);

		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		if (fxtr.line.chars[fxtr.pos] == '\n')
			continue;

		fxtr.pos++;
		WHITESPACES(fxtr)
		
		next = setoperand(fxtr, ic, next, extfp);
		ic += next;
	}

	for (i = 0; i < ic; i++)
		fprintf(obfp, "text %o\t%o\t%c\n", i + BEGINNING, IC[i].bits, IC[i].link);

	for (i = 0; i < dc; i++)
		fprintf(obfp, "text %o\t%o\n", i + ic + BEGINNING, DC[i]);
}

int getdirection(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	int i;
	String direction;

	fxtr.pos++;
	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &direction, ' ', '\t', AFTERLAST);

	/*
	for(i = 0, direct = initstring(); !EMPTY(line, linepos); direct.chars[i++] = line.chars[linepos++])
		;
	direction.chars[i] = '\0';
	*/

	for (i = 0; directionarr[i].func && strcmp(direction.chars, directionarr[i].type); i++)
		;

	if (!strcmp(direction.chars, ENTRY))
		return dc;

	if (!directionarr[i].func) {
		printf("(%d:%d) assembler: ~%s~ function not found\n", fxtr.linec, fxtr.pos, direction.chars);
		exit(1);
	}

	return (*(directionarr[i].func))(labelc, fxtr, dc, extlabelc);
}

int getdata(int *labelc, Fextr fxtr, int dc) {
	WHITESPACES(fxtr)

	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);
	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	for (; fxtr.line.chars[fxtr.pos] != '\n'; dc++) {
		DC[dc] = getnumber(&fxtr);

		if (fabs(DC[dc]) > MASK / 2) {
			printf("(%d:%d)assembler: usage - number out of range\n", fxtr.linec, fxtr.pos);
			exit(1);
		}
		dc++;

		WHITESPACES(fxtr)

		if (fxtr.line.chars[fxtr.pos] == '\n')
			return dc;

		if (fxtr.line.chars[fxtr.pos] != '\n') {
			EXPECTCHAR(fxtr, ',')
		}
	}

	return dc;
}

int getstring(int *labelc, Fextr fxtr, int dc) {
	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);

	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	WHITESPACES(fxtr)

	EXPECTCHAR(fxtr, '"')

	while(fxtr.line.chars[fxtr.pos] != '"' && fxtr.line.chars[fxtr.pos] != '\n')
		DC[dc++] = fxtr.line.chars[fxtr.pos++];
	DC[dc++] = 0;

	EXPECTCHAR(fxtr, '"')

	return dc;
}

void getentry(Fextr fxtr, FILE *entfp) {
	int i;
	String label;

	fxtr.pos++;
	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', AFTERLAST);

	/*
	for (i = 0, label = initstring(); line.chars[linepos] != ' ' && line.chars[linepos] != '\t'; label.chars[i++] = line.chars[linepos++])
		;
	label.chars[i] = '\0';
	 */

	if (!strcmp(label.chars, ENTRY)) {
		WHITESPACES(fxtr)

		fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', '\n', AFTERLAST);

		/*
				for (i = 0, label = initstring(); !EMPTY(line, linepos) && line.chars[linepos] != '\n'; (label.chars[i++] = line.chars[linepos++]))
					;
				label.chars[i] = '\0';
		*/
		
		for (i = 0; !emptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
			;
		
		if (!emptylabel(ltbl[i]))
			fprintf(entfp, "%s\t%o\n", ltbl[i].name, ltbl[i].address);
		else {
			printf("(%d:%d) assembler: \"%s\": label not found\n", fxtr.linec, fxtr.pos, label.chars);
			exit(1);
		}

		EXPECTCHAR(fxtr, '\n')
	}
}

int getextern(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	Label lbl;
	WHITESPACES(fxtr)

	lbl = getlabel(fxtr, 0, NOTRELEVANT, data);

	if (emptylabel(lbl)) {
		printf("(%d:%d) assembler: label exceeds maximum size or followed by junk\n", fxtr.linec, fxtr.pos);
		exit(1);
	}

	extltbl[(*extlabelc)++] = lbl;

	return dc;
}

Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark) {
	Label lbl;
	String name;
	int must;

	if ((labelflag && mark == data) || !labelflag)
		must = 1;
	else
		must = 0;

	fxtr.pos *=  !labelflag;

	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &name, ' ', '\t', '{', '}', ':', ',', '\n', AFTERLAST);
	tocharptr(name, lbl.name, NAMELENGTH);

	/*
		for (i = 0, lbl = initlabel(); linepos < LABELMAXSIZE && NODELIMITER(line, linepos); lbl.name[i++] = line.chars[linepos++])
			;
		lbl.name[i] = '\0';
	*/

	lbl.address = address;
	lbl.mark = mark;

	WHITESPACES(fxtr)

	if (must && emptylabel(lbl))
		EXIT(fxtr, "label expected")

	if (lbl.name[0] && !isalpha(lbl.name[0]))
		EXIT(fxtr, "elegal first char in label")

	if ((labelflag && fxtr.line.chars[fxtr.pos] == ':') || (!labelflag && fxtr.line.chars[fxtr.pos] == '\n'))
		return lbl;

	return lbl = initlabel();
}

int getinstruction(int *labelc, Fextr fxtr, int ic) {
	#define OPCODESIZE 5
	int i, l;
	char command[OPCODESIZE];

	ltbl[*labelc] = getlabel(fxtr, 1, ic, code);
		if (!emptylabel(ltbl[*labelc]))
			(*labelc)++;

	for (l = i = 0; fxtr.line.chars[fxtr.pos] != '/' && i < OPCODESIZE - 1 && !EMPTY(fxtr); command[i++] = fxtr.line.chars[fxtr.pos++])
		;
	command[i] = '\0';

	for (i = 0; opcode[i] && strcmp(opcode[i], command); i++)
		;

	if (!opcode[i]) {
		printf("%s:%d:%d assembler: command \"%s\" not found\n", fxtr.filename.chars, fxtr.linec, fxtr.pos, command);
		exit(1);
	}

	SETBITS(ic, i, OPCODE);
	EXPECTCHAR(fxtr, '/')

	ONEORZERO(fxtr)
	SETBITS(ic, fxtr.line.chars[fxtr.pos] - ASCIIZERO, TYPE);

	if (fxtr.line.chars[fxtr.pos++] - ASCIIZERO) {
		EXPECTCHAR(fxtr, '/')
		SETBITS(ic, getcomb(&fxtr), COMB);
	}

	EXPECTCHAR(fxtr, ',')
	SETBITS(ic, fxtr.line.chars[fxtr.pos++] - ASCIIZERO, DBL);

	WHITESPACES(fxtr)

	if (i < RTS && i != LEA && i >= NOT)
		l = countwords(&fxtr, ic, DESTINATION);

	if (i < NOT || i == LEA) {
		l = countwords(&fxtr, ic, ORIGIN);
										printf("%d:%d l is %d\n", fxtr.linec, fxtr.pos, l);

		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		EXPECTCHAR(fxtr, ',')
		l += countwords(&fxtr, ic, DESTINATION);
										printf("%d:%d l is %d\n", fxtr.linec, fxtr.pos, l);
	}

	EXPECTCHAR(fxtr, '\n')
	IC[ic].link = absolute;

	return ++ic + l;
}

/* getline: read a line into String s, return number of characters that were read or EOF */
int getline(String *line, FILE *fp) {
	int c, i;

	for (i = 0; i < LENGTH && (c = getc(fp)) != EOF && c != '\n'; (*line).chars[i++] = c)
		;

	if (c == EOF && !i) {
	 	fseek(fp, 0L, SEEK_END);
		return c;
	}

	(*line).chars[i] = '\n';

	return i;
}

int getreg(char *c) {
	int i;

	if (*c == 'r' || *c == 'R') {
		i = *(++c) - ASCIIZERO;
		if (i >= 0 && i < 8 && (*(++c) == '\t' || *c == ' ' || *c == '}' || *c == '\0' || *c == '\n' || *c == '\r'))
			return i;
	}

	return -1;
}

int getcomb(Fextr *fxtr) {
	int i;

	ONEORZERO(*fxtr)
	i = (*fxtr).line.chars[(*fxtr).pos++] - ASCIIZERO;
	i *= 2;
	
	EXPECTCHAR(*fxtr, '/')
	
	ONEORZERO(*fxtr)
	i += (*fxtr).line.chars[(*fxtr).pos++] - ASCIIZERO;

	return i;
}

int countlabelwords(int ic, Fextr *fxtr, int wordpos) {
	int reg;

	while (NODELIMITER(*fxtr))
		(*fxtr).pos++;

	WHITESPACES(*fxtr)

	if ((*fxtr).line.chars[(*fxtr).pos] == '{') {
		SETBITS(ic, INDEXADDR, wordpos + ADDR);

		(*fxtr).pos++;
		WHITESPACES(*fxtr)

		if ((reg = getreg(&(*fxtr).line.chars[(*fxtr).pos])) != -1) {
			SETBITS(ic, reg, wordpos);

			while (NODELIMITER(*fxtr))
				(*fxtr).pos++;
			EXPECTCHAR(*fxtr, '}')

			return 1;
		}

		if ((*fxtr).line.chars[(*fxtr).pos] == '*') {
			(*fxtr).pos++;
			WHITESPACES(*fxtr)
		}

		while (NODELIMITER(*fxtr))
			(*fxtr).pos++;
		EXPECTCHAR(*fxtr, '}')

		return 2;
	}
	else
		SETBITS(ic, DIRECTADDR, wordpos + ADDR);

	return 1;
}

int countwords(Fextr *fxtr, int ic, int wordpos) {
	int reg;

	if ((*fxtr).line.chars[(*fxtr).pos] == '#') {
		SETBITS(ic, IMADDR, wordpos + ADDR);

		if (wordpos == DESTINATION)
			EXIT(*fxtr, "destination operand")

		while (NODELIMITER(*fxtr))
			(*fxtr).pos++;

		WHITESPACES(*fxtr)

		return 1;
	}
	
	if ((reg = getreg(&(*fxtr).line.chars[(*fxtr).pos])) != -1) {
		SETBITS(ic, reg, wordpos);
		SETBITS(ic, REGADDR, wordpos + ADDR);
		
		while (NODELIMITER(*fxtr))
			(*fxtr).pos++;

		WHITESPACES(*fxtr)

		return 0;
	}
	
	return countlabelwords(ic, fxtr, wordpos);
}

int setoperand(Fextr fxtr, int ic, int next, FILE *extfp) {
	if (fxtr.line.chars[fxtr.pos] == '#') {

		IC[ic + next].bits = getnumber(&fxtr);
		IC[ic + next].link = absolute;

		return next + 1;
	}

	if (!REGISTERINSTR(fxtr))
		setaddress(fxtr, ic, next++, extfp);

	while (NODELIMITER(fxtr))
		fxtr.pos++;
	
	if (fxtr.line.chars[fxtr.pos++] == '{') {
		WHITESPACES(fxtr)

		if (REGISTERINSTR(fxtr))
			return next;

		if (fxtr.line.chars[fxtr.pos] == '*') {
			setaddress(fxtr, ic, next, extfp);
			return next + 1;
		}
		
		IC[ic + next].bits = getnumber(&fxtr);
		IC[ic + next].link = absolute;

		return next + 1;
	}

	return next;
}

void setaddress(Fextr fxtr, int ic, int next, FILE *extfp) {
	String label;
	int i, starflag;

	starflag = 0;
	if (fxtr.line.chars[fxtr.pos] == '*') {
		starflag = 1;
		fxtr.pos++;

		WHITESPACES(fxtr)
	}

	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', '\n', '{', '}', ':', ',', AFTERLAST);

/*
	for (i = 0, label = initstring(); (NODELIMITER(line, linepos)); label.chars[i++] = line.chars[linepos++])
		;
	label.chars[i] = '\0';
*/

	for (i = 0; i < MAXLENGTH && !emptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
		;

	if (i < MAXLENGTH && !emptylabel(ltbl[i])) {
		IC[ic + next].bits = ltbl[i].address - ic * starflag;
		IC[ic + next].link = (!starflag) * relocatable + starflag * absolute;
	}
	else {
		for (i = 0; i < MAXLENGTH && *extltbl[i].name && strcmp(label.chars, extltbl[i].name); i++)
			;

		if (i < MAXLENGTH && !emptylabel(extltbl[i])) {
			IC[ic + next].bits = 0;
			IC[ic + next].link = external;
			fprintf(extfp, "%s\t%o\n", extltbl[i].name, ic + next);
		}
		else {
			printf("%s\n%s:%d:%d assembler: label \"%s\" not found\n", fxtr.line.chars, fxtr.filename.chars, fxtr.linec, fxtr.pos, label.chars);
			exit(1);
		}
	}
}

int getnumber(Fextr *fxtr) {
	String number = initstring();
	int i = 0;

	if ((*fxtr).line.chars[(*fxtr).pos] == '+' || (*fxtr).line.chars[(*fxtr).pos] == '-')
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	while (isdigit((*fxtr).line.chars[(*fxtr).pos]))
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	if (!(i && isdigit(number.chars[i - 1]))) {
		EXIT(*fxtr, "invalid number input")
	}

	return  atoi(number.chars);
}

void verifylabels() {
	int i, j;

	for (i = 0; !emptylabel(ltbl[i]) && i < MAXLENGTH; i++) {
		printf("%s,%d\n", ltbl[i].name, ltbl[i].address);

		for (j = i + 1; !emptylabel(ltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, ltbl[j].name)) {
				printf("assembler: %s: double declaration\n", ltbl[j].name);
				exit(1);
			}
				
		for (j = 0; !emptylabel(extltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, extltbl[j].name)) {
				printf("assembler: %s: double declaration\n", ltbl[j].name);
				exit(1);
			}
				
		for (j = 0; opcode[j]; j++)
			if (!strcmp(ltbl[i].name, opcode[j])) {
				printf("assembler: label: invalid name 1 %s\n", ltbl[j].name);
				exit(1);
			}

		if (REGISTER(ltbl[i].name)) {
			printf("assembler: label: invalid name 2 %s\n", ltbl[j].name);
			exit(1);
		}
	}

	for (i = 0; !emptylabel(extltbl[i]) && i < MAXLENGTH; i++) {
		printf("%s,%d\n", extltbl[i].name, extltbl[i].address);
		for (j = i + 1; !emptylabel(extltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(extltbl[i].name, extltbl[j].name)) {
				printf("assembler: %s: double declaration\n", ltbl[j].name);
				exit(1);
			}
				
		for (j = 0; opcode[j]; j++)
			if (!strcmp(extltbl[i].name, opcode[j])) {
				printf("assembler: label: invalid name 3 %s\n", ltbl[j].name);
				exit(1);
			}

		if (REGISTER(extltbl[i].name)) {
			printf("assembler: label: invalid name 4 %s\n", ltbl[j].name);
			exit(1);
		}
	}
}


