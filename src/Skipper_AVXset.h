#ifndef SKIPPER_AVXSET_H_INCLUDED
#define SKIPPER_AVXSET_H_INCLUDED

#include <immintrin.h>

#include "settings.h"
#include "avx_util.h"


static const __m256i plus_1 = _mm256_set1_epi8 (1);

static const __m256i V_15_0_16 = _mm256_setr_epi8 (
	15, 14, 13, 12, 11, 10, 9, 8,
	7, 6, 5, 4, 3, 2, 1, 0,
	1, 2, 3, 4, 5, 6, 7, 8,
	9, 10, 11, 12, 13, 14, 15, 16);

/*
 * Similar to the single line node skipper (SLList), but without nodes.
 * Instead we store all possible nodes in an array and operate on it with SIMD.
 */
class Skipper
{
private:
	const size_t _len2;
	uint8_t* const avoid;

public:
	inline Skipper (const size_t len2)
		:
		_len2 (len2),
		avoid (persisting_malloc_align (16 + len2 + sizeof(__m256i), 64))
	{
	}

	inline void print () const
	{
		for (size_t d = _len2 - 100; d < _len2; d++) {
			uint8_t v = avoid [d];
			if (v == 0) {
				cout << ".";
			} else if (v < 10) {
				cout << (char)('0' + v);
			} else {
				cout << (char)('A' + v - 10);
			}
		}
		cout << endl;
	}

	inline void skipRange (__attribute__((unused)) const size_t i, const size_t j, int skip)
	{
		skip--;
		if (skip < 0) {
			return;
		}

		avoid [j] = max (avoid [j], (uint8_t)(skip + 1));
	}

	inline void finishRow (__attribute__((unused)) const size_t row_i)
	{
		#if 0
		for (size_t k = 0; k < _len2; k++) {
			avoid [k] = max (0, -1 + (int)avoid[k]);
		}
		#else
		for (size_t k = 0; k <= _len2 / sizeof(__m256i); k++) {
			__m256i* p = (__m256i*) & (avoid [k * sizeof(__m256i)]);
			__m256i r = _mm256_loadu_si256 (p);
			r = _mm256_subs_epu8 (r, plus_1);
			_mm256_storeu_si256 (p, r);
		}
		#endif
	}

	inline bool findUnskipped (__attribute__((unused)) const size_t i, size_t& j)
	{
		while (j < _len2) {
			#if 0
			bool ok = true;
			for (int d = -16; d <= +16; d++) {
				if (j + d < 0) continue;
				uint8_t val = avoid [j + d];
				int abs = d > 0 ? +d : -d;
				if (val > abs) {
					ok = false;
					break;
				}
			}
			#else
			__m256i r = _mm256_loadu_si256 ((__m256i*) & (avoid [j - 15]));
			r = _mm256_subs_epu8 (r, V_15_0_16);
			int ok = _mm256_testz_si256 (r, r);
			#endif
			if (ok) {
				return true;
			} else {
				j++;
			}
		}
		return false;
	}
};


#endif // SKIPPER_AVXSET_H_INCLUDED
