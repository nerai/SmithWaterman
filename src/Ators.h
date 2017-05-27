#ifndef ATORS_H_INCLUDED
#define ATORS_H_INCLUDED

#include <memory>
#include <queue>
using namespace std;


inline uint8_t* const persisting_malloc_align (const size_t bytes, const size_t alig)
{
	size_t n = bytes + alig;
	void* p = new uint8_t [n];
	align (alig, bytes, p, n);
	return (uint8_t*) p;
}


template <class T>
class IAllocator
{
public:
	virtual void* alloc () = 0;
	virtual void free (T* p) = 0;
};

template <class T>
class Allocator_C : public IAllocator <T>
{
public:
	inline void* alloc ()
	{
		return new char [sizeof(T)];
	}

	inline void free (T* p)
	{
		delete[] (char*) p;
	}
};

template <class T>
class Allocator_Q : public IAllocator <T>
{
private:
	queue<T*> _Q;

public:
	inline Allocator_Q ()
		:
		_Q ()
	{
	}

	inline void* alloc ()
	{
		if (_Q.empty()) {
			const int n = 1000;
			unsigned char* space = persisting_malloc_align (n * sizeof(T), 64);
			for (int i = 0; i < n; i++) {
				unsigned char* p = &( space [i * sizeof(T)] );
				_Q.push ((T*) p);
			}
		}

		void* space = _Q.front ();
		_Q.pop ();
		return space;
	}

	inline void free (T* p)
	{
		_Q.push (p);
	}
};

#endif // ATORS_H_INCLUDED
