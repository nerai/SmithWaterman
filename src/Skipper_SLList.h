#ifndef SKIPPER_SLLIST_H_INCLUDED
#define SKIPPER_SLLIST_H_INCLUDED

#include "settings.h"
#include "ForwardDottedLookupList.h"

class Skipper
{
private:
	const size_t _len2;
	IAllocator<FDLL_Node>* const _Ator;
	ForwardDottedLookupList _FDLL;

public:
	inline Skipper (const size_t len2)
		:
		_len2 (len2),
		_Ator (new Allocator_Q<FDLL_Node> ()),
		_FDLL (len2, *_Ator)
	{
	}

	inline void skipRange (__attribute__((unused)) const size_t i, const size_t j, int skip)
	{
		skip--;
		if (skip < 0) {
			return;
		}

		size_t j0 = max (j - skip, (size_t)0);
		size_t j1 = min (j + skip, _len2 - 1);
		_FDLL.skip (j0, j1);
	}

	inline void finishRow (__attribute__((unused)) const size_t row_i)
	{
		_FDLL.resetCursor ();
		_FDLL.nextRow ();
	}

	inline bool findUnskipped (__attribute__((unused)) const size_t i, size_t& j)
	{
		return _FDLL.findUnskipped (j);
	}
};

#endif // SKIPPER_SLLIST_H_INCLUDED
