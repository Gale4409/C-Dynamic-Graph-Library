#include "dynagraph.h"
#include "Queue.h"
#include "hash_table.h"
#include "UF.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct node *link;

struct node {
    int v; int wt;
    link next;
};

struct Edge {
    int v; int w; int wt;
};

typedef struct st_edges{
    Edge *edges;        // array of the selected edges of the ST
    int cont;   // numeber of edges in the ST
}ST_edges;

struct dynagraph {
    int V; int E;
    int n_vertex;
    HASH h;
    link *ladj;
    int *is_active;
    Queue queue; //I'll use a queue to save the not_active vertex and reuse them in case of necessity
};

static link newnode (int v, int wt, link next) {
    link x = malloc (sizeof(struct node));
    if (x == NULL) return NULL;
    x->v = v; x->wt = wt;
    x->next = next;
    return x;
}

G DynaGraphinit (int V) {
    G graph = malloc (sizeof(struct dynagraph));
    if (graph == NULL) return NULL;
    graph->V = V; graph->E = 0; graph->n_vertex = 0;
    graph->queue = Qinit();
    if (graph->queue == NULL) { free(graph); return NULL; }
    graph->ladj = calloc (V, sizeof(link));
    if (graph->ladj == NULL) { Qfree(graph->queue); free(graph); return NULL; }
    graph->h = hash_init(V);
    if (graph->h == NULL) { Qfree(graph->queue); free(graph->ladj); free(graph); return NULL; }
    graph->is_active = calloc (graph->V, sizeof(int));
    if (graph->is_active == NULL) { hash_free(graph->h); Qfree(graph->queue); free(graph->ladj); free(graph); return NULL;}
    return graph;
}

int DynaGraphNodeInsert (G graph) {
    if (graph == NULL) return -1;

    if (graph->V == graph->n_vertex && QisEmpty(graph->queue)) {
        link *tmp_ladj = malloc(2*graph->V*sizeof(link));
        if (tmp_ladj == NULL) return -1;
        int *tmp_active = malloc(2*graph->V*sizeof(int));
        if (tmp_active == NULL) {
            free(tmp_ladj);
            return -1;
        }
        int i;
        for (i=0; i<graph->V; i++) {
            tmp_ladj[i] = graph->ladj[i];
            tmp_active[i] = graph->is_active[i];
        }
        for (i=graph->V; i<2*graph->V; i++) {
            tmp_ladj[i] = NULL;
            tmp_active[i] = 0;
        }
        free(graph->ladj);
        free(graph->is_active);
        graph->ladj = tmp_ladj;
        graph->is_active = tmp_active;
        graph->V = 2*graph->V;
        resize_reverse_array(graph->h, graph->V);    // reallocating also the revrse array to make space for new IDs
    }

    if (QisEmpty(graph->queue)) {
        graph->is_active[graph->n_vertex] = 1; //activation of the node
        graph->n_vertex++;
        return graph->n_vertex-1; //return of the position that's representing the new vertex
    }
    //otherwhise:
    int new_index = Qget (graph->queue);
    graph->is_active[new_index] = 1;
    return new_index; //creation of the vertex in the old position
}

void DynaGraphEdgeInsert (G graph, Edge e) {
    if (graph == NULL || e.v >= graph->V || e.v < 0 || e.w >= graph->V || e.w < 0) return;
    if (graph->is_active[e.v] == 0 || graph->is_active[e.w] == 0) return; //if at least one of the 2 nodes creating the edge are not active, return
    graph->ladj[e.v] = newnode (e.w, e.wt, graph->ladj[e.v]);
    graph->ladj[e.w] = newnode (e.v, e.wt, graph->ladj[e.w]);
    graph->E++;
}

void DynaGraphNodeRemove (G graph, int v) {
    if (graph == NULL || graph->is_active[v] == 0) return;
    graph->is_active[v] = 0;
    Qput (graph->queue, v);
    //I'll remove every Edge connected to v:
    link x=graph->ladj[v], tmp=NULL;
    while (x != NULL) {
        tmp = x->next;
        DynaGraphEdgeRemove(graph, (Edge){v, x->v, -1});
        x = tmp;
    }
    graph->ladj[v] = NULL;

    hash_remove(graph->h, v);
}

void DynaGraphEdgeRemove (G graph, Edge e) {
    if (graph == NULL || e.v >= graph->V || e.v < 0 || e.w >= graph->V || e.w < 0) return;
    link x, p;
    for (x=graph->ladj[e.v], p=NULL; x!=NULL && x->v != e.w; p=x, x=x->next);
    if (x == NULL) return; //edge not found
    if (p == NULL) { //remove in head
        graph->ladj[e.v] = graph->ladj[e.v]->next;
        free(x); //free of the node
    }
    else {
        p->next = x->next; //bypass
        free (x); //free of the node
    }
    //do the same for the w->v edge:
    for (x=graph->ladj[e.w], p=NULL; x!=NULL && x->v != e.v; p=x, x=x->next);
    if (x == NULL) return; //edge not found
    if (p == NULL) { //remove in head
        graph->ladj[e.w] = graph->ladj[e.w]->next;
        free(x); //free of the node
    }
    else {
        p->next = x->next; //bypass
        free (x); //free of the node
    }
    graph->E--;
}

static void DFS_CC_R (int v, G graph, int *visited) {
    visited[v] = 1;
    link x;
    for (x=graph->ladj[v]; x!=NULL; x=x->next) {
        if (graph->is_active[x->v] == 0 || visited[x->v] == 1) continue;
        DFS_CC_R (x->v, graph, visited);
    }
}

static int DFS_CC (G graph, int *visited) {
    int cont = 0;
    int i;
    for (i=0; i<graph->V; i++) {
        if (visited[i] == 1 || graph->is_active[i] == 0) continue;
        DFS_CC_R (i, graph, visited);
        cont++;
    }
    return cont;
}

static void reset_vet (int *v, int len) {
    for (int i=0; i<len; i++) v[i] = 0;
}

int isArticulationPoint(G graph, int v) {
    if (graph == NULL || v < 0 || v >= graph->n_vertex) return -1;
    if (graph->is_active[v] == 0) return -1;
    int *visited = calloc (graph->V, sizeof(int));

    int n_CC_start = DFS_CC (graph, visited); //calculation of the initial number of CC
    //virtual removal of the vertex and recalculation of the n_CC:
    graph->is_active[v] = 0;
    reset_vet(visited, graph->V);
    int n_CC_post_removal = DFS_CC (graph, visited);
    graph->is_active[v] = 1; //reset the vertex as used, if the user wants to enable the vertex is able to do it later with the apposite function
    free (visited); //freed the memory to avoid memory leak

    return n_CC_start != n_CC_post_removal ? 1 : 0; //return of the result obtained
}

int isBridge(G graph, Edge e) { //copy of the function to calculate it for a single vertex but with the removal of the Edge
    if (graph == NULL || e.v < 0 || e.w < 0 || e.v >= graph->V || e.w >= graph->V) return -1;
    if (graph->is_active[e.v] == 0 || graph->is_active[e.w] == 0) return -1;
    int *visited = calloc (graph->V, sizeof(int));

    int n_CC_start = DFS_CC (graph, visited); //calculation of the initial number of CC
    //virtual removal of the vertex and recalculation of the n_CC:
    DynaGraphEdgeRemove(graph, e);
    reset_vet(visited, graph->V);
    int n_CC_post_removal = DFS_CC (graph, visited);
    DynaGraphEdgeInsert(graph, e);
    free (visited);

    return n_CC_start != n_CC_post_removal ? 1 : 0; //return of the result obtained
}

// returns the spanning forest of the graph or the spanning tree(connected graph), using UF module, high efficency, O(E*α(V)), almost linear time
ST_edges calculate_ST(G graph){
    UF uf = UF_init(graph->V);
    ST_edges st;
    st.edges = malloc((graph->V-1)*sizeof(Edge));       // worst case scenario: got to take V-1 edges
    st.cont = 0;
    for (int i = 0; i < graph->V; i++){     // cycling through all the edges of the graph and trying to add them to the ST, if the union is successful, the edge is added to the ST, otherwise it's not added
        if (graph->is_active[i] == 0) continue;
        link t;
        for (t = graph->ladj[i]; t != NULL; t = t->next){
            if (i < t->v){     //to avoid to check the same edge twice
                if (UF_union(uf, i, t->v) == 0) { //if the union is successful, the edge is added to the ST
                    st.edges[st.cont].v = i; 
                    st.edges[st.cont].w = t->v;
                    st.edges[st.cont].wt = t->wt;
                    st.cont++;
                }
            }
        }
    }
    UF_free(uf);
    return st;
}

void DynaGraphfree (G graph) {
    if (graph == NULL) return;
    int i;
    for (i=0; i<graph->V; i++) {
        link x=graph->ladj[i], tmp;
        while (x!=NULL) {
            tmp = x->next;
            free (x);
            x = tmp;
        }
    }
    free(graph->ladj);
    hash_free(graph->h);
    free(graph->is_active);
    Qfree(graph->queue);
    free(graph);
}