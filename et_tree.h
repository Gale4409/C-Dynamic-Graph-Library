/*
 * et_tree.h  --  Euler Tour Tree (ETT) built on an implicit-key splay tree.
 *
 * Each connected component of a spanning forest at a single HDT level is
 * represented as a linearised Euler tour stored in a splay tree.
 *
 * Two kinds of nodes live in every sequence:
 *   vertex node  -- one per vertex;  is_vertex == 1
 *   edge node    -- two per spanning-tree edge (the "uv" and "vu" halves);
 *                   is_vertex == 0
 *
 * Connectivity:
 *   u and v are in the same component iff their vertex nodes belong to the
 *   same splay tree (same root after splay).
 *
 * Augmented fields (maintained bottom-up in upd()):
 *   aug_nT  -- subtree contains a vertex with non-tree edges at this level.
 *   aug_tE  -- subtree contains a vertex with tree edges at exactly this level.
 *   These let the HDT deletion algorithm locate candidates in O(log n).
 */

#ifndef ET_TREE_H
#define ET_TREE_H

typedef struct ETNode ETNode;

struct ETNode {
    ETNode *ch[2];   /* splay children: 0=left, 1=right              */
    ETNode *p;       /* parent; NULL iff this node is the splay root  */

    int size;        /* total nodes in subtree                        */
    int vsize;       /* vertex nodes in subtree                       */

    /* per-node flags (meaningful only when is_vertex == 1) */
    int is_vertex;
    int vertex_id;
    int has_nT;      /* this vertex has non-tree edges at this level  */
    int has_tE;      /* this vertex has tree edges at exactly this lv */

    /* subtree-wide augmented flags */
    int aug_nT;
    int aug_tE;
};

/* ---- allocation -------------------------------------------------- */
ETNode *et_new_vertex(int vertex_id);
ETNode *et_new_edge  (void);
void    et_free_node (ETNode *x);

/* ---- connectivity ------------------------------------------------- */
int     et_connected(ETNode *u, ETNode *v);   /* 1 iff same sequence  */
int     et_vsize    (ETNode *v);              /* vertex count in comp. */

/* ---- link / cut --------------------------------------------------- */
/*
 * et_link(u, v, euv, evu)
 *   Merge the sequences of u and v (must be in distinct sequences).
 *   euv/evu are fresh isolated edge nodes for the two halves.
 *   Result:  [u-tour] euv [v-tour] evu
 */
void et_link(ETNode *u, ETNode *v, ETNode *euv, ETNode *evu);

/*
 * et_cut(euv, evu)
 *   Remove the spanning-tree edge whose halves are euv and evu.
 *   Both nodes are left isolated; the caller must free them.
 */
void et_cut(ETNode *euv, ETNode *evu);

/* ---- augmented queries -------------------------------------------- */
ETNode *et_find_nT(ETNode *v);   /* find vertex in comp. with has_nT  */
ETNode *et_find_tE(ETNode *v);   /* find vertex in comp. with has_tE  */

/* ---- flag updates ------------------------------------------------- */
void et_set_nT(ETNode *v, int val);
void et_set_tE(ETNode *v, int val);

#endif /* ET_TREE_H */
