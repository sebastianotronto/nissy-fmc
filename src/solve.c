#include <string.h>
#include <inttypes.h>

#include "cube.h"
#include "coord.h"
#include "solve.h"

#define MAX_N_COORD 3

typedef enum { NORMAL, INVERSE, NISS } SolutionType;
typedef struct { uint64_t val; Trans t; } Movable;
typedef struct {
	char *                    shortname;
	Moveset *                 moveset;
	Coordinate *              coord[MAX_N_COORD];
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

static bool moveset_HTM(Move m);
static bool moveset_eofb(Move m);
static bool moveset_drud(Move m);
static bool moveset_htr(Move m);

static uint64_t index_eofb(Cube *cube);
static void     invindex_eofb(uint64_t ind, Cube *ret);
Indexer i_eofb = {
	.n       = POW2TO11,
	.index   = index_eofb,
	.to_cube = invindex_eofb,
};

static uint64_t index_coud(Cube *cube);
static void     invindex_coud(uint64_t ind, Cube *ret);
Indexer i_coud = {
	.n       = POW3TO7,
	.index   = index_coud,
	.to_cube = invindex_coud,
};

static uint64_t index_cp(Cube *cube);
static void     invindex_cp(uint64_t ind, Cube *ret);
Indexer i_cp = {
	.n       = FACTORIAL8,
	.index   = index_cp,
	.to_cube = invindex_cp,
};

static uint64_t index_epos(Cube *cube);
static void     invindex_epos(uint64_t ind, Cube *ret);
Indexer i_epos = {
	.n       = BINOM12ON4,
	.index   = index_epos,
	.to_cube = invindex_epos,
};

static uint64_t index_epe(Cube *cube);
static void     invindex_epe(uint64_t ind, Cube *ret);
Indexer i_epe = {
	.n       = FACTORIAL4,
	.index   = index_epe,
	.to_cube = invindex_epe,
};

static uint64_t index_eposepe(Cube *cube);
static void     invindex_eposepe(uint64_t ind, Cube *ret);
Indexer i_eposepe = {
	.n       = BINOM12ON4 * FACTORIAL4,
	.index   = index_eposepe,
	.to_cube = invindex_eposepe,
};

static uint64_t index_epud(Cube *cube);
static void     invindex_epud(uint64_t ind, Cube *ret);
Indexer i_epud = {
	.n       = FACTORIAL8,
	.index   = index_epud,
	.to_cube = invindex_epud,
};

Coordinate coord_eofb = {
	.name = "eofb",
	.type = COMP_COORD,
	.i    = {&i_eofb, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_coud = {
	.name = "coud",
	.type = COMP_COORD,
	.i    = {&i_coud, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_cp = {
	.name = "cp",
	.type = COMP_COORD,
	.i    = {&i_cp, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_epos = {
	.name = "epos",
	.type = COMP_COORD,
	.i    = {&i_epos, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_epe = {
	.name = "epe",
	.type = COMP_COORD,
	.i    = {&i_epe, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_eposepe = {
	.name = "eposepe",
	.type = COMP_COORD,
	.i    = {&i_eposepe, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_epud = {
	.name = "epud",
	.type = COMP_COORD,
	.i    = {&i_epud, NULL},
	.moveset = moveset_drud,
};

Coordinate coord_eofbepos = {
	.name = "eofbepos",
	.type = COMP_COORD,
	.i    = {&i_epos, &i_eofb, NULL},
	.moveset = moveset_HTM,
};

Coordinate coord_eofbepos_sym16 = {
	.name = "eofbepos_sym16",
	.type = SYM_COORD,
	.base = {&coord_eofbepos, NULL},
	.tgrp = &tgrp_udfix,
	.moveset = moveset_HTM,
};

Coordinate coord_cp_sym16 = {
	.name = "cp_sym16",
	.type = SYM_COORD,
	.base = {&coord_cp, NULL},
	.tgrp = &tgrp_udfix,
	.moveset = moveset_HTM,
};

Coordinate coord_drud_sym16 = {
	.name = "drud_sym16",
	.type = SYMCOMP_COORD,
	.base = {&coord_eofbepos_sym16, &coord_coud},
	.moveset = moveset_HTM,
};

Coordinate coord_drudfin_noE_sym16 = {
	.name = "drudfin_noE_sym16",
	.type = SYMCOMP_COORD,
	.base = {&coord_cp_sym16, &coord_epud},
	.moveset = moveset_drud,
};

Coordinate *coordinates[] = {
	&coord_eofb, &coord_coud, &coord_cp, &coord_epos, &coord_epe,
	&coord_eposepe, &coord_epud, &coord_eofbepos, &coord_eofbepos_sym16,
	&coord_drud_sym16, &coord_cp_sym16, &coord_drudfin_noE_sym16,
	NULL
};

Step eofb_HTM = {
	.shortname      = "eofb",
	.moveset        = moveset_HTM,
	.coord          = {&coord_eofb, NULL},
};

Step drud_HTM = {
	.shortname      = "drud",
	.moveset        = moveset_HTM,
	.coord          = {&coord_drud_sym16, NULL},
};

Step drfin_drud = {
	.shortname      = "drudfin",
	.moveset        = moveset_drud,
	.coord          = {&coord_drudfin_noE_sym16, &coord_epe, NULL},
};

Step *steps[] = {&eofb_HTM, &drud_HTM, &drfin_drud, NULL};

static bool
moveset_HTM(Move m)
{
	return m >= U && m <= B3;
}

static bool
moveset_eofb(Move m)
{
	Move b = base_move(m);

	return b == U || b == D || b == R || b == L ||
               ((b == F || b == B) && m == b+1);
}

static bool
moveset_drud(Move m)
{
	Move b = base_move(m);

	return b == U || b == D ||
               ((b == R || b == L || b == F || b == B) && m == b + 1);
}

static bool
moveset_htr(Move m)
{
	return moveset_HTM(m) && m == base_move(m) + 1;
}

static int
factorial(int n)
{
	int i, ret = 1;

	if (n < 0)
		return 0;

	for (i = 1; i <= n; i++)
		ret *= i;

	return ret;
}

static int
perm_to_index(int *a, int n)
{
	int i, j, c, ret = 0;

	for (i = 0; i < n; i++) {
		c = 0;
		for (j = i+1; j < n; j++)
			c += (a[i] > a[j]) ? 1 : 0;
		ret += factorial(n-i-1) * c;
	}

	return ret;
}

static void
index_to_perm(int p, int n, int *r)
{
	int a[n];
	int i, j, c;

	for (i = 0; i < n; i++)
		a[i] = 0;

	if (p < 0 || p >= factorial(n))
		for (i = 0; i < n; i++)
			r[i] = -1;

	for (i = 0; i < n; i++) {
		c = 0;
		j = 0;
		while (c <= p / factorial(n-i-1))
			c += a[j++] ? 0 : 1;
		r[i] = j-1;
		a[j-1] = 1;
		p %= factorial(n-i-1);
	}
}

static int
binomial(int n, int k)
{
	if (n < 0 || k < 0 || k > n)
		return 0;

	return factorial(n) / (factorial(k) * factorial(n-k));
}

static int 
subset_to_index(int *a, int n, int k)
{
	int i, ret = 0;

	for (i = 0; i < n; i++) {
		if (k == n-i)
			return ret;
		if (a[i]) {
			ret += binomial(n-i-1, k);
			k--;
		}
	}

	return ret;
}

static void
index_to_subset(int s, int n, int k, int *r)
{
	int i, j, v;

	for (i = 0; i < n; i++) {
		if (k == n-i) {
			for (j = i; j < n; j++)
				r[j] = 1;
			return;
		}

		if (k == 0) {
			for (j = i; j < n; j++)
				r[j] = 0;
			return;
		}

		v = binomial(n-i-1, k);
		if (s >= v) {
			r[i] = 1;
			k--;
			s -= v;
		} else {
			r[i] = 0;
		}
	}
}

static int
digit_array_to_int(int *a, int n, int b)
{
	int i, ret = 0, p = 1;

	for (i = 0; i < n; i++, p *= b)
		ret += a[i] * p;

	return ret;
}

static void
int_to_digit_array(int a, int b, int n, int *r)
{
	int i;

	for (i = 0; i < n; i++, a /= b)
		r[i] = a % b;
}

static void
int_to_sum_zero_array(int x, int b, int n, int *a)
{
	int i, s = 0;

	int_to_digit_array(x, b, n-1, a);
	for (i = 0; i < n - 1; i++)
	    s = (s + a[i]) % b;
	a[n-1] = (b - s) % b;
}

static int
perm_sign(int *a, int n)
{
	int i, j, ret = 0;

	for (i = 0; i < n; i++)
		for (j = i+1; j < n; j++)
			ret += (a[i] > a[j]) ? 1 : 0;

	return ret % 2;
}

static uint64_t
index_eofb(Cube *cube)
{
	return (uint64_t)digit_array_to_int(cube->eo, 11, 2);
}

static uint64_t
index_coud(Cube *cube)
{
	return (uint64_t)digit_array_to_int(cube->co, 7, 3);
}

static uint64_t
index_cp(Cube *cube)
{
	return (uint64_t)perm_to_index(cube->cp, 8);
}

static uint64_t
index_epe(Cube *cube)
{
	int i, e[4];

	for (i = 0; i < 4; i++)
		e[i] = cube->ep[i+8] - 8;

	return (uint64_t)perm_to_index(e, 4);
}

static uint64_t
index_epud(Cube *cube)
{
	return (uint64_t)perm_to_index(cube->ep, 8);
}

static uint64_t
index_epos(Cube *cube)
{
	int i, a[12];

	for (i = 0; i < 12; i++)
		a[i] = (cube->ep[i] < 8) ? 0 : 1;

	return (uint64_t)subset_to_index(a, 12, 4);
}

static uint64_t
index_eposepe(Cube *cube)
{
	int i, j, e[4];
	uint64_t epos, epe;

	epos = (uint64_t)index_epos(cube);
	for (i = 0, j = 0; i < 12; i++)
		if (cube->ep[i] >= 8)
			e[j++] = cube->ep[i] - 8;
	epe = (uint64_t)perm_to_index(e, 4);

	return epos * FACTORIAL4 + epe;
}

static void
invindex_eofb(uint64_t ind, Cube *cube)
{
	int_to_sum_zero_array(ind, 2, 12, cube->eo);
}

static void
invindex_coud(uint64_t ind, Cube *cube)
{
	int_to_sum_zero_array(ind, 3, 8, cube->co);
}

static void
invindex_cp(uint64_t ind, Cube *cube)
{
	index_to_perm(ind, 8, cube->cp);
}

static void
invindex_epe(uint64_t ind, Cube *cube)
{
	int i;

	index_to_perm(ind, 4, &cube->ep[8]);
	for (i = 0; i < 4; i++)
		cube->ep[i+8] += 8;
}

static void
invindex_epud(uint64_t ind, Cube *cube)
{
	index_to_perm(ind, 8, cube->ep);
}

static void
invindex_epos(uint64_t ind, Cube *cube)
{
	int i, j, k;

	index_to_subset(ind, 12, 4, cube->ep);
	for (i = 0, j = 0, k = 8; i < 12; i++)
		if (cube->ep[i] == 0)
			cube->ep[i] = j++;
		else
			cube->ep[i] = k++;
}

static void
invindex_eposepe(uint64_t ind, Cube *cube)
{
	int i, j, k, e[4];
	uint64_t epos, epe;

	epos = ind / FACTORIAL4;
	epe = ind % FACTORIAL4;

	index_to_subset(epos, 12, 4, cube->ep);
	index_to_perm(epe, 4, e);

	for (i = 0, j = 0, k = 0; i < 12; i++)
		if (cube->ep[i] == 0)
			cube->ep[i] = j++;
		else
			cube->ep[i] = e[k++] + 8;
}

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
