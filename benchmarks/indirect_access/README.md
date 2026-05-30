# Indirect Access Benchmark
Benchmarks memory access pattern representative of unstructured 
computational fluid dynamics solvers, where cell connectivity results
in indirect (gather) access to solution data. The assumption in this example
is that the unstructured mesh has not been reordered.

## Kernels
There are 6 kernels in the benchmark:

**Sequential access** -- Fastest kernel thanks to HW prefetcher and trivial simd vectorization (compiler)
**Sequential access through index dereference** -- Second fastest kernel. Shows the cost of index indirection
**Scalar indirect access with shuffled indices** -- Representative of how codes are typically written. 
**Shuffled indices _ SW prefetching** -- The prefetcher requests new memory as the current one is incoming and about to be processed. Requires tuning of the prefetching distance.
**AVX512 explicit gather** -- Forcing the compiler to use SIMD gather instructions at any optimization level
**AVX512 explicit gather + SW prefetching** -- Adds SW prefetching to SIMD gather

## Results

The conclusions in this section are for an intel laptop (lscpu | grep -E "Model name|L1|L2|L3|Line size")

Model name:                              11th Gen Intel(R) Core(TM) i5-1145G7 @ 2.60GHz
L1d cache:                               192 KiB (4 instances)
L1i cache:                               128 KiB (4 instances)
L2 cache:                                5 MiB (4 instances)
L3 cache:                                8 MiB (1 instance)
Vulnerability L1tf:                      Not affected

with 64 bytes cache line (getconf LEVEL1_DCACHE_LINESIZE)



