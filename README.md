# -Plagiarism-Checker

# High-Performance Concurrent Plagiarism Checker (C++)

A scalable, multi-stage C++ system designed to detect software code similarity across both single-pair comparisons and massive batch submission sets. The engine combines token-based rolling hashing, dynamic programming, 2D segment tree spatial queries, and multi-threaded worker pools to identify global, paraphrased, and patchwork code duplication.

---

## Technical Context & Problem Statement

Standard line-by-line diff tools and basic string-matching algorithms fail when detecting source code plagiarism because simple modifications—such as renaming variables, reordering functions, changing loop structures, or stitching together fragments from multiple sources—bypass naive checks.

This project solves these limitations by tokenizing source code into abstract token streams and applying a **two-phase analysis pipeline**:
1. **Phase 1 (Granular Structural Analysis)**: Deep pair-wise comparison using advanced algorithmic structures to detect complex structural overlaps.
2. **Phase 2 (Batch Concurrency Engine)**: Multi-threaded batch comparison pipeline that scales linearly across thousands of student submissions without thread contention or memory bottlenecks.

---

## Architectural Deep Dive

### Phase 1: Pairwise Deep Inspection Engine

Phase 1 evaluates two submissions at a time to identify shared code blocks, variable renames, and structural reordering.

* **Tokenization & Rolling Hashing (Rabin-Karp)**  
  Source files are parsed into abstract syntax tokens (ignoring whitespace, comments, and variable names). A sliding window with Rabin-Karp rolling hashing produces $O(1)$ hash updates, allowing fast identification of matching $k$-grams across files.

* **Dynamic Programming (Block Match Alignment)**  
  A modified DP matrix calculates contiguous and non-contiguous structural alignment (similar to Longest Common Subsequence). This isolates structural logic from minor syntax variations, making the detection robust against variable renaming and control-flow modifications.

* **2D Segment Trees (Spatial Match Queries)**  
  To pinpoint dense clusters of matching tokens in $O(\log^2 N)$ query time, the engine overlays match results onto a 2D matrix where rows represent File A tokens and columns represent File B tokens. A **2D Segment Tree** performs range-maximum and range-sum queries over sub-grids, enabling rapid identification of dense matching sub-blocks without re-scanning the matrix in $O(N^2)$ time.

---

### Phase 2: Concurrent Batch Processing Engine

When analyzing a directory containing hundreds or thousands of submissions, performing $O(M^2)$ pairwise checks sequentially becomes a major bottleneck. Phase 2 introduces a thread-safe multi-threaded architecture.

* **Thread Pool & Work Stealing**  
  A fixed pool of worker threads (`std::thread`) continuously pulls submission pairs from a synchronized work queue.

* **Thread Safety & Synchronization**  
  Uses `std::mutex` for shared queue access and `std::condition_variable` for worker notification, preventing busy-waiting and ensuring safe concurrent writes to the global similarity matrix.

* **Thread-Local Storage & Fast-Path Hashing**  
  To minimize lock contention, each thread computes and caches rolling hashes into thread-local hash maps (`std::unordered_map`). Thread synchronization occurs only when pushing final similarity metrics to the global result collector.

---

## Plagiarism Detection Capability

| Plagiarism Type | Mechanism of Detection | Key Algorithm / Structure |
| :--- | :--- | :--- |
| **Global (Direct Copy)** | Full-file hash verification and token sequence matching. | Rabin-Karp Rolling Hash |
| **Paraphrased (Renamed/Reordered)** | Abstract tokenization combined with structural alignment matrices. | Dynamic Programming |
| **Patchwork (Multi-source Stitches)** | Spatial cluster detection across multiple source sub-grids. | 2D Segment Trees |

---

## Data Structures & STL Usage

* `std::unordered_map` / `std::unordered_set`: Used for $O(1)$ average-time hash lookup tables when matching $k$-gram token fingerprints.
* `std::vector`: Provides contiguous array storage for DP matrices and flattened 2D Segment Tree nodes to maximize CPU cache locality.
* `std::queue`: Manages submission pairs in the Phase 2 concurrent job queue.
* `std::mutex` & `std::condition_variable`: Coordinates job distribution and thread sleep/wake cycles.

---

## Build & Execution

### Prerequisites
* C++17 compliant compiler (`g++` 8+ or `clang++` 7+)
* POSIX Threads (`pthread`)

### Compilation

```bash
g++ -std=c++17 -O3 -pthread src/*.cpp -o plagiarism_checker


# Run pairwise detection (Phase 1)
./plagiarism_checker --mode=pairwise --file1=sub1.cpp --file2=sub2.cpp

# Run batch concurrent detection (Phase 2)
./plagiarism_checker --mode=batch --dir=./submissions --threads=8
