# C Fully Dynamic Graph Library (DynaGraph)

A high-performance, fully dynamic undirected graph library written in C. This library goes beyond standard static graph implementations by supporting real-time structural mutations (insertions and deletions of both nodes and edges) while maintaining connectivity information with state-of-the-art poly-logarithmic time complexities.

## 🏗️ High-Level Architecture

Building a graph that can efficiently handle millions of rapid edge insertions and deletions requires several advanced algorithmic layers working in unison. Here is a breakdown of the structures used and their purpose in the system.

### 1. The Core Engine: Holm-de Lichtenberg-Thorup (HDT)
* **The Problem:** In a standard graph, deleting an edge requires an `O(V + E)` Depth First Search (DFS) to figure out if the graph was split into two separate components. For dynamic graphs, this is too slow.
* **The Solution:** The HDT algorithm maintains a multi-level spanning forest in the background. Edges are assigned a "level" (from `0` to `log V`). 
* **How it works:** When a tree edge is deleted at level `i`, the algorithm only searches for replacement non-tree edges at level `i` or higher within the *smaller* of the two disconnected halves. If no replacement is found, edges are promoted to higher levels. This clever restriction ensures we don't waste time scanning massive components, achieving `O(log^2 n)` amortized time for deletions.

### 2. Component Tracking: Euler Tour Trees (ETT) & Splay Trees
* **The Problem:** We need a way to represent the connected components of the HDT forest so that we can split them (cut), join them (link), and check their sizes in `O(log n)` time.
* **The Solution:** Euler Tour Trees (ETT). Every spanning tree component at every HDT level is represented as a linearized sequence of nodes (an Euler Tour). 
* **How it works:** These sequences are stored in **implicit-key Splay Trees**. Splay trees naturally push frequently accessed nodes to the root. Furthermore, we augment the Splay Tree nodes with boolean flags (`has_tE` for tree edges, `has_nT` for non-tree edges). These flags propagate up the tree, allowing the HDT engine to search an entire component for a replacement edge in exactly `O(log n)` time by just looking at the root.

### 3. Memory & ID Management: Dynamic Hash Table
* **The Problem:** Graph algorithms require dense, contiguous integer IDs (`0` to `V-1`) for fast array indexing. However, real-world graph data uses strings/objects (e.g., city names). Furthermore, when a node is permanently deleted, it leaves a "hole" in the ID space.
* **The Solution:** A custom separate-chaining Hash Table paired with a dynamic Reverse-Lookup Array.
* **How it works:** * **Insertions:** Maps generic string `Items` to internal integer IDs.
  * **Reclamation:** When a node is deleted, its ID is safely recycled.
  * **Load Balancing:** The hash table maintains a strict maximum load factor (`TARGET_ALPHA = 5.0`). If the limit is breached, the table automatically doubles in size and re-hashes all elements to prevent linked-list degradation.

---

## 📊 Algorithmic & Complexity Analysis

### Graph Operations
By leveraging the HDT forest and ETT structures, the graph achieves the following theoretical limits:

| Operation | Function | Amortized Time Complexity |
| :--- | :--- | :--- |
| **Edge Insertion** | `DynaGraphEdgeInsert` | `O(log n)` |
| **Edge Deletion** | `DynaGraphEdgeRemove` | `O(log^2 n)` |
| **Connectivity Query** | `DynaGraphConnected` | `O(log n)` |
| **Node Insertion** | `DynaGraphNodeInsert` | `O(1)` |
| **Node Removal** | `DynaGraphNodeRemove` | `O(d * log^2 n)`* |
| **Articulation Points** | `isArticulationPoint` | `O(V+E)` (DFS Baseline) |
| **Bridges** | `isBridge` | `O(V+E)` (DFS Baseline) |

*\*Where `d` is the degree of the node being removed, as the library must safely isolate the node by removing all incident edges first.*

### Hash Table Memory Management
The internal hash table is highly optimized for performance over memory footprint:

| Operation | Average Time | Worst-Case Time | Space Complexity |
| :--- | :--- | :--- | :--- |
| **Init (`hash_init`)** | `O(M)` | `O(M)` | `O(M)` |
| **Insert (`hash_insert`)** | `O(L)` | `O(N)` | `O(1)` per node |
| **Search (`hash_search`)** | `O(L)` | `O(N)` | N/A |
| **Remove (`hash_remove`)** | `O(1)` | `O(N)` | N/A |
| **Reverse ID Lookup** | `O(1)` | `O(1)` | `O(V)` (Reverse Array) |

* *`M` is the table size, `N` is the number of elements, `V` is the maximum vertex count, and `L` is the length of the string key.*
* *The Worst-Case `O(N)` for search/insert only occurs if all strings produce the exact same hash collision, which is heavily mitigated by the dynamic table resizing (`TARGET_ALPHA`).*

---

## 📂 Project Structure

* **`dynagraph.h` / `dynagraph.c`:** The main public API. Manages the lifecycle of vertices and routes requests to the underlying data structures.
* **`hdt.h` / `hdt.c`:** The Holm-de Lichtenberg-Thorup connectivity engine. Manages level-promotion logic and the spanning forest.
* **`et_tree.h` / `et_tree.c`:** The implicit-key splay tree implementation forming the Euler Tour Trees.
* **`hash_table.h` / `hash_table.c`:** `O(1)` dynamic hash table for fast `Item`-to-`ID` mapping and `ID`-to-`Item` reverse lookups.
* **`item.h` / `item.c`:** Generic item definitions to abstract payload data away from the routing logic.
* **`main.c`:** A massive 6-phase stress-testing benchmark suite.

---

## 🚀 The 6-Phase Benchmark Suite

The included `main.c` file runs a comprehensive 6-phase stress test to validate the structural integrity of the algorithms under heavy loads. To compile and run manually via GCC:

```bash
gcc -O3 main.c hdt.c et_tree.c hash_table.c item.c -o DynaGraph
./DynaGraph
