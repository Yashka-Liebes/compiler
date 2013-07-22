#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "word.h"
#include "label.h"
#include "string.h"



/************************
 *
 *
 *
 *
 * 
 ************************/



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

#define DATA	"data"
#define STRING	"string"
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


#define REGISTERINSTR(s, k) getreg(&s.chars[k]) != -1

#define REGISTER(c) (getreg(c) != -1)

#define EMPTY(s, k)	(s.chars[k] == ' ' || s.chars[k] == '\t')


#define NODELIMITER(s, k) 					\
			(s.chars[k]) != ' '				\
				&& (s.chars[k]) != '\t'		\
				&& (s.chars[k]) != ':'		\
				&& (s.chars[k]) != '\n'		\
				&& (s.chars[k]) != '{'		\
				&& (s.chars[k]) != '}'		\
				&& (s.chars[k]) != ','


#define WHITESPACES(s, k)			\
			while (EMPTY(s, k))		\
				(k)++;


#define EXPECTCHAR(s, k, c)								\
			WHITESPACES(s,k)							\
														\
			if ((s).chars[(k)++] != c)	{				\
				printf("assembler: missing %c\n", c);	\
				exit(1);								\
			}											\
														\
			WHITESPACES(s,k)


#define ONEORZERO(s,k)													\
			if (s.chars[k] != '0' && s.chars[k] != '1') {				\
				printf("assembler: wrong type-dbl-comb assignment\n");	\
				exit(1);												\
			}


#define SETBITS(ic, val, pos)					\
			IC[ic].bits |= setbits(val, pos)


#define SKIPFIRSTLABEL(s, k) 				\
	for (k = 0; NODELIMITER(s, k); k++) 	\
		;									\
		k = s.chars[k] == ':' ? k + 1 : 0;	\
											\
		WHITESPACES(s, k)



/* function prototype */
void compile(FILE *assemblyfile, char *fname);
void init();
void secondpass(FILE *asf, char *fname, int dc);
void getentry(String line, int linepos, FILE *entfp);
void setaddress(String line, int linepos, int ic, int next, FILE *extfp);
void verifylabels();
int firstpass(FILE *asf);
int getline(String *line, FILE *fp);
int getdirect(int *labelc, int linepos, String line, int dc, int *extlabelc);
int getdata(int *labelc, int linepos, String line, int dc);
int getstring(int *labelc, int linepos, String line, int dc);
int getextern(int *labelc, int linepos, String line, int dc, int *extlabelc);
int getinstruction(int *labelc, String line, int linepos, int ic);
int getreg(char *line);
int getcomb(String s, int *k);
int countlabelwords(int ic, String line, int *linepos, int wordpos);
int countwords(String line, int *linepos, int ic, int wordpos);
int setoperand(String line, int linepos, int ic, int next, FILE *extfp);
int getnumber(String line, int *linepos);
Label getlabel(String line, int linepos, int address, Mark mark);



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
/*int labelflag;*/



int main(int argc, char *argv[]) {
	FILE *fp;
	String fname;

	while (--argc) {
		fname = tostring(argv[argc]);

		fp = fopen(strcat(fname.chars, ".as"), "r");
		if (!fp) {
			printf("assembler: could not open ro file %s\n", argv[argc]);
			exit(1);
		}

		compile(fp, argv[argc]);
	}

	return 1;
}

void compile(FILE *asf, char *fname) {
	init();
	
	secondpass(asf, fname, firstpass(asf));
}

void init() {
	int i;
	
	for (i = 0; i < MAXLENGTH; i++) {
		IC[i] = initword();
		DC[i] = 0;
		ltbl[i] = extltbl[i] = initlabel();
	}
}

int firstpass(FILE *asf) {
	String line;
	int ic, dc, labelc, extlabelc, linepos, i;
	
	for (extlabelc = labelc = ic = dc = 0, line = initstring(); getline(&line, asf) != EOF; line = initstring()) {

		linepos =  0;
		WHITESPACES(line, linepos)

		if (line.chars[0] != ';' && line.chars[linepos] != '\n') {
			SKIPFIRSTLABEL(line, linepos)

			if (line.chars[linepos] == '.')
				dc = getdirect(&labelc, ++linepos, line, dc, &extlabelc);
			else
				ic = getinstruction(&labelc, line, linepos, ic);
		}
	}

	for (i = 0; !emptylabel(ltbl[i]); i++)
		if (ltbl[i].mark == data)
			ltbl[i].address += ic;

	verifylabels();

	return dc;
}

void secondpass(FILE *asf, char *fname, int dc) {
	String obfname, entfname, extfname, line;
	FILE *obfp, *entfp, *extfp;
	int ic, i, linepos, next;

	obfname = entfname = extfname = tostring(fname);

	obfp = 	fopen(strcat(obfname.chars, ".ob"), "w");
	entfp = fopen(strcat(entfname.chars, ".ent"), "w");
	extfp = fopen(strcat(extfname.chars, ".ext"), "w");

	if (!obfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname, "ob"));
		exit(1);
	}

	if (!entfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname, "ent"));
		exit(1);
	}

	if (!extfp) {
		printf("assembler: could not open writable file %s\n", strcat(fname, "ext"));
		exit(1);
	}

	fseek(asf, 0L, SEEK_SET);

	for (ic = 0, line = initstring(); getline(&line, asf) != EOF; line = initstring()) {
		SKIPFIRSTLABEL(line, linepos)
		
		if (line.chars[linepos] == '\n' || line.chars[0] == ';')
			continue;
		
		if (line.chars[linepos++] == '.') {
			getentry(line, linepos, entfp);
			continue;
		}
		
		while (line.chars[linepos++] != '/')
			;

		while (!isalpha(line.chars[linepos]) && line.chars[linepos] != '#' && line.chars[linepos] != '\n')
			linepos++;

		if (line.chars[linepos] == '\n')
			continue;

		next = setoperand(line, linepos, ic, 1, extfp);

		while (line.chars[linepos] != ',' && line.chars[linepos] != '\n')
			linepos++;

		if (line.chars[linepos] == '\n')
			continue;

		linepos++;
		WHITESPACES(line, linepos)
		
		next = setoperand(line, linepos, ic, next, extfp);
		ic += next;
	}

	for (i = 0; i < ic; i++)
		fprintf(obfp, "%o\t%o\t%c\n", i + BEGINNING, IC[i].bits, IC[i].link);
	
	ic += BEGINNING;	
	for (i = 0; i < dc; i++)
		fprintf(obfp, "%o\t%o\n", i + ic, DC[i]);
}

int getdirect(int *labelc, int linepos, String line, int dc, int *extlabelc) {
	int i;
	String direct;


/*
	for(i = 0, direct = initstring(), linepos; !EMPTY(line, linepos); direct.chars[i++] = line.chars[linepos++])
		;
	direct.chars[i] = '\0';
*/

	linepos += gettocen(line, linepos, &direct, ' ', '\t', AFTERLAST);


	for (i = 0; directionarr[i].func && strcmp(direct.chars, directionarr[i].type); i++)
		;


						/*printf("direct: ~~%s~~\n00\n", direct.chars);*/


	if (!strcmp(direct.chars, ENTRY))
		return dc;

	if (!directionarr[i].func) {
		printf("assembler: ~~%s~~: function not found\n", direct.chars);
		exit(1);
	}

	return (*(directionarr[i].func))(labelc, linepos, line, dc, extlabelc);
}

int getdata(int *labelc, int linepos, String line, int dc) {
	WHITESPACES(line, linepos)

	ltbl[*labelc] = getlabel(line, 0, dc++, data);
	if (!emptylabel(ltbl[*labelc]))
		(*labelc)++;

	for (; line.chars[linepos] != '\n'; dc++) {
		DC[dc] = getnumber(line, &linepos);

		if (fabs(DC[dc]) > MASK / 2) {
			printf("assembler: usage - number out of range\n");
			exit(1);
		}

		WHITESPACES(line, linepos)

		if (line.chars[linepos] == '\n')
			continue;

		if (line.chars[linepos] != '\n') {
			EXPECTCHAR(line, linepos, ',')
		}
	}

	return dc;
}

int getstring(int *labelc, int linepos, String line, int dc) {
	int c;

	ltbl[*labelc] = getlabel(line, 0, dc++, data);
		if (!emptylabel(ltbl[*labelc]))
			(*labelc)++;

	WHITESPACES(line, linepos)

	if (line.chars[linepos++] != '"') {
		printf("assembler: .string: missing \"\n");
		exit(1);
	}

	while((c = line.chars[linepos++]) != '"')
		DC[dc++] = c;
	DC[dc++] = 0;

	return dc;
}

void getentry(String line, int linepos, FILE *entfp) {
	int i;
	String label;


	linepos += gettocen(line, linepos, &label, ' ', '\t', AFTERLAST);

/*
	for (i = 0, label = initstring(); line.chars[linepos] != ' ' && line.chars[linepos] != '\t'; label.chars[i++] = line.chars[linepos++])
		;
	label.chars[i] = '\0';
*/


	if (!strcmp(label.chars, ENTRY)) {
		WHITESPACES(line, linepos)

		linepos += gettocen(line, linepos, &label, ' ', '\t', '\n', AFTERLAST);

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
			printf("assembler: %s: label not found\n", label.chars);
			exit(1);
		}

		EXPECTCHAR(line, linepos, '\n')
	}
}

int getextern(int *labelc, int linepos, String line, int dc, int *extlabelc) {

	WHITESPACES(line, linepos)

	extltbl[(*extlabelc)] = getlabel(line, linepos, 0, data);

	if (emptylabel(extltbl[(*extlabelc)])) {
		printf("assembler: label exceeds maximum size or followed by junk\n");
		exit(1);
	}

	return dc;
}

Label getlabel(String line, int linepos, int address, Mark mark) {
	String name;
	Label lbl;
	int internal;

	internal = linepos == 0 ? 1 : 0;

	linepos += gettocen(line, linepos, &name, ' ', '\t', '{', '}', ':', ',', '\n', AFTERLAST);
	tocharptr(name, lbl.name, NAMELENGTH);	/* todo not the best line here */


/*
	for (i = 0, lbl = initlabel(); linepos < LABELMAXSIZE && NODELIMITER(line, linepos); lbl.name[i++] = line.chars[linepos++])
		;
	lbl.name[i] = '\0';
*/

	lbl.address = address;
	lbl.mark = mark;

	WHITESPACES(line, linepos)

	if (!isalpha(lbl.name[0])) {
		printf("assembler: label: first char\n");
		exit(1);
	}

/*
	if ((internal && line.chars[linepos] == ':') || (!internal && line.chars[linepos] == '\n'))
		return lbl;
*/

	return (internal && line.chars[linepos] == ':') || (!internal && line.chars[linepos] == '\n') ? lbl : EMPTYLABEL;
}

int getinstruction(int *labelc, String line, int linepos, int ic) {
	#define OPCODESIZE 5

	int i, l;
	char command[OPCODESIZE];


	ltbl[*labelc] = getlabel(line, 0, ic, code);
		if (!emptylabel(ltbl[*labelc]))
			(*labelc)++;

	for (i = 0; line.chars[linepos] != '/' && i < OPCODESIZE - 1 && !EMPTY(line, linepos); command[i++] = line.chars[linepos++])
		;
	command[i] = '\0';

	for (i = 0; opcode[i] && strcmp(opcode[i], command); i++)
		;

	if (!opcode[i]) {
		printf("assembler: %s: command not found\n", command);
		exit(1);
	}

	SETBITS(ic, i, OPCODE);
	EXPECTCHAR(line, linepos, '/')

	ONEORZERO(line,linepos)
	SETBITS(ic, line.chars[linepos] - ASCIIZERO, TYPE);

	if (line.chars[linepos++] - ASCIIZERO) {
		EXPECTCHAR(line, linepos, '/')
		SETBITS(ic, getcomb(line, &linepos), COMB);
	}

	EXPECTCHAR(line, linepos, ',')
	SETBITS(ic, line.chars[linepos++] - ASCIIZERO, DBL);

	WHITESPACES(line, linepos)

	if (i < RTS && i != LEA && i >= NOT)
		l = countwords(line, &linepos, ic, DESTINATION);

	if (i < NOT || i == LEA) {
		l = countwords(line, &linepos, ic, ORIGIN);

		while (line.chars[linepos] != ',' && line.chars[linepos] != '\n')
			linepos++;

		EXPECTCHAR(line, linepos, ',')
		l += countwords(line, &linepos, ic, DESTINATION);
	}

	EXPECTCHAR(line, linepos, '\n')
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

int getreg(char *line) {
	int i;

	if (*line == 'r')
		i = *(++line) - ASCIIZERO;
	
	line++;
	if (i >= 0 && i < 8 && (*line == '\t' || *line == ' ' || *line == '}' || *line == '\0'))
		return i;

	return -1;
}

int getcomb(String line, int *posptr) {
	int i;

	ONEORZERO(line, *posptr)
	i = line.chars[(*posptr)++] - ASCIIZERO;
	i *= 2;
	
	EXPECTCHAR(line, (*posptr), '/')
	
	ONEORZERO(line,*posptr)
	i += line.chars[(*posptr)++] - ASCIIZERO;

	return i;
}

int countlabelwords(int ic, String line, int *linepos, int wordpos) {
	int reg;

	while (NODELIMITER(line, *linepos))
		(*linepos)++;

	WHITESPACES(line, *linepos)

	if (line.chars[*linepos] == '{') {
		SETBITS(ic, INDEXADDR, wordpos + ADDR);

		(*linepos)++;
		WHITESPACES(line, *linepos)

		if (line.chars[*linepos] == '*') {
			(*linepos)++;
			WHITESPACES(line, *linepos)

			while (NODELIMITER(line, *linepos))
				(*linepos)++;
			EXPECTCHAR(line, *linepos, '}')

			return 2;
		}

		if ((reg = getreg(&line.chars[*linepos])) != -1)
			SETBITS(ic, reg, wordpos);

		while (NODELIMITER(line, *linepos))
			(*linepos)++;
		EXPECTCHAR(line, *linepos, '}')
	}
	else
		SETBITS(ic, DIRECTADDR, wordpos + ADDR);

	return 1;
}

int countwords(String line, int *linepos, int ic, int wordpos) {
	int reg;

	if (line.chars[*linepos] == '#') {
		SETBITS(ic, IMADDR, wordpos + ADDR);

		if (wordpos == DESTINATION) {
			printf("assembler: destination operand\n");
			exit(1);
		}

		while (NODELIMITER(line, *linepos))
			(*linepos)++;

		WHITESPACES(line, *linepos)

		return 1;
	}
	
	if ((reg = getreg(&line.chars[*linepos])) != -1) {
		SETBITS(ic, reg, wordpos);
		SETBITS(ic, REGADDR, wordpos + ADDR);
		
		while (NODELIMITER(line, *linepos))
			(*linepos)++;

		WHITESPACES(line, *linepos)

		return 0;
	}
	
	return countlabelwords(ic, line, linepos, wordpos);
}

int setoperand(String line, int linepos, int ic, int next, FILE *extfp) {
	if (line.chars[linepos] == '#') {

		IC[++ic].bits = getnumber(line, &linepos);
		IC[ic].link = absolute;

		return next + 1;
	}

	if (!REGISTERINSTR(line, linepos))
		setaddress(line, linepos, ic, next++, extfp);

	while ((NODELIMITER(line, linepos)))
		linepos++;
	
	if (line.chars[linepos++] == '{') {
		WHITESPACES(line,linepos)

		if (REGISTERINSTR(line, linepos))
			return next;

		if (line.chars[linepos] == '*') {
			setaddress(line, linepos, ic, next, extfp);
			return next + 1;
		}
		
		IC[ic + next].bits = getnumber(line, &linepos);
		IC[ic + next].link = absolute;

		return next + 1;
	}

	return next;
}

void setaddress(String line, int linepos, int ic, int next, FILE *extfp) {
	String label;
	int i, starflag;

	starflag = 0;
	if (line.chars[linepos] == '*') {
		starflag = 1;
		linepos++;

		WHITESPACES(line,linepos)
	}

	linepos += gettocen(line, linepos, &label, ' ', '\t', '{', '}', ':', ',', '\n', AFTERLAST);

/*
	for (i = 0, label = initstring(); NODELIMITER(line, linepos); label.chars[i++] = line.chars[linepos++])
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

		if (i < MAXLENGTH && extltbl[i].name) {
			IC[ic + next].bits = 0;
			IC[ic + next].link = external;
			fprintf(extfp, "%s\t%o\n", extltbl[i].name, ic + next);
		}
		else {
			printf("assembler: %s: label not found\n", label.chars);
			exit(1);
		}
	}
}

int getnumber(String line, int *linepos) {
	String number;
	int i = 0;

	if (line.chars[*linepos] == '+' || line.chars[*linepos] == '-')
		number.chars[i++] = line.chars[(*linepos)++];

	while (isdigit(line.chars[*linepos]))
		number.chars[i++] = line.chars[(*linepos)++];

	if (!(i && isdigit(number.chars[i - 1]))) {
		printf("assembler: .data: invalid input of number\n");
		exit(1);
	}

	number.chars[i] = '\0';

	return  atoi(number.chars);
}

void verifylabels() {
	int i, j;

	for (i = 0; !emptylabel(ltbl[i]) && i < MAXLENGTH; i++) {
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
				printf("assembler: %s: invalid name\n", ltbl[j].name);
				exit(1);
			}

		if (REGISTER(ltbl[i].name)) {
			printf("assembler: %s: invalid name\n", ltbl[j].name);
			exit(1);
		}
	}

	for (i = 0; !emptylabel(extltbl[j]) && i < MAXLENGTH; i++) {
		for (j = i + 1; !emptylabel(extltbl[j]) && j < MAXLENGTH; j++)
			if (!strcmp(extltbl[i].name, extltbl[j].name)) {
				printf("assembler: %s: double declaration\n", ltbl[j].name);
				exit(1);
			}
				
		for (j = 0; opcode[j]; j++)
			if (!strcmp(extltbl[i].name, opcode[j])) {
				printf("assembler: %s: invalid name\n", ltbl[j].name);
				exit(1);
			}

		if (REGISTER(extltbl[i].name)) {
			printf("assembler: %s: invalid name\n", ltbl[j].name);
			exit(1);
		}
	}
}


