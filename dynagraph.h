#ifndef DYNAGRAPH_H
#define DYNAGRAPH_H

typedef struct dynagraph *G;

typedef struct { int v; int w; int wt; } Edge; //weighted graph

G DynaGraphinit (int V);
int DynaGraphNodeInsert (G graph); //return the index in wich the new node is saved
void DynaGraphEdgeInsert (G graph, Edge e);
void DynaGraphNodeRemove (G graph, int v);
void DynaGraphEdgeRemove (G graph, Edge e);
void DynaGraphfree (G graph);

#endif