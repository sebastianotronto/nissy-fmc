#define ENTRIES_PER_GROUP         (2*sizeof(entry_group_t))
#define ENTRIES_PER_GROUP_COMPACT (4*sizeof(entry_group_t))

typedef uint8_t entry_group_t;
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
	uint64_t *selfsim;

	Moveset *moveset;
	uint64_t updated;
	entry_group_t *ptable;
	int8_t ptablebase;
	bool compact;
	uint64_t count[16];
} Coordinate;

uint64_t indexers_getind(Indexer **, Cube *);
uint64_t indexers_getmax(Indexer **);
void indexers_makecube(Indexer **, uint64_t, Cube *);

void alloc_sd(Coordinate *, bool);
void alloc_mtable(Coordinate *);
void alloc_ttrep_move(Coordinate *);
void alloc_ttable(Coordinate *);
void alloc_ptable(Coordinate *, bool);

uint64_t index_coord(Coordinate *, Cube *, Trans *);
uint64_t move_coord(Coordinate *, Move, uint64_t, Trans *);
uint64_t trans_coord(Coordinate *, Trans, uint64_t);

int ptableval(Coordinate *, uint64_t);
size_t ptablesize(Coordinate *);
void ptableupdate(Coordinate *, uint64_t, int);

size_t read_coord(Coordinate *, char *);
size_t write_coord(Coordinate *, char *);
