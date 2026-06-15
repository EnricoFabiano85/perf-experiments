# Strided Access Benchmark
Benchmarks memory access pattern representative of structured, directionally-split,
WENO-based computational fluid dynamics solvers. These kernels are notoriously memory-latency bound and represent a perfect benchmark for HW and SW prefetcher (and for their interaction).

## Kernels


## Results

The conclusions in this section are for an intel laptop (lscpu | grep -E "Model name|L1|L2|L3|Line size")


Model name:                              11th Gen Intel(R) Core(TM) i5-1145G7 @ 2.60GHz  
L1d cache:                               192 KiB (4 instances)  
L1i cache:                               128 KiB (4 instances)  
L2 cache:                                5 MiB (4 instances)  
L3 cache:                                8 MiB (1 instance)  


with 64 bytes cache line (getconf LEVEL1_DCACHE_LINESIZE).

All the kernels are run by pinning to a thread (taskset -c 2 ./indirect_access) in performance mode.


### Conclusions
