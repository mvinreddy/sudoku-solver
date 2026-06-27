# High-Performance Parallel Sudoku Constraint Solver

A lightweight, bare-metal Sudoku solver engine written in C++20. This project models Sudoku as an **Exact Cover problem**, utilizing **Knuth's Algorithm X** over a quadruply circular-linked **Dancing Links (DLX)** matrix. It features a lock-free parallel execution framework and an internal arena memory allocator to maximize CPU cache locality.

## 🚀 Performance Snapshot
* **Engine Throughput:** ~7,200+ puzzles / second
* **Average Latency:** ~0.13 ms per grid
* **Memory Strategy:** Zero heap allocations inside the hot execution loop.

---

## 🛠️ Project Architecture

* **`include/dlx_solver.hpp`**: The core exact cover engine. Manages the toroidal link network and handles state rollback during backtracking.
* **`src/solver.cpp`**: Interactive command-line application for verifying individual puzzle grids.
* **`src/benchmark_solver.cpp`**: Multi-threaded workload pipeline that divides batch puzzle files across all available physical CPU threads using lock-free synchronization.

---

## 💻 Getting Started

### Prerequisites
* A C++20 compliant compiler (GCC 10+ or Clang 13+)

### Compilation
Compile the project from the root directory using aggressive optimizations (`-O3`):

```bash
# Build the individual interactive solver
g++ -O3 -std=c++20 -I./include src/solver.cpp -o solver

# Build the parallel benchmark engine
g++ -O3 -std=c++20 -I./include src/benchmark_solver.cpp -o benchmark_solver
