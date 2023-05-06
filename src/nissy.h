/* TODO: this should take a char *buffer as parameter, not read files */
void nissy_init(void);

/* Returns 0 on success, 1-based index of bad arg on failure */
int nissy_solve(char *s, char *trans, int d, char *type, char *scr, char *sol);
