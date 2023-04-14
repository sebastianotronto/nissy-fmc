#include "solve.h"

#define MAX_SOLS 999

int
main(int argc, char *argv[])
{
	int i, m, n;
	long val;
	Cube c;
	SolutionType t;
	Alg sols[999];

	if (argc < 2) {
		fprintf(stderr, "Not enough arguments given\n");
		return -1;
	}

	m = 0;
	t = NORMAL;

	init_cube();

	for (i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-m") && i+1 < argc) {
			val = strtol(argv[++i], NULL, 10);
			if (val < 0 || val > 100) {
				fprintf(stderr, "Invalid number of moves\n");
				return 2;
			}
			m = val;
		} else if (!strcmp(argv[i], "-I")) {
			t = INVERSE;
		} else if (!strcmp(argv[i], "-N")) {
			t = NISS;
		} else {
			break;
		}
	}

	if (i == argc) {
		fprintf(stderr, "No scramble given\n");
		return 4;
	}

	make_solved(&c);
	if (!apply_scramble(argv[i], &c)) {
		fprintf(stderr, "Invalid scramble: %s\n", argv[i]);
		return 5;
	}

	n = solve(&c, argv[1], m, t, uf, sols); /* TODO: trans */
	for (i = 0; i < n; i++) {
		char buf[66];
		int l = alg_string(&sols[i], buf);
		printf("%s (%d)\n", buf, l);
	}

	return 0;
}
