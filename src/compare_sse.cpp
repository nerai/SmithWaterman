/*
 * SSSE3 implementation of Smith Waterman.
 */

#include <iostream>
using namespace std;

#include <emmintrin.h>
#include <tmmintrin.h>

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#include "compare.h"


inline void print (const __m128i m)
{
	alignas(16) uint8_t t[16];
	_mm_store_si128 ((__m128i*) &t, m);

	for (int i = 0; i < 16; i++) {
		cout << unsigned (t[i]) << " ";
	}
	cout << endl;
}

inline void print (const uint8_t (& m)[16 * 5])
{
	for (int i = 16 - 1; i < 16 + 50; i++) {
		cout << unsigned (m[i]) << " ";
	}
	cout << endl;
}

static const __m128i plus_1 = _mm_set1_epi8 (1);
static const __m128i plus_3 = _mm_set1_epi8 (3);
static const __m128i plus_15 = _mm_set1_epi8 (15);
static const __m128i ramp_1_16 = _mm_setr_epi8 (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

static const __m128i shufmask1 = _mm_setr_epi8 (-1, -1, +1, +1, -1, -1, +5, +5, -1, -1, +9, +9, -1, -1, 13, 13);
static const __m128i submask1  = _mm_setr_epi8 (+0, +0, +1, +2, +0, +0, +1, +2, +0, +0, +1, +2, +0, +0, +1, +2);
static const __m128i shufmask2 = _mm_setr_epi8 (-1, -1, -1, -1, +3, +3, +3, +3, -1, -1, -1, -1, 11, 11, 11, 11);
static const __m128i submask2  = _mm_setr_epi8 (+0, +0, +0, +0, +1, +2, +3, +4, +0, +0, +0, +0, +1, +2, +3, +4);
static const __m128i shufmask3 = _mm_setr_epi8 (-1, -1, -1, -1, -1, -1, -1, -1, +7, +7, +7, +7, +7, +7, +7, +7);
static const __m128i submask3  = _mm_setr_epi8 (+0, +0, +0, +0, +0, +0, +0, +0, +1, +2, +3, +4, +5, +6, +7, +8);

int compare_sse (
	const uint8_t* __restrict__ p1,
	const uint8_t* __restrict__ p2
)
{
	alignas(64) uint8_t mat0[16 * 5] = {0};
	alignas(64) uint8_t mat1[16 * 5] = {0};

	p1--;

	__m128i global_max = _mm_setzero_si128 ();

	for (int y = 1; y < 51; y++) {
		__m128i prev_res = _mm_setzero_si128 ();

		alignas(16) uint8_t (& prev)[16 * 5] = (y & 1) == 0 ? mat0 : mat1;
		alignas(16) uint8_t (& cur )[16 * 5] = (y & 1) != 0 ? mat0 : mat1;

		uint8_t ai = p1[y];
		__m128i aiv = _mm_set1_epi8 (ai);

		for (int x = 0; x < 4; x++) {
			__m128i bjv   = _mm_loadu_si128 ((__m128i*) &(p2[x * 16]));
			__m128i omega = _mm_cmpeq_epi8  (aiv, bjv);
				omega = _mm_and_si128   (omega, plus_3);
			__m128i prev0 = _mm_loadu_si128 ((__m128i*) &(prev[(x + 1) * 16 - 1]));
			__m128i i3    = _mm_adds_epu8   (prev0, omega);

			__m128i prev1 = _mm_load_si128  ((__m128i*) &(prev[(x + 1) * 16 - 0]));
			__m128i i1    = prev1;

			__m128i prelim = _mm_max_epu8   (i1, i3);
				prelim = _mm_subs_epu8  (prelim, plus_1);

			__m128i t;

			t      = _mm_slli_si128   (prelim, 1);
			t      = _mm_subs_epu8    (t, plus_1);
			prelim = _mm_max_epu8     (prelim, t);

			t      = _mm_shuffle_epi8 (prelim, shufmask1);
			t      = _mm_subs_epu8    (t, submask1);
			prelim = _mm_max_epu8     (prelim, t);

			t      = _mm_shuffle_epi8 (prelim, shufmask2);
			t      = _mm_subs_epu8    (t, submask2);
			prelim = _mm_max_epu8     (prelim, t);

			t      = _mm_shuffle_epi8 (prelim, shufmask3);
			t      = _mm_subs_epu8    (t, submask3);
			prelim = _mm_max_epu8     (prelim, t);

			prev_res = _mm_shuffle_epi8 (prev_res, plus_15);
			prev_res = _mm_subs_epu8    (prev_res, ramp_1_16);
			prelim   = _mm_max_epu8     (prelim, prev_res);

			prev_res = prelim;

			_mm_store_si128 ((__m128i*) &(cur[(x + 1) * 16]), prelim);

			if (x == 3) {
				// prelim contains overhead that should be excluded.
				prelim = _mm_slli_si128 (prelim, 16 - 2);
			}
			global_max = _mm_max_epu8 (global_max, prelim);
		}
	}

	global_max = _mm_max_epu8 (global_max, _mm_alignr_epi8 (global_max, global_max, 1));
	global_max = _mm_max_epu8 (global_max, _mm_alignr_epi8 (global_max, global_max, 2));
	global_max = _mm_max_epu8 (global_max, _mm_alignr_epi8 (global_max, global_max, 4));
	global_max = _mm_max_epu8 (global_max, _mm_alignr_epi8 (global_max, global_max, 8));

#ifdef __SSE4_1__
	int gm = _mm_extract_epi8 (global_max, 0);
#else
	int gm = _mm_extract_epi16 (global_max, 0) & 0xFF;
#endif
	return gm;
}
