#define NMOVES      55
#define NTRANS      48
#define MAX_ALG_LEN 22
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))

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

void compose(Cube *, Cube *);
void copy_cube(Cube *, Cube *);
void invert_cube(Cube *);
bool is_solved(Cube *);
void make_solved(Cube *);

Move base_move(Move);
bool commute(Move, Move);
Move inverse_move(Move);

void copy_alg(Alg *, Alg *);
void append_move(Alg *, Move, bool);
void apply_move(Move, Cube *);
void apply_alg(Alg *, Cube *);
bool apply_scramble(char *, Cube *);
int alg_string(Alg *, char *);

void apply_trans(Trans, Cube *);
Trans inverse_trans(Trans);
void transform_alg(Trans, Alg *);
Move transform_move(Trans, Move);
Trans transform_trans(Trans, Trans);

void init_cube(void);
