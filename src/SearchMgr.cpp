
#include <string.h>
#include <vector>
#include <thread>
#include <assert.h>
#include <queue>

#include "SearchMgr.h"
#include "Ators.h"
#include "compare.h"

#include "Skipper.h"


class SearchThread
{
private:
	const int _i0;
	const int _i1;
	Input& _inputs;
	ResultCollector& _results;

	const uint8_t* const g1;
	const uint8_t* const g2;
	const int len2;
#if REQUIRE_SKIP_MAP
	Skipper _skip;
#endif

public:
	inline SearchThread (
		__attribute__((unused)) int threadId,
		const int i0,
		const int i1,
		Input& inputs,
		ResultCollector& results)
		:
		_i0 (i0),
		_i1 (i1),
		_inputs (inputs),
		_results (results),

		g1 ((const uint8_t*) _inputs.Gene1.c_str ()),
		g2 ((const uint8_t*) _inputs.Gene2.c_str ()),
		len2 (_inputs.Len2)
#if REQUIRE_SKIP_MAP
		, _skip (len2)
#endif
	{
	}

	inline void solveForI (const int i, Result& best)
	{
		//cout << "solve for i = " << i << endl;

		const uint8_t* const p1 = &(g1[i]);
		for (int j = 0; j < len2; j++) {
			#if REQUIRE_SKIP_MAP
				#if LOOKUP_STRATEGY == STRATEGY_MEMSET
					if (_skip.isSkipped (i, j)) {
						#if SKIPPING_STATS
						_results.skippedVert++;
						#endif
						continue;
					}
				#else
					if (!_skip.findUnskipped (i, j)) {
						break;
					}
				#endif
			#endif

			#if SKIPPING_STATS
			_results.notskipped++;
			#endif

			const uint8_t* const p2 = &(g2[j]);
			int score = COMPARE (p1, p2);
			//cout << "score of " << unsigned (i) << "," << unsigned (j) << " = " << unsigned (score) << endl;
			if (score >= _inputs.Threshold) {
				best.improve (j, score);
				continue;
			}

			#if ENABLE_SKIPPING
				int skip = (_inputs.Threshold - score - 1) / 3;

				#if REQUIRE_SKIP_MAP
				_skip.skipRange (i, j, skip);
				#endif

				if (skip > 0) {
					j += skip;
					#if SKIPPING_STATS
					_results.skippedHoriz += skip;
					#endif
				}
			#endif
		}

		_skip.finishRow (i);
	}

	inline void run ()
	{
		//cout << "thread " << threadId << "begins: " << endl;
		int done = 0;

		for (int i = _i0; i < _i1; i++) {
			Result best (i);
			solveForI (i, best);
			_results.add (best);

			if (++done >= 1000) {
				_results.complete (done);
				done = 0;
			}
		}
		_results.complete (done);
		//cout << "thread finished: " << threadId << endl;
	}
};


SearchMgr::SearchMgr (
	Input& inputs,
	ofstream* const ofs)
	:
	_inputs (inputs),
	_results (inputs, ofs)
{
}

void SearchMgr::run ()
{
	vector <thread> threads;
	int prev = 0;
	for (int i = 0; i < _inputs.ThreadCount; i++) {
		int next = _inputs.Len1 * (i + 1) / _inputs.ThreadCount;
		auto st = new SearchThread (i, prev, next, _inputs, _results);
		threads.push_back (thread (st->run, st));
		prev = next;
	}
	for (auto& t : threads) {
		t.join ();
	}

	int hash = _results.resultHash;
	printf ("Result hash: %08X (the regular hash of sox3 & sry is 6976F3E0)\n", hash);
#if SKIPPING_STATS
	cout << _results.notskipped << " not skipped, "
	     << _results.skippedHoriz << " skipped H, "
	     << _results.skippedVert << " skipped V"
	     << endl;
#endif // SKIPPING_STATS
}
