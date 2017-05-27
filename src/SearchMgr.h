#ifndef SEARCHMGR_H
#define SEARCHMGR_H

#include <iostream>
using namespace std;

#include "settings.h"

#include "Results.h"


class SearchMgr
{
private:
	Input& _inputs;
	ResultCollector _results;

public:
	SearchMgr (Input& inputs, ofstream* const ofs);
	void run ();
};

#endif // SEARCHMGR_H
