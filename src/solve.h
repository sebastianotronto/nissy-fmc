#define MAX_N_COORD 3

typedef enum { NORMAL, INVERSE, NISS } SolutionType;
typedef struct { uint64_t val; Trans t; } CubeState;
typedef struct {
	char *shortname;
	Moveset *moveset;
	Coordinate *coord[MAX_N_COORD];
} Step;
typedef struct {
	Cube *cube;
	CubeState state[MAX_N_COORD];
	Step *s;
	Trans t;
	SolutionType st;
	char **sol;
	int d;
	bool niss;
	bool has_nissed;
	Move last[2];
	Move lastinv[2];
	Alg *current_alg;
} DfsArg;

int solve(Step *, Trans, int, SolutionType, Cube *, char *);
