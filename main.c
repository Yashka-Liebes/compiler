/* the program is key sensitive, for example label K and k are two different labels */
/*  */



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

#define EXTERNAL 			0
#define DIRECTIONMAXSIZE	80
#define BEGINNING			100
#define MASK				1048575
#define ASCIIZERO 			48
#define MAXLENGTH			2000
#define LABELMAXSIZE		30
#define NUMBEROFREGS		8


#define WHITESPACES(f)	while(EMPTY(f)) (f).pos++;

#define REGISTER(f) (getreg(f) != -1)

/* macro for skipping white spaces */
#define EMPTY(f) ((f).line.chars[(f).pos] == ' ' || (f).line.chars[(f).pos] == '\t')

#define SETBITS(ic, val, pos) IC[ic].bits |= setbits(val, pos)


/* macro for pointing the place of an error in the assembly file */
#define MARK(f) {										\
	int k;												\
														\
	for (k = 0, printf("\t"); k < (f).pos; k++)			\
		if ((f).line.chars[k] == '\t')					\
			printf("\t");								\
		else											\
			printf(" ");								\
														\
	printf("^\n");										\
}


#define NODELIMITER(f) (				\
	   (f).line.chars[(f).pos] != ' '	\
	&& (f).line.chars[(f).pos] != '\t'	\
	&& (f).line.chars[(f).pos] != ':'	\
	&& (f).line.chars[(f).pos] != '\n'	\
	&& (f).line.chars[(f).pos] != '{'	\
	&& (f).line.chars[(f).pos] != '}'	\
	&& (f).line.chars[(f).pos] != ',')


#define EXPECTCHAR(f, c) {																						\
	WHITESPACES(f)																								\
																												\
	if ((f).line.chars[(f).pos++] != c)	{																		\
		(f).pos--;																								\
		if (c == '\n')																							\
			printf("%s:%d:%d: assembler: unexpected character -> %c\n\t%s",										\
						(f).filename.chars, (f).linec, (f).pos, (f).line.chars[(f).pos], (f).line.chars);		\
		else 																									\
			printf("%s:%d:%d: assembler: missing expected -> %c\n\t%s",											\
						(f).filename.chars, (f).linec, (f).pos, c, (f).line.chars);								\
																												\
		MARK(f)																									\
		CLOSEFILES																								\
		REMOVEFILES																								\
		compilenext();																							\
		exit(1);																								\
	}																											\
																												\
	WHITESPACES(f)																								\
}


#define ONEORZERO(f)														\
	if ((f).line.chars[(f).pos] != '0' && (f).line.chars[(f).pos] != '1')	\
		EXIT(f, "wrong type-dbl-comb assignment")


#define SKIPFIRSTLABEL(f) 											\
	for (f.pos = 0; NODELIMITER(f); f.pos++) 						\
		;															\
																	\
	(f).pos = (f).line.chars[(f).pos] == ':' ? (f).pos + 1 : 0;		\
																	\
	WHITESPACES(f)


#define EXIT(f, s) {																						\
	printf("%s:%d:%d: assembler: %s\n\t%s", (f).filename.chars, (f).linec, (f).pos, s, (f).line.chars);		\
	MARK(f)																									\
	CLOSEFILES																								\
	REMOVEFILES																								\
	compilenext();																							\
	exit(1);																								\
}


#define LABELEXIT(s, file, label) {			\
	printf("%s: %s: " #s, file, label);		\
	CLOSEFILES								\
	REMOVEFILES								\
	compilenext();							\
	exit(1);								\
}


#define OPENFILES {																		\
	String fname, obfname, entfname, extfname;											\
																						\
	fname = obfname = entfname = extfname = tostring(*assemblyfile);					\
																						\
	fp = fopen(strcat(fname.chars, ".as"), "r");										\
	obfp = 	fopen(strcat(obfname.chars, ".ob"), "w");									\
	entfp = fopen(strcat(entfname.chars, ".ent"), "w");									\
	extfp = fopen(strcat(extfname.chars, ".ext"), "w");									\
																						\
	if (!fp || !obfp || !entfp || !extfp) {												\
		if (!fp)																		\
			printf("assembler: could not open ro file %s\n", fname.chars);				\
																						\
		if (!obfp)																		\
			printf("assembler: could not open writable file %s\n", obfname.chars);		\
																						\
		if (!entfp)																		\
			printf("assembler: could not open writable file %s\n", entfname.chars);		\
																						\
		if (!extfp)																		\
			printf("assembler: could not open writable file %s\n", extfname.chars);		\
																						\
		REMOVEFILES																		\
		compilenext();																	\
	}																					\
}


#define CLOSEFILES	{	\
	fclose(fp);			\
	fclose(obfp);		\
	fclose(entfp);		\
	fclose(extfp);		\
}


#define REMOVEFILES	{											\
	String obfname, entfname, extfname;							\
	obfname = entfname = extfname = tostring(*assemblyfile);	\
	remove(strcat(obfname.chars, ".ob"));						\
	remove(strcat(entfname.chars, ".ent"));						\
	remove(strcat(extfname.chars, ".ext"));						\
}



/* function prototype */
Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark);
void compilenext();
void init();
void secondpass(int dc);
void getentry(Fextr fxtr, FILE *entfp);
void setaddress(Fextr fxtr, int ic, int next, FILE *extfp);
void verifylabels(Fextr fxtr);
int firstpass();
int getline(String *line, FILE *fp);
int getdirect(int *labelc, Fextr fxtr, int dc, int *extlabelc);
int getdata(int *labelc, Fextr fxtr, int dc);
int getstring(int *labelc, Fextr fxtr, int dc);
int getextern(int *labelc, Fextr fxtr, int dc, int *extlabelc);
int getinstruction(int *labelc, Fextr fxtr, int ic);
int getreg(Fextr fxtr);
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


const char *opcode[] = {"mov", "cmp", "add", "sub", "not", "clr", "lea", "inc",
								"dec", "jmp", "bne", "red", "prn", "jsr", "rts", "stop", 0};


const char *registers[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
								"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", 0};



Word IC[MAXLENGTH];
Label ltbl[MAXLENGTH], extltbl[MAXLENGTH];
FILE *fp, *obfp, *entfp, *extfp;
int DC[MAXLENGTH];
char **assemblyfile;	/* pointer to the current assembly file that was passed thru the command line */



int main(int argc, char *argv[]) {

	assemblyfile = argv;
	compilenext();

	return 0;
}

void compilenext() {
	assemblyfile++;
	if (*assemblyfile == NULL)
		return;

	init();
	OPENFILES;
	secondpass(firstpass());
	CLOSEFILES

	compilenext();
}

void init() {
	int i;
	
	for (i = 0; i < MAXLENGTH; i++) {
		IC[i] = initword();
		DC[i] = 0;
		ltbl[i] = extltbl[i] = initlabel();
	}
}

int firstpass() {
	Fextr fxtr;
	int ic, dc, labelc, extlabelc, i;
	
	fxtr = initfextr();
	fxtr.filename = tostring(*assemblyfile);

	for (extlabelc = labelc = ic = dc = 0; getline(&(fxtr.line), fp) != EOF; fxtr.line = initstring(), fxtr.linec++) {
		fxtr.pos =  0;

		WHITESPACES(fxtr)

		if (fxtr.line.chars[0] != ';' && fxtr.line.chars[fxtr.pos] != '\n') {
			SKIPFIRSTLABEL(fxtr)

			if (fxtr.line.chars[fxtr.pos] == '.')
				dc = getdirect(&labelc, fxtr, dc, &extlabelc);
			else
				ic = getinstruction(&labelc, fxtr, ic);
		}
	}

	for (i = 0; !isemptylabel(ltbl[i]); i++)
		ltbl[i].address += BEGINNING + ic * !(ltbl[i].mark);

	verifylabels(fxtr);

	return dc;
}

void secondpass(int dc) {
	Fextr fxtr;
	int ic, i, next;

	fxtr = initfextr();
	fxtr.filename = tostring(*assemblyfile);
	fseek(fp, 0L, SEEK_SET);

	for (ic = 0; getline(&fxtr.line, fp) != EOF; fxtr.line = initstring(), fxtr.linec++) {
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

		if (fxtr.line.chars[fxtr.pos] == '\n'){
			ic++;
			continue;
		}
		next = setoperand(fxtr, ic, 1, extfp);

		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		if (fxtr.line.chars[fxtr.pos] == '\n'){
			ic += next;
			continue;
		}

		fxtr.pos++;
		WHITESPACES(fxtr)
		
		ic += setoperand(fxtr, ic, next, extfp);
	}

	for (i = 0; i < ic; i++)
		fprintf(obfp, "%o\t%07o\t%c\n", i + BEGINNING, IC[i].bits, IC[i].link);

	for (i = 0; i < dc; i++)
		fprintf(obfp, "%o\t%07o\n", i + ic + BEGINNING, DC[i]);
}

/* get the direction from the assembly file: .data, .string or .extern */
int getdirect(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	String direct;
	int i, temp;

	fxtr.pos++;
	temp = gettoken(fxtr.line, fxtr.pos, &direct, ' ', '\t', AFTERLAST);

	for (i = 0; directionarr[i].func && strcmp(direct.chars, directionarr[i].type); i++)
		;

	if (!strcmp(direct.chars, ENTRY))
		return dc;

	if (!directionarr[i].func)
		EXIT(fxtr, "direction not found")

	fxtr.pos += temp;

	return (*(directionarr[i].func))(labelc, fxtr, dc, extlabelc);
}

/* function that deals with .data line in the assembly file */
int getdata(int *labelc, Fextr fxtr, int dc) {
	WHITESPACES(fxtr)

	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);
	if (!isemptylabel(ltbl[*labelc]))
		(*labelc)++;

	for (; fxtr.line.chars[fxtr.pos] != '\n' && fxtr.pos < DIRECTIONMAXSIZE; dc++) {
		DC[dc] = getnumber(&fxtr);
	
		dc++;
		WHITESPACES(fxtr)

		if (fxtr.line.chars[fxtr.pos] == '\n')
			return dc;
		else
			EXPECTCHAR(fxtr, ',')
	}

	EXPECTCHAR(fxtr, '\n')

	return dc;
}

/* function that deals with .string line in the assembly file */
int getstring(int *labelc, Fextr fxtr, int dc) {
	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);
	if (!isemptylabel(ltbl[*labelc]))
		(*labelc)++;

	WHITESPACES(fxtr)

	EXPECTCHAR(fxtr, '"')

	while(fxtr.line.chars[fxtr.pos] != '"' && fxtr.line.chars[fxtr.pos] != '\n' && fxtr.pos < DIRECTIONMAXSIZE)
		DC[dc++] = fxtr.line.chars[fxtr.pos++];
	DC[dc++] = 0;

	EXPECTCHAR(fxtr, '"')
	EXPECTCHAR(fxtr, '\n')

	return dc;
}

/* function that deals with .entry line in the assembly file */
void getentry(Fextr fxtr, FILE *entfp) {
	String label;
	int i, temp;

	fxtr.pos++;
	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', AFTERLAST);

	if (!strcmp(label.chars, ENTRY)) {
		WHITESPACES(fxtr)

		temp = gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', '\n', AFTERLAST);

		for (i = 0; !isemptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
			;
		
		if (!isemptylabel(ltbl[i]))
			fprintf(entfp, "%s\t%o\n", ltbl[i].name, ltbl[i].address);
		else
			EXIT(fxtr, "label not found")

		fxtr.pos += temp;
		EXPECTCHAR(fxtr, '\n')
	}
}

/* function that deals with .extern line in the assembly file */
int getextern(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	Label tmp;

	WHITESPACES(fxtr)

	tmp = getlabel(fxtr, 0, EXTERNAL, data);

	if (isemptylabel(tmp))
		EXIT(fxtr, "label exceeds maximum size or followed by junk")

	extltbl[(*extlabelc)++]  = tmp;

	return dc;
}

Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark) {
	Label lbl;
	String name;
	int must, tokenlength;

	if ((labelflag && mark == data) || !labelflag)
		must = 1;
	else
		must = 0;

	fxtr.pos *=  !labelflag;

	tokenlength = gettoken(fxtr.line, fxtr.pos, &name, ' ', '\t', '{', '}', ':', ',', '\n', AFTERLAST);
	tocharptr(name, lbl.name, NAMELENGTH);

	if (REGISTER(fxtr))
		EXIT(fxtr, "invalid label name")

	lbl.address = address;
	lbl.mark = mark;

	WHITESPACES(fxtr)

	if (must && isemptylabel(lbl))
		EXIT(fxtr, "label expected")

	if (lbl.name[0] && !isalpha(lbl.name[0]))
		EXIT(fxtr, "illegal first char in label")

	fxtr.pos += tokenlength;
	if ((labelflag && fxtr.line.chars[fxtr.pos] == ':') || (!labelflag && fxtr.line.chars[fxtr.pos] == '\n'))
		return lbl;

	return lbl = initlabel();
}

int getinstruction(int *labelc, Fextr fxtr, int ic) {
	String command;
	int i, numofwords = 0, tokenlength;

	ltbl[*labelc] = getlabel(fxtr, 1, ic, code);
	if (!isemptylabel(ltbl[*labelc]))
		(*labelc)++;

	tokenlength = gettoken(fxtr.line, fxtr.pos, &command, ' ', '\t', '/', '\n', AFTERLAST);

	for (i = 0; opcode[i] && strcmp(opcode[i], command.chars); i++)
		;

	if (!opcode[i])
		EXIT(fxtr, "command not found")

	fxtr.pos += tokenlength;
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
		numofwords = countwords(&fxtr, ic, DESTINATION);

	if (i < NOT || i == LEA) {
		numofwords = countwords(&fxtr, ic, ORIGIN);

		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		EXPECTCHAR(fxtr, ',')
		numofwords += countwords(&fxtr, ic, DESTINATION);
	}

	EXPECTCHAR(fxtr, '\n')
	IC[ic].link = absolute;

	return ic + 1 + numofwords;
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

int getreg(Fextr fxtr) {
	String reg;
	int i;

	gettoken(fxtr.line, fxtr.pos, &reg, ' ', '\t', '}', '\n', ',', ':', AFTERLAST);

	for (i = 0; registers[i] && strcmp(registers[i], reg.chars); i++)
			;

	if (!registers[i])
		return -1;

	return i % NUMBEROFREGS;
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

		if ((reg = getreg(*fxtr)) != -1) {
			SETBITS(ic, reg, wordpos);

			while (NODELIMITER(*fxtr))
				(*fxtr).pos++;

			EXPECTCHAR(*fxtr, '}')

			return 1;
		}

		if ((*fxtr).line.chars[(*fxtr).pos] == '*')
			(*fxtr).pos++;

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

	if ((reg = getreg(*fxtr)) != -1) {
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
		fxtr.pos++;
		IC[ic + next].bits = getnumber(&fxtr);
		IC[ic + next].link = absolute;

		return next + 1;
	}

	if (!REGISTER(fxtr))
		setaddress(fxtr, ic, next++, extfp);

	while ((NODELIMITER(fxtr)))
		fxtr.pos++;

	WHITESPACES(fxtr)

	if (fxtr.line.chars[fxtr.pos++] == '{') {
		if (REGISTER(fxtr))
			return next;

		if (fxtr.line.chars[fxtr.pos] == '*') {
			setaddress(fxtr, ic, next++, extfp);
			return next;
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

	gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', '\n', '{', '}', ':', ',', AFTERLAST);
	for (i = 0; i < MAXLENGTH && !isemptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
		;

	if (i < MAXLENGTH && !isemptylabel(ltbl[i])) {
		IC[ic + next].bits = ltbl[i].address - starflag * (ic + BEGINNING);
		IC[ic + next].link = (!starflag) * relocatable + starflag * absolute;
	}
	else {
		for (i = 0; i < MAXLENGTH && *extltbl[i].name && strcmp(label.chars, extltbl[i].name); i++)
			;

		if (i < MAXLENGTH && !isemptylabel(extltbl[i])) {
			IC[ic + next].bits = 0;
			IC[ic + next].link = external;
			fprintf(extfp, "%s\t%o\n", extltbl[i].name, BEGINNING + ic + next);
		}
		else
			EXIT(fxtr, "label not found")
	}
}

int getnumber(Fextr *fxtr) {
	String number;
	int num, i = 0;

	if ((*fxtr).line.chars[(*fxtr).pos] == '+' || (*fxtr).line.chars[(*fxtr).pos] == '-')
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	while (isdigit((*fxtr).line.chars[(*fxtr).pos]))
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	if (!(i && isdigit(number.chars[i - 1])))
		EXIT(*fxtr, "invalid number input")

	number.chars[i] = '\0';
	num = atoi(number.chars);

	if (fabs(num) > MASK / 2){
		(*fxtr).pos -= i;
		EXIT(*fxtr, "usage - number out of range")
	}
	
	return  num & MASK;
}

void verifylabels(Fextr fxtr) {
	int i, j;

	for (i = 0; !isemptylabel(ltbl[i]) && i < MAXLENGTH; i++) {
		for (j = i + 1; !isemptylabel(ltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, ltbl[j].name))
				LABELEXIT("double declaration\n", fxtr.filename.chars, ltbl[i].name)
				
		for (j = 0; !isemptylabel(extltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, extltbl[j].name))
				LABELEXIT("double declaration\n", fxtr.filename.chars, ltbl[i].name)
				
		for (j = 0; opcode[j]; j++)
			if (!strcmp(ltbl[i].name, opcode[j]))
				LABELEXIT("invalid name\n", fxtr.filename.chars, ltbl[i].name)
	}

	for (i = 0; !isemptylabel(extltbl[i]) && i < MAXLENGTH; i++)
		for (j = 0; opcode[j]; j++)
			if (!strcmp(extltbl[i].name, opcode[j]))
				LABELEXIT("invalid name\n", fxtr.filename.chars, extltbl[i].name)
}
