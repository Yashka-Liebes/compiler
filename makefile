make:
	gcc -o assembler main.c label.c word.c fextract.c string.c -Wall -ansi -pedantic -lm
