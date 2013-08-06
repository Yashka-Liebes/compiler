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

	return str;
}

String initstring() {
	return ZERO_STR;
}

/*
int instring(String source, int sourceindex, String pattern) {
	int i;

	for (i = 0; sourceindex < LENGTH && pattern.chars[i]; i++, sourceindex++)
		if (source.chars[sourceindex] != pattern.chars[i])
			return 0;

	return i < LENGTH && pattern.chars[i] ? 0 : 1;
}
*/

int gettoken(String source, int pos, String *tocen, int delimiter, ...) {
	String dels;
	va_list p;
	int i, cnt;

	for (va_start(p, delimiter), dels = initstring(), i = 0;
								delimiter != AFTERLAST && i < LENGTH;
								delimiter = va_arg(p, int), i++)
		dels.chars[i] = delimiter;

	va_end(p);

	for (*tocen = initstring(), cnt = 0; pos < LENGTH; i++, pos++, cnt++) {
		for (i = 0; dels.chars[i]; i++)
			if (source.chars[pos] == dels.chars[i])
				return cnt;

		(*tocen).chars[cnt] = source.chars[pos];
	}

	return cnt;
}

void tocharptr(String source, char *p, int maxptrsize) {
	int i;

	for (i = 0; i < LENGTH && i < maxptrsize - 1 && source.chars[i]; *p++ = source.chars[i++])
		;
	*p = '\0';
}























