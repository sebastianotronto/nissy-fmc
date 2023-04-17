#ifndef COORD_H
#define COORD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "cube.h"

#define entry_group_t uint8_t

typedef bool (Moveset)(Move);
typedef enum { COMP_COORD, SYM_COORD, SYMCOMP_COORD } CoordType;
typedef struct {
	int n;
	uint64_t (*index)(Cube *);
	void (*to_cube)(uint64_t, Cube *);
} Indexer;
typedef struct coordinate {
	char *                    name;
	CoordType                 type;
	uint64_t                  max;
	uint64_t *                mtable[NMOVES];
	uint64_t *                ttable[NTRANS];
	TransGroup *              tgrp;
	struct coordinate *       base[2];
	uint64_t *                symclass;
	uint64_t *                symrep;
	Trans *                   transtorep;
	Trans *                   ttrep_move[NMOVES];

	bool                      generated;
	Indexer *                 i[99];
	uint64_t *                selfsim; /* used only in fixnasty */

	Moveset *                 moveset; /* for pruning, mt and compressing */
	uint64_t                  updated;
	entry_group_t *           ptable;
	int                       ptablebase; /* Renamed */
	bool                      compact; /* Only needed for generation, maybe keep */
	uint64_t                  count[16]; /* Only needed for generation and print */
} Coordinate;

void                    gen_coord(Coordinate *coord);
uint64_t                index_coord(Coordinate *coord, Cube *cube,
                            Trans *offtrans);
uint64_t                move_coord(Coordinate *coord, Move m,
                            uint64_t ind, Trans *offtrans);
bool                    test_coord(Coordinate *coord);
uint64_t                trans_coord(Coordinate *coord, Trans t, uint64_t ind);

void                    print_ptable(Coordinate *coord);
int                     ptableval(Coordinate *coord, uint64_t ind);

#endif
