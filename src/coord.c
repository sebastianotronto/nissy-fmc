#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "cube.h"
#include "coord.h"

typedef void (Copier)(void *, void *, size_t);

static size_t copy_coord_sd(Coordinate *, char *, Copier *);
static size_t copy_coord_mtable(Coordinate *, char *, Copier *);
static size_t copy_coord_ttrep_move(Coordinate *, char *, Copier *);
static size_t copy_coord_ttable(Coordinate *, char *, Copier *);
static size_t copy_ptable(Coordinate *, char *, Copier *);

static void readin  (void *t, void *b, size_t n) { memcpy(t, b, n); }
static void writeout(void *t, void *b, size_t n) { memcpy(b, t, n); }

coord_value_t
indexers_getmax(Indexer **is)
{
	int i;
	coord_value_t max = 1;

	for (i = 0; is[i] != NULL; i++)
		max *= is[i]->n;

	return max;
}

coord_value_t
indexers_getind(Indexer **is, Cube *c)
{
	int i;
	coord_value_t max = 0;

	for (i = 0; is[i] != NULL; i++) {
		max *= is[i]->n;
		max += is[i]->index(c);
	}

	return max;
}

void
indexers_makecube(Indexer **is, coord_value_t ind, Cube *c)
{
	/* Warning: anti-indexers are applied in the same order as indexers. */
	/* We assume order does not matter, but it would make more sense to  */
	/* Apply them in reverse.                                            */

	int i;
	coord_value_t m;

	make_solved(c);
	m = indexers_getmax(is);
	for (i = 0; is[i] != NULL; i++) {
		m /= is[i]->n;
		is[i]->to_cube(ind / m, c);
		ind %= m;
	}
}

void
alloc_sd(Coordinate *coord, bool gen)
{
	size_t M;

	M = coord->base[0]->max;
	coord->symclass = malloc(M * sizeof(coord_value_t));
	coord->transtorep = malloc(M * sizeof(Trans));
	if (gen) {
		coord->selfsim = malloc(M * sizeof(coord_value_t));
		coord->symrep = malloc(M * sizeof(coord_value_t));
	}
}

void
alloc_mtable(Coordinate *coord)
{
	Move m;

	for (m = 0; m < NMOVES_HTM; m++)
		coord->mtable[m] = malloc(coord->max * sizeof(coord_value_t));
}

void
alloc_ttrep_move(Coordinate *coord)
{
	Move m;

	for (m = 0; m < NMOVES_HTM; m++)
		coord->ttrep_move[m] = malloc(coord->max * sizeof(Trans));
}

void
alloc_ttable(Coordinate *coord)
{
	Trans t;

	for (t = 0; t < NTRANS; t++)
		coord->ttable[t] = malloc(coord->max * sizeof(coord_value_t));
}

void
alloc_ptable(Coordinate *coord, bool gen)
{
	size_t sz;

	coord->compact = coord->base[1] != NULL;
	sz = ptablesize(coord) * (coord->compact && gen ? 2 : 1);
	coord->ptable = malloc(sz * sizeof(entry_group_t));
}

static size_t
copy_coord_mtable(Coordinate *coord, char *buf, Copier *copy)
{
	Move m;
	size_t b, rowsize;

	b = 0;
	rowsize = coord->max * sizeof(coord_value_t);
	for (m = 0; m < NMOVES_HTM; m++) {
		copy(coord->mtable[m], &buf[b], rowsize);
		b += rowsize;
	}

	return b;
}

static size_t
copy_coord_ttrep_move(Coordinate *coord, char *buf, Copier *copy)
{
	Move m;
	size_t b, rowsize;

	b = 0;
	rowsize = coord->max * sizeof(Trans);
	for (m = 0; m < NMOVES_HTM; m++) {
		copy(coord->ttrep_move[m], &buf[b], rowsize);
		b += rowsize;
	}

	return b;
}

static size_t
copy_coord_sd(Coordinate *coord, char *buf, Copier *copy)
{
	size_t b, size_max, rowsize_ttr, rowsize_symc;

	b = 0;

	size_max = sizeof(coord_value_t);
	copy(&coord->max, &buf[b], size_max);
	b += size_max;

	rowsize_ttr = coord->base[0]->max * sizeof(Trans);
	copy(coord->transtorep, &buf[b], rowsize_ttr);
	b += rowsize_ttr;

	rowsize_symc = coord->base[0]->max * sizeof(coord_value_t);
	copy(coord->symclass, &buf[b], rowsize_symc);
	b += rowsize_symc;

	return b;
}

static size_t
copy_coord_ttable(Coordinate *coord, char *buf, Copier *copy)
{
	Trans t;
	size_t b, rowsize;

	b = 0;
	rowsize = coord->max * sizeof(coord_value_t);
	for (t = 0; t < NTRANS; t++) {
		copy(coord->ttable[t], &buf[b], rowsize);
		b += rowsize;
	}

	return b;
}

static size_t
copy_ptable(Coordinate *coord, char *buf, Copier *copy)
{
	size_t b, size_base, size_count, size_ptable;

	b = 0;

	size_base = sizeof(coord->ptablebase);
	copy(&coord->ptablebase, &buf[b], size_base);
	b += size_base;

	size_count = 16 * sizeof(coord_value_t);
	copy(&coord->count, &buf[b], size_count);
	b += size_count;

	size_ptable = ptablesize(coord) * sizeof(entry_group_t);
	copy(coord->ptable, &buf[b], size_ptable);
	b += size_ptable;

	return b;
}

coord_value_t
index_coord(Coordinate *coord, Cube *cube, Trans *offtrans)
{
	coord_value_t c[2], cnosym;
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

coord_value_t
move_coord(Coordinate *coord, Move m, coord_value_t ind, Trans *offtrans)
{
	coord_value_t i[2], M;
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

coord_value_t
trans_coord(Coordinate *coord, Trans t, coord_value_t ind)
{
	coord_value_t i[2], M;

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

size_t
ptablesize(Coordinate *coord)
{
	coord_value_t e;

	e = coord->compact ? ENTRIES_PER_GROUP_COMPACT : ENTRIES_PER_GROUP;

	return (coord->max + e - 1) / e;
}

void
ptableupdate(Coordinate *coord, coord_value_t ind, int n)
{
	int sh;
	entry_group_t mask;
	coord_value_t i;

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
ptableval(Coordinate *coord, coord_value_t ind)
{
	int ret, j, sh;
	coord_value_t e, ii;

	if (coord->compact) {
		e  = ENTRIES_PER_GROUP_COMPACT;
		sh = (ind % e) * 2;
		ret = (coord->ptable[ind/e] & (3 << sh)) >> sh;
		if (ret != 0)
			return ret + coord->ptablebase;
		for (j = 0; j < 2 && coord->base[j] != NULL; j++) ;
		j--;
		for ( ; j >= 0; j--) {
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

static size_t
copy_coord(Coordinate *coord, char *buf, bool alloc, Copier *copy)
{
	size_t b;

	b = 0;
	switch (coord->type) {
	case COMP_COORD:
		if (alloc) {
			coord->max = indexers_getmax(coord->i);
			alloc_mtable(coord);
		}
		b += copy_coord_mtable(coord, &buf[b], copy);

		if (alloc)
			alloc_ttable(coord);
		b += copy_coord_ttable(coord, &buf[b], copy);

		if (alloc)
			alloc_ptable(coord, false);
		b += copy_ptable(coord, &buf[b], copy);

		break;
	case SYM_COORD:
		if (alloc) {
			coord->base[0]->max =
			    indexers_getmax(coord->base[0]->i);
			alloc_sd(coord, false);
		}
		b += copy_coord_sd(coord, &buf[b], copy);

		if (alloc)
			alloc_mtable(coord);
		b += copy_coord_mtable(coord, &buf[b], copy);

		if (alloc)
			alloc_ttrep_move(coord);
		b += copy_coord_ttrep_move(coord, &buf[b], copy);

		if (alloc)
			alloc_ptable(coord, false);
		b += copy_ptable(coord, &buf[b], copy);

		break;
	case SYMCOMP_COORD:
		if (alloc) {
			coord->max = coord->base[0]->max * coord->base[1]->max;
			alloc_ptable(coord, false);
		}
		b += copy_ptable(coord, &buf[b], copy);

		break;
	default:
		break;
	}

	return b;
}

size_t
read_coord(Coordinate *coord, char *buf)
{
	return copy_coord(coord, buf, true, readin);
}

size_t
write_coord(Coordinate *coord, char *buf)
{
	return copy_coord(coord, buf, false, writeout);
}
