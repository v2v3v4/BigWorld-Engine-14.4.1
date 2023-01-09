#include "pch.hpp"
#include "data_section_cache_purger.hpp"

#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

/**
 *	DataSectionCachePurger constructor
 */
DataSectionCachePurger::DataSectionCachePurger()
{
}


/**
 *	DataSectionCachePurger destructor
 */
DataSectionCachePurger::~DataSectionCachePurger()
{
	BWResource::instance().purgeAll();
}

BW_END_NAMESPACE

// data_section_cache_purger.cpp