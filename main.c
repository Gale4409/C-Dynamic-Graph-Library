/*
 * main.c  --  DynaGraph benchmark suite.
 *
 * Reads benchmark.txt (format: E V on line 1, then V city names,
 * then E edges as "u v wt") and exercises all phases of the library:
 *
 *   Phase 1  Static build         -- insert all edges, time it.
 *   Phase 2  Connectivity sweep   -- all-pairs via HDT O(log^2 n).
 *   Phase 3  Dynamic deletions    -- delete all edges, verify isolation.
 *   Phase 4  Dynamic re-insertions-- restore graph, verify connectivity.
 *   Phase 5  Structural queries   -- articulation points & bridges (O(V+E)).
 *   Phase 6  Node remove/insert   -- stress test the vertex lifecycle.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dynagraph.h"

/* ------------------------------------------------------------------ */
/*  Timer                                                              */
/* ------------------------------------------------------------------ */

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec * 1e-6;
}

static void sep(void)
{
    printf("─────────────────────────────────────────────────────\n");
}

/* ================================================================== */
/*  Main                                                               */
/* ================================================================== */

int main(void)
{
    FILE   *fp;
    int     E, V;
    int     i, u, v, wt;
    char    name[256];
    double  t0, elapsed;
    int    *eu, *ev, *ewt;

    fp = fopen("../benchmark.txt", "r");
    if (!fp) {
        fprintf(stderr, "Cannot open benchmark.txt\n");
        return 1;
    }

    if (fscanf(fp, "%d %d", &E, &V) != 2) {
        fprintf(stderr, "Bad header in benchmark.txt\n");
        fclose(fp);
        return 1;
    }

    sep();
    printf("DynaGraph Benchmark  --  V=%d  E=%d\n", V, E);
    sep();

    eu  = (int *)malloc((size_t)E * sizeof(int));
    ev  = (int *)malloc((size_t)E * sizeof(int));
    ewt = (int *)malloc((size_t)E * sizeof(int));
    if (!eu || !ev || !ewt) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    /* ---- Phase 0: init and vertex insertion ----------------------- */
    G graph = DynaGraphinit(V);
    if (!graph) { fprintf(stderr, "DynaGraphinit failed\n"); return 1; }

    printf("Phase 0: inserting %d vertices\n", V);
    for (i = 0; i < V; i++) {
        int id = DynaGraphNodeInsert(graph);
        fscanf(fp, "%255s", name);
    }
    printf("  Vertex count: %d\n\n", DynaGraphVertexCount(graph));

    /* ---- Phase 1: static edge build ------------------------------- */
    printf("Phase 1: inserting %d edges\n", E);
    t0 = now_ms();
    for (i = 0; i < E; i++) {
        if (fscanf(fp, "%d %d %d", &u, &v, &wt) != 3) {
            fprintf(stderr, "Malformed edge %d\n", i); break;
        }
        eu[i] = u; ev[i] = v; ewt[i] = wt;
        DynaGraphEdgeInsert(graph, (Edge){u, v, wt});
    }
    elapsed = now_ms() - t0;
    printf("  Inserted %d edges in %.3f ms\n\n",
           DynaGraphEdgeCount(graph), elapsed);

    if (V <= 25) DynaGraphPrint(graph);
    sep();

    /* ---- Phase 2: all-pairs connectivity (HDT) -------------------- */
    printf("Phase 2: all-pairs connectivity via HDT (O(log^2 n))\n");
    {
        int pairs = 0;
        t0 = now_ms();
        for (u = 0; u < V; u++)
            for (v = u + 1; v < V; v++)
                if (DynaGraphConnected(graph, u, v) > 0) pairs++;
        elapsed = now_ms() - t0;
        printf("  Connected pairs : %d / %d\n", pairs, V*(V-1)/2);
        printf("  HDT time        : %.4f ms  (%d queries)\n",
               elapsed, V*(V-1)/2);
    }

    /* O(V+E) baseline for comparison (AP queries, one per vertex) */
    {
        t0 = now_ms();
        for (u = 0; u < V; u++) isArticulationPoint(graph, u);
        elapsed = now_ms() - t0;
        printf("  Baseline (%d AP) : %.4f ms  (O(V+E) each)\n\n", V, elapsed);
    }
    sep();

    /* ---- Phase 3: delete all edges -------------------------------- */
    printf("Phase 3: deleting all %d edges one by one\n", E);
    t0 = now_ms();
    for (i = 0; i < E; i++)
        DynaGraphEdgeRemove(graph, (Edge){eu[i], ev[i], ewt[i]});
    elapsed = now_ms() - t0;
    printf("  Deletions done in %.3f ms\n", elapsed);
    printf("  Edges remaining  : %d\n", DynaGraphEdgeCount(graph));
    {
        int ok = 1;
        for (u = 0; u < V && ok; u++)
            for (v = u+1; v < V && ok; v++)
                if (DynaGraphConnected(graph, u, v) == 1) ok = 0;
        printf("  All disconnected : %s\n\n", ok ? "YES ✓" : "NO ✗");
    }
    sep();

    /* ---- Phase 4: re-insert all edges ----------------------------- */
    printf("Phase 4: re-inserting all edges\n");
    t0 = now_ms();
    for (i = 0; i < E; i++)
        DynaGraphEdgeInsert(graph, (Edge){eu[i], ev[i], ewt[i]});
    elapsed = now_ms() - t0;
    printf("  Re-insertions done in %.3f ms\n", elapsed);
    {
        /* Count connected components by finding "roots" */
        int comps = 0;
        for (u = 0; u < V; u++) {
            int is_root = 1;
            for (v = 0; v < u; v++)
                if (DynaGraphConnected(graph, u, v) > 0) { is_root = 0; break; }
            if (is_root) comps++;
        }
        printf("  Connected components after rebuild : %d\n\n", comps);
    }
    sep();

    /* ---- Phase 5: structural queries ------------------------------ */
    printf("Phase 5: O(V+E) structural queries\n");
    {
        int ap_cnt = 0;
        printf("  Articulation points:\n");
        for (u = 0; u < V; u++)
            if (isArticulationPoint(graph, u) == 1) {
                printf("    vertex %2d\n", u); ap_cnt++;
            }
        if (!ap_cnt) printf("    (none)\n");
        printf("  Total AP: %d\n", ap_cnt);
    }
    {
        int br_cnt = 0;
        printf("\n  Bridge edges:\n");
        for (i = 0; i < E; i++) {
            Edge e = {eu[i], ev[i], ewt[i]};
            if (isBridge(graph, e) == 1) {
                printf("    (%d -- %d, wt=%d)\n", eu[i], ev[i], ewt[i]);
                br_cnt++;
            }
        }
        if (!br_cnt) printf("    (none)\n");
        printf("  Total bridges: %d\n\n", br_cnt);
    }
    sep();

    /* ---- Phase 6: node lifecycle ---------------------------------- */
    printf("Phase 6: node remove/insert stress test\n");
    {
        int target = V / 4;
        int old_e  = DynaGraphEdgeCount(graph);
        printf("  Removing first %d vertices...\n", target);
        for (u = 0; u < target; u++)
            DynaGraphNodeRemove(graph, u);
        printf("  Edges after node removal : %d (was %d)\n",
               DynaGraphEdgeCount(graph), old_e);
        printf("  Vertices remaining       : %d\n", DynaGraphVertexCount(graph));

        printf("  Re-inserting %d vertices...\n", target);
        for (i = 0; i < target; i++) {
            int id = DynaGraphNodeInsert(graph);
            printf("    new id = %d\n", id);
        }
        printf("  Vertex count after re-insert : %d\n\n",
               DynaGraphVertexCount(graph));
    }
    sep();

    /* ---- cleanup -------------------------------------------------- */
    fclose(fp);
    DynaGraphfree(graph);
    free(eu); free(ev); free(ewt);

    printf("All benchmark phases completed successfully.\n");
    return 0;
}
