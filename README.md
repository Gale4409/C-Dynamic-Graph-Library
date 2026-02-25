# DynaGraph
⚡ A high-performance C library for Fully Dynamic Graph Connectivity.

## 🚧 Status: In Development
**Current Progress:**
- ✅ Core graph structure with adjacency lists
- ✅ Dynamic node insertion with automatic resizing
- ✅ Dynamic edge insertion/deletion
- ✅ Node deletion with cascading edge cleanup
- ✅ Efficient node index reuse via Queue
- ✅ Symbol Table (hash table) - Completed
- ✅ Item module - Completed
- ✅ Baseline O(V+E) connectivity checks (articulation points & bridges)
- 🔄 Dynamic Spanning Forest - Planned
- 🔄 Benchmarking suite - Planned

## 📁 Current Structure
```
├── dynagraph.h       # Public API
├── dynagraph.c       # Core implementation
├── hash_table.h/.c   # Symbol table with bidirectional lookup
├── Queue.h/.c        # FIFO queue for node reuse
└── item.h/.c         # Generic item type for the symbol table
```

## 🔨 Building
Currently work-in-progress. Full build instructions coming soon.

## 📖 API Overview (Implemented)

### Graph
```c
G DynaGraphinit(int V);                        // Initialize graph with V slots
int DynaGraphNodeInsert(G graph);              // Insert node, returns index
void DynaGraphEdgeInsert(G graph, Edge e);     // Insert weighted edge
void DynaGraphNodeRemove(G graph, int v);      // Remove node and all edges
void DynaGraphEdgeRemove(G graph, Edge e);     // Remove edge
void DynaGraphfree(G graph);                   // Free all memory
```

### Connectivity Queries
```c
int isArticulationPoint(G graph, int v);  // Returns 1 if v is an articulation point, 0 if not, -1 on error
int isBridge(G graph, Edge e);            // Returns 1 if e is a bridge, 0 if not, -1 on error
```

### Symbol Table (Hash Table)
```c
HASH hash_init(int maxN);                          // Initialize table with maxN slots
void resize_reverse_array(HASH h, int new_size);   // Expand reverse array on graph resize
int  hash_search(HASH h, Item item);               // Lookup by key → returns id, or -1
void hash_insert(HASH h, Item item, int id);       // Insert item with associated id
void hash_remove(HASH h, int id);                  // Remove by id
void hash_free(HASH h);                            // Free all memory
```

## 🗂️ Symbol Table Design

The symbol table maps string vertex names to internal integer IDs and back, acting as the bridge between the user-facing API (where nodes have names) and the internal graph representation (where nodes are array indices).

### Item Module

The `Item` type has been extracted into its own `item.h`/`item.c` module, decoupling it from the hash table implementation. This makes it easy to swap out the key type in the future without touching the hash table logic. Currently an `Item` holds a `char *key`, and the module exposes:

```c
int item_compare(Item a, Item b);  // strcmp-based comparison
Item Item_set_void();              // returns an empty Item (key = NULL)
```

### Bidirectional Lookup

Two data structures work in tandem:

- **Hash table** (`key → id`): given a vertex name, find its internal index in O(1) average.
- **Reverse array** (`id → key`): given an internal index, find the vertex name in O(1) worst case.

The reverse array is essential for deletion — when a node is removed by ID (as `DynaGraphNodeRemove` does), we need to find its key to compute the hash and locate it in the table, without scanning the entire structure.

### Hash Function

```c
int hash_func(Item item, int M){
    char *p = item.key;
    unsigned int val = 0;
    for (; *p != '\0'; p++){
        val = 31 * val + *p;
    }
    return val % M;
}
```

This is the classical **Bernstein/K&P polynomial rolling hash**, where each character is incorporated as `31 * val + c`. The multiplier 31 is a small prime, which ensures that different character positions have different weights, minimizing collisions from anagrams and similar strings. It is the same function used internally by Java's `String.hashCode()` and is widely considered the standard choice for string hashing.

### Collision Resolution: Separate Chaining

Collisions are handled via **separate chaining** (linked lists at each bucket) rather than open addressing. This choice was driven by the dynamic nature of the graph:

- **Deletions are frequent** — open addressing requires tombstone markers or complex rehashing on removal, while chaining allows O(1) node unlinking.
- **Load factor can exceed 1** without correctness issues — chaining degrades gracefully, while open addressing breaks above load factor 1.
- **No clustering** — open addressing suffers from primary/secondary clustering that degrades performance non-uniformly; chaining keeps each bucket independent.

### Automatic Resizing and Target Alpha

The table resizes automatically when the load factor α = N/M exceeds `TARGET_ALPHA`:

```c
if ((float) h->currentN / (float) h->M > TARGET_ALPHA){
    resize_hash_table(h);
}
```

`TARGET_ALPHA` is set to **5.0**, meaning on average 5 elements per bucket before resizing. This is intentionally higher than the typical threshold of 0.75 used in open addressing schemes. With chaining, a load factor of 5 still gives O(1) average lookup (each chain scan averages 5 comparisons), while keeping memory usage low — important since the graph itself already allocates adjacency lists and auxiliary arrays. The table doubles in size on each resize, and rehashing is done **in-place** by relinking existing nodes into the new bucket array without any extra allocation, keeping the resize cost to O(N).

## 📐 Connectivity Checks — Baseline O(V+E)

Two query functions are now available to test whether removing a node or an edge would disconnect the graph (or increase the number of connected components):

- **`isArticulationPoint(G graph, int v)`** — virtually removes `v` by temporarily deactivating it, runs a DFS to count connected components before and after, and restores the node. Returns 1 if the CC count increases (articulation point), 0 otherwise.
- **`isBridge(G graph, Edge e)`** — removes the edge from the adjacency lists, runs the same DFS comparison, then reinserts the edge. Returns 1 if the CC count increases (bridge), 0 otherwise.

Both functions are **non-destructive**: the graph is left in exactly the same state as before the call. The user can then decide independently whether to actually perform the removal.

These work correctly on **generic graphs** (not necessarily connected): the baseline CC count before removal is always computed, so an increase of any magnitude is detected regardless of the initial topology.

> **Note on `isBridge`:** the edge is reinserted using the `wt` field from the `Edge` struct passed by the caller. Make sure to pass the correct weight, otherwise the edge will be restored with a wrong weight.

This O(V+E) implementation serves as the **correctness baseline and benchmark reference** for the upcoming sub-linear Dynamic Spanning Forest approach.

## 🎯 Goals
Achieve **sub-linear update complexity** for dynamic connectivity queries instead of naive O(V+E) recomputation using Dynamic Spanning Forest techniques.

---
**Note:** This is an active research project. Implementation is ongoing and API may change.
