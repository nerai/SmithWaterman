/*
 * A dead-simple implementation of the comparison of two 50 byte vectors according to Smith Waterman.
 */

#include "compare.h"

#include <iostream>
using namespace std;


inline void print (const int (& m)[51])
{
	for (int i = 0; i < 51; i++) {
		cout << unsigned (m[i]) << " ";
	}
	cout << endl;
}

int compare_scalar (
	const uint8_t* __restrict__ p1,
	const uint8_t* __restrict__ p2
	)
{
	alignas(64) int mat0[51] = {0};
	alignas(64) int mat1[51] = {0};

	p1--;
	p2--;

	int global_max = 0;

	for (int y = 1; y < 51; y++) {
		int (&prev)[51] = (y & 1) == 0 ? mat0 : mat1;
		int (&cur )[51] = (y & 1) == 0 ? mat1 : mat0;
		uint8_t ai = p1[y];

		for (int x = 1; x < 51; x++) {
			uint8_t bj = p2[x];
			int omega = ai == bj ? 3 : 0;

			const int i1 = prev[x];
			const int i2 = prev[x-1] + omega;
			const int i3 = cur [x-1];

			int max = 1;
			max = max > i1 ? max : i1;
			max = max > i2 ? max : i2;
			max = max > i3 ? max : i3;
			max--;
			cur[x] = max;
			global_max = max > global_max ? max : global_max;
		}
	}

	return global_max;
}
