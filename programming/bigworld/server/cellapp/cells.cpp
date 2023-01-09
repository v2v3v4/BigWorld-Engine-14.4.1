#include "script/first_include.hpp"

#include "cells.hpp"

#include "cell.hpp"
#include "entity.hpp"
#include "space.hpp"

#include "chunk/chunk_space.hpp"
#include "cstdmf/binary_stream.hpp"
#include "math/boundbox.hpp"



BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
Cells::Cells():
	container_()
{
}


/**
 *	This method streams on various boundaries to inform the CellAppMgr for the
 *	purposes of load balancing.
 *
 */
void Cells::writeBounds( BinaryOStream & stream ) const
{
	Container::const_iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->writeBounds( stream );

		++iter;
	}

}

void Cells::destroy( Cell * pCell )
{
	Container::iterator iter = container_.find( pCell->spaceID() );

	MF_ASSERT( iter != container_.end() );

	if (iter != container_.end())
	{
		container_.erase( iter );
		delete pCell;
	}
	else
	{
		ERROR_MSG( "Cells::deleteCell: Unable to kill cell %u\n",
									pCell->spaceID() );
	}

}


void Cells::destroyAll()
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		delete iter->second;
		++iter;
	}
	container_.clear();
}


/**
 *	This method returns the number of real entities across all cells.
 */
int Cells::numRealEntities() const
{
	int numReals = 0;
	Container::const_iterator iter = container_.begin();

	while (iter != container_.end())
	{
		numReals += iter->second->numRealEntities();

		++iter;
	}

	return numReals;
}


void Cells::handleCellAppDeath( const Mercury::Address & addr )
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->handleCellAppDeath( addr );

		++iter;
	}
}


/**
 *	This method checks whether any cells should offload any entities or create
 *	or destroy any ghosts.
 *
 *	It also destroys any cells that can now safely be destroyed.
 */
void Cells::checkOffloads()
{
	Container toDelete;

	Container::iterator iCell = container_.begin();

	while (iCell != container_.end())
	{
		Cell * pCell = iCell->second;

		bool isReadyForDeletion = pCell->checkOffloadsAndGhosts();

		if (isReadyForDeletion)
		{
			toDelete[ iCell->first ] = pCell;
		}

		++iCell;
	}

	iCell = toDelete.begin();

	while (iCell != toDelete.end())
	{
		this->destroy( iCell->second );

		++iCell;
	}
}


void Cells::backup( int index, int period )
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->backup( index, period );

		++iter;
	}
}


void Cells::add( Cell * pCell )
{
	Container::const_iterator iter = container_.find( pCell->spaceID() );

	MF_ASSERT( iter == container_.end() );

	container_[ pCell->spaceID() ] = pCell;
}


void Cells::debugDump()
{

	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		iter->second->debugDump();

		++iter;
	}
}


void Cells::tickRecordings()
{
	Container::iterator iCell = container_.begin();

	while (iCell != container_.end())
	{
		iCell->second->tickRecording();
		++iCell;
	}
}


/**
 * This method ticks profilers on all the cells
 */
void Cells::tickProfilers( uint64 tickDtInStamps, float smoothingFactor )
{
	Container::iterator iCell = container_.begin();

	while (iCell != container_.end())
	{
		iCell->second->tickProfilers( tickDtInStamps, smoothingFactor );

		++iCell;
	}
}

#if ENABLE_WATCHERS

WatcherPtr Cells::pWatcher()
{
	WatcherPtr pWatcher = new MapWatcher< Container >();
	pWatcher->addChild( "*", new BaseDereferenceWatcher( Cell::pWatcher() ) );

	return pWatcher;
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// cells.cpp

