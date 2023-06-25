/* TODO: find a better way to define this */
#define TABLESFILESIZE 700000 /* 700Kb */

/* Initialize nissy, to be called on startup */
void nissy_init(char *);

/* Test that nissy is responsive */
void nissy_test(char *);

/* Returns 0 on success, 1-based index of bad arg on failure */
int nissy_solve(
	char *step,  /* "eofb" */
	char *trans, /* "uf" or similar */
	int depth,   /* Number of moves */
	char *type,  /* "normal" or "inverse" or "niss" */
	char *scr,   /* The scramble */
	char *sol    /* The solution, as a single string, \n-separated */
);
