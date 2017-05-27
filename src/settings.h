#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED


/*
 * Which CPU instructions are allowed in score calculations?
 */
#define ALLOW_SSE 01
#define ALLOW_AVX 01

/*
 * Skip items that cannot lead to a winning result anyway.
 * Avoidance covers items in several groups:
 * - the current line (horizontal)
 * - the triangle below (lines with higher I)
 * - the triangle on top (lines with lower I) -- only if order is randomized (else these items would be completed already!)
 * Note: Upper triangle is never skipped currently due to poor cache behavior.
 */
#define ENABLE_SKIPPING 01

/*
 * Remember how many items were skipped (only for statistical purposes)
 * This slows the program down noticeably.
 * Currently only works with STRATEGY_MEMSET
 */
#define SKIPPING_STATS 0

/*
 * Maximum distance in which skipping should be applied. Large values lead to potentially more skipping but take longer to write to memory.
 * If this is small, the permutation cycle length can be decreased as well (it should be at least twice this value).
 *
 * Setting this to 0 disables vertical skipping.
 * Then only the current line will be avoided, upper and lower triangle will NOT be considered.
 * This leads to a severe slowdown).
 *
 * The maximum sensible value is 100/3 = 33
 *
 * Note: Some strategies ignore this (in particular the AVXset variants)
 */
//#define VERTICAL_SKIP_LIMIT (100/3)
#define VERTICAL_SKIP_LIMIT (16)
//#define VERTICAL_SKIP_LIMIT (5)
//#define VERTICAL_SKIP_LIMIT (1)

/*
 * If true, activates several integrity checks.
 * These are only for debugging and slow down the program.
 */
#define CHECK_INTEGRITY 0

/*
 * How should the lookup table be implemented?
 *
 * memset
 * n rows, one for each of the following lines. A single byte in each row denotes if this position can be skipped.
 * Speed: ~68k/s
 *
 * DLList
 * n doubly linked list of nodes, one for each following line. Each node represents an interval that is _not_ skipped. List self-adjusts during traversal when intervals overlap.
 * Speed varies, sometimes a bit slower, ~58k/s
 *
 * SLList
 * 1 singly linked list of nodes. Each node represents an interval that IS skipped. When switching to the next row, all intervals are decremented first and empty nodes are discarded.
 * Speed fluctuates wildly, sometimes extremely slow, generally not faster than memset
 *
 * AVXset1
 * 1 array of bytes, each representing how many times a position should be skipped. When switching to the next row, all values are decremented first. Similar to the memset strategy, but all operations are performed with AVX instead of scalar.
 * Speed ~91k/s
 *
 * AVXset2
 * Same as AVXset1, but the resulting maximums are calculated immediately. This allows faster calculation of the next non-skipped position.
 * Speed ~97k/s, fastest known method.
 */
#define STRATEGY_MEMSET 100
#define STRATEGY_DLLIST 200
#define STRATEGY_SLLIST 300
#define STRATEGY_AVXSET1 400
#define STRATEGY_AVXSET2 401

//#define LOOKUP_STRATEGY STRATEGY_MEMSET
//#define LOOKUP_STRATEGY STRATEGY_DLLIST
//#define LOOKUP_STRATEGY STRATEGY_SLLIST
//#define LOOKUP_STRATEGY STRATEGY_AVXSET1
#define LOOKUP_STRATEGY STRATEGY_AVXSET2


// ------------------------------------------------------------------------------------------------

#if VERTICAL_SKIP_LIMIT < 0
#error VERTICAL_SKIP_LIMIT must not be negative
#endif

#define REQUIRE_SKIP_MAP (VERTICAL_SKIP_LIMIT > 0 || SPECULATIVE_HORIZONTAL_SKIPPING)

#endif // SETTINGS_H_INCLUDED
