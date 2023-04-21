#include <string.h>
#include <inttypes.h>

#include "cube.h"
#include "coord.h"
#include "solve.h"
#include "steps.h"

static bool
set_step(char *str, Step **step)
{
	int i;

	for (i = 0; steps[i] != NULL; i++) {
		if (!strcmp(steps[i]->shortname, str)) {
			*step = steps[i];
			return true;
		}
	}
	
	return false;
}

static bool
set_solutiontype(char *str, SolutionType *st)
{
	if (!strcmp(str, "normal")) {
		*st = NORMAL;
		return true;
	}
	if (!strcmp(str, "inverse")) {
		*st = INVERSE;
		return true;
	}
	if (!strcmp(str, "niss")) {
		*st = NISS;
		return true;
	}

	return false;
}

static bool
set_trans(char *str, Trans *t)
{
	if (!strcmp(str, "uf")) {
		*t = uf;
		return true;
	}
	if (!strcmp(str, "fr")) {
		*t = fr;
		return true;
	}
	if (!strcmp(str, "rd")) {
		*t = rd;
		return true;
	}

	return false;
}

int
nissy_solve(char *step, char *trans, int d, char *type, char *scramble, char *sol)
{
	Cube c;
	Step *s;
	Trans t;
	SolutionType st;

	make_solved(&c);
	if (!set_step(step, &s)) return 1;
	if (!set_trans(trans, &t)) return 2;
	if (!set_solutiontype(type, &st)) return 4;
	if (!apply_scramble(scramble, &c)) return 5;

	return solve(s, t, d, st, &c, sol);
}

void
nissy_init()
{
	int i;

	init_cube();

	for (i = 0; coordinates[i] != NULL; i++)
		gen_coord(coordinates[i]);
}
