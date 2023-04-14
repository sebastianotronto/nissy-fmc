#ifndef SOLVE_H
#define SOLVE_H

#include "pruning.h"

typedef enum { NORMAL, INVERSE, NISS } SolutionType;

int solve(Cube *cube, char *step, int m, SolutionType st, Trans t, Alg *sol);

#endif
