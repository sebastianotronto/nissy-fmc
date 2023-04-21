#include <string.h>
#include <inttypes.h>

#include "cube.h"
#include "coord.h"
#include "solve.h"

static void append_sol(DfsArg *arg);
static bool allowed_next(Move move, Move l0, Move l1);
static void get_state(Coordinate *coord[], Cube *cube, CubeState *state);
static void copy_dfsarg(DfsArg *src, DfsArg *dst);
static int  lower_bound(Coordinate *coord[], CubeState *state);
static void dfs(DfsArg *arg);
static void dfs_niss(DfsArg *arg);
static bool dfs_move_checkstop(DfsArg *arg);
static bool niss_makes_sense(DfsArg *arg);

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
allowed_next(Move m, Move l0, Move l1)
{
	bool p, q, allowed, order;

	p = l0 != NULLMOVE && base_move(l0) == base_move(m);
	q = l1 != NULLMOVE && base_move(l1) == base_move(m);
	allowed = !(p || (commute(l0, l1) && q));
	order   = !commute(l0, m) || l0 < m;

	return allowed && order;
}

void
get_state(Coordinate *coord[], Cube *cube, CubeState *state)
{
	int i;

	for (i = 0; coord[i] != NULL; i++)
		state[i].val = index_coord(coord[i], cube, &state[i].t);
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
		dst->state[i].val = src->state[i].val;
		dst->state[i].t   = src->state[i].t;
	}
}

static int
lower_bound(Coordinate *coord[], CubeState *state)
{
	int i, ret;

	ret = -1;
	for (i = 0; coord[i] != NULL; i++)
		ret = MAX(ret, ptableval(coord[i], state[i].val));

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
		if (allowed_next(m, arg->last[0], arg->last[1])) {
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
	get_state(newarg.s->coord, newarg.cube, newarg.state);

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
			mm = transform_move(arg->state[i].t, arg->last[0]);
			arg->state[i].val = move_coord(arg->s->coord[i],
			    mm, arg->state[i].val, &tt);
			arg->state[i].t = transform_trans(tt, arg->state[i].t);
		}
	}

	/* Computing bound for coordinates */
	arg->bound = lower_bound(arg->s->coord, arg->state);
	if (arg->st == NISS && !arg->niss)
		arg->bound = MIN(1, arg->bound);

	return arg->bound + arg->current_alg->len > arg->d ;
}

static bool
niss_makes_sense(DfsArg *arg)
{
	Cube testcube;
	CubeState state[MAX_N_COORD];

	if (arg->niss || !(arg->st == NISS) || arg->current_alg->len == 0)
		return false;

	make_solved(&testcube);
	apply_move(inverse_move(arg->last[0]), &testcube);
	get_state(arg->s->coord, &testcube, state);

	return lower_bound(arg->s->coord, state) > 0;
}

int
solve(Step *s, Trans t, int d, SolutionType st, Cube *c, char *sol)
{
	Alg alg;
	DfsArg arg;

	arg.niss        = false;
	arg.has_nissed  = false;
	arg.last[0]     = NULLMOVE;
	arg.last[1]     = NULLMOVE;
	arg.lastinv[0]  = NULLMOVE;
	arg.lastinv[1]  = NULLMOVE;

	alg.len = 0;
	arg.current_alg = &alg;

	arg.cube = c;
	arg.s = s;
	arg.t = t;
	arg.st = st;

	arg.d = d;
	arg.sol = &sol;

	if (arg.st == INVERSE)
		invert_cube(c);
	apply_trans(arg.t, c);
	get_state(arg.s->coord, arg.cube, arg.state);

	dfs(&arg);
	return 0;
}
