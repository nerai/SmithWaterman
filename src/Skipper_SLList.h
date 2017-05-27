#ifndef SKIPPER_SLLIST_H_INCLUDED
#define SKIPPER_SLLIST_H_INCLUDED

#include "settings.h"
#include "ForwardDottedLookupList.h"

class Skipper
{
private:
	const int _len2;
	IAllocator<FDLL_Node>* const _Ator;
	ForwardDottedLookupList _FDLL;

public:
	inline Skipper (const int len2)
		:
		_len2 (len2),
		_Ator (new Allocator_Q<FDLL_Node> ()),
		_FDLL (len2, *_Ator)
	{
	}

	inline void skipRange (int i, int j, int skip)
	{
		skip--;
		if (skip < 0) {
			return;
		}

		int j0 = max (j - skip, 0);
		int j1 = min (j + skip, _len2 - 1);
		_FDLL.skip (j0, j1);
	}

	inline void finishRow (const int i)
	{
		_FDLL.resetCursor ();
		_FDLL.nextRow ();
	}

	inline bool findUnskipped (const int i, int& j)
	{
		return _FDLL.findUnskipped (j);
	}
};

#endif // SKIPPER_SLLIST_H_INCLUDED
