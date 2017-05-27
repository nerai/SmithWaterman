#ifndef COMPARE_H_INCLUDED
#define COMPARE_H_INCLUDED

#include <stdint.h>

#include "settings.h"

int compare_scalar (
	const uint8_t* __restrict__ p1,
	const uint8_t* __restrict__ p2
);

int compare_sse (
	const uint8_t* __restrict__ p1,
	const uint8_t* __restrict__ p2
);

int compare_avx (
	const uint8_t* __restrict__ p1,
	const uint8_t* __restrict__ p2
);


#if __AVX2__ && ALLOW_AVX
#define COMPARE compare_avx
#elif __SSSE3__ && ALLOW_SSE
#define COMPARE compare_sse
#else
#define COMPARE compare_scalar
#endif


#endif // COMPARE_H_INCLUDED
