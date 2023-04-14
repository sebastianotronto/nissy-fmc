#include "solve.h"

static Alg *
read_scramble(int c, char **v)
{
	int i, k, n;
	unsigned int j;
	char *algstr;
	Alg *scr;

	if (c < 1) {
		fprintf(stderr, "Error: no scramble given\n");
		return false;
	}

	for(n = 0, i = 0; i < c; i++)
		n += strlen(v[i]);

	algstr = malloc((n + 1) * sizeof(char));
	k = 0;
	for (i = 0; i < c; i++)
		for (j = 0; j < strlen(v[i]); j++)
			algstr[k++] = v[i][j];
	algstr[k] = 0;

	scr = new_alg(algstr);

	free(algstr);

	return scr;
}

int
main(int argc, char *argv[])
{
	int i, m;
	long val;
	Alg *scramble;
	AlgList *sols;
	Cube c;
	SolutionType t;

	if (argc < 2) {
		fprintf(stderr, "Not enough arguments given\n");
		return -1;
	}

	m = 0;
	t = NORMAL;
	scramble = new_alg("");

	init_env();
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

	scramble = read_scramble(argc - i, &argv[i]);
	if (scramble->len <= 0) {
		fprintf(stderr, "Cannot read scramble\n");
		return 4;
	}

	make_solved(&c);
	apply_alg(scramble, &c);
	sols = solve(&c, argv[1], m, t, uf); /* TODO: trans */
	print_alglist(sols);
	free_alglist(sols);

	return 0;
}
