/* TODO: find a better way to define this */
#define TABLESFILESIZE 150000000 /* 150Mb */

void nissy_init(char *);

/* Returns 0 on success, 1-based index of bad arg on failure */
int nissy_solve(char *s, char *trans, int d, char *type, char *scr, char *sol);
