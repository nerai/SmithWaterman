#include <string.h>
#include <iostream>

#include <chrono>

#include "compare.h"

#include "SearchMgr.h"


using namespace std;


chrono::time_point <chrono::system_clock> timeInit;
chrono::time_point <chrono::system_clock> timeStartLoop;

double elapsed (bool includeInitialization)
{
	auto t0 = includeInitialization
	          ? timeInit
	          : timeStartLoop;
	auto now = chrono::system_clock::now ();
	chrono::duration <double> dur = now - t0;
	return dur.count ();
}

string readFile (const char* const path, size_t& len, char fill)
{
	ifstream ifs (path, ifstream::binary | ifstream::ate);
	if (!ifs) {
		cout << "could not open path " << path << endl;
		exit (7);
	}

	string s;
	auto len0 = ifs.tellg ();
	s.reserve (static_cast<unsigned long long> (len0) + 51ull + 32ull);
	ifs.seekg (0);

	for (string line; getline (ifs, line);) {
		if (line[0] == '>') {
			cout << line << endl;
			continue;
		}
		s += line;
	}
	len = s.size ();

	/*
	 * Add bonus space.
	 * 50 for regular processing past the end of line.
	 * 32 for worst case overhead from AVX processing.
	 */
	s += string (50 + 32, fill);

	cout << "Loaded " << len << " effective bytes (file size: " << len0 << "b)" << endl;
	return s;
}

void printCPU ()
{

	__builtin_cpu_init ();
	cout << "CPU capabilities (actual/compiled):" << endl
		<< "\tSSE " << (__builtin_cpu_supports ("sse") ? "y" : "n") << "/" <<
#ifdef __SSE__
		"y"
#else
		"n"
#endif
		<< ", SSE2 " << (__builtin_cpu_supports ("sse2") ? "y" : "n") << "/" <<
#ifdef __SSE2__
		"y"
#else
		"n"
#endif
		<< ", SSE3 " << (__builtin_cpu_supports ("sse3") ? "y" : "n") << "/" <<
#ifdef __SSE3__
		"y"
#else
		"n"
#endif
		<< ", SSSE3 " << (__builtin_cpu_supports ("ssse3") ? "y" : "n") << "/" <<
#ifdef __SSSE3__
		"y"
#else
		"n"
#endif
		<< ", SSE4.1 " << (__builtin_cpu_supports ("sse4.1") ? "y" : "n") << "/" <<
#ifdef __SSE4_1__
		"y"
#else
		"n"
#endif
		<< ", SSE4.2 " << (__builtin_cpu_supports ("sse4.2") ? "y" : "n") << "/" <<
#ifdef __SSE4_2__
		"y"
#else
		"n"
#endif
		<< endl
		<< "\tAVX " << (__builtin_cpu_supports ("avx") ? "y" : "n") << "/" <<
#ifdef __AVX__
		"y"
#else
		"n"
#endif
		<< ", AVX2 " << (__builtin_cpu_supports ("avx2") ? "y" : "n") << "/" <<
#ifdef __AVX2__
		"y"
#else
		"n"
#endif
		<< ", AVX512F " << (__builtin_cpu_supports ("avx512f") ? "y" : "n") << "/" <<
#ifdef __AVX512F__
		"y"
#else
		"n"
#endif
		<< ", AVX512BW " << "?" << "/" <<
#ifdef __AVX512BW__
		"y"
#else
		"n"
#endif
		<< ", AVX512VBMI: " << "?" << "/" <<
#ifdef __AVX512VBMI__
		"y"
#else
		"n"
#endif
		<< endl;
}

int main (int argc, char* argv[])
{
	timeInit = chrono::system_clock::now ();

	const char* inPath1;
	const char* inPath2;
	const char* outPath;
	int nThreads;
	int threshold;

	if (argc == 1) {
		cout << "Usage:" << endl;
		cout << "ex in_path1 in_path2 nThreads threshold out_path" << endl;
		cout << "\"ex\" with no parameters uses default settings for 4 cores" << endl;
		cout << endl;

		cout << "Using default parameters." << endl;
		inPath1 = "data/sox3.fas";
		#if 0
		inPath1 = "data/246.fas";
		#endif
		inPath2 = "data/sry.fas";
		nThreads = 4;
		threshold = 70;
		outPath = "out.csv";
	} else {
		inPath1 = argv[1];
		inPath2 = argv[2];
		nThreads = atoi (argv[3]);
		threshold = atoi (argv[4]);
		outPath = argv[5];
	}
	cout << endl << endl;

	if (1) {
		cout << "Compilation flags: "
		     << "ENABLE_SKIPPING = " << ENABLE_SKIPPING
		     << ", SKIPPING_STATS = " << SKIPPING_STATS
		     << ", VERTICAL_SKIP_LIMIT = " << VERTICAL_SKIP_LIMIT
		     << ", LOOKUP_STRATEGY = " << LOOKUP_STRATEGY
		     << " (" << ( LOOKUP_STRATEGY == STRATEGY_MEMSET  ? "memset"
				: LOOKUP_STRATEGY == STRATEGY_DLLIST  ? "dllist"
				: LOOKUP_STRATEGY == STRATEGY_SLLIST  ? "sllist"
				: LOOKUP_STRATEGY == STRATEGY_AVXSET1 ? "avxset1"
				: LOOKUP_STRATEGY == STRATEGY_AVXSET2 ? "avxset2"
				: "???") << ")"
		     << endl;
		printCPU ();
		auto mode =
			COMPARE == compare_avx ? "256" :
			COMPARE == compare_sse ? "128" :
			"64";
		cout << "Selected operation width: " << mode << " bit" << endl;
		cout << "Thread count: " << nThreads << endl;
		cout << endl << endl;
	}

	cout << "Reading genes..." << endl;
	size_t len1;
	size_t len2;
	string gene1 = readFile (inPath1, len1, '1');
	string gene2 = readFile (inPath2, len2, '2');
	printf ("All files read in %.3f s\n", elapsed (true));
	cout << endl << endl;

	ofstream ofs (outPath);
	ofs << "start in " << inPath1 << ",";
	ofs << "start in " << inPath2 << ",";
	ofs << "score" << ",";
	ofs << "\n";

	cout << "Begin processing ..." << endl;
	timeStartLoop = chrono::system_clock::now ();

	Input inputs (len1, len2, gene1, gene2, threshold, nThreads, elapsed);
	SearchMgr sm (inputs, &ofs);
	sm.run ();

	cout << endl << endl;
	printf ("Processing complete, total %.3f s, iterations %.3f s)\n", elapsed (true), elapsed (false));

	cout << endl;
	return 1;
}
