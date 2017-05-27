#ifndef AVX_UTIL_H_INCLUDED
#define AVX_UTIL_H_INCLUDED

#include <iostream>
using namespace std;


inline void print (const __m256i m)
{
	alignas(32) uint8_t t[32];
	_mm256_store_si256 ((__m256i*) &t, m);

	for (int i = 0; i < 32; i++) {
		cout << unsigned (t[i]) << " ";
	}
	cout << endl;
}


#endif // AVX_UTIL_H_INCLUDED
