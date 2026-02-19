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
- 🔄 Dynamic Spanning Forest - Planned
- 🔄 Connectivity queries - Planned
- 🔄 Benchmarking suite - Planned

## 📁 Current Structure
```
├── dynagraph.h       # Public API
├── dynagraph.c       # Core implementation
├── hash_table.h/.c   # Symbol table with bidirectional lookup
├── Queue.h/.c        # FIFO queue for node reuse
└── Item.h/.c         # Generic item type (coming soon)
```

## 🔨 Building
Currently work-in-progress. Full build instructions coming soon.

## 📖 API Overview (Implemented)

### Graph
```c
G DynaGraphinit(int V);                    // Initialize graph with V slots
int DynaGraphNodeInsert(G graph);          // Insert node, returns index
void DynaGraphEdgeInsert(G graph, Edge e); // Insert weighted edge
void DynaGraphNodeRemove(G graph, int v);  // Remove node and all edges
void DynaGraphEdgeRemove(G graph, Edge e); // Remove edge
void DynaGraphfree(G graph);               // Free all memory
```

### Symbol Table (Hash Table)
```c
HASH hash_init(int maxN);                          // Initialize table with maxN slots
void reverse_array_init(HASH h, int max_ids);      // Initialize reverse lookup array
void resize_reverse_array(HASH h, int new_size);   // Expand reverse array on graph resize
int  hash_search(HASH h, Item item);               // Lookup by key → returns id, or -1
void hash_insert(HASH h, Item item, int id);       // Insert item with associated id
void hash_remove(HASH h, int id);                  // Remove by id
void free_hash_table(HASH h);                      // Free all memory
```

## 🗂️ Symbol Table Design

The symbol table maps string vertex names to internal integer IDs and back, acting as the bridge between the user-facing API (where nodes have names) and the internal graph representation (where nodes are array indices).

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

## 🎯 Goals
Achieve **sub-linear update complexity** for dynamic connectivity queries instead of naive $O(V+E)$ recomputation using Dynamic Spanning Forest techniques.

---
**Note:** This is an active research project. Implementation is ongoing and API may change.
