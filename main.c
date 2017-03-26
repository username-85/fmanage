#include "fm.h"
#include "rc.h"
#include <stdlib.h>

int main(void) {
	if (fm_prepare() == SUCCESS) {
		fm_run();
		fm_end(); 
	}
	else {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

