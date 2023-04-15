#define SOLVE_C

#include "solve.h"
#include "pruning.h"

#define MAX_N_COORD 3

typedef enum { NORMAL, INVERSE, NISS } SolutionType;
typedef struct { uint64_t val; Trans t; } Movable;
typedef struct {
	char *                    shortname;
	bool                      filter;
	Moveset *                 moveset;
	Coordinate *              coord[MAX_N_COORD];
	PruneData *               pd[MAX_N_COORD];
	bool                      compact_pd[MAX_N_COORD];
	Coordinate *              fallback_coord[MAX_N_COORD];
	PruneData *               fallback_pd[MAX_N_COORD];
	uint64_t                  fbmod[MAX_N_COORD];

} Step;
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
static bool        singlecwend(Alg *alg);

Step eofb_HTM = {
	.shortname      = "eofb",
	.filter         = true,
	.moveset        = allowed_HTM,
	.coord          = {&coord_eofb, NULL},
	.compact_pd     = {false},
};

Step drud_HTM = {
	.shortname      = "drud",
	.filter         = true,
	.moveset        = allowed_HTM,
	.coord          = {&coord_drud_sym16, NULL},
	.compact_pd     = {false}, /* TODO: compactify! */
};

Step drfin_drud = {
	.shortname      = "drudfin",
	.filter         = false,
	.moveset        = allowed_drud,
	.coord          = {&coord_drudfin_noE_sym16, &coord_epe, NULL},
	.compact_pd     = {false}, /* TODO: compactify! */
};

Step *steps[] = { &eofb_HTM, &drud_HTM, &drfin_drud, NULL };

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
	int i, v, ret = -1;

	for (i = 0; step->coord[i] != NULL; i++) {
		v = ptableval(step->pd[i], ind[i].val);
		if (v == step->pd[i]->base && step->compact_pd[i])
			v = ptableval(step->fallback_pd[i],
			    ind[i].val / step->fbmod[i]);
		if (v == 0 && ind[i].val != 0)
			v = 1;
		ret = MAX(ret, v);
	}

	return ret;
}

static void
dfs(DfsArg *arg)
{
	Move m;
	DfsArg newarg;
	bool len, singlecw, niss;

	if (dfs_move_checkstop(arg))
		return;

	if (arg->bound == 0) {
		len = arg->current_alg->len == arg->d;
		singlecw = singlecwend(arg->current_alg);
		niss = !(arg->st == NISS) || arg->has_nissed;
		if (len && singlecw && niss)
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

bool
singlecwend(Alg *alg)
{
	int i;
	bool nor, inv;
	Move l2 = NULLMOVE, l1 = NULLMOVE, l2i = NULLMOVE, l1i = NULLMOVE;

	for (i = 0; i < alg->len; i++) {
		if (alg->inv[i]) {
			l2i = l1i;
			l1i = alg->move[i];
		} else {
			l2 = l1;
			l1 = alg->move[i];
		}
	}

	nor = l1 ==base_move(l1)  && (!commute(l1, l2) ||l2 ==base_move(l2));
	inv = l1i==base_move(l1i) && (!commute(l1i,l2i)||l2i==base_move(l2i));

	return nor && inv;
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
	PDGenData pdg;
	pdg.moveset = arg.s->moveset;
	for (i = 0; arg.s->coord[i] != NULL; i++) {
		gen_coord(arg.s->coord[i]);
		pdg.coord   = arg.s->coord[i];
		pdg.compact = arg.s->compact_pd[i];
		pdg.pd      = NULL;
		arg.s->pd[i] = genptable(&pdg);
		if (arg.s->compact_pd[i]) {
			gen_coord(arg.s->fallback_coord[i]);
			pdg.coord   = arg.s->fallback_coord[i];
			pdg.compact = false;
			pdg.pd      = NULL;
			arg.s->fallback_pd[i] = genptable(&pdg);
		}
	}

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

