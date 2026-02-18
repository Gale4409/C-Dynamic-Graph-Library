#include "dynagraph.h"
#include "Queue.h"
#include "ST.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct node *link;

struct node {
    int v; int wt;
    link next;
};

struct dynagraph {
    int V; int E;
    int n_vertex;
    ST tab;
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
    graph->tab = STinit(V);
    if (graph->tab == NULL) { Qfree(graph->queue); free(graph->ladj); free(graph); return NULL; }
    graph->is_active = calloc (graph->V, sizeof(int));
    if (graph->is_active == NULL) { STfree(graph->tab); Qfree(graph->queue); free(graph->ladj); free(graph); return NULL;}
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
    STfree(graph->tab);
    free(graph->is_active);
    Qfree(graph->queue);
    free(graph);
}