#ifndef STRING_H_
#define STRING_H_



#define LENGTH 2000
#define AFTERLAST -1



typedef struct {
	char chars[LENGTH];
} String;



extern const String ZERO_STR;



/* returns String initialized with zeros */
String initstring();
String tostring(char* source);
int gettoken(String source, int pos, String *tocen, int delimiter, ...);
int instring(String source, int sourceindex, String pattern);
void tocharptr(String source, char *p, int maxptrsize);


#endif /* STRING_H_ */
