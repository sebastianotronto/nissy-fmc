#ifndef COORD_H
#define COORD_H

#include <inttypes.h>

#include "cube.h"
#include "env.h"

typedef enum { COMP_COORD, SYM_COORD, SYMCOMP_COORD } CoordType;
typedef struct {
	int n;
	uint64_t (*index)(Cube *);
	void (*to_cube)(uint64_t, Cube *);
} Indexer;
typedef struct coordinate {
	char *                    name;
	CoordType                 type;
	bool                      generated;
	Indexer *                 i[99];
	uint64_t                  max;
	uint64_t *                mtable[NMOVES];
	uint64_t *                ttable[NTRANS];
	TransGroup *              tgrp;
	struct coordinate *       base[2];
	uint64_t *                symclass;
	uint64_t *                symrep;
	Trans *                   transtorep;
	Trans *                   ttrep_move[NMOVES];
	uint64_t *                selfsim;
} Coordinate;

void                    gen_coord(Coordinate *coord);
uint64_t                index_coord(Coordinate *coord, Cube *cube,
                            Trans *offtrans);
uint64_t                move_coord(Coordinate *coord, Move m,
                            uint64_t ind, Trans *offtrans);
bool                    test_coord(Coordinate *coord);
uint64_t                trans_coord(Coordinate *coord, Trans t, uint64_t ind);

/* Base coordinates and their index functions ********************************/

#ifndef COORD_C

extern Coordinate coord_eofb;
extern Coordinate coord_coud;
extern Coordinate coord_cp;
extern Coordinate coord_epos;
extern Coordinate coord_epe;
extern Coordinate coord_eposepe;
extern Coordinate coord_epud;
extern Coordinate coord_eofbepos;
extern Coordinate coord_eofbepos_sym16;
extern Coordinate coord_drud_sym16;
extern Coordinate coord_cp_sym16;
extern Coordinate coord_drudfin_noE_sym16;

#else

/* Indexers ******************************************************************/

uint64_t index_eofb(Cube *cube);
void     invindex_eofb(uint64_t ind, Cube *ret);
Indexer
i_eofb = {
	.n       = POW2TO11,
	.index   = index_eofb,
	.to_cube = invindex_eofb,
};

uint64_t index_coud(Cube *cube);
void     invindex_coud(uint64_t ind, Cube *ret);
Indexer
i_coud = {
	.n       = POW3TO7,
	.index   = index_coud,
	.to_cube = invindex_coud,
};

uint64_t index_cp(Cube *cube);
void     invindex_cp(uint64_t ind, Cube *ret);
Indexer
i_cp = {
	.n       = FACTORIAL8,
	.index   = index_cp,
	.to_cube = invindex_cp,
};

uint64_t index_epos(Cube *cube);
void     invindex_epos(uint64_t ind, Cube *ret);
Indexer
i_epos = {
	.n       = BINOM12ON4,
	.index   = index_epos,
	.to_cube = invindex_epos,
};

uint64_t index_epe(Cube *cube);
void     invindex_epe(uint64_t ind, Cube *ret);
Indexer
i_epe = {
	.n       = FACTORIAL4,
	.index   = index_epe,
	.to_cube = invindex_epe,
};

uint64_t index_eposepe(Cube *cube);
void     invindex_eposepe(uint64_t ind, Cube *ret);
Indexer
i_eposepe = {
	.n       = BINOM12ON4 * FACTORIAL4,
	.index   = index_eposepe,
	.to_cube = invindex_eposepe,
};

uint64_t index_epud(Cube *cube);
void     invindex_epud(uint64_t ind, Cube *ret);
Indexer
i_epud = {
	.n       = FACTORIAL8,
	.index   = index_epud,
	.to_cube = invindex_epud,
};

/* Composite coordinates *****************************************************/

Coordinate
coord_eofb = {
	.name = "eofb",
	.type = COMP_COORD,
	.i    = {&i_eofb, NULL},
};

Coordinate
coord_coud = {
	.name = "coud",
	.type = COMP_COORD,
	.i    = {&i_coud, NULL},
};

Coordinate
coord_cp = {
	.name = "cp",
	.type = COMP_COORD,
	.i    = {&i_cp, NULL},
};

Coordinate
coord_epos = {
	.name = "epos",
	.type = COMP_COORD,
	.i    = {&i_epos, NULL},
};

Coordinate
coord_epe = {
	.name = "epe",
	.type = COMP_COORD,
	.i    = {&i_epe, NULL},
};

Coordinate
coord_eposepe = {
	.name = "eposepe",
	.type = COMP_COORD,
	.i    = {&i_eposepe, NULL},
};

Coordinate
coord_epud = {
	.name = "epud",
	.type = COMP_COORD,
	.i    = {&i_epud, NULL},
};

Coordinate
coord_eofbepos = {
	.name = "eofbepos",
	.type = COMP_COORD,
	.i    = {&i_epos, &i_eofb, NULL},
};

/* Symcoordinates ************************************************************/

Coordinate
coord_eofbepos_sym16 = {
	.name = "eofbepos_sym16",
	.type = SYM_COORD,
	.base = {&coord_eofbepos, NULL},
	.tgrp = &tgrp_udfix,
};

Coordinate
coord_cp_sym16 = {
	.name = "cp_sym16",
	.type = SYM_COORD,
	.base = {&coord_cp, NULL},
	.tgrp = &tgrp_udfix,
};

/* "Symcomp" coordinates *****************************************************/

Coordinate
coord_drud_sym16 = {
	.name = "drud_sym16",
	.type = SYMCOMP_COORD,
	.base = {&coord_eofbepos_sym16, &coord_coud},
};

Coordinate
coord_drudfin_noE_sym16 = {
	.name = "drudfin_noE_sym16",
	.type = SYMCOMP_COORD,
	.base = {&coord_cp_sym16, &coord_epud},
};

#endif

#endif

