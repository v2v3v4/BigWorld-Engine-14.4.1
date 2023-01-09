#include "cells.hpp"

#include "cell_data.hpp"
#include "cellapp.hpp"
#include "space.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Cells
// -----------------------------------------------------------------------------

/**
 *	Destructor.
 */
Cells::~Cells()
{
	Container::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		delete *iter;
		++iter;
	}
}


/**
 *	This method returns the cell that is in the input space.
 *
 *	@return If the desired cell is not found, NULL is returned.
 */
CellData * Cells::findFromSpaceID( SpaceID spaceID ) const
{
	Container::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		if ((*iter)->space().id() == spaceID)
		{
			return *iter;
		}

		++iter;
	}

	return NULL;
}


/**
 *	This method returns the cell that is in the input app.
 *
 *	@return If the desired cell is not found, NULL is returned.
 */
CellData * Cells::findFromCellAppAddr( const Mercury::Address & addr ) const
{
	Container::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		if ((*iter)->addr() == addr)
		{
			return *iter;
		}

		++iter;
	}

	INFO_MSG( "Cells::findFromCellAppAddr: Did not find %s\n", addr.c_str() );
	return NULL;
}


/**
 *
 */
void Cells::notifyOfCellRemoval( SpaceID spaceID, CellData & removedCell ) const
{
	const Mercury::Address removedAddress = removedCell.addr();
	Mercury::Bundle & bundleToRemoved = removedCell.cellApp().bundle();
	bundleToRemoved.startMessage( CellAppInterface::removeCell );
	bundleToRemoved << spaceID;

	Container::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		CellData * pCell = *iter;

		// Should have been removed already.
		MF_ASSERT( pCell != &removedCell );

		CellApp & cellApp = pCell->cellApp();

		bundleToRemoved << cellApp.addr();

		Mercury::Bundle & bundle = cellApp.bundle();
		bundle.startMessage( CellAppInterface::notifyOfCellRemoval );
		bundle << spaceID;
		bundle << removedAddress;
		cellApp.channel().delayedSend();

		++iter;
	}

	removedCell.cellApp().channel().delayedSend();
}


/**
 *	This method removes the cell this collection.
 */
void Cells::erase( CellData * pData )
{
	Container::iterator iter = std::find( cells_.begin(), cells_.end(), pData );

	if (iter != cells_.end())
	{
		cells_.erase( iter );
	}
}


/**
 *	This method deletes all cells in this collection.
 *	Note that the cell is reponsible for removing itself from the collection.
 */
void Cells::deleteAll()
{
	if (!cells_.empty())
	{
		WARNING_MSG( "Cells::deleteAll: Still have %" PRIzu " cells\n",
				cells_.size() );

		while (!cells_.empty())
		{
			delete cells_.front();
		}
	}
}


/**
 *	This method tells all cells to start retiring.
 */
void Cells::retireAll()
{
	Container::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		(*iter)->startRetiring();
		++iter;
	}
}


/**
 *
 */
void Cells::cancelRetiring()
{
	Container::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		(*iter)->cancelRetiring();

		++iter;
	}
}


WatcherPtr Cells::pWatcher()
{
	WatcherPtr pWatcher = new SequenceWatcher< Container >();
	pWatcher->addChild( "*", new BaseDereferenceWatcher(
		CellData::pWatcher() ) );

	return pWatcher;
}

BW_END_NAMESPACE

// cells.cpp
