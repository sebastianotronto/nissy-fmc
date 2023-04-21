#define false               0
#define true                1
#define NMOVES              55
#define NTRANS              48
#define MAX_ALG_LEN         22
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))

typedef int bool;
typedef enum edge { UF, UL, UB, UR, DF, DL, DB, DR, FR, FL, BL, BR } Edge;
typedef enum { UFR, UFL, UBL, UBR, DFR, DFL, DBL, DBR } Corner;
typedef enum {
        NULLMOVE,
        U, U2, U3, D, D2, D3,
        R, R2, R3, L, L2, L3,
        F, F2, F3, B, B2, B3,
        Uw, Uw2, Uw3, Dw, Dw2, Dw3,
        Rw, Rw2, Rw3, Lw, Lw2, Lw3,
        Fw, Fw2, Fw3, Bw, Bw2, Bw3,
        M, M2, M3, S, S2, S3, E, E2, E3,
        x, x2, x3, y, y2, y3, z, z2, z3,
} Move;
typedef enum {
        uf, ur, ub, ul,
        df, dr, db, dl,
        rf, rd, rb, ru,
        lf, ld, lb, lu,
        fu, fr, fd, fl,
        bu, br, bd, bl,
        uf_mirror, ur_mirror, ub_mirror, ul_mirror,
        df_mirror, dr_mirror, db_mirror, dl_mirror,
        rf_mirror, rd_mirror, rb_mirror, ru_mirror,
        lf_mirror, ld_mirror, lb_mirror, lu_mirror,
        fu_mirror, fr_mirror, fd_mirror, fl_mirror,
        bu_mirror, br_mirror, bd_mirror, bl_mirror,
} Trans;

typedef struct { int ep[12]; int eo[12]; int cp[8]; int co[8]; } Cube;
typedef struct { Move move[MAX_ALG_LEN]; bool inv[MAX_ALG_LEN]; int len; } Alg;
typedef struct { int n; Trans t[NTRANS]; } TransGroup;

extern TransGroup tgrp_udfix;

void compose(Cube *c2, Cube *c1); /* Use c2 as an alg on c1 */
void copy_cube(Cube *src, Cube *dst);
void invert_cube(Cube *cube);
bool is_solved(Cube *cube);
void make_solved(Cube *cube);

Move base_move(Move m);
bool commute(Move m1, Move m2);
Move inverse_move(Move m);

void copy_alg(Alg *src, Alg *dst);
void append_move(Alg *alg, Move m, bool inverse);
void apply_move(Move m, Cube *cube);
void apply_alg(Alg *alg, Cube *cube);
bool apply_scramble(char *str, Cube *c);
int alg_string(Alg *alg, char *str);

void apply_trans(Trans t, Cube *cube);
Trans inverse_trans(Trans t);
void transform_alg(Trans t, Alg *alg);
Move transform_move(Trans t, Move m);
Trans transform_trans(Trans t, Trans m);

void init_cube();
