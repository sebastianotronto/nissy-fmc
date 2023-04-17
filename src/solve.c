#define SOLVE_C

#include "coord.h"
#include "steps.h"
#include "solve.h"

typedef enum { NORMAL, INVERSE, NISS } SolutionType;
typedef struct { uint64_t val; Trans t; } Movable;
typedef struct {
	Cube *                    cube;
	Movable                   ind[MAX_N_COORD];
	Step *                    s;
	Trans                     t;
	SolutionType              st;
	char **                   sol;
	int                       d;
	int                       bound;
	bool                      niss;
	bool                      has_nissed;
	Move                      last[2];
	Move                      lastinv[2];
	Alg *                     current_alg;
} DfsArg;

static void        append_sol(DfsArg *arg);
static bool        allowed_next(Move move, Step *s, Move l0, Move l1);
static void        compute_ind(DfsArg *arg);
static void        copy_dfsarg(DfsArg *src, DfsArg *dst);
static int         estimate_step(Step *step, Movable *ind);
static void        dfs(DfsArg *arg);
static void        dfs_niss(DfsArg *arg);
static bool        dfs_move_checkstop(DfsArg *arg);
static bool        niss_makes_sense(DfsArg *arg);

static void
append_sol(DfsArg *arg)
{
	Alg alg;
	int i, n;

	copy_alg(arg->current_alg, &alg);
	transform_alg(inverse_trans(arg->t), &alg);

	if (arg->st == INVERSE)
		for (i = 0; i < alg.len; i++)
			alg.inv[i] = true;

	n = alg_string(&alg, *arg->sol);
	(*arg->sol)[n] = '\n';
	(*arg->sol)[n+1] = 0;
	*arg->sol += (n+1);
}

static bool
allowed_next(Move m, Step *s, Move l0, Move l1)
{
	bool p, q, allowed, order;

	p = l0 != NULLMOVE && base_move(l0) == base_move(m);
	q = l1 != NULLMOVE && base_move(l1) == base_move(m);
	allowed = !(p || (commute(l0, l1) && q));
	order   = !commute(l0, m) || l0 < m;

	return allowed && order;
}

void
compute_ind(DfsArg *arg)
{
	int i;

	for (i = 0; arg->s->coord[i] != NULL; i++)
		arg->ind[i].val = index_coord(arg->s->coord[i],
		    arg->cube, &arg->ind[i].t);
}

static void
copy_dfsarg(DfsArg *src, DfsArg *dst)
{
	int i;

	dst->cube        = src->cube;
	dst->s           = src->s;
	dst->t           = src->t;
	dst->st          = src->st;
	dst->d           = src->d;
	dst->bound       = src->bound; /* In theory not needed */
	dst->niss        = src->niss;
	dst->has_nissed  = src->has_nissed;
	dst->sol         = src->sol;
	dst->current_alg = src->current_alg;

	for (i = 0; i < 2; i++) {
		dst->last[i]    = src->last[i];
		dst->lastinv[i] = src->lastinv[i];
	}

	for (i = 0; src->s->coord[i] != NULL; i++) {
		dst->ind[i].val = src->ind[i].val;
		dst->ind[i].t   = src->ind[i].t;
	}
}

static int
estimate_step(Step *step, Movable *ind)
{
	int i, ret;

	ret = -1;
	for (i = 0; step->coord[i] != NULL; i++)
		ret = MAX(ret, ptableval(step->coord[i], ind[i].val));

	return ret;
}

static void
dfs(DfsArg *arg)
{
	Move m;
	DfsArg newarg;
	bool len, niss;

	if (dfs_move_checkstop(arg))
		return;

	if (arg->bound == 0) {
		len = arg->current_alg->len == arg->d;
		niss = !(arg->st == NISS) || arg->has_nissed;
		if (len && niss)
			append_sol(arg);
		return;
	}

	for (m = U; m <= B3; m++) {
		if (!arg->s->moveset(m))
			continue;
		if (allowed_next(m, arg->s, arg->last[0], arg->last[1])) {
			copy_dfsarg(arg, &newarg);
			newarg.last[1] = arg->last[0];
			newarg.last[0] = m;
			append_move(arg->current_alg, m, newarg.niss);
			dfs(&newarg);
			arg->current_alg->len--;
		}
	}

	if (niss_makes_sense(arg))
		dfs_niss(arg);
}

static void
dfs_niss(DfsArg *arg)
{
	int i;
	DfsArg newarg;
	Cube c, newcube;
	Move aux;

	copy_dfsarg(arg, &newarg);

	/* Invert current alg and scramble */
	newarg.cube = &newcube;

	make_solved(newarg.cube);
	apply_alg(arg->current_alg, newarg.cube);
	invert_cube(newarg.cube);

	copy_cube(arg->cube, &c);
	invert_cube(&c);
	compose(&c, newarg.cube);

	/* New indexes */
	compute_ind(&newarg);

	/* Swap last moves */
	for (i = 0; i < 2; i++) {
		aux = newarg.last[i];
		newarg.last[i] = newarg.lastinv[i];
		newarg.lastinv[i] = aux;
	}

	newarg.niss = !(arg->niss);
	newarg.has_nissed = true;

	dfs(&newarg);
}

static bool
dfs_move_checkstop(DfsArg *arg)
{
	int i;
	Move mm;
	Trans tt = uf; /* Avoid uninitialized warning */

	/* Moving */
	if (arg->last[0] != NULLMOVE) {
		for (i = 0; arg->s->coord[i] != NULL; i++) {
			mm = transform_move(arg->ind[i].t, arg->last[0]);
			arg->ind[i].val = move_coord(arg->s->coord[i],
			    mm, arg->ind[i].val, &tt);
			arg->ind[i].t = transform_trans(tt, arg->ind[i].t);
		}
	}

	/* Computing bound for coordinates */
	arg->bound = estimate_step(arg->s, arg->ind);
	if (arg->st == NISS && !arg->niss)
		arg->bound = MIN(1, arg->bound);

	return arg->bound + arg->current_alg->len > arg->d ;
}

static bool
niss_makes_sense(DfsArg *arg)
{
	Cube testcube;
	DfsArg aux;

	if (arg->niss || !(arg->st == NISS) || arg->current_alg->len == 0)
		return false;

	make_solved(&testcube);
	apply_move(inverse_move(arg->last[0]), &testcube);
	aux.cube = &testcube;
	aux.s = arg->s;
	/* TODO: refactor compute_ind so we don't need full arg */
	compute_ind(&aux);

	return estimate_step(aux.s, aux.ind) > 0;
}

/* Public functions **********************************************************/

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
solve(char *step, char *trans, int d, char *type, char *scramble, char *sol)
{
	int i;
	Alg alg;
	Cube c;
	DfsArg arg;

	arg.niss        = false;
	arg.has_nissed  = false;
	arg.last[0]     = NULLMOVE;
	arg.last[1]     = NULLMOVE;
	arg.lastinv[0]  = NULLMOVE;
	arg.lastinv[1]  = NULLMOVE;

	alg.len = 0;
	arg.current_alg = &alg;

	arg.d = d;
	arg.sol = &sol;

	if (!set_step(step, &arg.s)) return 1;

	/* Prepare step TODO: remove all initialization! */
	for (i = 0; arg.s->coord[i] != NULL; i++)
		gen_coord(arg.s->coord[i]);

	if (!set_trans(trans, &arg.t)) return 2;

	if (!set_solutiontype(type, &arg.st)) return 4;

	make_solved(&c);
	if (!apply_scramble(scramble, &c)) return 5;
	if (arg.st == INVERSE)
		invert_cube(&c);
	apply_trans(arg.t, &c);
	arg.cube = &c;
	compute_ind(&arg);

	dfs(&arg);
	return 0;
}

