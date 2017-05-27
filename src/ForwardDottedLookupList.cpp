#include "ForwardDottedLookupList.h"


ostream& operator << (ostream &os, FDLL_Node const & node) {
	return os << "["
		  << &node << "="
		  << node._I0 << ","
		  << node._I1 << " |"
		  << node._Next << "]";
}

void FDLL_Node::printChain (FDLL_Node const * p)
{
	cout << p << " = { ";
	while (p != 0){
		cout << *p;
		if (p->_Next) {
			assert (p->_Next->_I0 >= p->_I0);
		}
		cout << ", ";
		p = p->_Next;
	}
	cout << "}" << endl;
}
