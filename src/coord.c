#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "cube.h"
#include "coord.h"

#define ENTRIES_PER_GROUP         (2*sizeof(entry_group_t))
#define ENTRIES_PER_GROUP_COMPACT (4*sizeof(entry_group_t))

static uint64_t indexers_getind(Indexer **, Cube *);
static uint64_t indexers_getmax(Indexer **);
static void indexers_makecube(Indexer **, uint64_t, Cube *);
static int coord_base_number(Coordinate *);
static void gen_coord_comp(Coordinate *);
static void gen_coord_sym(Coordinate *);
static bool read_coord_mtable(Coordinate *);
static bool read_coord_sd(Coordinate *);
static bool read_coord_ttable(Coordinate *);
static bool write_coord_mtable(Coordinate *);
static bool write_coord_sd(Coordinate *);
static bool write_coord_ttable(Coordinate *);

static void genptable(Coordinate *);
static void genptable_bfs(Coordinate *, int);
static void fixnasty(Coordinate *, uint64_t, int);
static void genptable_compress(Coordinate *);
static void genptable_setbase(Coordinate *);
static uint64_t ptablesize(Coordinate *);
static void ptable_update(Coordinate *, uint64_t, int);
static bool read_ptable_file(Coordinate *);
static bool write_ptable_file(Coordinate *);

static uint64_t
indexers_getmax(Indexer **is)
{
	int i;
	uint64_t max = 1;

	for (i = 0; is[i] != NULL; i++)
		max *= is[i]->n;

	return max;
}

static uint64_t
indexers_getind(Indexer **is, Cube *c)
{
	int i;
	uint64_t max = 0;

	for (i = 0; is[i] != NULL; i++) {
		max *= is[i]->n;
		max += is[i]->index(c);
	}

	return max;
}

static void
indexers_makecube(Indexer **is, uint64_t ind, Cube *c)
{
	/* Warning: anti-indexers are applied in the same order as indexers. */
	/* We assume order does not matter, but it would make more sense to  */
	/* Apply them in reverse.                                            */

	int i;
	uint64_t m;

	make_solved(c);
	m = indexers_getmax(is);
	for (i = 0; is[i] != NULL; i++) {
		m /= is[i]->n;
		is[i]->to_cube(ind / m, c);
		ind %= m;
	}
}

static int
coord_base_number(Coordinate *coord)
{
	int j;

	for (j = 0; coord->base[j] != NULL; j++) ;

	return j;
}

static void
gen_coord_comp(Coordinate *coord)
{
	uint64_t ui;
	Cube c, mvd;
	Move m;
	Trans t;

	coord->max = indexers_getmax(coord->i);

	for (m = 0; m < NMOVES; m++)
		coord->mtable[m] = malloc(coord->max * sizeof(uint64_t));

	for (t = 0; t < NTRANS; t++)
		coord->ttable[t] = malloc(coord->max * sizeof(uint64_t));

	if (!read_coord_mtable(coord)) {
		fprintf(stderr, "%s: generating mtable\n", coord->name);

		for (ui = 0; ui < coord->max; ui++) {
			indexers_makecube(coord->i, ui, &c);
			for (m = 0; m < NMOVES; m++) {
				copy_cube(&c, &mvd);
				apply_move(m, &mvd);
				coord->mtable[m][ui] =
				    indexers_getind(coord->i, &mvd);
			}
		}
		if (!write_coord_mtable(coord))
			fprintf(stderr, "%s: error writing mtable\n",
			    coord->name);
		
		fprintf(stderr, "%s: mtable generated\n", coord->name);
	}

	if (!read_coord_ttable(coord)) {
		fprintf(stderr, "%s: generating ttable\n", coord->name);

		for (ui = 0; ui < coord->max; ui++) {
			indexers_makecube(coord->i, ui, &c);
			for (t = 0; t < NTRANS; t++) {
				copy_cube(&c, &mvd);
				apply_trans(t, &mvd);
				coord->ttable[t][ui] =
				    indexers_getind(coord->i, &mvd);
			}
		}
		if (!write_coord_ttable(coord))
			fprintf(stderr, "%s: error writing ttable\n",
			    coord->name);
	}
}

static void
gen_coord_sym(Coordinate *coord)
{
	uint64_t i, in, ui, uj, uu, M, nr;
	int j;
	Move m;
	Trans t;

	M = coord->base[0]->max;
	coord->selfsim    = malloc(M * sizeof(uint64_t));
	coord->symclass   = malloc(M * sizeof(uint64_t));
	coord->symrep     = malloc(M * sizeof(uint64_t));
	coord->transtorep = malloc(M * sizeof(Trans));

	if (!read_coord_sd(coord)) {
		fprintf(stderr, "%s: generating syms\n", coord->name);

		for (i = 0; i < M; i++)
			coord->symclass[i] = M+1;

		for (i = 0, nr = 0; i < M; i++) {
			if (coord->symclass[i] != M+1)
				continue;

			coord->symrep[nr]    = i;
			coord->transtorep[i] = uf;
			coord->selfsim[nr]   = (uint64_t)0;
			for (j = 0; j < coord->tgrp->n; j++) {
				t = coord->tgrp->t[j];
				in = trans_coord(coord->base[0], t, i);
				coord->symclass[in] = nr;
				if (in == i)
					coord->selfsim[nr] |= ((uint64_t)1<<t);
				else
					coord->transtorep[in] =
					    inverse_trans(t);
			}
			nr++;
		}

		coord->max = nr;

		fprintf(stderr, "%s: found %" PRIu64 " classes\n",
		    coord->name, nr);
		if (!write_coord_sd(coord))
			fprintf(stderr, "%s: error writing symdata\n",
			    coord->name);
	}

	coord->symrep = realloc(coord->symrep, coord->max*sizeof(uint64_t));
	coord->selfsim = realloc(coord->selfsim, coord->max*sizeof(uint64_t));

	for (m = 0; m < NMOVES; m++) {
		coord->mtable[m] = malloc(coord->max*sizeof(uint64_t));
		coord->ttrep_move[m] = malloc(coord->max*sizeof(Trans));
	}

	if (!read_coord_mtable(coord)) {
		for (ui = 0; ui < coord->max; ui++) {
			uu = coord->symrep[ui];
			for (m = 0; m < NMOVES; m++) {
				uj = move_coord(coord->base[0], m, uu, NULL);
				coord->mtable[m][ui] = coord->symclass[uj];
				coord->ttrep_move[m][ui] =
				    coord->transtorep[uj];
			}
		}
		if (!write_coord_mtable(coord))
			fprintf(stderr, "%s: error writing mtable\n",
			    coord->name);
	}
}

static bool
read_coord_mtable(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	Move m;
	uint64_t M;
	bool r;

	strcpy(fname, "tables/mt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "rb")) == NULL)
		return false;

	M = coord->max;
	r = true;
	for (m = 0; m < NMOVES; m++)
		r = r && fread(coord->mtable[m], sizeof(uint64_t), M, f) == M;

	if (coord->type == SYM_COORD)
		for (m = 0; m < NMOVES; m++)
			r = r && fread(coord->ttrep_move[m],
			    sizeof(Trans), M, f) == M;

	fclose(f);
	return r;
}

static bool
read_coord_sd(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	uint64_t M, N;
	bool r;

	strcpy(fname, "tables/sd_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "rb")) == NULL)
		return false;

	r = true;
	r = r && fread(&coord->max,       sizeof(uint64_t), 1, f) == 1;
	M = coord->max;
	N = coord->base[0]->max;
	r = r && fread(coord->symrep,     sizeof(uint64_t), M, f) == M;
	r = r && fread(coord->selfsim,    sizeof(uint64_t), M, f) == M;
	r = r && fread(coord->symclass,   sizeof(uint64_t), N, f) == N;
	r = r && fread(coord->transtorep, sizeof(Trans),    N, f) == N;

	fclose(f);
	return r;
}

static bool
read_coord_ttable(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	Trans t;
	uint64_t M;
	bool r;

	strcpy(fname, "tables/tt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "rb")) == NULL)
		return false;

	M = coord->max;
	r = true;
	for (t = 0; t < NTRANS; t++)
		r = r && fread(coord->ttable[t], sizeof(uint64_t), M, f) == M;

	fclose(f);
	return r;
}

static bool
write_coord_mtable(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	Move m;
	uint64_t M;
	bool r;

	strcpy(fname, "tables/mt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "wb")) == NULL)
		return false;

	M = coord->max;
	r = true;
	for (m = 0; m < NMOVES; m++)
		r = r && fwrite(coord->mtable[m], sizeof(uint64_t), M, f) == M;

	if (coord->type == SYM_COORD)
		for (m = 0; m < NMOVES; m++)
			r = r && fwrite(coord->ttrep_move[m],
			    sizeof(Trans), M, f) == M;

	fclose(f);
	return r;
}

static bool
write_coord_sd(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	uint64_t M, N;
	bool r;

	strcpy(fname, "tables/sd_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "wb")) == NULL)
		return false;

	r = true;
	M = coord->max;
	N = coord->base[0]->max;
	r = r && fwrite(&coord->max,       sizeof(uint64_t), 1, f) == 1;
	r = r && fwrite(coord->symrep,     sizeof(uint64_t), M, f) == M;
	r = r && fwrite(coord->selfsim,    sizeof(uint64_t), M, f) == M;
	r = r && fwrite(coord->symclass,   sizeof(uint64_t), N, f) == N;
	r = r && fwrite(coord->transtorep, sizeof(Trans),    N, f) == N;

	fclose(f);
	return r;
}

static bool
write_coord_ttable(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	Trans t;
	uint64_t M;
	bool r;

	strcpy(fname, "tables/tt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "wb")) == NULL)
		return false;

	M = coord->max;
	r = true;
	for (t = 0; t < NTRANS; t++)
		r = r && fwrite(coord->ttable[t], sizeof(uint64_t), M, f) == M;

	fclose(f);
	return r;
}

/* Public functions **********************************************************/

void
gen_coord(Coordinate *coord)
{
	int i;

	if (coord == NULL || coord->generated)
		return;

	/* TODO: for SYM_COORD, we do not want to save base to file */
	for (i = 0; i < 2; i++)
		gen_coord(coord->base[i]);

	switch (coord->type) {
	case COMP_COORD:
		if (coord->i[0] == NULL)
			goto error_gc;
		gen_coord_comp(coord);
		break;
	case SYM_COORD:
		if (coord->base[0] == NULL || coord->tgrp == NULL)
			goto error_gc;
		gen_coord_sym(coord);
		break;
	case SYMCOMP_COORD:
		if (coord->base[0] == NULL || coord->base[1] == NULL)
			goto error_gc;
		coord->max = coord->base[0]->max * coord->base[1]->max;
		break;
	default:
		break;
	}

	coord->generated = true;
	genptable(coord);

	return;

error_gc:
	fprintf(stderr, "Error generating coordinates.\n"
			"This is a bug, pleae report.\n");
	exit(1);
}

uint64_t
index_coord(Coordinate *coord, Cube *cube, Trans *offtrans)
{
	uint64_t c[2], cnosym;
	Trans ttr;

	switch (coord->type) {
	case COMP_COORD:
		if (offtrans != NULL)
			*offtrans = uf;

		return indexers_getind(coord->i, cube);
	case SYM_COORD:
		cnosym = index_coord(coord->base[0], cube, NULL);
		ttr = coord->transtorep[cnosym];

		if (offtrans != NULL)
			*offtrans = ttr;

		return coord->symclass[cnosym];
	case SYMCOMP_COORD:
		c[0] = index_coord(coord->base[0], cube, NULL);
		cnosym = index_coord(coord->base[0]->base[0], cube, NULL);
		ttr = coord->base[0]->transtorep[cnosym];
		c[1] = index_coord(coord->base[1], cube, NULL);
		c[1] = trans_coord(coord->base[1], ttr, c[1]);

		if (offtrans != NULL)
			*offtrans = ttr;

		return c[0] * coord->base[1]->max + c[1];
	default:
		break;
	}

	return coord->max; /* Only reached in case of error */
}

uint64_t
move_coord(Coordinate *coord, Move m, uint64_t ind, Trans *offtrans)
{
	uint64_t i[2], M;
	Trans ttr;

	/* Some safety checks should be done here, but for performance   *
	 * reasons we'd rather do them before calling this function.     *
	 * We should check if coord is generated.                        */

	switch (coord->type) {
	case COMP_COORD:
		if (offtrans != NULL)
			*offtrans = uf;

		return coord->mtable[m][ind];
	case SYM_COORD:
		ttr = coord->ttrep_move[m][ind];

		if (offtrans != NULL)
			*offtrans = ttr;

		return coord->mtable[m][ind];
	case SYMCOMP_COORD:
		M = coord->base[1]->max;
		i[0] = ind / M;
		i[1] = ind % M;
		ttr = coord->base[0]->ttrep_move[m][i[0]];
		i[0] = coord->base[0]->mtable[m][i[0]];
		i[1] = coord->base[1]->mtable[m][i[1]];
		i[1] = coord->base[1]->ttable[ttr][i[1]];

		if (offtrans != NULL)
			*offtrans = ttr;

		return i[0] * M + i[1];
	default:
		break;
	}

	return coord->max; /* Only reached in case of error */
}

uint64_t
trans_coord(Coordinate *coord, Trans t, uint64_t ind)
{
	uint64_t i[2], M;

	/* Some safety checks should be done here, but for performance   *
	 * reasons we'd rather do them before calling this function.     *
	 * We should check if coord is generated.                        */

	switch (coord->type) {
	case COMP_COORD:
		return coord->ttable[t][ind];
	case SYM_COORD:
		return ind;
	case SYMCOMP_COORD:
		M = coord->base[1]->max;
		i[0] = ind / M; /* Always fixed */
		i[1] = ind % M;
		i[1] = coord->base[1]->ttable[t][i[1]];
		return i[0] * M + i[1];
	default:
		break;
	}

	return coord->max; /* Only reached in case of error */
}

static void
genptable(Coordinate *coord)
{
	bool compact;
	int d, i;
	uint64_t oldn, sz;

	coord->compact = coord->base[1] != NULL;
	sz = ptablesize(coord) * (coord->compact ? 2 : 1);
	coord->ptable = malloc(sz * sizeof(entry_group_t));

	if (read_ptable_file(coord))
		return;

	/* For the first steps we proceed the same way for compact and not */
	compact = coord->compact;
	coord->compact = false;

	fprintf(stderr, "Generating pt_%s\n", coord->name); 

	/* TODO: replace with loop */
	memset(coord->ptable, ~(uint8_t)0,
	    ptablesize(coord)*sizeof(entry_group_t));
	for (i = 0; i < 16; i++)
		coord->count[i] = 0;

	coord->updated = 0;
	oldn = 0;
	ptable_update(coord, 0, 0);
	fixnasty(coord, 0, 0);
	fprintf(stderr, "Depth %d done, generated %"
		PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
		0, coord->updated - oldn, coord->updated, coord->max);
	oldn = coord->updated;
	coord->count[0] = coord->updated;
	for (d = 0; d < 15 && coord->updated < coord->max; d++) {
		genptable_bfs(coord, d);
		fprintf(stderr, "Depth %d done, generated %"
			PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
			d+1, coord->updated-oldn, coord->updated, coord->max);
		coord->count[d+1] = coord->updated - oldn;
		oldn = coord->updated;
	}
	fprintf(stderr, "Pruning table generated!\n");
	
	genptable_setbase(coord);
	if (compact)
		genptable_compress(coord);

	if (!write_ptable_file(coord))
		fprintf(stderr, "Error writing ptable file\n");
}

static void
genptable_bfs(Coordinate *coord, int d)
{
	uint64_t i, ii;
	int pval;
	Move m;

	for (i = 0; i < coord->max; i++) {
		pval = ptableval(coord, i);
		if (pval != d)
			continue;
		for (m = U; m <= B3; m++) {
			if (!coord->moveset(m))
				continue;
			ii = move_coord(coord, m, i, NULL);
			ptable_update(coord, ii, d+1);
			fixnasty(coord, ii, d+1);
		}
	}
}

static void
fixnasty(Coordinate *coord, uint64_t i, int d)
{
	uint64_t ii, ss, M;
	int j;
	Trans t;

	if (coord->type != SYMCOMP_COORD)
		return;

	M = coord->base[1]->max;
	ss = coord->base[0]->selfsim[i/M];
	for (j = 0; j < coord->base[0]->tgrp->n; j++) {
		t = coord->base[0]->tgrp->t[j];
		if (t == uf || !(ss & ((uint64_t)1<<t)))
			continue;
		ii = trans_coord(coord, t, i);
		ptable_update(coord, ii, d);
	}
}

static void
genptable_compress(Coordinate *coord)
{
	int val;
	uint64_t i, j;
	entry_group_t mask, v;

	fprintf(stderr, "Compressing table to 2 bits per entry\n");

	for (i = 0; i < coord->max; i += ENTRIES_PER_GROUP_COMPACT) {
		mask = (entry_group_t)0;
		for (j = 0; j < ENTRIES_PER_GROUP_COMPACT; j++) {
			if (i+j >= coord->max)
				break;
			val = ptableval(coord, i+j) - coord->ptablebase;
			v = (entry_group_t)MIN(3, MAX(0, val));
			mask |= v << (2*j);
		}
		coord->ptable[i/ENTRIES_PER_GROUP_COMPACT] = mask;
	}

	coord->compact = true;
	coord->ptable = realloc(coord->ptable,
	    sizeof(entry_group_t)*ptablesize(coord));
}

static void
genptable_setbase(Coordinate *coord)
{
	int i;
	uint64_t sum, newsum;

	coord->ptablebase = 0;
	sum = coord->count[0] + coord->count[1] + coord->count[2];
	for (i = 3; i < 16; i++) {
		newsum = sum + coord->count[i] - coord->count[i-3];
		if (newsum > sum)
			coord->ptablebase = i-3;
		sum = newsum;
	}
}

static uint64_t
ptablesize(Coordinate *coord)
{
	uint64_t e;

	e = coord->compact ? ENTRIES_PER_GROUP_COMPACT : ENTRIES_PER_GROUP;

	return (coord->max + e - 1) / e;
}

static void
ptable_update(Coordinate *coord, uint64_t ind, int n)
{
	int sh;
	entry_group_t mask;
	uint64_t i;

	if (ptableval(coord, ind) <= n)
		return;

	sh = 4 * (ind % ENTRIES_PER_GROUP);
	mask = ((entry_group_t)15) << sh;
	i = ind/ENTRIES_PER_GROUP;
	coord->ptable[i] &= ~mask;
	coord->ptable[i] |= (((entry_group_t)n)&15) << sh;

	coord->updated++;
}

int
ptableval(Coordinate *coord, uint64_t ind)
{
	int ret, j, sh;
	uint64_t e, ii;

	if (coord->compact) {
		e  = ENTRIES_PER_GROUP_COMPACT;
		sh = (ind % e) * 2;
		ret = (coord->ptable[ind/e] & (3 << sh)) >> sh;
		if (ret != 0)
			return ret + coord->ptablebase;
		for (j = coord_base_number(coord); j >= 0; j--) {
			ii = ind % coord->base[j]->max;
			ret = MAX(ret, ptableval(coord->base[j], ii));
			ind /= coord->base[j]->max;
		}
		return ret;
	}

	e  = ENTRIES_PER_GROUP;
	sh = (ind % e) * 4;
	return (coord->ptable[ind/e] & (15 << sh)) >> sh;
}

static bool
read_ptable_file(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	int i;
	uint64_t r;

	strcpy(fname, "tables/pt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "rb")) == NULL)
		return false;

	r = fread(&(coord->ptablebase), sizeof(int), 1, f);
	for (i = 0; i < 16; i++)
		r += fread(&(coord->count[i]), sizeof(uint64_t), 1, f);
	r += fread(coord->ptable, sizeof(entry_group_t), ptablesize(coord), f);
	fclose(f);

	return r == 17 + ptablesize(coord);
}

static bool
write_ptable_file(Coordinate *coord)
{
	FILE *f;
	char fname[256];
	int i;
	uint64_t w;

	strcpy(fname, "tables/pt_");
	strcat(fname, coord->name);

	if ((f = fopen(fname, "wb")) == NULL)
		return false;

	w = fwrite(&(coord->ptablebase), sizeof(int), 1, f);
	for (i = 0; i < 16; i++)
		w += fwrite(&(coord->count[i]), sizeof(uint64_t), 1, f);
	w += fwrite(coord->ptable, sizeof(entry_group_t), ptablesize(coord), f);
	fclose(f);

	return w == 17 + ptablesize(coord);
}
