#ifndef STEPS_H
#define STEPS_H

#include "coord.h"
#include "cube.h"

#define MAX_N_COORD 3

typedef struct {
	char *                    shortname;
	Moveset *                 moveset;
	Coordinate *              coord[MAX_N_COORD];
} Step;

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

extern Coordinate *coordinates[];

extern Step eofb_HTM;
extern Step drud_HTM;
extern Step drfin_drud;

extern Step *steps[];

#endif
