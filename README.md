This repo contains a series of empirical studies into CPU performance — memory hierarchy, SIMD, branch prediction, and concurrency — with per-benchmark write-ups, plots, and reproducibility scripts.

Measurement uses a small purpose-built C++23 module called `perf` rather than Google Benchmark. The goal is to keep the methodology — cold vs. warm caches, prewarm passes, iteration statistics — visible in code rather than hidden behind a macro-based framework.

Each benchmark lives in its own subdirectory under `benchmarks/`, with a write-up explaining the question, the setup, and the results. Start with prefetching.