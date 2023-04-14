#ifndef ALG_H
#define ALG_H

// solve, private
typedef struct alglistnode {
	Alg *alg;
	struct alglistnode *next;
} AlgListNode;
typedef struct {
	AlgListNode *first;
	AlgListNode *last;
	int len;
} AlgList;

//cube
void        copy_alg(Alg *src, Alg *dst);
void        free_alg(Alg *alg);
Alg *       new_alg(char *str);
void        append_move(Alg *alg, Move m, bool inverse);
Alg *       inverse_alg(Alg *alg);
Move        inverse_move(Move m);

// pruning
bool        allowed_all(Move m);
bool        allowed_HTM(Move m);
bool        allowed_URF(Move m);
bool        allowed_eofb(Move m);
bool        allowed_drud(Move m);
bool        allowed_htr(Move m);

// pruning and solve?
Move        base_move(Move m);
bool        commute(Move m1, Move m2);

// solve
void        free_alglist(AlgList *l);
AlgList *   new_alglist();
void        append_alg(AlgList *l, Alg *alg);

// solve, private
bool        singlecwend(Alg *alg);
void        swapmove(Move *m1, Move *m2);

// solve, change interface
void        print_alg(Alg *alg);
void        print_alglist(AlgList *al);

#endif

