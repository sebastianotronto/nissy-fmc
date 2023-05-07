#include <stdio.h>
#include <stdlib.h>

#include "../src/nissy.h"

#define MAX_SOLS 999

static char buf[TABLESFILESIZE];

int
main(int argc, char *argv[])
{
	char sols[99999];
	FILE *file;
	long size;

	if ((file = fopen("tables", "rb")) == NULL)
		return -2;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);
	fread(buf, 1, size, file);

	nissy_init(buf);

	if (argc != 6) {
		fprintf(stderr, "Not enough arguments given\n");
		return -1;
	}

	char *step = argv[1];
	char *trans = argv[2];
	int d = strtol(argv[3], NULL, 10);
	char *type = argv[4];
	char *scramble = argv[5];

	switch (nissy_solve(step, trans, d, type, scramble, sols)) {
	case 1:
		fprintf(stderr, "Error parsing step: %s\n", step);
		return -1;
	case 2:
		fprintf(stderr, "Error parsing trans: %s\n", trans);
		return -1;
	case 4:
		fprintf(stderr, "Error parsing type: %s\n", type);
		return -1;
	case 5:
		fprintf(stderr, "Error applying scramble: %s\n", scramble);
		return -1;
	default:
		printf("%s", sols);
	}

	return 0;
}
