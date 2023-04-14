#include "cube.h"

static Cube move_array[NMOVES];
Move  moves_ttable[NTRANS][NMOVES];
Trans trans_ttable[NTRANS][NTRANS];
Trans trans_itable[NTRANS];

static char move_string[NMOVES][7] = {
	[NULLMOVE] = "-",
	[U]  = "U",  [U2]  = "U2",  [U3]  = "U\'",
	[D]  = "D",  [D2]  = "D2",  [D3]  = "D\'",
	[R]  = "R",  [R2]  = "R2",  [R3]  = "R\'",
	[L]  = "L",  [L2]  = "L2",  [L3]  = "L\'",
	[F]  = "F",  [F2]  = "F2",  [F3]  = "F\'",
	[B]  = "B",  [B2]  = "B2",  [B3]  = "B\'",
	[Uw] = "Uw", [Uw2] = "Uw2", [Uw3] = "Uw\'",
	[Dw] = "Dw", [Dw2] = "Dw2", [Dw3] = "Dw\'",
	[Rw] = "Rw", [Rw2] = "Rw2", [Rw3] = "Rw\'",
	[Lw] = "Lw", [Lw2] = "Lw2", [Lw3] = "Lw\'",
	[Fw] = "Fw", [Fw2] = "Fw2", [Fw3] = "Fw\'",
	[Bw] = "Bw", [Bw2] = "Bw2", [Bw3] = "Bw\'",
	[M]  = "M",  [M2]  = "M2",  [M3]  = "M\'",
	[E]  = "E",  [E2]  = "E2",  [E3]  = "E\'",
	[S]  = "S",  [S2]  = "S2",  [S3]  = "S\'",
	[x]  = "x",  [x2]  = "x2",  [x3]  = "x\'",
	[y]  = "y",  [y2]  = "y2",  [y3]  = "y\'",
	[z]  = "z",  [z2]  = "z2",  [z3]  = "z\'",
};
static char rotation_string[100][NTRANS/2] = {
	[uf] = "",     [ur] = "y",    [ub] = "y2",    [ul] = "y3",
	[df] = "z2",   [dr] = "y z2", [db] = "x2",    [dl] = "y3 z2",
	[rf] = "z3",   [rd] = "z3 y", [rb] = "z3 y2", [ru] = "z3 y3",
	[lf] = "z",    [ld] = "z y3", [lb] = "z y2",  [lu] = "z y",
	[fu] = "x y2", [fr] = "x y",  [fd] = "x",     [fl] = "x y3",
	[bu] = "x3",   [br] = "x3 y", [bd] = "x3 y2", [bl] = "x3 y3",
};

TransGroup
tgrp_udfix = {
	.n = 16,
	.t = {
	    uf, ur, ub, ul,
	    df, dr, db, dl,
	    uf_mirror, ur_mirror, ub_mirror, ul_mirror,
	    df_mirror, dr_mirror, db_mirror, dl_mirror
	},
};

static void
apply_permutation(int *perm, int *set, int n, int *aux)
{
	int i;

	for (i = 0; i < n; i++)
		aux[i] = set[perm[i]];

	memcpy(set, aux, n * sizeof(int));
}

static void
sum_arrays_mod(int *src, int *dst, int n, int m)
{
	int i;

	for (i = 0; i < n; i++)
		dst[i] = (src[i] + dst[i]) % m;
}

void
compose(Cube *c2, Cube *c1)
{
	int aux[12];

	apply_permutation(c2->ep, c1->ep, 12, aux);
	apply_permutation(c2->ep, c1->eo, 12, aux);
	sum_arrays_mod(c2->eo, c1->eo, 12, 2);

	apply_permutation(c2->cp, c1->cp, 8, aux);
	apply_permutation(c2->cp, c1->co, 8, aux);

	sum_arrays_mod(c2->co, c1->co, 8, 3);
}

void
copy_cube(Cube *src, Cube *dst)
{
	memcpy(dst->ep, src->ep, 12 * sizeof(int));
	memcpy(dst->eo, src->eo, 12 * sizeof(int));
	memcpy(dst->cp, src->cp,  8 * sizeof(int));
	memcpy(dst->co, src->co,  8 * sizeof(int));
}

void
invert_cube(Cube *cube)
{
	Cube aux;
	int i;

	copy_cube(cube, &aux);

	for (i = 0; i < 12; i++) {
		cube->ep[aux.ep[i]] = i;
		cube->eo[aux.ep[i]] = aux.eo[i];
	}

	for (i = 0; i < 8; i++) {
		cube->cp[aux.cp[i]] = i;
		cube->co[aux.cp[i]] = (3 - aux.co[i]) % 3;
	}
}

bool
is_solved(Cube *cube)
{
	int i;

	for (i = 0; i < 12; i++)
		if (cube->eo[i] != 0 || cube->ep[i] != i)
			return false;

	for (i = 0; i < 8; i++)
		if (cube->co[i] != 0 || cube->cp[i] != i)
			return false;

	return true;
}

void
make_solved(Cube *cube)
{
	static int sorted[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

	memcpy(cube->ep, sorted, 12 * sizeof(int));
	memset(cube->eo, 0,      12 * sizeof(int));
	memcpy(cube->cp, sorted,  8 * sizeof(int));
	memset(cube->co, 0,       8 * sizeof(int));
}

Move
base_move(Move m)
{
	return m == NULLMOVE ? NULLMOVE : m - (m-1)%3;
}

Move
inverse_move(Move m)
{
	return m == NULLMOVE ? NULLMOVE : m + 2 - 2*((m-1) % 3);
}

bool
commute(Move m1, Move m2)
{
	if (m1 > B3 || m2 > B3) {
		fprintf(stderr, "Unexpected commute for non-HTM\n");
		return false;
	}

	if (m1 == NULLMOVE || m2 == NULLMOVE)
		return m1 == m2;

	return (m1-1)/6 == (m2-1)/6;
}

void
copy_alg(Alg *src, Alg *dst)
{
	int i;

	dst->len = src->len;
	for (i = 0; i < src->len; i++) {
		dst->move[i] = src->move[i];
		dst->inv[i] = src->inv[i];
	}
}

void
append_move(Alg *alg, Move m, bool inverse)
{
	alg->move[alg->len] = m;
	alg->inv[alg->len] = inverse;
	alg->len++;
}

void
apply_move(Move m, Cube *cube)
{
	compose(&move_array[m], cube);
}

void
apply_alg(Alg *alg, Cube *cube)
{
	Cube aux;
	int i;

	copy_cube(cube, &aux);
	make_solved(cube);

	for (i = 0; i < alg->len; i++)
		if (alg->inv[i])
			apply_move(alg->move[i], cube);

	invert_cube(cube);
	compose(&aux, cube);

	for (i = 0; i < alg->len; i++)
		if (!alg->inv[i])
			apply_move(alg->move[i], cube);
}

static Move
read_move(char *str, int *i)
{
	Move j, m;
	char upper, lower, inv;

	for (j = U; j < NMOVES; j++) {
		upper = move_string[j][0];
		lower = upper - ('A' - 'a');
		if (str[*i] == upper || (str[*i] == lower && j <= B)) {
			m = j;
			if (str[*i] == lower)
				m += Uw - U;
			if (m <= B && str[*i+1] == 'w') {
				m += Uw - U;
				*i += 1;
			}
			if (str[*i+1]=='2') {
				m += 1;
				*i += 1;
			}
			inv = str[*i+1] == '\'' ||
			      str[*i+1] == '3'  ||
			      str[*i+1] == '`';
			/* TODO: add weird apostrophes */
			if (inv) {
				m += 2;
				*i += 1;
			}
			return m;
		}
	}

	return NULLMOVE;
}

bool 
apply_scramble(char *str, Cube *c)
{
	int i;
	bool niss;
	Cube normal, inverse;
	Move move;

	niss = false;
	make_solved(&normal);
	make_solved(&inverse);
	for (i = 0; str[i]; i++) {
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n')
			continue;

		if ((str[i] == '(' && niss) || (str[i] == ')' && !niss)) {
			fprintf(stderr,
			    "Error reading moves: unmatched %c\n", str[i]);
			return false;
		}

		if (str[i] == '(' || str[i] == ')') {
			niss = !niss;
			continue;
		}

		/* Single slash for comments */
		if (str[i] == '/') {
			while (str[i] && str[i] != '\n') i++;
			if (!str[i]) i--;
			continue;
		}

		move = read_move(str, &i);
		if (move == NULLMOVE)
			return false;
		apply_move(move, niss ? &inverse : &normal);
	}

	if (niss) {
		fprintf(stderr, "Error reading moves: unmatched (\n");
		return false;
	}

	invert_cube(&inverse);
	compose(c, &inverse);
	copy_cube(&inverse, c);
	compose(&normal, c);

	return true;
}

int
alg_string(Alg *alg, char *str)
{
	int i, n;
	bool niss;
	Move m;

	niss = false;
	for (i = 0, n = 0; i < alg->len; i++) {
		if (!niss && alg->inv[i]) {
			if (i != 0)
				str[n++] = ' ';
			str[n++] = '(';
		}
		if (niss && !alg->inv[i]) {
			str[n++] = ')';
			str[n++] = ' ';
		}
		if (niss == alg->inv[i] && i != 0)
			str[n++] = ' ';
		m = alg->move[i];
		str[n++] = move_string[m][0];
		if (move_string[m][1])
			str[n++] = move_string[m][1];
		niss = alg->inv[i];
	}

	if (niss)
		str[n++] = ')';
	str[n] = 0;

	return n;
}

void
apply_trans(Trans t, Cube *cube)
{
	Cube aux, inv;
	int i;

	static Cube mirror_cube = {
	.ep = { [UF] = UF, [UL] = UR, [UB] = UB, [UR] = UL,
		[DF] = DF, [DL] = DR, [DB] = DB, [DR] = DL,
		[FR] = FL, [FL] = FR, [BL] = BR, [BR] = BL },
	.cp = { [UFR] = UFL, [UFL] = UFR, [UBL] = UBR, [UBR] = UBL,
		[DFR] = DFL, [DFL] = DFR, [DBL] = DBR, [DBR] = DBL },
	};

	copy_cube(cube, &aux);
	make_solved(cube);

	if (t >= NTRANS/2)
		compose(&mirror_cube, cube);
	make_solved(&inv);
	apply_scramble(rotation_string[t % (NTRANS/2)], &inv);
	invert_cube(&inv);
	compose(&inv, cube);
	compose(&aux, cube);
	apply_scramble(rotation_string[t % (NTRANS/2)], cube);
	if (t >= NTRANS/2) {
		compose(&mirror_cube, cube);
		for (i = 0; i < 8; i++)
			cube->co[i] = (3 - cube->co[i]) % 3;
	}
}

Trans
inverse_trans(Trans t)
{
	return trans_itable[t];
}

void
transform_alg(Trans t, Alg *alg)
{
	int i;

	for (i = 0; i < alg->len; i++)
		alg->move[i] = transform_move(t, alg->move[i]);
}

Move
transform_move(Trans t, Move m)
{
	return moves_ttable[t][m];
}

Trans
transform_trans(Trans t, Trans m)
{
	return trans_ttable[t][m];
}

/* Initialization ************************************************************/

static void
init_moves() {
	Move m;

	/* Moves are represented as cubes and applied using compose().
	 * Every move is translated to a an <U, x, y> alg before filling
	 * the transition tables. */
	char equiv_alg_string[100][NMOVES] = {
		[NULLMOVE] = "",

		[U]   = "        U           ",
		[U2]  = "        UU          ",
		[U3]  = "        UUU         ",
		[D]   = "  xx    U    xx      ",
		[D2]  = "  xx    UU   xx      ",
		[D3]  = "  xx    UUU  xx      ",
		[R]   = "  yx    U    xxxyyy  ",
		[R2]  = "  yx    UU   xxxyyy  ",
		[R3]  = "  yx    UUU  xxxyyy  ",
		[L]   = "  yyyx  U    xxxy    ",
		[L2]  = "  yyyx  UU   xxxy    ",
		[L3]  = "  yyyx  UUU  xxxy    ",
		[F]   = "  x     U    xxx     ",
		[F2]  = "  x     UU   xxx     ",
		[F3]  = "  x     UUU  xxx     ",
		[B]   = "  xxx   U    x       ",
		[B2]  = "  xxx   UU   x       ",
		[B3]  = "  xxx   UUU  x       ",

		[Uw]  = "  xx    U    xx      y        ",
		[Uw2] = "  xx    UU   xx      yy       ",
		[Uw3] = "  xx    UUU  xx      yyy      ",
		[Dw]  = "        U            yyy      ",
		[Dw2] = "        UU           yy       ",
		[Dw3] = "        UUU          y        ",
		[Rw]  = "  yyyx  U    xxxy    x        ",
		[Rw2] = "  yyyx  UU   xxxy    xx       ",
		[Rw3] = "  yyyx  UUU  xxxy    xxx      ",
		[Lw]  = "  yx    U    xxxyyy  xxx      ",
		[Lw2] = "  yx    UU   xxxyyy  xx       ",
		[Lw3] = "  yx    UUU  xxxyyy  x        ",
		[Fw]  = "  xxx   U    x       yxxxyyy  ",
		[Fw2] = "  xxx   UU   x       yxxyyy   ",
		[Fw3] = "  xxx   UUU  x       yxyyy    ",
		[Bw]  = "  x     U    xxx     yxyyy    ",
		[Bw2] = "  x     UU   xxx     yxxyyy   ",
		[Bw3] = "  x     UUU  xxx     yxxxyyy  ",

		[M]   = "  yx  U    xx  UUU  yxyyy  ",
		[M2]  = "  yx  UU   xx  UU   xxxy   ",
		[M3]  = "  yx  UUU  xx  U    yxxxy  ",
		[S]   = "  x   UUU  xx  U    yyyx   ",
		[S2]  = "  x   UU   xx  UU   yyx    ",
		[S3]  = "  x   U    xx  UUU  yx     ",
		[E]   = "      U    xx  UUU  xxyyy  ",
		[E2]  = "      UU   xx  UU   xxyy   ",
		[E3]  = "      UUU  xx  U    xxy    ",

		[x]   = "       x         ",
		[x2]  = "       xx        ",
		[x3]  = "       xxx       ",
		[y]   = "       y         ",
		[y2]  = "       yy        ",
		[y3]  = "       yyy       ",
		[z]   = "  yyy  x    y    ",
		[z2]  = "  yy   xx        ",
		[z3]  = "  y    x    yyy  "
	};

	const Cube mcu = {
		.ep = { UR, UF, UL, UB, DF, DL, DB, DR, FR, FL, BL, BR },
		.cp = { UBR, UFR, UFL, UBL, DFR, DFL, DBL, DBR },
	};

	const Cube mcx = {
		.ep = { DF, FL, UF, FR, DB, BL, UB, BR, DR, DL, UL, UR },
		.eo = { [UF] = 1, [UB] = 1, [DF] = 1, [DB] = 1 },
		.cp = { DFR, DFL, UFL, UFR, DBR, DBL, UBL, UBR },
		.co = { [UFR] = 2, [UBR] = 1, [UFL] = 1, [UBL] = 2,
			[DBR] = 2, [DFR] = 1, [DBL] = 1, [DFL] = 2 },
	};

	const Cube mcy = {
		.ep = { UR, UF, UL, UB, DR, DF, DL, DB, BR, FR, FL, BL },
		.eo = { [FR] = 1, [FL] = 1, [BL] = 1, [BR] = 1 },
		.cp = { UBR, UFR, UFL, UBL, DBR, DFR, DFL, DBL },
	};

	move_array[U] = mcu;
	move_array[x] = mcx;
	move_array[y] = mcy;

	for (m = 0; m < NMOVES; m++) {
		switch (m) {
		case NULLMOVE:
			make_solved(&move_array[m]);
			break;
		case U:
		case x:
		case y:
			break;
		default:
			make_solved(&move_array[m]);
			apply_scramble(equiv_alg_string[m], &move_array[m]);
			break;
		}
	}
}

static void
init_trans() {
	Cube aux, cube;
	Move mi, move;
	Trans t, u, v;

	for (t = 0; t < NTRANS; t++) {
		for (mi = 0; mi < NMOVES; mi++) {
			make_solved(&aux);
			apply_move(mi, &aux);
			apply_trans(t, &aux);
			for (move = 0; move < NMOVES; move++) {
				copy_cube(&aux, &cube);
				apply_move(inverse_move(move), &cube);
				if (is_solved(&cube)) {
					moves_ttable[t][mi] = move;
					break;
				}
			}
		}
	}

	for (t = 0; t < NTRANS; t++) {
		for (u = 0; u < NTRANS; u++) {
			make_solved(&aux);
			apply_scramble("R' U' F", &aux);
			apply_trans(u, &aux);
			apply_trans(t, &aux);
			for (v = 0; v < NTRANS; v++) {
				copy_cube(&aux, &cube);
				apply_trans(v, &cube);
				apply_scramble("F' U R", &cube);
				if (is_solved(&cube)) {
					/* This is the inverse of the correct
					   value, it will be inverted later */
					trans_ttable[t][u] = v;
					if (v == uf)
						trans_itable[t] = u;
					break;
				}
			}
		}
	}
	for (t = 0; t < NTRANS; t++)
		for (u = 0; u < NTRANS; u++)
			trans_ttable[t][u] = trans_itable[trans_ttable[t][u]];
}

void
init_cube()
{
	init_moves();
	init_trans();
}
