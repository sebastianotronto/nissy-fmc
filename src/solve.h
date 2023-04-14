#ifndef SOLVE_H
#define SOLVE_H

#include "pruning.h"
#include "alg.h"

typedef enum { NORMAL, INVERSE, NISS } SolutionType;

AlgList *solve(Cube *cube, char *stepstr, int m, SolutionType st, Trans t);

#endif
