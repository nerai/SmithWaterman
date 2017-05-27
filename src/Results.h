#ifndef RESULTS_H_INCLUDED
#define RESULTS_H_INCLUDED

#include <sstream>
#include <mutex>
#include <atomic>
#include <fstream>


class Input
{
public:
	const int Len1;
	const int Len2;
	const string Gene1;
	const string Gene2;
	const int Threshold;
	const int ThreadCount;

	double (* const Elapsed) (bool);

	inline Input (
		int len1,
		int len2,
		string& gene1,
		string& gene2,
		int threshold,
		int nthreads,
		double (* const elapsed) (bool))
	:
		Len1 (len1),
		Len2 (len2),
		Gene1 (gene1),
		Gene2 (gene2),
		Threshold (threshold),
		ThreadCount (nthreads),
		Elapsed (elapsed)
	{
	}
};

class Result
{
private:
	const int I;
	int J;
	int Score;

public:
	inline Result (const int i)
		: I (i), J (-1), Score (-1)
	{
	}

	inline bool isValid () const
	{
		return Score >= 0;
	}

	inline string toString () const
	{
		stringstream s;
		s << I << ",";
		s << J << ",";
		s << Score << ",";
		return s.str ();
	}

	inline int hash () const
	{
		return (I << 20) | (J << 8) | Score;
	}

	inline void improve (int j, int score)
	{
		if (score < Score) {
			return;
		}
		if (score > Score || j < J) {
			Score = score;
			J = j;
		}
	}
};

class ResultCollector
{
#if SKIPPING_STATS
public:
	atomic<int> notskipped;
	atomic<int> skippedHoriz;
	atomic<int> skippedVert;
#endif

private:
	Input& _inputs;
	mutex _lockResults;
	ofstream* const _ofs;
	atomic<int> _completed;

public:
	int resultHash;

	inline ResultCollector (Input& inputs, ofstream* const ofs)
	:
#if SKIPPING_STATS
		notskipped (0),
		skippedHoriz (0),
		skippedVert (0),
#endif
		_inputs (inputs),
		_lockResults (),
		_ofs (ofs),
		_completed (0),
		resultHash (0)
	{
	}

	inline void add (Result& res)
	{
		if (res.isValid ()) {
			_lockResults.lock ();
			(*_ofs) << res.toString () << "\n";
			(*_ofs) << flush;
			resultHash += res.hash ();
			_lockResults.unlock ();
		}
	}

	inline void complete (int count)
	{
		int done = _completed += count;
		const int output_interval = 100000;
		if (done / output_interval != (done - count) / output_interval) {
			auto perc = 100.0 * done / _inputs.Len1;
			auto dur = _inputs.Elapsed (false);
			auto spd = 1.0 * done / dur;
			printf ("%d / %d (%.3f%%, %.0f/s)\n",
			        done, _inputs.Len1, perc, spd);
		}
	}
};


#endif // RESULTS_H_INCLUDED
