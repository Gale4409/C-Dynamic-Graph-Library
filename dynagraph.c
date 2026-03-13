/*
 * dynagraph.c  --  DynaGraph core implementation.
 *
 * Architecture
 * ------------
 *   1. Adjacency lists   -- singly-linked list per vertex, stores the
 *                           graph for structural queries (DFS, etc.).
 *   2. HDT               -- Holm-de Lichtenberg-Thorup dynamic spanning
 *                           forest; answers DynaGraphConnected in O(log^2 n).
 *   3. Free queue        -- FIFO of recycled vertex ids (compact id space).
 *   4. Symbol table      -- Hash table mapping string names to integer ids.
 *
 * The HDT is kept in sync with every edge insertion / deletion.
 * When the adjacency array grows (capacity doubles), the HDT is rebuilt
 * from the current adjacency lists in O(E log^2 E) -- this is amortised
 * O(log^2 n) per operation over all future insertions.
 */

#include "dynagraph.h"
#include "hdt.h"
#include "Queue.h"
#include "hash_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/*  Internal types                                                     */
/* ================================================================== */

typedef struct node *link;

struct node {
    int  v;          /* neighbour id                                  */
    int  wt;         /* edge weight                                   */
    link next;
};

struct dynagraph {
    int    V;          /* allocated capacity (number of id slots)     */
    int    E;          /* current edge count                          */
    int    n_vertex;   /* next never-used id                          */
    HASH   h;          /* symbol table: name -> id                    */
    link  *ladj;       /* adjacency lists (size V)                    */
    int   *is_active;  /* is_active[i] == 1 iff vertex i exists       */
    Queue  queue;      /* recycled ids                                */
    HDT   *hdt;        /* HDT for O(log^2 n) connectivity             */
};

/* ================================================================== */
/*  Adjacency-list helpers                                             */
/* ================================================================== */

static link newnode(int v, int wt, link next)
{
    link x = (link)malloc(sizeof(struct node));
    if (!x) return NULL;
    x->v = v; x->wt = wt; x->next = next;
    return x;
}

/* ================================================================== */
/*  HDT rebuild after capacity growth                                  */
/* ================================================================== */

static void hdt_rebuild(G graph)
{
    int i;
    link x;
    hdt_free(graph->hdt);
    graph->hdt = hdt_init(graph->V);
    if (!graph->hdt) return;
    for (i = 0; i < graph->V; i++) {
        if (!graph->is_active[i]) continue;
        for (x = graph->ladj[i]; x != NULL; x = x->next)
            if (x->v > i)  /* insert each edge once (canonical u < v) */
                hdt_insert(graph->hdt, i, x->v);
    }
}

/* ================================================================== */
/*  Lifecycle                                                          */
/* ================================================================== */

G DynaGraphinit(int V)
{
    G graph;
    if (V <= 0) V = 8;
    graph = (G)malloc(sizeof(struct dynagraph));
    if (!graph) return NULL;

    graph->V        = V;
    graph->E        = 0;
    graph->n_vertex = 0;

    graph->queue = Qinit();
    if (!graph->queue) { free(graph); return NULL; }

    graph->ladj = (link *)calloc((size_t)V, sizeof(link));
    if (!graph->ladj) { Qfree(graph->queue); free(graph); return NULL; }

    graph->h = hash_init(V);
    if (!graph->h) {
        Qfree(graph->queue); free(graph->ladj); free(graph); return NULL;
    }

    graph->is_active = (int *)calloc((size_t)V, sizeof(int));
    if (!graph->is_active) {
        hash_free(graph->h); Qfree(graph->queue);
        free(graph->ladj); free(graph); return NULL;
    }

    graph->hdt = hdt_init(V);
    if (!graph->hdt) {
        free(graph->is_active); hash_free(graph->h);
        Qfree(graph->queue); free(graph->ladj); free(graph); return NULL;
    }

    return graph;
}

void DynaGraphfree(G graph)
{
    int i;
    link x, tmp;
    if (!graph) return;

    for (i = 0; i < graph->V; i++) {
        x = graph->ladj[i];
        while (x) { tmp = x->next; free(x); x = tmp; }
    }
    free(graph->ladj);
    hash_free(graph->h);
    free(graph->is_active);
    Qfree(graph->queue);
    hdt_free(graph->hdt);
    free(graph);
}

/* ================================================================== */
/*  Node insert / remove                                               */
/* ================================================================== */

int DynaGraphNodeInsert(G graph)
{
    int i, old_V, new_index;
    link *tmp_ladj;
    int  *tmp_active;

    if (!graph) return -1;

    if (graph->V == graph->n_vertex && QisEmpty(graph->queue)) {
        /* Need to grow all arrays. */
        old_V = graph->V;

        tmp_ladj = (link *)malloc((size_t)(2 * graph->V) * sizeof(link));
        if (!tmp_ladj) return -1;

        tmp_active = (int *)malloc((size_t)(2 * graph->V) * sizeof(int));
        if (!tmp_active) { free(tmp_ladj); return -1; }

        for (i = 0; i < graph->V; i++) {
            tmp_ladj  [i] = graph->ladj[i];
            tmp_active[i] = graph->is_active[i];
        }
        for (i = graph->V; i < 2 * graph->V; i++) {
            tmp_ladj  [i] = NULL;
            tmp_active[i] = 0;
        }

        free(graph->ladj);
        free(graph->is_active);
        graph->ladj      = tmp_ladj;
        graph->is_active = tmp_active;
        graph->V         = 2 * old_V;

        resize_reverse_array(graph->h, graph->V);
        hdt_rebuild(graph);   /* rebuild HDT with new capacity         */
    }

    if (QisEmpty(graph->queue)) {
        graph->is_active[graph->n_vertex] = 1;
        graph->n_vertex++;
        return graph->n_vertex - 1;
    }

    new_index = Qget(graph->queue);
    graph->is_active[new_index] = 1;
    return new_index;
}

void DynaGraphNodeRemove(G graph, int v)
{
    link x, tmp;
    if (!graph || v < 0 || v >= graph->V) return;
    if (!graph->is_active[v]) return;

    /* Remove all incident edges first. */
    x = graph->ladj[v];
    while (x) {
        tmp = x->next;
        DynaGraphEdgeRemove(graph, (Edge){v, x->v, -1});
        x = tmp;
    }
    graph->ladj[v] = NULL;

    graph->is_active[v] = 0;
    Qput(graph->queue, v);
    hash_remove(graph->h, v);
}

/* ================================================================== */
/*  Edge insert / remove                                               */
/* ================================================================== */

void DynaGraphEdgeInsert(G graph, Edge e)
{
    if (!graph) return;
    if (e.v < 0 || e.v >= graph->V || e.w < 0 || e.w >= graph->V) return;
    if (!graph->is_active[e.v] || !graph->is_active[e.w]) return;
    if (e.v == e.w) return;

    graph->ladj[e.v] = newnode(e.w, e.wt, graph->ladj[e.v]);
    graph->ladj[e.w] = newnode(e.v, e.wt, graph->ladj[e.w]);
    hdt_insert(graph->hdt, e.v, e.w);
    graph->E++;
}

void DynaGraphEdgeRemove(G graph, Edge e)
{
    link x, p;
    if (!graph) return;
    if (e.v < 0 || e.v >= graph->V || e.w < 0 || e.w >= graph->V) return;

    /* Remove e.w from e.v's adjacency list */
    for (x = graph->ladj[e.v], p = NULL;
         x != NULL && x->v != e.w;
         p = x, x = x->next);
    if (!x) return;   /* edge not found */
    if (!p) graph->ladj[e.v] = x->next;
    else    p->next           = x->next;
    free(x);

    /* Remove e.v from e.w's adjacency list */
    for (x = graph->ladj[e.w], p = NULL;
         x != NULL && x->v != e.v;
         p = x, x = x->next);
    if (!x) return;
    if (!p) graph->ladj[e.w] = x->next;
    else    p->next           = x->next;
    free(x);

    hdt_delete(graph->hdt, e.v, e.w);
    graph->E--;
}

/* ================================================================== */
/*  O(log^2 n) connectivity via HDT                                   */
/* ================================================================== */

int DynaGraphConnected(G graph, int u, int v)
{
    if (!graph) return -1;
    if (u < 0 || u >= graph->V || !graph->is_active[u]) return -1;
    if (v < 0 || v >= graph->V || !graph->is_active[v]) return -1;
    return hdt_connected(graph->hdt, u, v);
}

/* ================================================================== */
/*  Diagnostics                                                        */
/* ================================================================== */

int DynaGraphVertexCount(G graph)
{
    int i, cnt = 0;
    if (!graph) return 0;
    for (i = 0; i < graph->V; i++)
        if (graph->is_active[i]) cnt++;
    return cnt;
}

int DynaGraphEdgeCount(G graph)
{
    return graph ? graph->E : 0;
}

void DynaGraphPrint(G graph)
{
    int i;
    link x;
    if (!graph) return;
    printf("DynaGraph  V=%d  E=%d\n", DynaGraphVertexCount(graph), graph->E);
    for (i = 0; i < graph->V; i++) {
        if (!graph->is_active[i]) continue;
        printf("  [%2d]:", i);
        for (x = graph->ladj[i]; x; x = x->next)
            printf(" %d(w=%d)", x->v, x->wt);
        printf("\n");
    }
}

/* ================================================================== */
/*  O(V+E) DFS baseline — structural queries                          */
/* ================================================================== */

static int count_cc(G graph, int skip_v, int skip_eu, int skip_ev)
{
    int i, comps = 0;
    int *visited = (int *)calloc((size_t)graph->V, sizeof(int));
    link x;
    if (!visited) return -1;

    for (i = 0; i < graph->V; i++) {
        if (!graph->is_active[i]) continue;
        if (i == skip_v)          continue;
        if (visited[i])           continue;

        /* Iterative DFS */
        {
            int top = 0;
            int *stk = (int *)malloc((size_t)graph->V * sizeof(int));
            if (!stk) { free(visited); return -1; }
            comps++;
            stk[top++] = i;
            visited[i] = 1;
            while (top > 0) {
                int cur = stk[--top];
                for (x = graph->ladj[cur]; x; x = x->next) {
                    int nb = x->v;
                    if (!graph->is_active[nb]) continue;
                    if (nb == skip_v)          continue;
                    if ((cur == skip_eu && nb == skip_ev) ||
                        (cur == skip_ev && nb == skip_eu)) continue;
                    if (!visited[nb]) {
                        visited[nb] = 1;
                        stk[top++]  = nb;
                    }
                }
            }
            free(stk);
        }
    }
    free(visited);
    return comps;
}

int isArticulationPoint(G graph, int v)
{
    int before, after;
    if (!graph || v < 0 || v >= graph->V) return -1;
    if (!graph->is_active[v])             return -1;

    before = count_cc(graph, -1, -1, -1);
    after  = count_cc(graph,  v, -1, -1);
    return (after > before) ? 1 : 0;
}

int isBridge(G graph, Edge e)
{
    int before, after;
    if (!graph)                                         return -1;
    if (e.v < 0 || e.v >= graph->V)                    return -1;
    if (e.w < 0 || e.w >= graph->V)                    return -1;
    if (!graph->is_active[e.v] || !graph->is_active[e.w]) return -1;

    before = count_cc(graph, -1, -1,  -1);
    after  = count_cc(graph, -1, e.v, e.w);
    return (after > before) ? 1 : 0;
}
