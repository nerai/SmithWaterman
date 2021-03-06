#ifndef SKIPPER_MEMSET_H_INCLUDED
#define SKIPPER_MEMSET_H_INCLUDED

#include "settings.h"


class Skipper
{
private:
	const size_t _len2;
	const int _nRows;
	uint8_t* const avoid;

	inline size_t getLookupIndex (const int index) const
	{
		#if CHECK_INTEGRITY
		assert (index >= 0);
		#endif
		return index % _nRows;
	}

	inline const uint8_t* getSkips (const int index) const
	{
		return &(avoid [getLookupIndex (index) * _len2]);
	}

	inline uint8_t* getSkips (const int index)
	{
		return const_cast <uint8_t*> (
			static_cast <const Skipper &> (*this)
			.getSkips (index));
	}

public:
	inline Skipper (size_t len2)
		:
		_len2 (len2),
		_nRows (1 + VERTICAL_SKIP_LIMIT),
		avoid (persisting_malloc_align (_nRows * len2, 64))
	{
	}

	inline void skipRange (const size_t i, size_t j, int skip)
	{
		for (int di = 1; di <= VERTICAL_SKIP_LIMIT; di++) {
			skip--;
			if (skip < 0) {
				break;
			}

			int effI = i + di;
			uint8_t* const pArray = getSkips (effI);

			size_t j0 = max (j - skip, (size_t)0);
			size_t j1 = min (j + skip, _len2 - 1);

			#if 0
			cout << "in i = " << effI
			     << " skip j = " << j0
			     << " to " << j1
			     << endl;
			#endif

			memset (&(pArray[j0]), 1, j1 - j0 + 1);
		}
	}

	inline bool isSkipped (const size_t i, const size_t j) const
	{
		const uint8_t* const mySkips = getSkips (i);
		uint8_t skipJ = mySkips[j];
		#if CHECK_INTEGRITY
		assert (skipJ == 0 || skipJ == 1);
		#endif
		return (bool) skipJ;
	}

	inline void finishRow (const size_t row_i)
	{
		uint8_t* const mySkips = getSkips (row_i);
		memset (mySkips, 0, _len2);
	}
};

#endif // SKIPPER_MEMSET_H_INCLUDED
