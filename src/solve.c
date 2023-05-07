#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#include "cube.h"
#include "coord.h"
#include "solve.h"

static void append_sol(DfsArg *);
static bool allowed_next(Move m, Move l0, Move l1);
static void get_state(Coordinate *[], Cube *, CubeState *);
static int lower_bound(Coordinate *[], CubeState *);
static void dfs(DfsArg *);
static void dfs_niss(DfsArg *);
static void dfs_move(Move, DfsArg *);
static bool niss_makes_sense(DfsArg *);

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
	Move m, last[2];
	bool len, niss;
	int bound;

	bound = lower_bound(arg->s->coord, arg->state);
	if (arg->st == NISS && !arg->niss)
		bound = MIN(1, bound);

	if (bound + arg->current_alg->len > arg->d)
		return;

	if (bound == 0) {
		len = arg->current_alg->len == arg->d;
		niss = !(arg->st == NISS) || arg->has_nissed;
		if (len && niss)
			append_sol(arg);
		return;
	}

	last[0] = arg->last[0];
	last[1] = arg->last[1];
	arg->last[1] = arg->last[0];
	for (m = U; m <= B3; m++) {
		if (!arg->s->moveset(m))
			continue;
		if (allowed_next(m, last[0], last[1])) {
			arg->last[0] = m;
			append_move(arg->current_alg, m, arg->niss);
			dfs_move(m, arg);
			dfs(arg);
			dfs_move(inverse_move(m), arg);
			arg->current_alg->len--;
		}
	}
	arg->last[0] = last[0];
	arg->last[1] = last[1];

	if (niss_makes_sense(arg))
		dfs_niss(arg);
}

static void
dfs_niss(DfsArg *arg)
{
	int i;
	DfsArg newarg;
	Cube c, newcube;

	newarg.s           = arg->s;
	newarg.t           = arg->t;
	newarg.st          = arg->st;
	newarg.d           = arg->d;
	newarg.sol         = arg->sol;
	newarg.current_alg = arg->current_alg;

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
		newarg.last[i] = arg->lastinv[i];
		newarg.lastinv[i] = arg->last[i];
	}

	newarg.niss = true;
	newarg.has_nissed = true;

	dfs(&newarg);
}

static void
dfs_move(Move m, DfsArg *arg)
{
	int i;
	Move mm;
	Trans tt = uf; /* Avoid uninitialized warning */

	for (i = 0; arg->s->coord[i] != NULL; i++) {
		mm = transform_move(arg->state[i].t, m);
		arg->state[i].val = move_coord(
		    arg->s->coord[i], mm, arg->state[i].val, &tt);
		arg->state[i].t = transform_trans(tt, arg->state[i].t);
	}
}

static bool
niss_makes_sense(DfsArg *arg)
{
	Cube testcube;
	CubeState state[MAX_N_COORD];
	bool b1, b2, comm;

	if (arg->niss || !(arg->st == NISS) || arg->current_alg->len == 0)
		return false;

	make_solved(&testcube);
	apply_move(inverse_move(arg->last[0]), &testcube);
	get_state(arg->s->coord, &testcube, state);
	b1 = lower_bound(arg->s->coord, state) > 0;

	make_solved(&testcube);
	apply_move(inverse_move(arg->last[1]), &testcube);
	get_state(arg->s->coord, &testcube, state);
	b2 = lower_bound(arg->s->coord, state) > 0;

	comm = commute(arg->last[0], arg->last[1]);

	return b1 > 0 && !(comm && b2 == 0);
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
