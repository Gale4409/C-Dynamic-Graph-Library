/*
 * hdt.h  --  Fully Dynamic Graph Connectivity (Holm-de Lichtenberg-Thorup).
 *
 * Reference:
 *   J. Holm, K. de Lichtenberg, M. Thorup.
 *   "Poly-logarithmic deterministic fully-dynamic graph algorithms for
 *    connectivity, minimum spanning tree, 2-edge, and biconnectivity."
 *   JACM 48(4): 723-760, 2001.
 *
 * Amortised time complexities:
 *   hdt_insert    -- O(log n)
 *   hdt_delete    -- O(log^2 n)
 *   hdt_connected -- O(log n)
 */

#ifndef HDT_H
#define HDT_H

typedef struct HDT HDT;

HDT *hdt_init     (int n);
void hdt_free     (HDT *h);
void hdt_insert   (HDT *h, int u, int v);
void hdt_delete   (HDT *h, int u, int v);
int  hdt_connected(HDT *h, int u, int v);
int  hdt_edge_count(const HDT *h);

#endif
