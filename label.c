#include <string.h>
#include "label.h"



const Label EMPTYLABEL = {
		{0},
		0
};



Label initlabel() {
	return EMPTYLABEL;
}

int emptylabel(Label l) {
	return l.name[0] == '\0';
}







