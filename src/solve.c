#define SOLVE_C

#include "solve.h"

#define MAX_N_COORD 3

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
	int *                     nsols;
	Alg *                     sols;
	int                       d;
	int                       bound;
	bool                      can_niss;
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
	int i;

	copy_alg(arg->current_alg, &arg->sols[*arg->nsols]);
	transform_alg(inverse_trans(arg->t), &arg->sols[*arg->nsols]);

	if (arg->st == INVERSE)
		for (i = 0; i < arg->sols[*arg->nsols].len; i++)
			arg->sols[*arg->nsols].inv[i] = true;

	(*arg->nsols)++;
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
	dst->can_niss    = src->can_niss;
	dst->niss        = src->niss;
	dst->has_nissed  = src->has_nissed;
	dst->nsols       = src->nsols;
	dst->sols        = src->sols;
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
		niss = !arg->can_niss || arg->has_nissed;
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
	if (arg->can_niss && !arg->niss)
		arg->bound = MIN(1, arg->bound);

	return arg->bound + arg->current_alg->len > arg->d ;
}

static bool
niss_makes_sense(DfsArg *arg)
{
	Cube testcube;
	DfsArg aux;

	if (arg->niss || !arg->can_niss || arg->current_alg->len == 0)
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

int
solve(Cube *cube, char *stepstr, int m, SolutionType st, Trans t, Alg *sol)
{
	int i, n;
	Alg alg;
	Cube c;
	DfsArg arg;
	Step *step;

	/* Prepare step TODO: remove all initialization! */
	step = NULL;
	for (i = 0; steps[i] != NULL; i++)
		if (!strcmp(steps[i]->shortname, stepstr))
			step = steps[i];
	if (step == NULL) {
		fprintf(stderr, "No step to solve!\n");
		return 0;
	}
	PDGenData pdg;
	pdg.moveset = step->moveset;
	for (i = 0; step->coord[i] != NULL; i++) {
		gen_coord(step->coord[i]);
		pdg.coord   = step->coord[i];
		pdg.compact = step->compact_pd[i];
		pdg.pd      = NULL;
		step->pd[i] = genptable(&pdg);
		if (step->compact_pd[i]) {
			gen_coord(step->fallback_coord[i]);
			pdg.coord   = step->fallback_coord[i];
			pdg.compact = false;
			pdg.pd      = NULL;
			step->fallback_pd[i] = genptable(&pdg);
		}
	}

	alg.len = 0;
	n = 0;

	/* Prepare cube for solve */
	arg.cube = &c;
	copy_cube(cube, arg.cube);
	if (st == INVERSE)
		invert_cube(arg.cube);
	apply_trans(t, arg.cube);
	arg.s           = step;
	compute_ind(&arg);
	arg.t           = t;
	arg.st          = st;
	arg.can_niss    = st == NISS;
	arg.nsols       = &n;
	arg.sols        = sol;
	arg.d           = m;
	arg.niss        = false;
	arg.has_nissed  = false;
	arg.last[0]     = NULLMOVE;
	arg.last[1]     = NULLMOVE;
	arg.lastinv[0]  = NULLMOVE;
	arg.lastinv[1]  = NULLMOVE;
	arg.current_alg = &alg;

	dfs(&arg);

	return n;
}

