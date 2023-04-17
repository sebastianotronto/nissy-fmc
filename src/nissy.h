/* TODO: this should take a char *buffer as parameter, not read files */
void init_nissy();

/* Returns 0 on success, 1-based index of bad arg on failure */
int solve(char *step, char *trans, int d, char *type, char *scr, char *sol);
