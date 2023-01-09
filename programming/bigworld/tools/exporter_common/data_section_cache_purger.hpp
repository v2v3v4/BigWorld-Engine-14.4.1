#ifndef _DATA_SECTION_CACHE_PURGER_HPP_
#define _DATA_SECTION_CACHE_PURGER_HPP_

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Data section cache resource manager class.  This class purges all entries
 *	from the data section cache on destruction.
 */
class DataSectionCachePurger
{
public:
	DataSectionCachePurger();
	~DataSectionCachePurger();
};

BW_END_NAMESPACE

#endif // data_section_cache_purger.hpp