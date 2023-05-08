#define ENTRIES_PER_GROUP         (2*sizeof(entry_group_t))
#define ENTRIES_PER_GROUP_COMPACT (4*sizeof(entry_group_t))

typedef uint8_t entry_group_t;
typedef uint32_t coord_value_t;
typedef bool (Moveset)(Move);
typedef enum { COMP_COORD, SYM_COORD, SYMCOMP_COORD } CoordType;
typedef struct {
	int n;
	coord_value_t (*index)(Cube *);
	void (*to_cube)(coord_value_t, Cube *);
} Indexer;
typedef struct coordinate {
	char *name;
	CoordType type;
	coord_value_t max;
	coord_value_t *mtable[NMOVES_HTM];
	coord_value_t *ttable[NTRANS];
	TransGroup *tgrp;
	struct coordinate *base[2];

	coord_value_t *symclass;
	coord_value_t *symrep;
	Trans *transtorep;
	Trans *ttrep_move[NMOVES_HTM];

	bool generated;
	Indexer *i[99];
	coord_value_t *selfsim;

	Moveset *moveset;
	coord_value_t updated;
	entry_group_t *ptable;
	int8_t ptablebase;
	bool compact;
	coord_value_t count[16];
} Coordinate;

coord_value_t indexers_getind(Indexer **, Cube *);
coord_value_t indexers_getmax(Indexer **);
void indexers_makecube(Indexer **, coord_value_t, Cube *);

void alloc_sd(Coordinate *, bool);
void alloc_mtable(Coordinate *);
void alloc_ttrep_move(Coordinate *);
void alloc_ttable(Coordinate *);
void alloc_ptable(Coordinate *, bool);

coord_value_t index_coord(Coordinate *, Cube *, Trans *);
coord_value_t move_coord(Coordinate *, Move, coord_value_t, Trans *);
coord_value_t trans_coord(Coordinate *, Trans, coord_value_t);

int ptableval(Coordinate *, coord_value_t);
size_t ptablesize(Coordinate *);
void ptableupdate(Coordinate *, coord_value_t, int);

size_t read_coord(Coordinate *, char *);
size_t write_coord(Coordinate *, char *);
