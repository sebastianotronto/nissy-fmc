#ifndef PRUNING_H
#define PRUNING_H

#include "coord.h"

#define entry_group_t uint8_t

typedef bool (Moveset)(Move);
typedef struct {
	entry_group_t *           ptable;
	uint64_t                  n;
	Coordinate *              coord;
	Moveset *                 moveset;
	bool                      compact;
	int                       base;
	uint64_t                  count[16];
	uint64_t                  fbmod;
} PruneData;
typedef struct {
	Coordinate *              coord;
	Moveset *                 moveset;
	bool                      compact;
	PruneData *               pd;
} PDGenData;

bool        allowed_all(Move m);
bool        allowed_HTM(Move m);
bool        allowed_URF(Move m);
bool        allowed_eofb(Move m);
bool        allowed_drud(Move m);
bool        allowed_htr(Move m);

void        free_pd(PruneData *pd);
PruneData * genptable(PDGenData *data);
void        print_ptable(PruneData *pd);
uint64_t    ptablesize(PruneData *pd);
int         ptableval(PruneData *pd, uint64_t ind);

extern PDGenData *active_pdg[256];

#endif

