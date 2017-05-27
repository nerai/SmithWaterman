#ifndef SKIPPER_DLLIST_H_INCLUDED
#define SKIPPER_DLLIST_H_INCLUDED

#include "settings.h"
#include "BitmapNode.h"


class SkipRow
{
private:
	const int _len2;
	BitmapNode* _Root;
	BitmapNode* _Cursor;
	IAllocator<BitmapNode>& _Ator;

public:
	inline SkipRow (const int len2, IAllocator<BitmapNode>& ator)
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

	inline void skip (const int j0, const int j1)
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

	inline bool isSkipped (const int j) const
	{
		if (!_Cursor) {
			return true;
		}
		return ! _Cursor->chain_contains (j);
	}

	inline bool findUnskipped (int& j) const
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
	const int _len2;
	const int _nRows;
	SkipRow** const _rows;
	IAllocator<BitmapNode>* const _Ator;

	inline int getLookupIndex (const int index)
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
	inline Skipper (const int len2)
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

	inline void skipRange (int i, int j, int skip)
	{
		for (int di = 1; di <= VERTICAL_SKIP_LIMIT; di++) {
			skip--;
			if (skip < 0) {
				break;
			}

			int effI = i + di;
			SkipRow& pArray = getSkips (effI);

			int j0 = max (j - skip, 0);
			int j1 = min (j + skip, _len2 - 1);

			pArray.skip (j0, j1);
		}
	}

	inline bool isSkipped (const int i, const int j)
	{
		SkipRow& mySkips = getSkips (i);
		auto res = mySkips.isSkipped (j);
		return res;
	}

	inline void finishRow (const int i)
	{
		for (int i = 0; i < _nRows; i++) {
			_rows [i]->resetCursor ();
		}
		getSkips (i).clear ();
	}

	inline bool findUnskipped (const int i, int& j)
	{
		SkipRow& mySkips = getSkips (i);
		return mySkips.findUnskipped (j);
	}
};


#endif // SKIPPER_DLLIST_H_INCLUDED
