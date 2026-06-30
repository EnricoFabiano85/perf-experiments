# Strided Access Benchmark
Benchmarks memory access pattern representative of structured, directionally-split,
WENO-based CFD solvers. While WENO schemes perform more FLOPS per memory
access and may benefit from hardware prefetching, 
the memory access pattern remains one of the primary determinants of the scheme's performance.
This benchmark focuses on the memory behavior by investigating effects like grid size, loop ordering, 
cache and TLB misses for a 3D grid. Perhaps with the exception of the loop where the stride-1 sweep 
and sweep direction align, the fastest loop ordering is not reliably predicted by common 
rules of thumb (contiguous-innermost or sweep-direction-innermost), and the optimal loop ordering 
depends on a complex interaction between compiler vectorization and cache reuse.

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


### Results
<table>
<tr>
<th>Y loop - Slow</th>
<th>Y loop - Fast</th>
</tr>
<tr>
<td>

             1,205      dTLB-load-misses
       773,053,548      dTLB-loads                                                            
       183,647,639      L1-dcache-load-misses                                                 
       149,208,844      LLC-load-misses                                                       
     5,967,889,942      cycles                                                                
     2,884,107,915      instructions  

</td>
<td>

             1,044      dTLB-load-misses
       770,551,932      dTLB-loads                                                            
       220,121,085      L1-dcache-load-misses                                                 
           820,920      LLC-load-misses                                                       
     1,519,155,035      cycles                                                                
     2,860,611,359      instructions

</td>
</tr>
</table>

### Conclusions
