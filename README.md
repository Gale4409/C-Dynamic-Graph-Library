# ⚡ DynaGraph

**A high-performance C library for Fully Dynamic Graph Connectivity.**

*🚧 Note: This project is currently in active development. 🚧*

## 🎯 Overview
The goal of `DynaGraph` is to efficiently maintain graph connectivity and answer component queries (e.g., "Are nodes A and B in the same connected component?") while edges are continuously inserted or deleted in real-time. 

Instead of naively recalculating the components from scratch using a standard Breadth-First Search or Tarjan's algorithm—which takes $O(V+E)$ time per update—this library explores advanced dynamic data structures (like Dynamic Spanning Forests) to handle updates in sub-linear time.

## 🗺️ Current Roadmap
- [x] Initial architecture design and data structure planning.
- [ ] Implementation of the naive baseline (Full BFS/DFS recalculation) for rigorous benchmarking.
- [ ] Core implementation of the Dynamic Spanning Forest.
- [ ] Complete memory safety verification via rigorous Valgrind testing.
- [ ] Performance benchmarking (Latency vs. Throughput) on large-scale datasets.
