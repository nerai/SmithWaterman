#ifndef FORWARDDOTTEDLOOKUPLIST_H
#define FORWARDDOTTEDLOOKUPLIST_H

#include <assert.h>
#include <iostream>
using namespace std;

#include "settings.h"
#include "Ators.h"

/*
 * Guarantee: i0 <= i1
 * Asymptotic guarantee: i0 < next.i0 && i1 < next.i1
 */
class FDLL_Node
{
private:
	int _I0;
	int _I1;
	FDLL_Node* _Next;

public:
	inline FDLL_Node (const int i0, const int i1, FDLL_Node* const next)
		:
		_I0 (i0),
                _I1 (i1),
                _Next (next)
	{
	}

	friend ostream& operator << (ostream &os, FDLL_Node const & node);
	static void printChain (FDLL_Node const * p);

	inline bool operator < (FDLL_Node const & rhs)
	{
		return _I0 < rhs._I0;
	}

	inline FDLL_Node* skip (int const i0, int const i1, IAllocator<FDLL_Node>& ator)
	{
		assert (i0 <= i1);

		if (i0 < _I0) {
			if (i1 >= _I1) {
				// suffices to update this node
				_I0 = i0;
                                _I1 = i1;
				 return this;
			} else {
				void* space = ator.alloc ();
				auto add = new(space) FDLL_Node (i0, i1, this);
				return add;
			}
		}

		if (i1 <= _I1) {
			// can safely ignore
			return this;
		}
		if (i0 == _I0 && i1 >= _I1) {
			_I1 = i1;
			return this;
		}

		if (_Next && _Next->_I0 >= i0) {
			return _Next->skip (i0, i1, ator);
		}

		void* space = ator.alloc ();
		_Next = new(space) FDLL_Node (i0, i1, _Next);
		return _Next;
	}

	inline static void findUnskipped (FDLL_Node*& p, int & i)
	{
		if (!p) {
			return;
		}

		while (1) {
			if (i < p->_I0) {
				return;
			}

			while (p->_Next && p->_Next->_I0 <= i) {
				p = p->_Next;
			}
			if (i > p->_I1) {
				return;
			}

			i = p->_I1 + 1;
			if (!p->_Next) {
				return;
			}
			if (p->_Next->_I0 > i) {
				return;
			}
			p = p->_Next;
		}
	}

	inline static FDLL_Node* nextRow (FDLL_Node* root, IAllocator<FDLL_Node>& ator)
	{
		FDLL_Node* prev = 0;
		FDLL_Node* p = root;

		while (p) {
                        p->_I0++;
                        p->_I1--;

                        if (prev && prev->_I0 >= p->_I0) {
				assert (prev->_I1 <= p->_I1);
				prev->_I0 = p->_I0;
				prev->_I1 = p->_I1;
				prev->_Next = p->_Next;
				ator.free (p);
				p = prev->_Next;
				continue;
                        }

                        if (p->_I0 <= p->_I1) {
				prev = p;
				p = p->_Next;
				continue;
                        }

                        auto cont = p->_Next;
			if (prev) {
				prev->_Next = cont;
			}
			else {
				root = cont;
			}
			ator.free (p);
			p = cont;
		}

		return root;
	}
};

/*
 * Supports only forward-directional scanning.
 * Stores only ONE line consisting of points with a radius.
 * Decreasing the radius is equivalent to stepping into the next line.
 */
class ForwardDottedLookupList
{
private:
	const int _len2;
	FDLL_Node* _Root;
	FDLL_Node* _Cursor;
	IAllocator<FDLL_Node>& _Ator;

public:
	inline ForwardDottedLookupList (const int len2, IAllocator<FDLL_Node>& ator)
		:
		_len2 (len2),
		_Root (0),
		_Cursor (0),
		_Ator (ator)
	{
	}

	inline void printSelf () const
	{
		FDLL_Node::printChain (_Root);
	}

	inline void skip (const int j0, const int j1)
	{
		if (!_Root) {
			_Root = new(_Ator.alloc()) FDLL_Node (j0, j1, 0);
			_Cursor = _Root;
			return;
		}

		_Cursor = _Cursor->skip (j0, j1, _Ator);
		if (_Cursor && _Cursor < _Root) {
			_Root = _Cursor;
		}
	}

	inline bool findUnskipped (int& j)
	{
		FDLL_Node::findUnskipped (_Cursor, j);
		return j < _len2;
	}

	inline void resetCursor ()
	{
		_Cursor = _Root;
	}

	inline void nextRow ()
	{
		_Root = FDLL_Node::nextRow (_Root, _Ator);
	}
};


#endif // FORWARDDOTTEDLOOKUPLIST_H
