#ifndef BITMAPNODE_H_INCLUDED
#define BITMAPNODE_H_INCLUDED

#include <iostream>
using namespace std;

#include "Ators.h"


class BitmapNode {
private:
	int _I0;
	int _I1;
	BitmapNode* _Prev;
	BitmapNode* _Next;

	inline BitmapNode (const int i0, const int i1, BitmapNode* const prev, BitmapNode* const next)
		:
		_I0 (i0),
                _I1 (i1),
                _Prev (prev),
                _Next (next)
	{
	}

public:
	inline BitmapNode (const int size)
		:
		_I0 (0),
		_I1 (size - 1),
		_Prev (0),
		_Next (0)
	{
	}

	friend ostream &operator << (ostream &os, BitmapNode const & node);

	static void printChain (BitmapNode const * p);

	inline BitmapNode* unlink ()
	{
		if (_Prev) {
			_Prev->_Next = _Next;
		}
		if (_Next) {
			_Next->_Prev = _Prev;
		}

		BitmapNode* ret = _Prev ? _Prev : _Next;
		return ret;
	}

	inline static BitmapNode* skip (
		BitmapNode* start,
		int i0,
		int i1,
		IAllocator<BitmapNode>& ator)
	{
		auto p = start;
		while (p) {
			if (i0 <= p->_I0) {
				p->_I0 = max (p->_I0, i1 + 1);
			}
			if (i1 >= p->_I1) {
				p->_I1 = min (p->_I1, i0 - 1);
			}
			if (p->_I0 > p->_I1) {
				auto removed = p;
				p = p->unlink ();
				ator.free (removed);
				continue;
			}
			if (p->_I0 < i0 && i1 < p->_I1) {
				void* space = ator.alloc ();
				BitmapNode* add = new(space) BitmapNode (i1 + 1, p->_I1, p, p->_Next);
				if (p->_Next) p->_Next->_Prev = add;
				p->_Next = add;
				p->_I1 = i0 - 1;
				return p;
			}

			if (p->_Prev && i0 <= p->_Prev->_I1) {
				p = p->_Prev;
			}
			else if (p->_Next && i1 >= p->_Next->_I0) {
				p = p->_Next;
			}
			else {
				break;
			}
		}
		return p;
	}

	inline const BitmapNode* find (int i) const
	{
		const BitmapNode* p = this;
		while (i < p->_I0 && p->_Prev) {
			p = p->_Prev;
		}
		while (i > p->_I1 && p->_Next) {
			p = p->_Next;
		}
		return p;
	}

	inline BitmapNode* find (int i)
	{
		return const_cast<BitmapNode*> (
			static_cast <const BitmapNode &> (*this)
			.find (i));
	}

	inline bool chain_contains (int i) const
	{
		const BitmapNode* p = find (i);
		return p->_I0 <= i && i <= p->_I1;
	}

	inline static BitmapNode* findRoot (BitmapNode* p)
	{
		if (!p) {
			return 0;
		}
		while (p->_Prev) {
			p = p->_Prev;
		}
		return p;
	}

	inline bool findUnskipped (int& i) const
	{
		auto p = find (i);
		if (i < p->_I0) {
                        i = p->_I0;
                        return true;
		}
		if (i > p->_I1) {
			p = p->_Next;
			if (!p) return false;
                        i = p->_I0;
                        return true;
		}
		return true;
	}
};

typedef Allocator_C<BitmapNode> BitmapNode_Ator_C;
typedef Allocator_Q<BitmapNode> BitmapNode_Ator_Q;

#endif // BITMAPNODE_H_INCLUDED
