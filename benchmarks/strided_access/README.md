# Strided Access Benchmark
Benchmarks memory access pattern representative of structured, directionally-split,
WENO-based CFD solvers. While WENO schemes perform more FLOPs per memory
access and may benefit from hardware prefetching, 
the memory access pattern remains one of the primary determinants of the scheme's performance.
This benchmark focuses on the memory behavior by investigating effects like grid size, loop ordering, 
cache and TLB misses for a 3D grid. Perhaps with the exception of the loop where the stride-1 sweep 
and sweep direction align, the fastest loop ordering is not reliably predicted by common 
rules of thumb (contiguous-innermost or sweep-direction-innermost), and the optimal loop ordering 
depends on a complex interaction between compiler vectorization and cache reuse.

## Kernels
The benchmark investigates the performance of the X, Y and Z sweep of the 
directionally split 3D WENO scheme. Storage for the benchmark is provided by a mmap-backed
buffer wrapped in a RAII class. An object of this class satisfies the range interface
and allows for huge-page-backed allocation at run time. 
A std::mdspan with layout_left (column major) enbles
sweeping in the different directions. Additionally the cost of the mdspan abstraction
is compared to the cost of hand-rolled indexing for the sweep in the z direction.
To reduce the amount of loops to measure, we focus on loop orderings that we would expect to
see in production CFD codes or that appear to be memory friendly according to common
rules of thumb that can be found in perf optimization literature. Typically these either require the 
sweep direction be the innermost loop, or that the unit stride direction is the innermost loop.
Regardless of the innermost loop, little is said about the order of the other two loops. 
Because we are not measuring all the loop permutations, it's possible that we do not 
discover the global optimum.

### X sweep
For layout_left, a sweep in the X direction is a stride-1 sweep. For this sweep direction we 
test only two permutations of the loops. Both loops have the unit stride as the innermost loop, but
they differ in the order of the outer loops. The first loop we measure is the textbook loop
```C++
for k = ...
	for j = ...
		for i = ...
			sum += sol[i-2,j,k] + sol[i-1,j,k] + sol[i,j,k] + sol[i+1,j,k] + sol[i+2,j,k]
```
which is also expected to be the fastest loop of any sweep. We then measure the performance of
```C++
for j = ...
	for k = ...
		for i = ...
			sum += sol[i-2,j,k] + sol[i-1,j,k] + sol[i,j,k] + sol[i+1,j,k] + sol[i+2,j,k]
```
### Y sweep
For the sweep in the y direction we decided to investigate first the loop 
```C++
for k = ...
	for i = ...
		for j = ...
			sum += sol[i,j-2,k] + sol[i,j-1,k] + sol[i,j,k] + sol[i,j+1,k] + sol[i,j+2,k]
```
that follows the sweep-direction-innermost rule of thumb. We then investigate the loop
```C++
for i = ...
	for k = ...
		for j = ...
			sum += sol[i,j-2,k] + sol[i,j-1,k] + sol[i,j,k] + sol[i,j+1,k] + sol[i,j+2,k]
```
that could possibly represent the worst case for this sweep direction. We then look at having the
1-strided loop as the innermost loop with the sweep direction second innermost and finally the 
Z direction as the outermost loop 
```C++
for k = ...
	for j = ...
		for i = ...
			sum += sol[i,j-2,k] + sol[i,j-1,k] + sol[i,j,k] + sol[i,j+1,k] + sol[i,j+2,k]
```
And finally we keep the unit stride as the innermost loop and we invert the order of the outer loops
```C++
for j = ...
	for k = ...
		for i = ...
			sum += sol[i,j-2,k] + sol[i,j-1,k] + sol[i,j,k] + sol[i,j+1,k] + sol[i,j+2,k]
```
### Z sweep
For the sweep in the Z direction we use the data gathered from the study of the Y sweep and
we focus only on two loops. The first will have the sweep direction as the innermost loop and the 
other loops are chosen in a somewhat arbitrary fashion
```C++
for j = ...
	for i = ...
		for k = ...
			sum += sol[i,j,k-2] + sol[i,j,k-1] + sol[i,j,k] + sol[i,j,k+1] + sol[i,j,k+2]
```
Then we switch the order of the k and i loops from before so that the 1-strided loop is innermost 
followed by the sweep direction
```C++
for j = ...
	for k = ...
		for i = ...
			sum += sol[i,j,k-2] + sol[i,j,k-1] + sol[i,j,k] + sol[i,j,k+1] + sol[i,j,k+2]
```
Furthermore, for this last loop we also compare the performance of mdspan's operator[] with the
performance of direct indexing into linear memory.
## Results

The conclusions in this section are for an intel laptop (lscpu | grep -E "Model name|L1|L2|L3|Line size")


Model name:                              11th Gen Intel(R) Core(TM) i5-1145G7 @ 2.60GHz  
L1d cache:                               192 KiB (4 instances)  
L1i cache:                               128 KiB (4 instances)  
L2 cache:                                5 MiB (4 instances)  
L3 cache:                                8 MiB (1 instance)  


with 64 bytes cache line (getconf LEVEL1_DCACHE_LINESIZE).

The code is compiled with optimization level 3 (O3) and with -march=native in order to enable AVX512 
instructions.
All the kernels are run by pinning to a thread (taskset -c 2 ./strided_access) in performance mode.


### X sweep
The asm code for the two loop ordering for the X sweep shows that in both cases the compiler 
emits identically AVX512 instructions for the inner loop.
<table>
<tr>
<th>X sweep -- k-j-i</th>
<th>X sweep -- j-k-i</th>
</tr>
<tr>
<td>

	vmovdqu	-100(%rdx,%r8,4), %ymm5
	vmovdqu	-68(%rdx,%r8,4), %ymm6
	vmovdqu	-36(%rdx,%r8,4), %ymm7
	valignd	$7, %ymm0, %ymm5, %ymm8         # ymm8 = ymm0[7],ymm5[0,1,2,3,4,5,6]
	vmovdqu	-4(%rdx,%r8,4), %ymm0
	.loc	1 126 38 is_stmt 0              # benchmarks/strided_access/main.cpp:126:38 
	vpaddd	%ymm1, %ymm8, %ymm1
	valignd	$7, %ymm5, %ymm6, %ymm8         # ymm8 = ymm5[7],ymm6[0,1,2,3,4,5,6]
	vpaddd	%ymm2, %ymm8, %ymm2
	valignd	$7, %ymm6, %ymm7, %ymm8         # ymm8 = ymm6[7],ymm7[0,1,2,3,4,5,6]
	vpaddd	%ymm3, %ymm8, %ymm3
	valignd	$7, %ymm7, %ymm0, %ymm8         # ymm8 = ymm7[7],ymm0[0,1,2,3,4,5,6]
	vpaddd	%ymm4, %ymm8, %ymm4
	.loc	1 126 58                        # benchmarks/strided_access/main.cpp:126:58 
	vpaddd	-112(%rdx,%r8,4), %ymm1, %ymm1
	vpaddd	-80(%rdx,%r8,4), %ymm2, %ymm2
	vpaddd	-48(%rdx,%r8,4), %ymm3, %ymm3
	vpaddd	-16(%rdx,%r8,4), %ymm4, %ymm4
	...

</td>
<td>

	vmovdqu	-100(%rdx,%r8,4), %ymm5
	vmovdqu	-68(%rdx,%r8,4), %ymm6
	vmovdqu	-36(%rdx,%r8,4), %ymm7
	valignd	$7, %ymm0, %ymm5, %ymm8         # ymm8 = ymm0[7],ymm5[0,1,2,3,4,5,6]
	vmovdqu	-4(%rdx,%r8,4), %ymm0
	.loc	1 146 38 is_stmt 0              # benchmarks/strided_access/main.cpp:146:38 
	vpaddd	%ymm1, %ymm8, %ymm1
	valignd	$7, %ymm5, %ymm6, %ymm8         # ymm8 = ymm5[7],ymm6[0,1,2,3,4,5,6]
	vpaddd	%ymm2, %ymm8, %ymm2
	valignd	$7, %ymm6, %ymm7, %ymm8         # ymm8 = ymm6[7],ymm7[0,1,2,3,4,5,6]
	vpaddd	%ymm3, %ymm8, %ymm3
	valignd	$7, %ymm7, %ymm0, %ymm8         # ymm8 = ymm7[7],ymm0[0,1,2,3,4,5,6]
	vpaddd	%ymm4, %ymm8, %ymm4
	.loc	1 146 58                        # benchmarks/strided_access/main.cpp:146:58 
	vpaddd	-112(%rdx,%r8,4), %ymm1, %ymm1
	vpaddd	-80(%rdx,%r8,4), %ymm2, %ymm2
	vpaddd	-48(%rdx,%r8,4), %ymm3, %ymm3
	vpaddd	-16(%rdx,%r8,4), %ymm4, %ymm4
	...

</td>
</tr>
</table>
However, the memory traffic between the two loops differs slightly
<table>
<tr>
<th>X sweep -- k-j-i</th>
<th>X sweep -- j-k-i</th>
</tr>
<tr>
<td>

	399,592         dTLB-load-misses #    0.00% of all dTLB cache accesses
	18,950,758,693  dTLB-loads                                                            
	894,625,588     L1-dcache-load-misses                                                 
	20,713,683      LLC-load-misses                                                       
	30,664,984,729  cycles                                                                
	45,455,903,777  instructions #    1.48  insn per cycle

</td>
<td>

	11,673,345      dTLB-load-misses #    0.06% of all dTLB cache accesses
	18,747,154,607  dTLB-loads                                                            
	910,704,600     L1-dcache-load-misses                                                 
	34,705,636      LLC-load-misses                                                       
	31,079,451,614  cycles                                                                
	44,575,022,015  instructions #    1.43  insn per cycle 

</td>
</tr>
</table>

The k-j-i loop has fewer TLB load misses and L1 cache misses so that the CPU can issue slightly more
instructions per cycle compared to the j-k-i loop. Practically, this means that the k-j-i loop is
approximately 25ms faster than the j-k-i with a 1% standard deviation in both cases. 

### Y sweep

<table>
<tr>
<th>Y sweep -- k-i-j</th>
<th>Y sweep -- j-k-i</th>
</tr>
<tr>
<td>

	#   Parent Loop BB0_17 Depth=1
	# =>  This Loop Header: Depth=2
	#       Child Loop BB0_19 Depth 3
	#         Child Loop BB0_20 Depth 4
    movq    %rbx, %rax
    shlq    $10, %rax
    movabsq    $4503599627370494, %rdx         # imm = 0xFFFFFFFFFFFFE
    leaq    (%rax,%rdx), %rcx
    leaq    1(%rax,%rdx), %rdx
    orq    $2, %rax
    vpbroadcastq    %rcx, %ymm0
    vpsllq    $12, %ymm0, %ymm1
    vmovdqa    .LCPI0_17(%rip), %ymm7          # ymm7 = [4136960,4141056,4145152,4149248]
	.Ltmp213:
    .loc    74 53 110 is_stmt 1             # __mdspan/default_accessor.h:53:110 @[ main.cpp:166:20]
    vpaddq    %ymm7, %ymm1, %ymm2
    vmovdqu    %ymm2, 112(%rsp)                # 32-byte Spill
    vpbroadcastq    %rdx, %ymm2
    vpbroadcastq    %rbx, %ymm8

		...

	#   Parent Loop BB0_17 Depth=1
	#     Parent Loop BB0_18 Depth=2
	# =>    This Loop Header: Depth=3
	#         Child Loop BB0_20 Depth 4
    leaq    (%r12,%rax,4), %rcx
    vmovd    %ebp, %xmm25
    vpxord    %xmm26, %xmm26, %xmm26
    movl    $1008, %edx                     # imm = 0x3F0
    vmovdqa64    .LCPI0_26(%rip), %ymm21 # ymm21 = [2,3,4,5]
    vmovdqa64    .LCPI0_25(%rip), %ymm22 # ymm22 = [6,7,8,9]
    vpbroadcastq    .LCPI0_27(%rip), %ymm9  # ymm9 = [8,8,8,8]
    vpbroadcastq    .LCPI0_28(%rip), %ymm10 # ymm10 = [16,16,16,16]

		...

	#   Parent Loop BB0_17 Depth=1
	#     Parent Loop BB0_18 Depth=2
	#       Parent Loop BB0_19 Depth=3
	# =>      This Inner Loop Header: Depth=4
    vpaddq    %ymm9, %ymm21, %ymm23
    vpaddq    %ymm9, %ymm22, %ymm24
    vpaddq    %ymm22, %ymm0, %ymm1
    vpaddq    %ymm21, %ymm0, %ymm28
	.Ltmp218:
    .loc    74 53 110 is_stmt 1             # __mdspan/default_accessor.h @[ main.cpp:166:20 ]
    vpsllq    $12, %ymm28, %ymm28
    vpsllq    $12, %ymm1, %ymm1
	.Ltmp219:
	.loc    1 166 20                        # benchmarks/strided_access/main.cpp:166:20 
	kxnorw    %k0, %k0, %k1
	vpxord    %xmm29, %xmm29, %xmm29
	vpgatherqd    (%rcx,%ymm1), %xmm29 {%k1}
	kxnorw    %k0, %k0, %k1
	vpxor    %xmm1, %xmm1, %xmm1
	vpsllq	$10, %ymm8, %ymm3
	vpbroadcastq	%rax, %ymm4

</td>
<td>

	vmovdqu	-100(%rdx,%r8,4), %ymm5
	vmovdqu	-68(%rdx,%r8,4), %ymm6
	vmovdqu	-36(%rdx,%r8,4), %ymm7
	valignd	$7, %ymm0, %ymm5, %ymm8         # ymm8 = ymm0[7],ymm5[0,1,2,3,4,5,6]
	vmovdqu	-4(%rdx,%r8,4), %ymm0
	.loc	1 146 38 is_stmt 0              # benchmarks/strided_access/main.cpp:146:38 
	vpaddd	%ymm1, %ymm8, %ymm1
	valignd	$7, %ymm5, %ymm6, %ymm8         # ymm8 = ymm5[7],ymm6[0,1,2,3,4,5,6]
	vpaddd	%ymm2, %ymm8, %ymm2
	valignd	$7, %ymm6, %ymm7, %ymm8         # ymm8 = ymm6[7],ymm7[0,1,2,3,4,5,6]
	vpaddd	%ymm3, %ymm8, %ymm3
	valignd	$7, %ymm7, %ymm0, %ymm8         # ymm8 = ymm7[7],ymm0[0,1,2,3,4,5,6]
	vpaddd	%ymm4, %ymm8, %ymm4
	.loc	1 146 58                        # benchmarks/strided_access/main.cpp:146:58 
	vpaddd	-112(%rdx,%r8,4), %ymm1, %ymm1
	vpaddd	-80(%rdx,%r8,4), %ymm2, %ymm2
	vpaddd	-48(%rdx,%r8,4), %ymm3, %ymm3
	vpaddd	-16(%rdx,%r8,4), %ymm4, %ymm4
	...

</td>
</tr>
</table>


### Conclusions
