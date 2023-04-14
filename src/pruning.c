#define PRUNING_C

#include "pruning.h"

#define ENTRIES_PER_GROUP              (2*sizeof(entry_group_t))
#define ENTRIES_PER_GROUP_COMPACT      (4*sizeof(entry_group_t))

static void        genptable_bfs(PruneData *pd, int d);
static void        fixnasty(PruneData *pd, uint64_t i, int d);
static void        genptable_compress(PruneData *pd);
static void        genptable_setbase(PruneData *pd);
static void        ptable_update(PruneData *pd, uint64_t ind, int m);
static bool        read_ptable_file(PruneData *pd);
static bool        write_ptable_file(PruneData *pd);

PDGenData *active_pdg[256];

bool
allowed_HTM(Move m)
{
	return m >= U && m <= B3;
}

bool
allowed_URF(Move m)
{
	Move b = base_move(m);

	return b == U || b == R || b == F;
}

bool
allowed_eofb(Move m)
{
	Move b = base_move(m);

	return b == U || b == D || b == R || b == L ||
		((b == F || b == B) && m == b+1);
}

bool
allowed_drud(Move m)
{
	Move b = base_move(m);

	return b == U || b == D ||
		((b == R || b == L || b == F || b == B) && m == b + 1);
}

bool
allowed_htr(Move m)
{
	return allowed_HTM(m) && m == base_move(m) + 1;
}

PruneData *
genptable(PDGenData *pdg)
{
	bool compact;
	int d, i;
	uint64_t oldn, sz;
	PruneData *pd;

	for (i = 0; active_pdg[i] != NULL; i++) {
		pd = active_pdg[i]->pd;
		if (pd->coord == pdg->coord &&
		    pd->moveset == pdg->moveset &&
		    pd->compact == pdg->compact)
			return pd;
	}

	pd = malloc(sizeof(PruneData));
	pdg->pd = pd;
	pd->coord   = pdg->coord;
	pd->moveset = pdg->moveset;
	pd->compact = pdg->compact;

	sz = ptablesize(pd) * (pd->compact ? 2 : 1);
	pd->ptable = malloc(sz * sizeof(entry_group_t));

	gen_coord(pd->coord);

	if (read_ptable_file(pd))
		goto genptable_done;

	/* For the first steps we proceed the same way for compact and not */
	compact = pd->compact;
	pd->compact = false;

	fprintf(stderr, "Generating pt_%s\n", pd->coord->name); 

	memset(pd->ptable, ~(uint8_t)0, ptablesize(pd)*sizeof(entry_group_t));
	for (i = 0; i < 16; i++)
		pd->count[i] = 0;

	ptable_update(pd, 0, 0);
	pd->n = 1;
	oldn = 0;
	fixnasty(pd, 0, 0);
	fprintf(stderr, "Depth %d done, generated %"
		PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
		0, pd->n - oldn, pd->n, pd->coord->max);
	oldn = pd->n;
	pd->count[0] = pd->n;
	for (d = 0; d < 15 && pd->n < pd->coord->max; d++) {
		genptable_bfs(pd, d);
		fprintf(stderr, "Depth %d done, generated %"
			PRIu64 "\t(%" PRIu64 "/%" PRIu64 ")\n",
			d+1, pd->n - oldn, pd->n, pd->coord->max);
		pd->count[d+1] = pd->n - oldn;
		oldn = pd->n;
	}
	fprintf(stderr, "Pruning table generated!\n");
	
	genptable_setbase(pd);
	if (compact)
		genptable_compress(pd);

	if (!write_ptable_file(pd))
		fprintf(stderr, "Error writing ptable file\n");

genptable_done:
	for (i = 0; active_pdg[i] != NULL; i++);
	active_pdg[i] = malloc(sizeof(PDGenData));
	active_pdg[i]->coord   = pdg->coord;
	active_pdg[i]->moveset = pdg->moveset;
	active_pdg[i]->compact = pdg->compact;
	active_pdg[i]->pd      = pd;

	return pd;
}

static void
genptable_bfs(PruneData *pd, int d)
{
	uint64_t i, ii;
	int pval;
	Move m;

	for (i = 0; i < pd->coord->max; i++) {
		pval = ptableval(pd, i);
		if (pval != d)
			continue;
		for (m = U; m <= B3; m++) {
			if (!pd->moveset(m))
				continue;
			ii = move_coord(pd->coord, m, i, NULL);
			ptable_update(pd, ii, d+1);
			fixnasty(pd, ii, d+1);
		}
	}
}

static void
fixnasty(PruneData *pd, uint64_t i, int d)
{
	uint64_t ii, ss, M;
	int j;
	Trans t;

	if (pd->coord->type != SYMCOMP_COORD)
		return;

	M = pd->coord->base[1]->max;
	ss = pd->coord->base[0]->selfsim[i/M];
	for (j = 0; j < pd->coord->base[0]->tgrp->n; j++) {
		t = pd->coord->base[0]->tgrp->t[j];
		if (t == uf || !(ss & ((uint64_t)1<<t)))
			continue;
		ii = trans_coord(pd->coord, t, i);
		ptable_update(pd, ii, d);
	}
}

static void
genptable_compress(PruneData *pd)
{
	int val;
	uint64_t i, j;
	entry_group_t mask, v;

	fprintf(stderr, "Compressing table to 2 bits per entry\n");

	for (i = 0; i < pd->coord->max; i += ENTRIES_PER_GROUP_COMPACT) {
		mask = (entry_group_t)0;
		for (j = 0; j < ENTRIES_PER_GROUP_COMPACT; j++) {
			if (i+j >= pd->coord->max)
				break;
			val = ptableval(pd, i+j) - pd->base;
			v = (entry_group_t)MIN(3, MAX(0, val));
			mask |= v << (2*j);
		}
		pd->ptable[i/ENTRIES_PER_GROUP_COMPACT] = mask;
	}

	pd->compact = true;
	pd->ptable = realloc(pd->ptable, sizeof(entry_group_t)*ptablesize(pd));
}

static void
genptable_setbase(PruneData *pd)
{
	int i;
	uint64_t sum, newsum;

	pd->base = 0;
	sum = pd->count[0] + pd->count[1] + pd->count[2];
	for (i = 3; i < 16; i++) {
		newsum = sum + pd->count[i] - pd->count[i-3];
		if (newsum > sum)
			pd->base = i-3;
		sum = newsum;
	}
}

void
print_ptable(PruneData *pd)
{
	uint64_t i;

	printf("Table %s\n", pd->coord->name);
	printf("Base value: %d\n", pd->base);
	for (i = 0; i < 16; i++)
		printf("%2" PRIu64 "\t%10" PRIu64 "\n", i, pd->count[i]);
}

uint64_t
ptablesize(PruneData *pd)
{
	uint64_t e;

	e = pd->compact ? ENTRIES_PER_GROUP_COMPACT : ENTRIES_PER_GROUP;

	return (pd->coord->max + e - 1) / e;
}

static void
ptable_update(PruneData *pd, uint64_t ind, int n)
{
	int sh;
	entry_group_t mask;
	uint64_t i;

	if (ptableval(pd, ind) <= n)
		return;

	sh = 4 * (ind % ENTRIES_PER_GROUP);
	mask = ((entry_group_t)15) << sh;
	i = ind/ENTRIES_PER_GROUP;
	pd->ptable[i] &= ~mask;
	pd->ptable[i] |= (((entry_group_t)n)&15) << sh;
	pd->n++;
}

int
ptableval(PruneData *pd, uint64_t ind)
{
	int sh, ret;
	uint64_t e;
	entry_group_t m;

	if (pd->compact) {
		e  = ENTRIES_PER_GROUP_COMPACT;
		m  = 3;
		sh = (ind % e) * 2;
	} else {
		e  = ENTRIES_PER_GROUP;
		m  = 15;
		sh = (ind % e) * 4;
	}

	ret = (pd->ptable[ind/e] & (m << sh)) >> sh;

	return pd->compact ? ret + pd->base : ret;
}

static bool
read_ptable_file(PruneData *pd)
{
	init_env();

	FILE *f;
	char fname[strlen(tabledir)+256];
	int i;
	uint64_t r;

	strcpy(fname, tabledir);
	strcat(fname, "/pt_");
	strcat(fname, pd->coord->name);

	if ((f = fopen(fname, "rb")) == NULL)
		return false;

	r = fread(&(pd->base), sizeof(int), 1, f);
	for (i = 0; i < 16; i++)
		r += fread(&(pd->count[i]), sizeof(uint64_t), 1, f);
	r += fread(pd->ptable, sizeof(entry_group_t), ptablesize(pd), f);

	fclose(f);

	return r == 17 + ptablesize(pd);
}

static bool
write_ptable_file(PruneData *pd)
{
	init_env();

	FILE *f;
	char fname[strlen(tabledir)+256];
	int i;
	uint64_t w;

	strcpy(fname, tabledir);
	strcat(fname, "/pt_");
	strcat(fname, pd->coord->name);

	if ((f = fopen(fname, "wb")) == NULL)
		return false;

	w = fwrite(&(pd->base), sizeof(int), 1, f);
	for (i = 0; i < 16; i++)
		w += fwrite(&(pd->count[i]), sizeof(uint64_t), 1, f);
	w += fwrite(pd->ptable, sizeof(entry_group_t), ptablesize(pd), f);
	fclose(f);

	return w == 17 + ptablesize(pd);
}

