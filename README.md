# C Fully Dynamic Graph Library (DynaGraph)

A high-performance, fully dynamic undirected graph library written in C. This library goes beyond standard static graph implementations by supporting real-time structural mutations (insertions and deletions of both nodes and edges) while maintaining connectivity information with state-of-the-art amortized time complexities.

At its core, it features an implementation of the **Holm-de Lichtenberg-Thorup (HDT)** algorithm, allowing it to answer connectivity queries and process edge modifications in poly-logarithmic time.

## 🚀 Major Features & Architecture

* **Fully Dynamic Connectivity (HDT):** Maintains a multi-level spanning forest in the background. When an edge is deleted, the graph dynamically adjusts and searches for non-tree replacement edges across specific levels without requiring a full recalculation.
* **Euler Tour Trees (ETT):** Sequence semantics are maintained via implicit-key splay trees. Every connected component at every HDT level is represented as a linearized Euler tour, enabling ultra-fast sub-tree queries.
* **Augmented Node Tracking:** Nodes dynamically track the presence of tree (`has_tE`) and non-tree (`has_nT`) edges. This allows the HDT deletion algorithm to locate replacement candidates in `O(log n)` time.
* **Custom Dynamic Hash Table:** Uses linear chaining with dynamic array resizing (re-hashing at an alpha limit of `5.0`) to guarantee `O(1)` amortized access for reverse ID lookups and edge location.
* **Memory-Safe Vertex Lifecycle:** Safely handles massive, targeted node removals. The library isolates the node, cleans up all incident edges, and reclaims freed IDs for subsequent insertions without artificially inflating the ID pool.
* **Classic Structural Queries:** Fully supports traditional `O(V+E)` baseline queries via DFS for Articulation Points and Bridges.

## ⏱️ Time Complexities

By leveraging the HDT forest and ETT structures, the library achieves the following algorithmic limits:

| Operation | Function | Amortized Complexity |
| :--- | :--- | :--- |
| **Edge Insertion** | `DynaGraphEdgeInsert` | `O(log n)` |
| **Edge Deletion** | `DynaGraphEdgeRemove` | `O(log^2 n)` |
| **Connectivity Query** | `DynaGraphConnected` | `O(log n)` |
| **Node Insertion** | `DynaGraphNodeInsert` | `O(1)` |
| **Node Removal** | `DynaGraphNodeRemove` | `O(d * log^2 n)`* |
| **Articulation Points** | `isArticulationPoint` | `O(V+E)` |
| **Bridges** | `isBridge` | `O(V+E)` |

*\*Where `d` is the degree of the node being removed (as the library safely removes all incident edges first).*

## 📂 Project Structure

* **`dynagraph.h` / `dynagraph.c`:** The main public API for the dynamic graph.
* **`hdt.h` / `hdt.c`:** The Holm-de Lichtenberg-Thorup connectivity engine and level-promotion logic.
* **`et_tree.h` / `et_tree.c`:** The implicit-key splay tree implementation forming the Euler Tour Trees.
* **`hash_table.h` / `hash_table.c`:** `O(1)` dynamic hash table for fast vertex mapping and edge hashing.
* **`item.h` / `item.c`:** Generic item definitions for hash table payloads.
* **`main.c`:** A massive 6-phase stress-testing benchmark suite.

## 🛠️ Building & Running

A `CMakeLists.txt` is recommended for standard IDE builds (like CLion or VS Code). To compile and run the benchmark manually via GCC:

```bash
gcc -O3 main.c hdt.c et_tree.c hash_table.c item.c -o DynaGraph
./DynaGraph
