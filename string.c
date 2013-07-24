#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "string.h"



const String ZERO_STR = {
		{0}
};



String tostring(char* source) {
	String str;
	int i;

	str = initstring();
	strcat(source, "\0");

	for(i = 0; *source && i < LENGTH; str.chars[i++] = *source++)
		;

	/* printf("i = %d;source char: ~~%c~~; int value %d\n", i, *source, *source) */

	return str;
}

String initstring() {
	return ZERO_STR;
}

int instring(String source, int sourceindex, String pattern) {
	int i;

	for (i = 0; sourceindex < LENGTH && pattern.chars[i]; i++, sourceindex++)
		if (source.chars[sourceindex] != pattern.chars[i])
			return 0;

	return i < LENGTH && pattern.chars[i] ? 0 : 1;
}

int gettoken(String source, int pos, String *tocen, int delimiter, ...) {
	String dels;
	va_list p;
	int i, cnt;

							/*printf("line: ~~%s~~\n", source.chars);*/

	for (va_start(p, delimiter), dels = initstring(), i = 0;
								delimiter != AFTERLAST && i < LENGTH;
								delimiter = va_arg(p, int), i++)
		dels.chars[i] = delimiter;
							/*printf("del: char = ~~%c~~; val = %d\n", delimiter, delimiter);*/
	va_end(p);

							/*printf("va end\n\n\n");*/

	for (*tocen = initstring(), cnt = 0; pos < LENGTH; i++, pos++, cnt++) {
		for (i = 0; dels.chars[i]; i++) {

							/*printf("source.chars[%d], char: ~~%c~~, int %d\n", pos, source.chars[pos], source.chars[pos]);
							printf("dels.chars[%d]: char = ~~%c~~; val = %d\n", i, dels.chars[i], dels.chars[i]);*/

			if (source.chars[pos] == dels.chars[i]) {

							/*printf("tocen: ~~%s~~\n\n\n", (*tocen).chars);*/

				return cnt;
			}
		}

		(*tocen).chars[cnt] = source.chars[pos];
	}

							/*printf("tocen: ~~%s~~\n\n\n", (*tocen).chars);*/

	return cnt;
}

void tocharptr(String source, char *p, int maxptrsize) {
	int i;
/*	char *q = p;*/


	for (i = 0; i < LENGTH && i < maxptrsize - 1 && source.chars[i]; *p++ = source.chars[i++])
		;
	*p = '\0';

/*
	printf("source: ~~%s~~\n", source.chars);
	printf("ptr   : ~~%s~~\n", q);
*/
}























