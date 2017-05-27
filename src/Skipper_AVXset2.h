#ifndef SKIPPER_AVXSET2_H_INCLUDED
#define SKIPPER_AVXSET2_H_INCLUDED

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
		if (skip <= 0) {
			return;
		}

		auto p = (__m256i*) & (avoid [j - 15]);
		__m256i rs = _mm256_set1_epi8 (skip);
			rs = _mm256_subs_epu8 (rs, V_15_0_16);
		__m256i r0 = _mm256_loadu_si256 (p);
			r0 = _mm256_max_epu8 (r0, rs);
		_mm256_storeu_si256 (p, r0);
	}

	inline void finishRow (__attribute__((unused)) const size_t row_i)
	{
		for (size_t k = 0; k <= _len2 / sizeof(__m256i); k++) {
			__m256i* p = (__m256i*) & (avoid [k * sizeof(__m256i)]);
			__m256i r = _mm256_loadu_si256 (p);
			r = _mm256_subs_epu8 (r, plus_1);
			_mm256_storeu_si256 (p, r);
		}
	}

	inline bool findUnskipped (__attribute__((unused)) const size_t i, size_t& j)
	{
		while (j < _len2) {
			uint8_t v = avoid [j];
			if (!v) {
				return true;
			} else {
				j += v;
			}
		}
		return false;
	}
};


#endif // SKIPPER_AVXSET2_H_INCLUDED
