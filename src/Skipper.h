#ifndef SKIPPER_H_INCLUDED
#define SKIPPER_H_INCLUDED

#include "settings.h"

#if REQUIRE_SKIP_MAP
	#if LOOKUP_STRATEGY == STRATEGY_MEMSET
		#include "Skipper_Memset.h"
	#elif LOOKUP_STRATEGY == STRATEGY_DLLIST
		#include "Skipper_DLList.h"
	#elif LOOKUP_STRATEGY == STRATEGY_SLLIST
		#include "Skipper_SLList.h"
	#elif LOOKUP_STRATEGY == STRATEGY_AVXSET1
		#include "Skipper_AVXset.h"
	#elif LOOKUP_STRATEGY == STRATEGY_AVXSET2
		#include "Skipper_AVXset2.h"
	#else
		#error Unknown skipping strategy
	#endif
#endif


#endif // SKIPPER_H_INCLUDED
