#define entry_group_t uint8_t

typedef bool (Moveset)(Move);
typedef enum { COMP_COORD, SYM_COORD, SYMCOMP_COORD } CoordType;
typedef struct {
	int n;
	uint64_t (*index)(Cube *);
	void (*to_cube)(uint64_t, Cube *);
} Indexer;
typedef struct coordinate {
	char *name;
	CoordType type;
	uint64_t max;
	uint64_t *mtable[NMOVES];
	uint64_t *ttable[NTRANS];
	TransGroup *tgrp;
	struct coordinate *base[2];
	uint64_t *symclass;
	uint64_t *symrep;
	Trans *transtorep;
	Trans *ttrep_move[NMOVES];

	bool generated;
	Indexer *i[99];
	uint64_t *selfsim; /* used only in fixnasty */

	Moveset *moveset; /* for pruning, mt and compressing */
	uint64_t updated;
	entry_group_t *ptable;
	int ptablebase; /* Renamed */
	bool compact; /* Only needed for generation, maybe keep */
	uint64_t count[16]; /* Only needed for generation and print */
} Coordinate;

void gen_coord(Coordinate *);
uint64_t index_coord(Coordinate *, Cube *, Trans *);
uint64_t move_coord(Coordinate *, Move, uint64_t, Trans *);
uint64_t trans_coord(Coordinate *, Trans, uint64_t);
int ptableval(Coordinate *, uint64_t);
