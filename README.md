This repo contains a series of empirical studies into CPU performance — memory hierarchy, SIMD, branch prediction, and concurrency — with per-benchmark write-ups, plots, and reproducibility scripts.

Measurement uses a small purpose-built C++23 module called `perf` rather than Google Benchmark. The goal is to keep the methodology — cold vs. warm caches, prewarm passes, iteration statistics — visible in code rather than hidden behind a macro-based framework.

Each benchmark lives in its own subdirectory under `benchmarks/`, with a write-up explaining the question, the setup, and the results.

## Benchmarks

| Benchmark | Question | Key finding |
|---|---|---|
| [indirect_access](benchmarks/indirect_access) | What is the cost of an indirect memory access patter representative of unstructured CFD solvers, and what optimizations help (prefetch, SIMD) help? | On my Intel i5 CPU with clang21 the auto vectorization results in similar assembly to the hand vectorized code. SW prefetch gives ~3% on scalar at optimal distance at the cost of vectorization; no benefit from prefetching for AVX-512 gather; effect is governor- and distribution-dependent. |
| [strided_access](benchmarks/strided_access) | What is the cost of a strided memory access pattern representative of directionally-split 5-point WENO schemses, and what is the role of loop ordering? | Loop ordering affects cache and TLB reuse. Vectorization strategies and instruction overhead play a significant role in memory-efficient loop orderings. Furthermore, these loops all have comparable performance. Experiments confirm the zero-cost overhead of the `mdspan` abstraction (at least with the clang21 compiler).  |
