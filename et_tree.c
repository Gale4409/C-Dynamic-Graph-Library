/*
 * et_tree.c  --  Euler Tour Tree backed by an implicit-key splay tree.
 *
 * Sequence semantics
 * ------------------
 *   Each splay tree represents a contiguous sequence of nodes (the Euler
 *   tour of one component).  There are no explicit keys -- a node's position
 *   in the sequence is determined implicitly by subtree sizes.
 *
 * Invariants maintained at all times:
 *   size(x)   = 1 + size(ch[0]) + size(ch[1])
 *   vsize     = (is_vertex?1:0) + vsize(ch[0]) + vsize(ch[1])
 *   aug_nT    = (is_vertex&&has_nT) | aug_nT(ch[0]) | aug_nT(ch[1])
 *   aug_tE    = (is_vertex&&has_tE) | aug_tE(ch[0]) | aug_tE(ch[1])
 */

#include "et_tree.h"
#include <stdlib.h>
#include <assert.h>

/* ================================================================== */
/*  Internal splay helpers                                             */
/* ================================================================== */

/* Recompute aggregated fields of x from its children.                */
static void upd(ETNode *x)
{
    int ls, lv, ln, lt;
    int rs, rv, rn, rt;
    if (!x) return;

    ls = x->ch[0] ? x->ch[0]->size   : 0;
    lv = x->ch[0] ? x->ch[0]->vsize  : 0;
    ln = x->ch[0] ? x->ch[0]->aug_nT : 0;
    lt = x->ch[0] ? x->ch[0]->aug_tE : 0;

    rs = x->ch[1] ? x->ch[1]->size   : 0;
    rv = x->ch[1] ? x->ch[1]->vsize  : 0;
    rn = x->ch[1] ? x->ch[1]->aug_nT : 0;
    rt = x->ch[1] ? x->ch[1]->aug_tE : 0;

    x->size   = 1 + ls + rs;
    x->vsize  = (x->is_vertex ? 1 : 0) + lv + rv;
    x->aug_nT = (x->is_vertex && x->has_nT) | ln | rn;
    x->aug_tE = (x->is_vertex && x->has_tE) | lt | rt;
}

/* 1 if x is the right child of its parent, 0 if left child.         */
static int dir(const ETNode *x)
{
    return x->p->ch[1] == x;
}

/* Set parent->ch[d] = child, child->p = parent  (NULL-safe).        */
static void setc(ETNode *parent, int d, ETNode *child)
{
    if (parent) parent->ch[d] = child;
    if (child)  child->p      = parent;
}

/* Single rotation: lift x over its parent p.                         */
static void rot(ETNode *x)
{
    ETNode *p  = x->p;
    ETNode *g  = p->p;
    int     dx = dir(x);
    int     dp = (g) ? dir(p) : 0;

    setc(p, dx,   x->ch[dx ^ 1]);  /* inner child of x → p          */
    setc(x, dx^1, p);              /* p → outer child of x           */

    if (g) { g->ch[dp] = x; x->p = g; }
    else   { x->p = NULL; }

    upd(p);
    upd(x);
}

/* Splay x to the root of its splay tree.                             */
static void splay(ETNode *x)
{
    while (x->p) {
        ETNode *p = x->p;
        if (p->p) {
            /* zig-zig: same direction → rotate parent first          */
            /* zig-zag: different direction → rotate x first          */
            if (dir(x) == dir(p)) rot(p); else rot(x);
        }
        rot(x);
    }
}

/* Concatenate two sequences (a entirely before b).
   Both a and b must be splay-tree roots (p == NULL).
   Returns the new root.                                               */
static ETNode *seq_merge(ETNode *a, ETNode *b)
{
    ETNode *cur;
    if (!a) return b;
    if (!b) return a;
    /* Find and splay the rightmost node of a. */
    cur = a;
    while (cur->ch[1]) cur = cur->ch[1];
    splay(cur);          /* cur becomes root; cur->ch[1] == NULL      */
    cur->ch[1] = b;
    b->p = cur;
    upd(cur);
    return cur;
}

/* Rearrange v's sequence so v is the leftmost element.
   Before:  [L] [v] [R]   (L = left subtree, R = right subtree)
   After :  [v] [R] [L]                                               */
static void reroot(ETNode *v)
{
    ETNode *left;
    splay(v);              /* v becomes root                          */
    left = v->ch[0];
    if (!left) return;     /* already leftmost                        */
    v->ch[0] = NULL;
    left->p  = NULL;
    upd(v);
    seq_merge(v, left);    /* append L after [v R]                   */
}

/* ================================================================== */
/*  Allocation                                                         */
/* ================================================================== */

ETNode *et_new_vertex(int vertex_id)
{
    ETNode *x = (ETNode *)calloc(1, sizeof *x);
    if (!x) return NULL;
    x->size      = 1;
    x->vsize     = 1;
    x->is_vertex = 1;
    x->vertex_id = vertex_id;
    return x;
}

ETNode *et_new_edge(void)
{
    ETNode *x = (ETNode *)calloc(1, sizeof *x);
    if (!x) return NULL;
    x->size = 1;
    return x;
}

void et_free_node(ETNode *x)
{
    free(x);
}

/* ================================================================== */
/*  Connectivity                                                       */
/* ================================================================== */

/* Return the root (representative) of x's splay tree.
   As a side-effect, x is splayed, which amortises future operations. */
static ETNode *get_root(ETNode *x)
{
    ETNode *cur;
    splay(x);
    cur = x;
    while (cur->ch[0]) cur = cur->ch[0];
    splay(cur);
    return cur;
}

int et_connected(ETNode *u, ETNode *v)
{
    if (!u || !v) return 0;
    if (u == v)   return 1;
    return get_root(u) == get_root(v);
}

int et_vsize(ETNode *v)
{
    splay(v);
    return v->vsize;
}

/* ================================================================== */
/*  Link / Cut                                                         */
/* ================================================================== */

void et_link(ETNode *u, ETNode *v, ETNode *euv, ETNode *evu)
{
    /* Both u and v must be in distinct sequences. */
    reroot(u);  /* u is now leftmost in its sequence                  */
    reroot(v);  /* v is now leftmost in its sequence                  */
    splay(u);   /* u->ch[0] == NULL (leftmost)                       */
    splay(v);   /* v->ch[0] == NULL                                   */
    /* Build: [u-seq] ++ euv ++ [v-seq] ++ evu */
    seq_merge(seq_merge(seq_merge(u, euv), v), evu);
}

void et_cut(ETNode *euv, ETNode *evu)
{
    /*
     * Layout:  [A] euv [B] evu [C]
     *   A ++ C  = u's component after the cut
     *   B       = v's component after the cut
     *
     * We need to determine which of euv/evu comes first.  After splay(x),
     * the rank of x equals the size of its left subtree.  This rank is
     * invariant under the relabelling of nodes that splay performs.
     */
    int puv, pvu;
    ETNode *A, *rest, *B, *C;

    splay(euv);
    puv = euv->ch[0] ? euv->ch[0]->size : 0;

    splay(evu);
    pvu = evu->ch[0] ? evu->ch[0]->size : 0;

    /* Swap so that euv comes first in the sequence. */
    if (puv > pvu) {
        ETNode *tmp = euv; euv = evu; evu = tmp;
    }

    /* -- Step 1: isolate euv ---------------------------------------- */
    splay(euv);
    A = euv->ch[0];
    rest = euv->ch[1];   /* contains [B] evu [C]                     */
    if (A)    { A->p    = NULL; euv->ch[0] = NULL; }
    if (rest) { rest->p = NULL; euv->ch[1] = NULL; }
    upd(euv); /* euv is now isolated                                  */

    /* -- Step 2: within rest, isolate evu --------------------------- */
    splay(evu);  /* evu splays to root of rest; rest->p was NULL      */
    B = evu->ch[0];   /* v's Euler tour                               */
    C = evu->ch[1];   /* tail of u's tour                             */
    if (B) { B->p = NULL; evu->ch[0] = NULL; }
    if (C) { C->p = NULL; evu->ch[1] = NULL; }
    upd(evu); /* evu is now isolated                                  */

    /* -- Step 3: rebuild u's sequence as A ++ C --------------------- */
    seq_merge(A, C);
    /* B is v's new standalone sequence (accessed through any vertex
       node contained in B).                                           */
    (void)B;
}

/* ================================================================== */
/*  Augmented queries                                                  */
/* ================================================================== */

ETNode *et_find_nT(ETNode *v)
{
    ETNode *cur;
    splay(v);
    if (!v->aug_nT) return NULL;
    cur = v;
    for (;;) {
        if (cur->is_vertex && cur->has_nT) return cur;
        if (cur->ch[0] && cur->ch[0]->aug_nT) cur = cur->ch[0];
        else                                   cur = cur->ch[1];
    }
}

ETNode *et_find_tE(ETNode *v)
{
    ETNode *cur;
    splay(v);
    if (!v->aug_tE) return NULL;
    cur = v;
    for (;;) {
        if (cur->is_vertex && cur->has_tE) return cur;
        if (cur->ch[0] && cur->ch[0]->aug_tE) cur = cur->ch[0];
        else                                   cur = cur->ch[1];
    }
}

/* ================================================================== */
/*  Flag updates                                                       */
/* ================================================================== */

void et_set_nT(ETNode *v, int val)
{
    splay(v);
    v->has_nT = val;
    upd(v);
}

void et_set_tE(ETNode *v, int val)
{
    splay(v);
    v->has_tE = val;
    upd(v);
}
