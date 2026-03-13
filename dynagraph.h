/*
 * dynagraph.h  --  Public API for the DynaGraph Fully Dynamic Graph Library.
 *
 * Connectivity queries (isArticulationPoint, isBridge) run in the original
 * O(V+E) DFS baseline.
 *
 * DynaGraphConnected runs in O(log^2 n) amortised via the HDT spanning
 * forest maintained in the background.
 */

#ifndef DYNAGRAPH_H
#define DYNAGRAPH_H

typedef struct dynagraph *G;

typedef struct { int v; int w; int wt; } Edge;  /* weighted undirected edge */

/* ---- lifecycle ---------------------------------------------------- */
G    DynaGraphinit        (int V);
void DynaGraphfree        (G graph);

/* ---- structural mutations ----------------------------------------- */
int  DynaGraphNodeInsert  (G graph);  /* returns new vertex id, or -1  */
void DynaGraphNodeRemove  (G graph, int v);
void DynaGraphEdgeInsert  (G graph, Edge e);
void DynaGraphEdgeRemove  (G graph, Edge e);

/* ---- O(log^2 n) connectivity via HDT ------------------------------ */
/*
 * DynaGraphConnected(graph, u, v)
 *   Returns 1 if u and v are in the same connected component,
 *   0 if not, -1 on invalid input.
 */
int  DynaGraphConnected   (G graph, int u, int v);

/* ---- O(V+E) structural queries (DFS baseline) --------------------- */
int  isArticulationPoint  (G graph, int v);
int  isBridge             (G graph, Edge e);

/* ---- diagnostics -------------------------------------------------- */
int  DynaGraphVertexCount (G graph);
int  DynaGraphEdgeCount   (G graph);
void DynaGraphPrint       (G graph);

#endif /* DYNAGRAPH_H */
