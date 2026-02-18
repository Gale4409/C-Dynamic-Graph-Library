# DynaGraph

⚡ A high-performance C library for Fully Dynamic Graph Connectivity.

## 🚧 Status: In Development

**Current Progress:**
- ✅ Core graph structure with adjacency lists
- ✅ Dynamic node insertion with automatic resizing
- ✅ Dynamic edge insertion/deletion
- ✅ Node deletion with cascading edge cleanup
- ✅ Efficient node index reuse via Queue
- 🔄 Symbol Table (hash table) - In progress
- 🔄 Dynamic Spanning Forest - Planned
- 🔄 Connectivity queries - Planned
- 🔄 Benchmarking suite - Planned

## 📁 Current Structure
```
├── dynagraph.h       # Public API
├── dynagraph.c       # Core implementation
├── Queue.h/.c        # FIFO queue for node reuse
├── ST.h/.c           # Symbol table (coming soon)
└── Item.h/.c         # Generic item type (coming soon)
```

## 🔨 Building

Currently work-in-progress. Full build instructions coming soon.

## 📖 API Overview (Implemented)
```c
G DynaGraphinit(int V);                    // Initialize graph with V slots
int DynaGraphNodeInsert(G graph);          // Insert node, returns index
void DynaGraphEdgeInsert(G graph, Edge e); // Insert weighted edge
void DynaGraphNodeRemove(G graph, int v);  // Remove node and all edges
void DynaGraphEdgeRemove(G graph, Edge e); // Remove edge
void DynaGraphfree(G graph);               // Free all memory
```

## 🎯 Goals

Achieve **sub-linear update complexity** for dynamic connectivity queries instead of naive $O(V+E)$ recomputation using Dynamic Spanning Forest techniques.

---

**Note:** This is an active research project. Implementation is ongoing and API may change.
