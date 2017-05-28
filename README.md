Two basic enhancements to Smith Waterman genome matching on modern x64 processors
=================================================================================
For the C++ programming challenge 2017 of the University of Paderborn


Abstract
--------
Beyond simply calculating the correct result for Smith Waterman genome matching, there is a wide range of possible performance improvements that speed up the algorithm by several magnitudes. We describe an improved comparison function using vector instructions and several structures to efficiently avoid comparisons altogether.


Introduction
------------
The task given [<a name="rref-SCH17" href="#ref-SCH17">SCH17</a>] is to implement the Smith Waterman algorithm (SWA) [<a name="rref-SW81" href="#ref-SW81">SW81</a>] to match two genome sequences. SWA is on of the few algorithms guaranteed to find an locally optimal alignment, but for this reason it is also one of the slowest. As such, the challenge is to find a way to improve the runtime of the program. The match length is fixed to 50. In the comparison, matches gain 2 points, mismatches and gaps suffer -1 point. The given sample files are in FASTA format [<a name="rref-WIK17" href="#ref-WIK17">WIK17</a>].

The author is not familiar with the field of bioinformatics and had no exposure to the relevant literature. As such, the ideas presented here are his own and are not to be comparable with the state of the art. Code samples are given in C++ making use of intrinsics for vector instructions [<a name="rref-INT16" href="#ref-INT16">INT16</a>]



Improving SWA comparison with vector instructions
=================================================

It is very important to quickly calculate the matrix of the SWA, in which we compare to given genome parts of fixed length 50. The basic idea to improve this is to work on multiple bytes together instead of only a single byte. x64 processors allow this with vector operations: The same operation is applied to many values at once, up to 16 in the case of SSE, and up to 32 in the case of AVX.

As a general note, buffer size will often be increased beyond the required size. This is done to avoid manually checking the bounds: It is faster to just do a few more operations than to check each time if an operation is required. For instance, the SSE algorithm uses buffers of size 65 (4 * 16 + 1) instead of 50.

Our goal is to fill all rows in the SMA matrix row by row. Values depend on the previous row as well as previous values in the same row, so this is not entirely trivial. The final value in a row is the maximum of these values:
a) 0
b) -1 + value at same position in previous row
c) -1 + value at previous position in same row
d) -1 + value at previous position in previous row, if the character there is not equal to the current base character
e) +2 + value at previous position in previous row, if the character there is equal to the current base character


Case a) using saturated subtraction
------------------------------------
Common to many cases is subtraction of 1. This means we may just as well adjust to delay this subtraction. Consider these cases instead:

<ol type="a">
<li>0</li>
<li>value at same position in previous row</li>
<li>value at previous position in same row</li>
<li>value at previous position in previous row, if the character there is not equal to the current base character</li>
<li>3 + value at previous position in previous row, if the character there is equal to the current base character</li>
</ol>

Add -1 to the result with saturation. Saturation means that the result is bounded by 0, so saturated addition of 4 and -7 is 0, not -3. In fact, this allows us to omit case a) altogether (this can easily be proven by induction).


Cases b) and d) accessing the previous row
-------------------------------------------
Case b) is one of the best cases for parallelization. We may simply read a vector of 16 bytes from the previous row and are done.

	__m128i prev1 = _mm_load_si128  ((__m128i*) &(prev[(x + 1) * 16 - 0]));
	__m128i i1    = prev1;

Case d) and e) are handled by comparing 16 bytes at once with a vector that consists entirely of the base character. The comparison result will be 0 or -1, so we AND the result with the value 3 to get a vector of 0s and 3s. Finally, we add the value in the previous row (shifted by 1).

	uint8_t ai    = p1[y];
	__m128i aiv   = _mm_set1_epi8   (ai);
	__m128i bjv   = _mm_loadu_si128 ((__m128i*) &(p2[x * 16]));
	__m128i omega = _mm_cmpeq_epi8  (aiv, bjv);
	        omega = _mm_and_si128   (omega, plus_3);
	__m128i prev0 = _mm_loadu_si128 ((__m128i*) &(prev[(x + 1) * 16 - 1]));
	__m128i i3    = _mm_adds_epu8   (prev0, omega);


Case c) Self-dependent decrementing maximum
-------------------------------------------
This case is by far the most difficult. Each value depends on _all_ previous values.

	c[0] = max{ c[0] }
	c[1] = max{ c[0] - 1, c[1] }
	c[2] = max{ c[0] - 2, c[1] - 1, c[2] }
	...
	c[49] = max { c[0] - 49, ..., c[48] - 1, c[49] }

Ordinarily, we need to loop like this:

	uint8_t (&c) [51] = ...
	for (int x = 0; x < 50; x++) {
		if (c[x + 1] < c[x] - 1) {
			c[x + 1] = c[x] - 1;
		}
	}

This approach suffers _a lot_ from branch misprediction. Larger values are often near then end, but we can _not_ improve by doing the operation in reverse, as the required order is from left to right. We may of course use a cmov instead of a mov instruction to avoid branch penalties to an extend, but the resulting algorithm is still not very fast.

However, it turns out this dilemma can still be solved. First, take the bytewise maximum of all of the above result. We call the resulting vector c[0..49]. We shuffle every second byte one to the right, and fill the rest with zeros. Shuffling refers to the act of saying which byte goes where in a vector, it has nothing random to it. The result is stored in c'.

	c' = 0, c[0], 0, c[1], ..., 0, c[48]

Subtract 1 from each value in c' with saturation, such that

	c' = 0, c[0]-1, 0, c[2]-1, ..., c[48]-1

We now calculate the bytewise maximum of c and c' and store it in c:

	c = max{c[0], 0}, max{c[1], c[0]-1}, max{c[2], 0}, max{c[3], c[2]-1}, ..., max{c[49], c[48]-1}

This way, c contains the pairwise maximums of adjacent bytes. How does this help? We are now free to work with only every second byte and ignore the rest. We do so by again shuffling the bytes in position. The result again goes to c'.

	c' = 0, 0, c[1], c[1], 0, 0, c[5], c[5], ..., 0, 0, c[13], c[13]

That is to say, the maximum value at every position 1 + 4k is copied to the range [4k+2..4k+3]. We now subtract a repetition of 0, 0, 1, 2 from c':

	c' = 0, 0, c[1]-1, c[1]-2, 0, 0, c[5]-1, c[5]-2, ..., 0, 0, c[13]-1, c[13]-2

And finally, we again take the maximum of c and c':

	c = max{ max{c[0], 0     }, 0 },
	    max{ max{c[1], c[0]-1}, 0 },
	    max{ max{c[2], 0     }, max{c[1], c[0]-1} - 1},
	    max{ max{c[3], c[2]-1}, max{c[1], c[0]-1} - 2},
	    ...
	    max{ max{c[49], c[48]-1}, max{c[47], c[46]-1} - 2}

This looks quite complicated, but it is just the same as:

	c = max{ c[0] },
	    max{ c[1], c[0] - 1 },
	    max{ c[2], c[1] - 1, c[0] - 2},
	    max{ c[3], c[2] - 1, c[1] - 2, c[0] - 3},
	    max{ c[4] },
	    max{ c[5], c[4] - 1 },
	    ...,
	    max{ c[49], c[48] - 1, c[47] - 2, c[46] - 3}

Let us do one more step to visualize the pattern. We build c' by shuffling the values at positions 3 + 8k to ranges [8k+4..8k+7] and subtracting a repetition of 0, 0, 0, 0, 1, 2, 3, 4:

	c' = 0, 0, 0, 0, c[3] - 1, c[3] - 2, c[3] - 3, c[3] - 4, 0, ..., c[11] - 4

c again is set to the maximum of c and c'. To show just one example, the final value in c is then:

	c[49] = max{
	            max{ c[49], c[48] - 1, c[47] - 2, c[46] - 3},
	            max{ c[45], c[44] - 1, c[43] - 2, c[42] - 3} - 4
	        }

In other words:

	c = max{ max{ c[0] }, 0},
	    max{ max{ c[1], c[0] - 1 }, 0},
	    max{ max{ c[2], c[1] - 1, c[0] - 2}, 0},
	    max{ max{ c[3], c[2] - 1, c[1] - 2, c[0] - 3}, 0},
	    max{ max{ c[4], max{ c[3], c[2] - 1, c[1] - 2, c[0] - 3} - 1}},
	    max{ max{ c[5], c[4] - 1}, max{ c[3], c[2] - 1, c[1] - 2, c[0] - 3} - 2} }},
	    ...,
	    max{ max{ c[49], c[48] - 1, c[47] - 2, c[46] - 3}, max{ c[45], c[44] - 1, c[43] - 2, c[42] - 3} - 4}

Or more plainly:

	c = max{ c[0] },
	    max{ c[1], c[0] - 1 },
	    max{ c[2], c[1] - 1, c[0] - 2},
	    max{ c[3], c[2] - 1, c[1] - 2, c[0] - 3},
	    max{ c[4], c[3] - 1, c[2] - 2, c[1] - 3, c[0] - 4},
	    max{ c[5], c[4] - 1, c[3] - 2, c[2] - 3, c[1] - 4, c[0] - 5},
	    ...,
	    max{ c[49], c[48] - 1, c[47] - 2, c[46] - 3, c[45] - 4, c[44] - 5, c[43] - 6, c[42] - 7}

Cleary, each step doubles the range we covered. This means the algorithm runs in O(log n), which is better than the previous O(n) because the involved constants are small, and more importantly, it is entirely branchless. After a few more steps, c will contain all applicable maximums.

As a side note: Instead of the first shuffle, we can also just subtract 1 everywhere:

	c' = 0, c[0]-1, c[1]-1, ..., c[49]-1

The bytewise maximum then becomes:

	c = max{c[0], 0}, max{c[1], c[0]-1}, max{c[2], c[1]-1}, ..., max{c[49], c[48]-1}

This does not affect the result, as the values in c' which were 0 before are guaranteed to be less than their counterparts. At the same time, it saves us an SSE register.


Splitting a comparison into register-sized parts
---------------------------------
Since the vector has size 50, but SSE only works on 16 bytes at once, we split all of the previously mentioned operations into four parts, each working on 16 bytes. To avoid checking for special cases near the end, the buffer gets size 65 instead of 50 to accomodate.

To combine the four parts, we must fill a vector with the bytewise maximum of the last value in the previous part, subtract the vector (1, 2, 3, ... 16) from it, and finally take the byte maximum with c. This is nothing else than one more application of the tree algorithm of the previous section, and will glue both parts together well.

In the case of AVX, we use only two parts of 32 bytes instead of four parts of 16 bytes. Gluing then requires a ramp of 1 to 32 instead of 1 to 16, obviously.


Finding the horizonal maximum of a vector
-----------------------------------------
Any time we completed a row, we must find the maximum value it contains. For this purpose, we apply an idea  [<a name="rref-RUS16" href="#ref-RUS16">RUS16</a>] similar to the above tree algorithm. This way, we remain in the xmm registers and avoid branches at O(log n) instead of O(n) runtime.

	__mm256 max = ...;
	max = _mm_max_epu8 (max, _mm_alignr_epi8 (max, max, 1));
	max = _mm_max_epu8 (max, _mm_alignr_epi8 (max, max, 2));
	max = _mm_max_epu8 (max, _mm_alignr_epi8 (max, max, 4));
	max = _mm_max_epu8 (max, _mm_alignr_epi8 (max, max, 8));
	int gm = _mm_extract_epi8 (global_max, 0);

The alignr instructions is used to emulate rotating a whole xmm register, as there sadly is no native rotation operator in SIMD. In the case of AVX, line separation makes this impossible, but we may just use a shift instruction instead and extract the highest byte.



Avoiding comparisons in Smith Waterman
======================================

Considering the matrix of all positions in the first compared to all positions in the second genome, it can easily be shown that the maximum difference between two adjacent cells is limited to 3: A positive match increases the score by 2, a negative match reduces it by 1.

Usually, a threshold is given such that any score below it is discarded as mismatch. This means that any value with distance d (in any direction, horizontally as well as vertically, in short, using Manhatten metric) can only differ by d/3, no calculations within a square of edge length 2*d/3 around it need to be done, as they could not produce results passing the limit.

In our tests, this shortcut improved speed significantly, in particular for the relatively large thresholds that are usually employed. On average, we found that 98-99% of calculations can be skipped for a threshold of 70.

It remains to chose a fitting strategy to implement this square. We explored several options. Often, a skip limit (L) is used: This limit refers to the maximum possible influence radius of a poor comparison result. It is naturally bounded by the maximum score minus the minimum score, again divided by the maximum difference per cell. As such, it must be between 0 and 100/3. Practical values are around 15 to 20.

As a general notice, we found it detrimental not to go through all rows in order. Several permutation strategies were attempted, including randomization, structured maximum distances and structured fixed small or large distances, but they always lead to great failure. This is not supposed to be due to limited cache sizes, as even simply switching every second node lead to noticeable degression. We were unable to ascertain a reason for this phenomenon. As such, we ignore the upper triangle of the square entirely and exclusively work with the rows in natural order.

The strategies were tested with a default sample set consisting of a 2kb and a 230mb file. The measured performance relates to the number of rows checked per second on a 4 core i5 CPU using 4 parallel threads.

<table>
<tr>
<th>Strategy</th>
<th>Performance</th>
<th>Description</th>
</tr>
<tr>
<td>L doubly linked lists, positive</td>
<td>~58k/s</td>
<td>L doubly linked lists of nodes, one for each following line. Each node represents an interval that is _not_ skipped. The list self-adjusts during traversal when intervals overlap.</td>
</tr>
<tr>
<td>1 singly linked list, negative</td>
<td><10k/s</td>
<td>A single singly linked list of nodes. Each node represents an interval that _is_ skipped. When switching to the next row, all intervals are decremented first, empty nodes are discarded.</td>
</tr>
<tr>
<td>L byte arrays with memset</td>
<td>~68k/s</td>
<td>L rows, one for each row subsequent to the current one. A single byte in each row denotes if a position can be skipped.</td>
</tr>
<tr>
<td>1 byte array with lazy AVX</td>
<td>~91k/s</td>
<td>A single array of bytes, each byte representing for how many rows a position should be skipped. The bytes are only set when actively calculated. When switching to the next row, all values are decremented first. Somewhat similar to the memset strategy, but all operations are performed with AVX instead of scalar.</td>
</tr>
<tr>
<td>1 byte array with eager AVX</td>
<td>~97k/s</td>
<td>Same as above, but the resulting maximums are calculated immediately for the whole L-sized interval. This allows faster calculation of the next non-skipped position.</td>
</tr>
</table>

An AVX-enhanced byte array storing the skip values turned out to be generally the fastest. It also had low overhead, as only a single row of the second file size is needed to store all values. Despite our best efforts, we could not devise a list-based strategy that beats even the naive memset approach. While we used a custom allocator to carefully ensure that all nodes are located in adjacent memory pages, it appears pointer following is still a relatively slow operation regardless.



Summary
=======

It was explained how the comparison algorithm could be transformed from linear to logarithmic runtime while becoming entirely branchless. For this purpose, we applied a self-overlapping tree structure. Almost all operations stay in the SIMD registers. A speedup by factor 9 to 10 was observed on a typical machine for an SSSE3 variant. Applying AVX2 lead to an additional speedup of around 20%. If AVX512 were available, we could remove the gluing completely, also lessening register pressure in the process, and likely increase speed significantly again.

We also found a way to avoid comparisons altogether, depending on the poorness of results of nearby calculations. This allowed us reduce the actual work by about 99% with minimal overhead. This also discourages early quitting of comparisons, as it is more benefitial to complete the calculation to be able to use its result to skip more nearby comparisons.

Combining these improvements, we were able to calculate all matches with a threshold of 70 with a 2kb and a 5kb file in 0.025 seconds compared to 22 seconds of an optimized scalar version.



References
==========
[<a name="ref-SCH17" href="#rref-SCH17">SCH17</a>] Schubert, Philipp (2017). Final Project: Implementing the Smith-Waterman Algorithm. Software engineering group of the University of Paderborn, 2017-01-25. https://www.hni.uni-paderborn.de/fileadmin/Fachgruppen/Softwaretechnik/Lehre/CPP_Programming/WS2016_2017/cpp_project.pdf

[<a name="ref-SW81" href="#rref-SW81">SW81</a>] Smith, Temple F. & Waterman, Michael S. (1981). Identification of Common Molecular Subsequences. Journal of Molecular Biology, 147: 195–197. doi: 10.1016/0022-2836(81)90087-5

[<a name="ref-WIK17" href="#rref-WIK17">WIK17</a>] In bioinformatics, FASTA format is a text-based format for representing either nucleotide sequences or peptide sequences, in which nucleotides or amino acids are represented using single-letter codes. The format also allows for sequence names and comments to precede the sequences. The format originates from the FASTA software package, but has now become a standard in the field of bioinformatics. [Wikipedia contributors (2017). FASTA format. Wikipedia, 2017-05-07. https://en.wikipedia.org/w/index.php?title=FASTA_format&oldid=779159111]

[<a name="ref-INT16" href="#rref-INT16">INT16</a>] Intel corporation (2016). 64 and IA-32 Architectures Software Developer’s Manual, Volumes 2A, 2B, 2C & 2D: Instruction Set Reference. 2016-09. http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf

[<a name="ref-RUS16" href="#rref-RUS16">RUS16</a>] Russell, Paul (2016). SIMD finding min/max from an _m128i. Stackoverflow, 2016-12-06. http://stackoverflow.com/a/40989144/39590



Appendix
========

Typical program output

    Usage:
    swa in_path1 in_path2 nThreads threshold out_path
    Starting the program without parameters will use default settings for 4 cores
    
    Using default parameters.
    
    
    Compilation flags: ENABLE_SKIPPING = 1, SKIPPING_STATS = 0, VERTICAL_SKIP_LIMIT = 16, LOOKUP_STRATEGY = 401 (avxset2)
    CPU capabilities (actual/compiled):
            SSE y/y, SSE2 y/y, SSE3 y/y, SSSE3 y/y, SSE4.1 y/y, SSE4.2 y/y
            AVX y/y, AVX2 y/y, AVX512F n/n, AVX512BW ?/n, AVX512VBMI: ?/n
    Selected operation width: 256 bit
    Thread count: 4
    
    
    Reading genes...
    >X71135.1 H.sapiens sox3 gene
    Loaded 2509 effective bytes (file size: 2576b)
    >L08063.1 Homo sapiens sex determination protein (SRY) gene, complete cds
    Loaded 4737 effective bytes (file size: 4880b)
    All files read in 0.002 s
    
    
    Begin processing ...
    Result hash: 6976F3E0 (the regular hash of sox3 & sry is 6976F3E0)
    
    
    Processing complete, total 0.027 s, iterations 0.025 s)
