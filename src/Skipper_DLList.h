#ifndef SKIPPER_DLLIST_H_INCLUDED
#define SKIPPER_DLLIST_H_INCLUDED

#include "settings.h"
#include "BitmapNode.h"


class SkipRow
{
private:
	const size_t _len2;
	BitmapNode* _Root;
	BitmapNode* _Cursor;
	IAllocator<BitmapNode>& _Ator;

public:
	inline SkipRow (const size_t len2, IAllocator<BitmapNode>& ator)
		:
		_len2 (len2),
		_Root (new BitmapNode (len2)),
		_Cursor (_Root),
		_Ator (ator)
	{
	}

	inline void printSelf () const
	{
		BitmapNode::printChain (_Root);
	}

	inline void skip (const size_t j0, const size_t j1)
	{
		if (!_Cursor) {
			return;
		}
		bool cursorIsRoot = _Cursor == _Root;
		_Cursor = BitmapNode::skip (_Cursor, j0, j1, _Ator);
		if (cursorIsRoot) {
			_Root = BitmapNode::findRoot (_Cursor);
		}
	}

	inline bool isSkipped (const size_t j) const
	{
		if (!_Cursor) {
			return true;
		}
		return ! _Cursor->chain_contains (j);
	}

	inline bool findUnskipped (size_t& j) const
	{
		if (!_Cursor) {
			return false;
		}
		return _Cursor->findUnskipped (j);
	}

	inline void resetCursor ()
	{
		_Cursor = _Root;
	}

	inline void clear ()
	{
		_Cursor = 0;
                while (_Root) {
			auto removed = _Root;
			_Root = _Root->unlink ();
			_Ator.free (removed);
		}
		void* space = _Ator.alloc ();
		_Root = new(space) BitmapNode (_len2);
		_Cursor = _Root;
	}
};

class Skipper
{
private:
	const size_t _len2;
	const int _nRows;
	SkipRow** const _rows;
	IAllocator<BitmapNode>* const _Ator;

	inline size_t getLookupIndex (const int index)
	{
		#if CHECK_INTEGRITY
		assert (index >= 0);
		#endif
		return index % _nRows;
	}

	inline SkipRow& getSkips (const int index)
	{
		return *_rows [getLookupIndex (index)];
	}

public:
	inline Skipper (const size_t len2)
		:
		_len2 (len2),
		_nRows (1 + VERTICAL_SKIP_LIMIT),
		_rows (new SkipRow* [_nRows]),
		_Ator (new BitmapNode_Ator_Q ())
	{
		for (int i = 0; i < _nRows; i++) {
			_rows [i] = new SkipRow (len2, *_Ator);
		}
	}

	inline void skipRange (size_t i, size_t j, int skip)
	{
		for (int di = 1; di <= VERTICAL_SKIP_LIMIT; di++) {
			skip--;
			if (skip < 0) {
				break;
			}

			int effI = i + di;
			SkipRow& pArray = getSkips (effI);

			size_t j0 = max (j - skip, (size_t)0);
			size_t j1 = min (j + skip, _len2 - 1);
			pArray.skip (j0, j1);
		}
	}

	inline bool isSkipped (const size_t i, const size_t j)
	{
		SkipRow& mySkips = getSkips (i);
		auto res = mySkips.isSkipped (j);
		return res;
	}

	inline void finishRow (const size_t row_i)
	{
		for (int i = 0; i < _nRows; i++) {
			_rows [i]->resetCursor ();
		}
		getSkips (row_i).clear ();
	}

	inline bool findUnskipped (const size_t i, size_t& j)
	{
		SkipRow& mySkips = getSkips (i);
		return mySkips.findUnskipped (j);
	}
};


#endif // SKIPPER_DLLIST_H_INCLUDED
