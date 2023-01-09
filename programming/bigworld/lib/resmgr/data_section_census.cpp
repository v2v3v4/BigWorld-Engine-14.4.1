#include "pch.hpp"
#include "data_section_census.hpp"

#include "datasection.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/slot_tracker.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stringmap.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DataSectionCensus
// -----------------------------------------------------------------------------

namespace // anonymous
{
typedef StringRefMap< DataSection * > DSStringMap;
typedef BW::map< const DataSection *, BW::string > DSPointerMap;

struct DSCData
{
	DSStringMap		strMap_;
	DSPointerMap	ptrMap_;
	RecursiveMutex	mutex_;
};

DSCData * s_pDSCData = NULL;

/// Common DataSectionCensus removal code
void remove( const DataSection * pSect )
{
	// requires external locking
	MF_ASSERT( s_pDSCData->mutex_.lockCount() > 0 );

	DSPointerMap::iterator found = s_pDSCData->ptrMap_.find( pSect );
	if (found != s_pDSCData->ptrMap_.end())
	{
		DSStringMap::iterator found2 = s_pDSCData->strMap_.find( found->second );
		s_pDSCData->strMap_.erase( found2 );
		s_pDSCData->ptrMap_.erase( found );
	}
}

} // anonymous namespace


/**
 *	This method finds the data section matched to the given identifier
 *	if one was created and is still around
 */
DataSectionPtr DataSectionCensus::find( const BW::StringRef & id )
{
	BW_GUARD;

	if (s_pDSCData == NULL)
	{
		return NULL;
	}

	RecursiveMutexHolder lock( s_pDSCData->mutex_ );
	DSStringMap::iterator found = s_pDSCData->strMap_.find( id );
	if (found != s_pDSCData->strMap_.end())
	{
		return found->second;
	}

	return NULL;
}


/**
 *	This method adds a data section to the census. They can't add themselves,
 *	because they do not know (and may not have) their own file name.
 */
DataSectionPtr DataSectionCensus::add( const BW::StringRef & id,
	const DataSectionPtr & pSect )
{
	BW_GUARD;

	// DataSectionCensus::init() should have been called before now
	MF_ASSERT( s_pDSCData != NULL );
	if (s_pDSCData == NULL)
	{
		return NULL;
	}

	RecursiveMutexHolder lock( s_pDSCData->mutex_ );

	// First check to see if we already have this one. This only happens due
	// to a race condition between checking the cache and going and loading
	// then adding it when it isn't there. (Two threads can do this simultaneously)
	DSStringMap::iterator found = s_pDSCData->strMap_.find( id );
	if (found != s_pDSCData->strMap_.end())
	{
		return found->second;
	}

	s_pDSCData->strMap_.insert( 
		std::make_pair( id, pSect.getObject() ) );
	s_pDSCData->ptrMap_.insert( 
		std::make_pair( pSect.getObject(), id.to_string() ) );

	return pSect;
}


/**
 *	This method removes a data section from the census.
 */
void DataSectionCensus::del( const DataSection * pSect )
{
	BW_GUARD;

	if (s_pDSCData == NULL)
	{
		return;
	}

	RecursiveMutexHolder lock( s_pDSCData->mutex_ );
	remove( pSect );
}


/**
 *	This method removes a data section from the census. This should only
 *	be called from DataSection::destroy when the DataSection ref count is 0.
 */
void DataSectionCensus::tryDestroy( const DataSection * pSect )
{
	BW_GUARD;

	// s_pDSCData may have been destroyed before the last data section
	if (s_pDSCData == NULL)
	{
		delete pSect;
		return;
	}

	RecursiveMutexHolder lock( s_pDSCData->mutex_ );

	// Now we are inside the global lock, double check that something hasn't
	// incremented our ref count again.
	if (pSect->refCount() == 0)
	{
		remove( pSect );
		delete pSect;
	}
}


/**
 *	Allocate internal data
 */
void DataSectionCensus::init()
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	if (s_pDSCData == NULL)
	{
		s_pDSCData = new DSCData;
	}
}


/**
 *	Free internal allocations
 */
void DataSectionCensus::fini()
{
	BW_GUARD;
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	bw_safe_delete( s_pDSCData );
}


/**
 *	This method clears everything from the census
 */
void DataSectionCensus::clear()
{
	BW_GUARD;

	if (s_pDSCData == NULL)
	{
		return;
	}

	RecursiveMutexHolder lock( s_pDSCData->mutex_ );

	s_pDSCData->strMap_.clear();
	s_pDSCData->ptrMap_.clear();
}


BW_END_NAMESPACE

// data_section_census.cpp
