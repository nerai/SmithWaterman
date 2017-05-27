/*
 * AVX2 implementation of Smith Waterman.
 *
 * Compared to SSE, AVX2 shifts work in 128 bit lanes (not on the full 256 bit range). I consider this stupid, and while
 * it was fixed in AVX512, supporting CPUs are not broadly available yet, so we have to go with what we got.
 *
 * Consequence: All shuffle masks are per 128 bit lane, not global for all 256 bit. It suffices to copy the SSE values twice.
 */

#include <stdint.h>
#include <immintrin.h>
#include <iostream>
using namespace std;

//#include "avx_util.h"


static const __m256i plus_1 = _mm256_set1_epi8 (1);
static const __m256i plus_3 = _mm256_set1_epi8 (3);
static const __m256i plus_15 = _mm256_set1_epi8 (15);
static const __m256i ramp_1_32 = _mm256_setr_epi8 (
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32);

static const __m256i shufmask1 = _mm256_setr_epi8 (
	-1, -1, 1, 1, -1, -1, 5, 5,
	-1, -1, 9, 9, -1, -1, 13, 13,
	-1, -1, 1, 1, -1, -1, 5, 5,
	-1, -1, 9, 9, -1, -1, 13, 13);
static const __m256i submask1 = _mm256_setr_epi8 (
	-1, -1, 1, 2, -1, -1, 1, 2,
	-1, -1, 1, 2, -1, -1, 1, 2,
	-1, -1, 1, 2, -1, -1, 1, 2,
	-1, -1, 1, 2, -1, -1, 1, 2);
static const __m256i shufmask2 = _mm256_setr_epi8 (
	-1, -1, -1, -1, 3, 3, 3, 3,
	-1, -1, -1, -1, 11, 11, 11, 11,
	-1, -1, -1, -1, 3, 3, 3, 3,
	-1, -1, -1, -1, 11, 11, 11, 11);
static const __m256i submask2 = _mm256_setr_epi8 (
	-1, -1, -1, -1, 1, 2, 3, 4,
	-1, -1, -1, -1, 1, 2, 3, 4,
	-1, -1, -1, -1, 1, 2, 3, 4,
	-1, -1, -1, -1, 1, 2, 3, 4);
static const __m256i shufmask3 = _mm256_setr_epi8 (
	-1, -1, -1, -1, -1, -1, -1, -1,
	7, 7, 7, 7, 7, 7, 7, 7,
	-1, -1, -1, -1, -1, -1, -1, -1,
	7, 7, 7, 7, 7, 7, 7, 7);
static const __m256i submask3 = _mm256_setr_epi8 (
	-1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 3, 4, 5, 6, 7, 8,
	-1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 3, 4, 5, 6, 7, 8);
static const __m256i shufmask4 = plus_15;
static const __m256i submask4 = _mm256_setr_epi8 (
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15, 16);

int compare_avx (const uint8_t* __restrict__ p1, const uint8_t* __restrict__ p2)
{
	alignas(64) uint8_t mat0[32 * 3] = {0};
	alignas(64) uint8_t mat1[32 * 3] = {0};

	p1--;

	__m256i global_max = _mm256_setzero_si256 ();

	for (int y = 1; y < 51; y++) {
		__m256i prev_res = _mm256_setzero_si256 ();

		alignas(32) uint8_t (& prev)[32 * 3] = (y & 1) == 0 ? mat0 : mat1;
		alignas(32) uint8_t (& cur )[32 * 3] = (y & 1) != 0 ? mat0 : mat1;

		uint8_t ai = p1[y];
		__m256i aiv = _mm256_set1_epi8 (ai);

		for (int x = 0; x < 2; x++) {
			__m256i bjv     = _mm256_loadu_si256 ((__m256i*) &(p2[x * 32]));
			__m256i omega   = _mm256_cmpeq_epi8  (aiv, bjv);
				omega   = _mm256_and_si256   (omega, plus_3);
			__m256i prev0   = _mm256_loadu_si256 ((__m256i*) &(prev[(x + 1) * 32 - 1]));
			__m256i i3      = _mm256_adds_epu8   (prev0, omega);

			__m256i prev1   = _mm256_load_si256  ((__m256i*) &(prev[(x + 1) * 32 - 0]));
			__m256i i1      = prev1;

			__m256i prelim  = _mm256_max_epu8    (i1, i3);
				prelim  = _mm256_subs_epu8   (prelim, plus_1);

			__m256i t;

			t      = _mm256_slli_si256   (prelim, 1);
			t      = _mm256_subs_epu8    (t, plus_1);
			prelim = _mm256_max_epu8     (prelim, t);

			t      = _mm256_shuffle_epi8 (prelim, shufmask1);
			t      = _mm256_subs_epu8    (t, submask1);
			prelim = _mm256_max_epu8     (prelim, t);

			t      = _mm256_shuffle_epi8 (prelim, shufmask2);
			t      = _mm256_subs_epu8    (t, submask2);
			prelim = _mm256_max_epu8     (prelim, t);

			t      = _mm256_shuffle_epi8 (prelim, shufmask3);
			t      = _mm256_subs_epu8    (t, submask3);
			prelim = _mm256_max_epu8     (prelim, t);

			t      = _mm256_permute4x64_epi64 (prelim, 0b01000100);
			t      = _mm256_shuffle_epi8 (t, shufmask4);
			t      = _mm256_subs_epu8    (t, submask4);
			prelim = _mm256_max_epu8     (prelim, t);

			prev_res = _mm256_permute4x64_epi64 (prev_res, 0b11101110);
			prev_res = _mm256_shuffle_epi8 (prev_res, plus_15);
			prev_res = _mm256_subs_epu8    (prev_res, ramp_1_32);
			prelim   = _mm256_max_epu8     (prelim, prev_res);

			prev_res = prelim;

			_mm256_store_si256 ((__m256i*) &(cur[(x + 1) * 32]), prelim);

			if (x == 1) {
				// prelim contains overhead that should be excluded.
				prelim = _mm256_slli_si256 (prelim, 32 - 18);
			}
			global_max = _mm256_max_epu8 (global_max, prelim);
		}
	}

	global_max = _mm256_max_epu8 (global_max, _mm256_slli_si256 (global_max, 1));
	global_max = _mm256_max_epu8 (global_max, _mm256_slli_si256 (global_max, 2));
	global_max = _mm256_max_epu8 (global_max, _mm256_slli_si256 (global_max, 4));
	global_max = _mm256_max_epu8 (global_max, _mm256_slli_si256 (global_max, 8));
	global_max = _mm256_max_epu8 (global_max, _mm256_slli_si256 (global_max, 16));
	int gm = _mm256_extract_epi8 (global_max, 31);
	return gm;
}
