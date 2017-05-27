
#include <assert.h>

#include "BitmapNode.h"


ostream& operator << (ostream &os, BitmapNode const & node) {
	return os << "["
		  << node._Prev << " | "
		  << &node << "="
		  << node._I0 << ","
		  << node._I1 << " |"
		  << node._Next << "]";
}

void BitmapNode::printChain (BitmapNode const * p)
{
	cout << p << " = { ";
	while (p != 0){
		cout << *p;

		if (p->_Prev) {
			assert (p->_Prev->_Next == p);
			assert (p->_Prev->_I1 < p->_I0);
		}
		if (p->_Next) {
			assert (p->_Next->_Prev == p);
			assert (p->_Next->_I0 > p->_I1);
		}

		cout << ", ";
		p = p->_Next;
	}
	cout << "}" << endl;
}
