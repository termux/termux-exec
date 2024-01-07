#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		fprintf(stdout, i == 1 ? "%s" : " %s", argv[i]);
	}

	fprintf(stdout, "\n");
	fflush(stdout);
}
