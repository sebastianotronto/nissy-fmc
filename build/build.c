#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/cube.h"
#include "../src/coord.h"
#include "../src/solve.h"
#include "../src/steps.h"
#include "../src/nissy.h"

static void gen_coord_comp(Coordinate *);
static void gen_coord_sym(Coordinate *);
static void gen_ptable(Coordinate *);
static void gen_ptable_bfs(Coordinate *, int);
static void gen_ptable_fixnasty(Coordinate *, uint64_t, int);
static void gen_ptable_compress(Coordinate *);
static void gen_ptable_setbase(Coordinate *);

static char buf[TABLESFILESIZE];

static void
gen_coord_comp(Coordinate *coord)
{
	uint64_t ui;
	Cube c, mvd;
	Move m;
	Trans t;

	fprintf(stderr, "%s: generating COMP coordinate\n", coord->name);

	coord->max = indexers_getmax(coord->i);

	fprintf(stderr, "%s: size is %" PRIu64 "\n", coord->name, coord->max);

	fprintf(stderr, "%s: generating mtable\n", coord->name);
	alloc_mtable(coord);
	for (ui = 0; ui < coord->max; ui++) {
		if (ui % 100000 == 0)
			fprintf(stderr, "\t(%" PRIu64 " done)\n", ui);
		indexers_makecube(coord->i, ui, &c);
		for (m = 0; m < NMOVES_HTM; m++) {
			copy_cube(&c, &mvd);
			apply_move(m, &mvd);
			coord->mtable[m][ui] = indexers_getind(coord->i, &mvd);
		}
	}
	fprintf(stderr, "\t(%" PRIu64 " done)\n", coord->max);

	fprintf(stderr, "%s: generating ttable\n", coord->name);
	alloc_ttable(coord);
	for (ui = 0; ui < coord->max; ui++) {
		if (ui % 100000 == 0)
			fprintf(stderr, "\t(%" PRIu64 " done)\n", ui);
		indexers_makecube(coord->i, ui, &c);
		for (t = 0; t < NTRANS; t++) {
			copy_cube(&c, &mvd);
			apply_trans(t, &mvd);
			coord->ttable[t][ui] = indexers_getind(coord->i, &mvd);
		}
	}
	fprintf(stderr, "\t(%" PRIu64 " done)\n", coord->max);
}

static void
gen_coord_sym(Coordinate *coord)
{
	uint64_t i, in, ui, uj, uu, nr;
	int j;
	Move m;
	Trans t;

	fprintf(stderr, "%s: generating SYM coordinate\n", coord->name);

	alloc_sd(coord, true);

	fprintf(stderr, "%s: generating symdata\n", coord->name);
	for (i = 0; i < coord->base[0]->max; i++)
		coord->symclass[i] = coord->base[0]->max + 1;
	for (i = 0, nr = 0; i < coord->base[0]->max; i++) {
		if (coord->symclass[i] != coord->base[0]->max + 1)
			continue;

		coord->symrep[nr] = i;
		coord->transtorep[i] = uf;
		coord->selfsim[nr] = (uint64_t)0;
		for (j = 0; j < coord->tgrp->n; j++) {
			t = coord->tgrp->t[j];
			in = trans_coord(coord->base[0], t, i);
			coord->symclass[in] = nr;
			if (in == i)
				coord->selfsim[nr] |= ((uint64_t)1<<t);
			else
				coord->transtorep[in] = inverse_trans(t);
		}
		nr++;
	}

	coord->max = nr;

	fprintf(stderr, "%s: number of classes is %" PRIu64 "\n",
	    coord->name, coord->max);

	/* Reallocating for maximum number of classes found */
	/* TODO: remove, not needed anymore because not writing to file */
	/*
	coord->symrep = realloc(coord->symrep, coord->max*sizeof(uint64_t));
	coord->selfsim = realloc(coord->selfsim, coord->max*sizeof(uint64_t));
	*/

	fprintf(stderr, "%s: generating mtable and ttrep_move\n", coord->name);
	alloc_mtable(coord);
	alloc_ttrep_move(coord);
	for (ui = 0; ui < coord->max; ui++) {
		if (ui % 100000 == 0)
			fprintf(stderr, "\t(%" PRIu64 " done)\n", ui);
		uu = coord->symrep[ui];
		for (m = 0; m < NMOVES_HTM; m++) {
			uj = move_coord(coord->base[0], m, uu, NULL);
			coord->mtable[m][ui] = coord->symclass[uj];
			coord->ttrep_move[m][ui] = coord->transtorep[uj];
		}
	}
	fprintf(stderr, "\t(%" PRIu64 " done)\n", coord->max);
}

void
gen_coord(Coordinate *coord)
{
	int i;

	if (coord == NULL || coord->generated)
		return;

	fprintf(stderr, "%s: gen_coord started\n", coord->name);
	for (i = 0; i < 2; i++) {
		if (coord->base[i] != NULL) {
			fprintf(stderr, "%s: generating base[%d] = %s\n",
			    coord->name, i, coord->base[i]->name);
			gen_coord(coord->base[i]);
		}
	}

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
	gen_ptable(coord);

	fprintf(stderr, "%s: gen_coord completed\n", coord->name);

	return;

error_gc:
	fprintf(stderr, "Error generating coordinates.\n");
	exit(1);
}

static void
gen_ptable(Coordinate *coord)
{
	bool compact;
	int d, i;
	uint64_t oldn, sz;

	alloc_ptable(coord, true);

	/* For the first steps we proceed the same way for compact and not */
	compact = coord->compact;
	coord->compact = false;

	fprintf(stderr, "%s: generating ptable\n", coord->name); 

	sz = ptablesize(coord) * sizeof(entry_group_t);
	memset(coord->ptable, ~(uint8_t)0, sz);
	for (i = 0; i < 16; i++)
		coord->count[i] = 0;

	coord->updated = 0;
	oldn = 0;
	ptableupdate(coord, 0, 0);
	gen_ptable_fixnasty(coord, 0, 0);
	fprintf(stderr, "\tDepth %d done, generated %"
		PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
		0, coord->updated - oldn, coord->updated, coord->max);
	oldn = coord->updated;
	coord->count[0] = coord->updated;
	for (d = 0; d < 15 && coord->updated < coord->max; d++) {
		gen_ptable_bfs(coord, d);
		fprintf(stderr, "\tDepth %d done, generated %"
			PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
			d+1, coord->updated-oldn, coord->updated, coord->max);
		coord->count[d+1] = coord->updated - oldn;
		oldn = coord->updated;
	}
	
	gen_ptable_setbase(coord);

	if (compact) {
		fprintf(stderr, "%s: compressing ptable\n", coord->name);
		gen_ptable_compress(coord);
	}

	fprintf(stderr, "%s: ptable generated\n", coord->name);
}

static void
gen_ptable_bfs(Coordinate *coord, int d)
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
			ptableupdate(coord, ii, d+1);
			gen_ptable_fixnasty(coord, ii, d+1);
		}
	}
}

static void
gen_ptable_fixnasty(Coordinate *coord, uint64_t i, int d)
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
		ptableupdate(coord, ii, d);
	}
}

static void
gen_ptable_compress(Coordinate *coord)
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

	/* TODO: remove, not necessary anymore */
	/*
	coord->ptable = realloc(coord->ptable,
	    sizeof(entry_group_t)*ptablesize(coord));
	*/
}

static void
gen_ptable_setbase(Coordinate *coord)
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

int
main(void)
{
	int i;
	size_t b;
	FILE *file;

	init_cube();

	b = 0;
	for (i = 0; coordinates[i] != NULL; i++) {
		gen_coord(coordinates[i]);
		b += write_coord(coordinates[i], &buf[b]);
	}

	if ((file = fopen("tables", "wb")) == NULL)
		return 1;

	if (fwrite(buf, 1, b, file) != b)
		return 1;

	return 0;
}
