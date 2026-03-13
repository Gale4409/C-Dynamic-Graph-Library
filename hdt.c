/*
 * hdt.c  --  HDT fully dynamic connectivity (Holm-de Lichtenberg-Thorup 2001).
 *
 * Bug-free edge-list design
 * -------------------------
 *   Each vertex v at each level ℓ owns THREE singly-linked lists:
 *     verts[v].tree_u[ℓ]    -- tree   edges e with e->u == v
 *     verts[v].tree_v[ℓ]    -- tree   edges e with e->v == v
 *     verts[v].nontree_u[ℓ] -- non-tree edges e with e->u == v
 *     verts[v].nontree_v[ℓ] -- non-tree edges e with e->v == v
 *   The "u" lists are linked through e->u_next;
 *   the "v" lists are linked through e->v_next.
 *   Both next fields are exclusively owned by the list they serve, so
 *   traversal and removal are trivially correct.
 *
 *   The combined "tree edges at level ℓ for vertex v" is traversed by
 *   first scanning tree_u[ℓ] (via u_next) then tree_v[ℓ] (via v_next).
 *   Similarly for non-tree edges.
 *
 *   The ET-tree augmented flags (has_tE, has_nT) are 1 iff the
 *   combined tree / non-tree list for that vertex+level is non-empty.
 */

#include "hdt.h"
#include "et_tree.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ================================================================== */
/*  Constants                                                          */
/* ================================================================== */

#define HDT_LEVELS 30          /* covers n up to 2^29 ≈ 5×10^8        */
#define HT_LOAD    0.50
#define HT_INIT    64

/* ================================================================== */
/*  Edge                                                               */
/* ================================================================== */

typedef struct HDTEdge HDTEdge;

struct HDTEdge {
    int u, v;          /* canonical: u < v                            */
    int level;
    int is_tree;

    /* ET half-edge nodes for levels 0…level (tree edges only).       */
    ETNode *et_uv[HDT_LEVELS];
    ETNode *et_vu[HDT_LEVELS];

    /* u_next: next in verts[e->u].tree_u[level]  (or nontree_u)     */
    HDTEdge *u_next;
    /* v_next: next in verts[e->v].tree_v[level]  (or nontree_v)     */
    HDTEdge *v_next;

    /* Hash-table chain.                                               */
    HDTEdge *ht_next;
};

/* ================================================================== */
/*  Vertex                                                             */
/* ================================================================== */

typedef struct {
    ETNode  *et         [HDT_LEVELS]; /* ET vertex node per level      */
    /* Separate lists for edges where THIS vertex is u vs v.          */
    HDTEdge *tree_u     [HDT_LEVELS]; /* tree   edges, I am u          */
    HDTEdge *tree_v     [HDT_LEVELS]; /* tree   edges, I am v          */
    HDTEdge *nontree_u  [HDT_LEVELS]; /* non-tree, I am u              */
    HDTEdge *nontree_v  [HDT_LEVELS]; /* non-tree, I am v              */
} Vert;

/* ================================================================== */
/*  HDT struct                                                         */
/* ================================================================== */

struct HDT {
    int    n;
    int    edge_cnt;
    Vert  *verts;

    HDTEdge **ht;
    int       ht_slots;
    int       ht_used;
};

/* ================================================================== */
/*  Hash table                                                         */
/* ================================================================== */

static unsigned ht_hash(int u, int v, int slots)
{
    unsigned h = (unsigned)u * 2654435761u ^ (unsigned)v * 40503u;
    return h % (unsigned)slots;
}

static HDTEdge *ht_find(const HDT *h, int u, int v)
{
    unsigned idx = ht_hash(u, v, h->ht_slots);
    HDTEdge *e   = h->ht[idx];
    while (e) { if (e->u==u && e->v==v) return e; e = e->ht_next; }
    return NULL;
}

static void ht_ins(HDT *h, HDTEdge *e)
{
    unsigned idx = ht_hash(e->u, e->v, h->ht_slots);
    e->ht_next   = h->ht[idx];
    h->ht[idx]   = e;
    h->ht_used++;
}

static HDTEdge *ht_del(HDT *h, int u, int v)
{
    unsigned  idx = ht_hash(u, v, h->ht_slots);
    HDTEdge **pp  = &h->ht[idx];
    while (*pp) {
        if ((*pp)->u==u && (*pp)->v==v) {
            HDTEdge *e = *pp;
            *pp = e->ht_next; e->ht_next = NULL; h->ht_used--; return e;
        }
        pp = &(*pp)->ht_next;
    }
    return NULL;
}

static void ht_grow(HDT *h)
{
    int       ns = h->ht_slots * 2;
    HDTEdge **t  = (HDTEdge **)calloc((size_t)ns, sizeof(HDTEdge *));
    int i;
    if (!t) return;
    for (i = 0; i < h->ht_slots; i++) {
        HDTEdge *e = h->ht[i];
        while (e) {
            HDTEdge *nx = e->ht_next;
            unsigned idx = ht_hash(e->u, e->v, ns);
            e->ht_next = t[idx]; t[idx] = e;
            e = nx;
        }
    }
    free(h->ht); h->ht = t; h->ht_slots = ns;
}

/* ================================================================== */
/*  List helpers -- simple singly-linked, one list per (vertex, role) */
/* ================================================================== */

/* Remove e from *head, following u_next.                             */
static void rm_u(HDTEdge **head, HDTEdge *e)
{
    while (*head && *head != e) head = &(*head)->u_next;
    if (*head) { *head = e->u_next; e->u_next = NULL; }
}

/* Remove e from *head, following v_next.                             */
static void rm_v(HDTEdge **head, HDTEdge *e)
{
    while (*head && *head != e) head = &(*head)->v_next;
    if (*head) { *head = e->v_next; e->v_next = NULL; }
}

/* Push e to front of *head via u_next.                               */
static void push_u(HDTEdge **head, HDTEdge *e)
{
    e->u_next = *head; *head = e;
}

/* Push e to front of *head via v_next.                               */
static void push_v(HDTEdge **head, HDTEdge *e)
{
    e->v_next = *head; *head = e;
}

/* ================================================================== */
/*  ET-flag refresh                                                    */
/* ================================================================== */

/* is_tree_present(h,vid,lev): 1 iff vertex vid has any tree edge at lev */
static int tree_present(const HDT *h, int vid, int lev)
{
    return h->verts[vid].tree_u[lev] || h->verts[vid].tree_v[lev];
}

static int nontree_present(const HDT *h, int vid, int lev)
{
    return h->verts[vid].nontree_u[lev] || h->verts[vid].nontree_v[lev];
}

static void refresh_tE(HDT *h, int vid, int lev)
{
    et_set_tE(h->verts[vid].et[lev], tree_present(h, vid, lev));
}

static void refresh_nT(HDT *h, int vid, int lev)
{
    et_set_nT(h->verts[vid].et[lev], nontree_present(h, vid, lev));
}

/* ================================================================== */
/*  Vertex-list operations                                             */
/* ================================================================== */

static void add_tree(HDT *h, HDTEdge *e)
{
    int lev = e->level;
    push_u(&h->verts[e->u].tree_u[lev], e);
    push_v(&h->verts[e->v].tree_v[lev], e);
    refresh_tE(h, e->u, lev);
    refresh_tE(h, e->v, lev);
}

static void rm_tree(HDT *h, HDTEdge *e)
{
    int lev = e->level;
    rm_u(&h->verts[e->u].tree_u[lev], e);
    rm_v(&h->verts[e->v].tree_v[lev], e);
    refresh_tE(h, e->u, lev);
    refresh_tE(h, e->v, lev);
}

static void add_nontree(HDT *h, HDTEdge *e)
{
    int lev = e->level;
    push_u(&h->verts[e->u].nontree_u[lev], e);
    push_v(&h->verts[e->v].nontree_v[lev], e);
    refresh_nT(h, e->u, lev);
    refresh_nT(h, e->v, lev);
}

static void rm_nontree(HDT *h, HDTEdge *e)
{
    int lev = e->level;
    rm_u(&h->verts[e->u].nontree_u[lev], e);
    rm_v(&h->verts[e->v].nontree_v[lev], e);
    refresh_nT(h, e->u, lev);
    refresh_nT(h, e->v, lev);
}

/* ================================================================== */
/*  Get any tree/nontree edge incident to vertex vid at level lev.    */
/*  Returns NULL if none.                                              */
/* ================================================================== */

static HDTEdge *any_tree(const HDT *h, int vid, int lev)
{
    if (h->verts[vid].tree_u[lev]) return h->verts[vid].tree_u[lev];
    return h->verts[vid].tree_v[lev];
}

static HDTEdge *any_nontree(const HDT *h, int vid, int lev)
{
    if (h->verts[vid].nontree_u[lev]) return h->verts[vid].nontree_u[lev];
    return h->verts[vid].nontree_v[lev];
}

/* ================================================================== */
/*  Promote tree edge level i → i+1                                   */
/* ================================================================== */

static void promote_tree(HDT *h, HDTEdge *te)
{
    int nxt = te->level + 1;
    if (nxt >= HDT_LEVELS) return;   /* safety guard                  */
    rm_tree(h, te);
    te->et_uv[nxt] = et_new_edge();
    te->et_vu[nxt] = et_new_edge();
    et_link(h->verts[te->u].et[nxt], h->verts[te->v].et[nxt],
            te->et_uv[nxt], te->et_vu[nxt]);
    te->level = nxt;
    add_tree(h, te);
}

/* ================================================================== */
/*  Promote non-tree edge level i → i+1                               */
/* ================================================================== */

static void promote_nontree(HDT *h, HDTEdge *nte)
{
    int nxt = nte->level + 1;
    if (nxt >= HDT_LEVELS) return;
    rm_nontree(h, nte);
    nte->level = nxt;
    add_nontree(h, nte);
}

/* ================================================================== */
/*  Make replacement: non-tree edge becomes tree edge at given level  */
/* ================================================================== */

static void make_replacement(HDT *h, HDTEdge *rep, int lev)
{
    int j;
    rm_nontree(h, rep);
    rep->is_tree = 1;
    rep->level   = lev;
    for (j = 0; j <= lev && j < HDT_LEVELS; j++) {
        rep->et_uv[j] = et_new_edge();
        rep->et_vu[j] = et_new_edge();
        et_link(h->verts[rep->u].et[j], h->verts[rep->v].et[j],
                rep->et_uv[j], rep->et_vu[j]);
    }
    add_tree(h, rep);
}

/* ================================================================== */
/*  Public API                                                         */
/* ================================================================== */

HDT *hdt_init(int n)
{
    int i, j;
    HDT *h;
    assert(n > 0);
    h = (HDT *)calloc(1, sizeof *h);
    if (!h) return NULL;
    h->n = n;

    h->verts = (Vert *)calloc((size_t)n, sizeof(Vert));
    if (!h->verts) { free(h); return NULL; }

    for (i = 0; i < n; i++)
        for (j = 0; j < HDT_LEVELS; j++)
            h->verts[i].et[j] = et_new_vertex(i);

    h->ht_slots = HT_INIT;
    h->ht = (HDTEdge **)calloc((size_t)HT_INIT, sizeof(HDTEdge *));
    if (!h->ht) {
        for (i=0;i<n;i++) for(j=0;j<HDT_LEVELS;j++) et_free_node(h->verts[i].et[j]);
        free(h->verts); free(h); return NULL;
    }
    return h;
}

/* ------------------------------------------------------------------ */

void hdt_free(HDT *h)
{
    int i, j;
    HDTEdge *e, *nx;
    if (!h) return;
    for (i = 0; i < h->ht_slots; i++) {
        e = h->ht[i];
        while (e) {
            nx = e->ht_next;
            for (j = 0; j < HDT_LEVELS; j++) {
                if (e->et_uv[j]) et_free_node(e->et_uv[j]);
                if (e->et_vu[j]) et_free_node(e->et_vu[j]);
            }
            free(e); e = nx;
        }
    }
    free(h->ht);
    for (i = 0; i < h->n; i++)
        for (j = 0; j < HDT_LEVELS; j++)
            et_free_node(h->verts[i].et[j]);
    free(h->verts);
    free(h);
}

/* ------------------------------------------------------------------ */

void hdt_insert(HDT *h, int u, int v)
{
    HDTEdge *e;
    assert(u >= 0 && u < h->n && v >= 0 && v < h->n && u != v);
    if (u > v) { int t=u; u=v; v=t; }
    assert(!ht_find(h, u, v));

    if ((double)h->ht_used / h->ht_slots > HT_LOAD) ht_grow(h);

    e = (HDTEdge *)calloc(1, sizeof *e);
    e->u = u; e->v = v; e->level = 0;

    if (!et_connected(h->verts[u].et[0], h->verts[v].et[0])) {
        e->is_tree  = 1;
        e->et_uv[0] = et_new_edge();
        e->et_vu[0] = et_new_edge();
        et_link(h->verts[u].et[0], h->verts[v].et[0],
                e->et_uv[0], e->et_vu[0]);
        add_tree(h, e);
    } else {
        e->is_tree = 0;
        add_nontree(h, e);
    }
    ht_ins(h, e);
    h->edge_cnt++;
}

/* ------------------------------------------------------------------ */

void hdt_delete(HDT *h, int u, int v)
{
    HDTEdge *e;
    int lev, i, small_v, large_v;

    assert(u >= 0 && u < h->n && v >= 0 && v < h->n);
    if (u > v) { int t=u; u=v; v=t; }

    e = ht_del(h, u, v);
    assert(e);
    h->edge_cnt--;

    /* ---- non-tree: discard --------------------------------------- */
    if (!e->is_tree) {
        rm_nontree(h, e);
        free(e);
        return;
    }

    /* ---- tree: cut then search for replacement ------------------- */
    lev = e->level;
    for (i = 0; i <= lev && i < HDT_LEVELS; i++) {
        et_cut(e->et_uv[i], e->et_vu[i]);
        et_free_node(e->et_uv[i]); e->et_uv[i] = NULL;
        et_free_node(e->et_vu[i]); e->et_vu[i] = NULL;
    }
    rm_tree(h, e);
    free(e);
    /* NOTE: e must not be accessed below this line. */

    for (i = lev; i >= 0; i--) {
        ETNode *eu_node = h->verts[u].et[i];
        ETNode *ev_node = h->verts[v].et[i];
        int sz_u = et_vsize(eu_node);
        int sz_v = et_vsize(ev_node);
        int found = 0;
        ETNode *esmall;

        small_v = (sz_v <= sz_u) ? v : u;
        large_v = (sz_v <= sz_u) ? u : v;
        esmall  = h->verts[small_v].et[i];
        (void)large_v;

        /* 2c. Promote tree edges at level i in smaller component. */
        for (;;) {
            ETNode *vn = et_find_tE(esmall);
            int vid;
            HDTEdge *te;
            int other;
            if (!vn) break;
            vid = vn->vertex_id;

            while ((te = any_tree(h, vid, i)) != NULL) {
                other = (te->u == vid) ? te->v : te->u;
                if (et_connected(esmall, h->verts[other].et[i])) {
                    promote_tree(h, te);
                    esmall = h->verts[small_v].et[i];
                } else {
                    /* Tree edge crossing the cut: this shouldn't happen
                       in a correctly maintained spanning forest, but
                       break to avoid an infinite loop.               */
                    break;
                }
            }
            esmall = h->verts[small_v].et[i];
        }

        /* 2d. Scan non-tree edges at level i in smaller component. */
        for (;;) {
            ETNode *vn = et_find_nT(esmall);
            int vid;
            HDTEdge *nte;
            int other;
            if (!vn) break;
            vid = vn->vertex_id;

            while ((nte = any_nontree(h, vid, i)) != NULL) {
                other = (nte->u == vid) ? nte->v : nte->u;
                if (!et_connected(esmall, h->verts[other].et[i])) {
                    make_replacement(h, nte, i);
                    found = 1;
                    goto done_level;
                } else {
                    promote_nontree(h, nte);
                    esmall = h->verts[small_v].et[i];
                }
            }
            esmall = h->verts[small_v].et[i];
        }

        done_level:
        if (found) break;
    }
}

/* ------------------------------------------------------------------ */

int hdt_connected(HDT *h, int u, int v)
{
    assert(u >= 0 && u < h->n && v >= 0 && v < h->n);
    if (u == v) return 1;
    return et_connected(h->verts[u].et[0], h->verts[v].et[0]);
}

int hdt_edge_count(const HDT *h) { return h->edge_cnt; }
