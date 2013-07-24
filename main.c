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

#define NOMOREFILES -1
#define NOTIMPORTANT 0
#define MAXFILESAMOUNT 200

#define BEGINNING		100
#define MASK			524288
#define ASCIIZERO 		48
#define NFILES 			100
#define MAXLENGTH		2000
#define LABELMAXSIZE	30
#define OPCODESIZE 5


#define REGISTERINSTR(f) (getreg(&(f).line.chars[(f).pos]) != -1)

#define REGISTER(c) (getreg(c) != -1)

#define EMPTY(f)	((f).line.chars[(f).pos] == ' ' || (f).line.chars[(f).pos] == '\t')

#define SETBITS(ic, val, pos) IC[ic].bits |= setbits(val, pos)

#define CHECKERR(re) if (err) return re;


#define NODELIMITER(f) (					\
			(f).line.chars[(f).pos] != ' '	\
		 && (f).line.chars[(f).pos] != '\t'	\
		 && (f).line.chars[(f).pos] != ':'	\
		 && (f).line.chars[(f).pos] != '\n'	\
		 && (f).line.chars[(f).pos] != '{'	\
		 && (f).line.chars[(f).pos] != '}'	\
		 && (f).line.chars[(f).pos] != ',')


#define WHITESPACES(f)			\
			while (EMPTY(f))	\
				(f).pos++;


#define EXPECTCHAR(f, c, re)						\
			WHITESPACES(f)							\
													\
			if ((f).line.chars[(f).pos++] != c)	{	\
				printf("%s:%d:%d assembler: missing %c\n", (f).filename.chars, (f).linec, (f).pos, c);	\
				ERROR(re)							\
			}										\
													\
			WHITESPACES(f)


#define ONEORZERO(f)																\
			if ((f).line.chars[(f).pos] != '0' && (f).line.chars[(f).pos] != '1')	\
				EXIT(f, "wrong type-dbl-comb assignment", 0)


#define SKIPFIRSTLABEL(f) 													\
			for (f.pos = 0; NODELIMITER(f); f.pos++) 						\
				;															\
																			\
			(f).pos = (f).line.chars[(f).pos] == ':' ? (f).pos + 1 : 0;		\
																			\
			WHITESPACES(f)


#define ERROR(re) {		\
			err = 1;	\
			return re;	\
		}


#define EXIT(f, s, returntype) {																	\
			printf("%s:%d:%d assembler: %s\n", (f).filename.chars, (f).linec, (f).pos, s);	\
			ERROR(returntype)																		\
		}



/* function prototype */
Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark);
void compile();
void init();
void secondpass(FILE *asf, String fname, int dc);
void getentry(Fextr fxtr, FILE *entfp);
void setaddress(Fextr fxtr, int ic, int next, FILE *extfp);
void verifylabels();
int firstpass(FILE *asf, String fname);
int getline(String *line, FILE *fp);
int getdirect(int *labelc, Fextr fxtr, int dc, int *extlabelc);
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
String filenames[MAXFILESAMOUNT];
int DC[MAXLENGTH], filecounter;
char err;



int main(int argc, char *argv[]) {
	filecounter = argc - 1;

	for(argc-- ;argc; argc--)
		filenames[argc - 1] = tostring(argv[argc]);

	compile();

	return 0;
}

void compile() {
	FILE *fp;
	String temp;

	filecounter--;
	if (filecounter == NOMOREFILES)
		exit(0);

	init();
	temp = filenames[filecounter];

	fp = fopen(strcat(temp.chars, ".as"), "r");
	if (!fp) {
		printf("assembler: could not open ro file %s\n", temp.chars);
		return;
	}

	secondpass(fp, filenames[filecounter], firstpass(fp, filenames[filecounter]));

	compile();
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
	

	fxtr = initfextr();
	fxtr.filename = fname;


	for (extlabelc = labelc = ic = dc = 0; getline(&(fxtr.line), asf) != EOF; fxtr.line = initstring(), fxtr.linec++) {

		fxtr.pos =  0;
		WHITESPACES(fxtr)

		if (fxtr.line.chars[0] != ';' && fxtr.line.chars[fxtr.pos] != '\n') {
			SKIPFIRSTLABEL(fxtr)

			if (fxtr.line.chars[fxtr.pos] == '.')
				dc = getdirect(&labelc, fxtr, dc, &extlabelc);
			else
				ic = getinstruction(&labelc, fxtr, ic);

			CHECKERR(0)
		}
	}

	for (i = 0; !emptylabel(ltbl[i]); i++)
		ltbl[i].address += BEGINNING + ic * !(ltbl[i].mark);

	verifylabels();
	CHECKERR(0)

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
		return;
	}

	if (!entfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname.chars, "ent"));
		return;
	}

	if (!extfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname.chars, "ext"));
		return;
	}

	fseek(asf, 0L, SEEK_SET);

	for (ic = 0; getline(&fxtr.line, asf) != EOF; fxtr.line = initstring(), fxtr.linec++) {
		SKIPFIRSTLABEL(fxtr)
		
		if (fxtr.line.chars[fxtr.pos] == '\n' || fxtr.line.chars[0] == ';')
			continue;
		
		if (fxtr.line.chars[fxtr.pos] == '.') {
			getentry(fxtr, entfp);
			CHECKERR(;)
			continue;
		}
		
		while (fxtr.line.chars[fxtr.pos++] != '/')
			;

		while (!isalpha(fxtr.line.chars[fxtr.pos]) && fxtr.line.chars[fxtr.pos] != '#' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		if (fxtr.line.chars[fxtr.pos] == '\n')
			continue;

		next = setoperand(fxtr, ic, 1, extfp);
		CHECKERR(;)

		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		if (fxtr.line.chars[fxtr.pos] == '\n'){
			ic += next;
			continue;
		}

		fxtr.pos++;
		WHITESPACES(fxtr)
		
		next = setoperand(fxtr, ic, next, extfp);
		CHECKERR(;)

		ic += next;
	}

	for (i = 0; i < ic; i++)
		fprintf(obfp, "text %o\t%07o\t%c\n", i + BEGINNING, IC[i].bits, IC[i].link);

	for (i = 0; i < dc; i++)
		fprintf(obfp, "text %o\t%07o\n", i + ic + BEGINNING, DC[i]);
}

int getdirect(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	int i;
	String direct;

	fxtr.pos++;
	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &direct, ' ', '\t', AFTERLAST);
	for (i = 0; directionarr[i].func && strcmp(direct.chars, directionarr[i].type); i++)
		;

	if (!strcmp(direct.chars, ENTRY))
		return dc;

	if (!directionarr[i].func) {
		printf("(%d:%d) assembler: ~%s~ function not found\n", fxtr.linec, fxtr.pos, direct.chars);
		ERROR(0);
	}

	return (*(directionarr[i].func))(labelc, fxtr, dc, extlabelc);
}

int getdata(int *labelc, Fextr fxtr, int dc) {
	WHITESPACES(fxtr)

	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);
	CHECKERR(0)
	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	for (; fxtr.line.chars[fxtr.pos] != '\n'; dc++) {
		DC[dc] = getnumber(&fxtr);
		CHECKERR(0)

		if (fabs(DC[dc]) > MASK / 2) {
			EXIT(fxtr, "usage - number out of range", 0)
		}
		dc++;

		WHITESPACES(fxtr)

		if (fxtr.line.chars[fxtr.pos] == '\n')
			return dc;
		else {
			EXPECTCHAR(fxtr, ',', 0)
		}
	}

	return dc;
}

int getstring(int *labelc, Fextr fxtr, int dc) {
	ltbl[*labelc] = getlabel(fxtr, 1, dc, data);
	CHECKERR(0)
	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	WHITESPACES(fxtr)

	EXPECTCHAR(fxtr, '"', 0)

	while(fxtr.line.chars[fxtr.pos] != '"' && fxtr.line.chars[fxtr.pos] != '\n')
		DC[dc++] = fxtr.line.chars[fxtr.pos++];
	DC[dc++] = 0;

	EXPECTCHAR(fxtr, '"', 0)

	return dc;
}

void getentry(Fextr fxtr, FILE *entfp) {
	int i;
	String label;

	fxtr.pos++;
	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', AFTERLAST);

	if (!strcmp(label.chars, ENTRY)) {
		WHITESPACES(fxtr)

		fxtr.pos += gettoken(fxtr.line, fxtr.pos, &label, ' ', '\t', '\n', AFTERLAST);
		for (i = 0; !emptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
			;
		
		if (!emptylabel(ltbl[i]))
			fprintf(entfp, "%s\t%o\n", ltbl[i].name, ltbl[i].address);
		else {
			printf("(%d:%d) assembler: \"%s\": label not found\n", fxtr.linec, fxtr.pos, label.chars);
			ERROR(;)
		}

		EXPECTCHAR(fxtr, '\n', ;)
	}
}

int getextern(int *labelc, Fextr fxtr, int dc, int *extlabelc) {
	WHITESPACES(fxtr)

	extltbl[(*extlabelc)] = getlabel(fxtr, 0, NOTIMPORTANT, data);
	CHECKERR(0)

	if (emptylabel(extltbl[(*extlabelc)])) {
		EXIT(fxtr, "label exceeds maximum size or followed by junk", 0)
	}
	(*extlabelc)++;

	return dc;
}

Label getlabel(Fextr fxtr, int labelflag, int address, Mark mark) {
	Label lbl;
	int must;
	String name;

	CHECKERR(lbl)

	if ((labelflag && mark == data) || !labelflag)
		must = 1;
	else
		must = 0;

	fxtr.pos *=  !labelflag;

	fxtr.pos += gettoken(fxtr.line, fxtr.pos, &name, ' ', '\t', '{', '}', ':', ',', '\n', AFTERLAST);
	tocharptr(name, lbl.name, NAMELENGTH);
	lbl.address = address;
	lbl.mark = mark;

	WHITESPACES(fxtr)

	if (must && emptylabel(lbl))
		EXIT(fxtr, "label expected", lbl)

	if (lbl.name[0] && !isalpha(lbl.name[0]))
		EXIT(fxtr, "elegal first char in label", lbl)


	if ((labelflag && fxtr.line.chars[fxtr.pos] == ':') || (!labelflag && fxtr.line.chars[fxtr.pos] == '\n'))
		return lbl;

	return lbl = initlabel();
}

int getinstruction(int *labelc, Fextr fxtr, int ic) {
	int i, l;
	char command[OPCODESIZE];

	ltbl[*labelc] = getlabel(fxtr, 1, ic, code);
	CHECKERR(0)
	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	for (l = i = 0; fxtr.line.chars[fxtr.pos] != '/' && i < OPCODESIZE - 1 && !EMPTY(fxtr); command[i++] = fxtr.line.chars[fxtr.pos++])
		;
	command[i] = '\0';

	for (i = 0; opcode[i] && strcmp(opcode[i], command); i++)
		;

	if (!opcode[i]) {
		printf("%s:%d:%d assembler: command \"%s\" not found\n", fxtr.filename.chars, fxtr.linec, fxtr.pos, command);
		ERROR(0);
	}

	SETBITS(ic, i, OPCODE);
	EXPECTCHAR(fxtr, '/', 0)

	ONEORZERO(fxtr)
	SETBITS(ic, fxtr.line.chars[fxtr.pos] - ASCIIZERO, TYPE);

	if (fxtr.line.chars[fxtr.pos++] - ASCIIZERO) {
		EXPECTCHAR(fxtr, '/', 0)
		SETBITS(ic, getcomb(&fxtr), COMB);
	}

	EXPECTCHAR(fxtr, ',', 0)
	SETBITS(ic, fxtr.line.chars[fxtr.pos++] - ASCIIZERO, DBL);

	WHITESPACES(fxtr)

	if (i < RTS && i != LEA && i >= NOT)
		l = countwords(&fxtr, ic, DESTINATION);
	CHECKERR(0)

	if (i < NOT || i == LEA) {
		l = countwords(&fxtr, ic, ORIGIN);
		CHECKERR(0)
		while (fxtr.line.chars[fxtr.pos] != ',' && fxtr.line.chars[fxtr.pos] != '\n')
			fxtr.pos++;

		EXPECTCHAR(fxtr, ',', 0)
		l += countwords(&fxtr, ic, DESTINATION);
		CHECKERR(0)
	}

	EXPECTCHAR(fxtr, '\n', 0)
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
	
	EXPECTCHAR(*fxtr, '/', 0)
	
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

		if ((reg = getreg(&(*fxtr).line.chars[(*fxtr).pos])) != -1) {
			SETBITS(ic, reg, wordpos);

			while (NODELIMITER(*fxtr))
				(*fxtr).pos++;
			EXPECTCHAR(*fxtr, '}', 0)

			return 1;
		}

		if ((*fxtr).line.chars[(*fxtr).pos] == '*')
			(*fxtr).pos++;

		while (NODELIMITER(*fxtr))
			(*fxtr).pos++;
		EXPECTCHAR(*fxtr, '}', 0)

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
			EXIT(*fxtr, "destination operand", 0)

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
	CHECKERR(0)

	if (fxtr.line.chars[fxtr.pos] == '#') {

		IC[ic + next].bits = getnumber(&fxtr);
		IC[ic + next].link = absolute;

		return next + 1;
	}

	if (!REGISTERINSTR(fxtr))
		setaddress(fxtr, ic, next++, extfp);

	while ((NODELIMITER(fxtr)))
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

	for (i = 0; i < MAXLENGTH && !emptylabel(ltbl[i]) && strcmp(label.chars, ltbl[i].name); i++)
		;

	if (i < MAXLENGTH && !emptylabel(ltbl[i])) {
		IC[ic + next].bits = ltbl[i].address - starflag * (ic + BEGINNING);
		IC[ic + next].link = (!starflag) * relocatable + starflag * absolute;
	}
	else {
		for (i = 0; i < MAXLENGTH && *extltbl[i].name && strcmp(label.chars, extltbl[i].name); i++)
			;

		if (i < MAXLENGTH && !emptylabel(extltbl[i])) {
			IC[ic + next].bits = 0;
			IC[ic + next].link = external;
			fprintf(extfp, "%s\t%o\n", extltbl[i].name, BEGINNING + ic + next);
		}
		else {
			printf("%s:%d:%d assembler: label \"%s\" not found\n", fxtr.filename.chars, fxtr.linec, fxtr.pos, label.chars);
			ERROR(;)
		}
	}
}

int getnumber(Fextr *fxtr) {
	String number;
	int i = 0;

	if ((*fxtr).line.chars[(*fxtr).pos] == '+' || (*fxtr).line.chars[(*fxtr).pos] == '-')
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	while (isdigit((*fxtr).line.chars[(*fxtr).pos]))
		number.chars[i++] = (*fxtr).line.chars[(*fxtr).pos++];

	if (!(i && isdigit(number.chars[i - 1]))) {
		EXIT(*fxtr, "invalid number input", 0)
	}

	number.chars[i] = '\0';

	return  atoi(number.chars);
}

void verifylabels() {
	int i, j;

	for (i = 0; !emptylabel(ltbl[i]) && i < MAXLENGTH; i++) {
		for (j = i + 1; !emptylabel(ltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, ltbl[j].name)) {
				printf("assembler: %s: double declaration of\n", ltbl[j].name);
				ERROR(;)
			}
				
		for (j = 0; !emptylabel(extltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(ltbl[i].name, extltbl[j].name)) {
				printf("assembler: %s: double declaration of\n", ltbl[j].name);
				ERROR(;)
			}
				
		for (j = 0; opcode[j]; j++)
			if (!strcmp(ltbl[i].name, opcode[j])) {
				printf("assembler: label: invalid name 1 %s\n", ltbl[j].name);
				ERROR(;)
			}

		if (REGISTER(ltbl[i].name)) {
			printf("assembler: label: invalid name 2 %s\n", ltbl[j].name);
			ERROR(;)
		}
	}

	for (i = 0; !emptylabel(extltbl[i]) && i < MAXLENGTH; i++) {
		for (j = 0; opcode[j]; j++)
			if (!strcmp(extltbl[i].name, opcode[j])) {
				printf("assembler: label: invalid name 3 %s\n", ltbl[j].name);
				ERROR(;)
			}

		if (REGISTER(extltbl[i].name)) {
			printf("assembler: label: invalid name 4 %s\n", ltbl[j].name);
			ERROR(;)
		}
	}
}


