#include "space.hpp"

#include "cellapp.hpp"
#include "cell_app_group.hpp"
#include "cell_app_groups.hpp"
#include "cell_app_load_config.hpp"
#include "cellappmgr.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "common/space_data_types.hpp"

#include "cstdmf/polymorphic_watcher.hpp"

#include "network/nub_exception.hpp"
#include "network/bundle.hpp"

#include "server/bwconfig.hpp"

#include <limits>


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "space.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: AddCellReplyHandler
// -----------------------------------------------------------------------------

/**
 *
 */
class AddCellReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	AddCellReplyHandler( const Mercury::Address & addr,
			SpaceID spaceID ) :
		addr_( addr ),
		spaceID_( spaceID )
	{
	}

private:
	void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * /*arg*/ )
	{
		MF_ASSERT( source == addr_ );

		CellAppMgr & mgr = ServerApp::getApp< CellAppMgr >( header );
		Space * pSpace = mgr.findSpace( spaceID_ );

		if (pSpace)
		{
			CellData * pCell = pSpace->findCell( addr_ );

			if (pCell)
			{
				pCell->hasBeenCreated( true );
			}
			else
			{
				WARNING_MSG( "AddCellReplyHandler::handleMessage: "
							"Space %d has no cell on %s\n",
						spaceID_, addr_.c_str() );
			}
		}
		else
		{
			WARNING_MSG( "AddCellReplyHandler::handleMessage: "
								"No space %d from %s\n",
							spaceID_, addr_.c_str() );
		}

		delete this;
	}

	void handleException( const Mercury::NubException & ne, void * arg )
	{
		WARNING_MSG( "AddCellReplyHandler::handleException: "
				"Received exception (%s) addr = %s. spaceID_ = %d\n",
				Mercury::reasonToString( ne.reason() ),
				addr_.c_str(), spaceID_ );

		delete this;
	}

	const Mercury::Address addr_;
	const SpaceID spaceID_;
};


// -----------------------------------------------------------------------------
// Section: Static data
// -----------------------------------------------------------------------------

Space::CellsToDelete Space::cellsToDelete_;


// -----------------------------------------------------------------------------
// Section: Space
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Space::Space( SpaceID id, bool isNewSpace, bool isFromDB, uint32 preferredIP ) :
	id_( id ),
	pRoot_( NULL ),
	isBalancing_( false ),
	preferredIP_( preferredIP ),
	isFirstCell_( isNewSpace ),
	isFromDB_( isFromDB ),
	hasHadEntities_( !isFromDB ),
	waitForChunkBoundUpdateCount_( 0 ),
	spaceGrid_( 0.f ),
	spaceBounds_( 0.f, 0.f, 0.f, 0.f ),
	artificialMinLoad_( 0.f )
{
}


/**
 *	Destructor.
 */
Space::~Space()
{
	CM::BSPNode * pRoot = pRoot_;
	pRoot_ = NULL;
	delete pRoot;

	MF_ASSERT( cells_.empty() );
}


/**
 *	This method shuts down this space and removes it from the system.
 */
void Space::shutDown()
{
	INFO_MSG( "Space::shutDown: Shutting down space %u "
				"(remaining cells: %" PRIzu ")\n",
			id_, cells_.size() );

	Cells::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		CellApp * pApp = (*iter)->pCellApp();

		if (pApp)
		{
			pApp->shutDownSpace( this->id() );
		}

		++iter;
	}
}


/**
 *
 */
void Space::informCellAppsOfGeometry( bool shouldSend )
{

	Cells::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		Mercury::Bundle & bundle = (*iter)->cellApp().bundle();
		bundle.startMessage( CellAppInterface::updateGeometry );
		this->addToStream( bundle );

		// TODO: This could be optimised so that we do not send this if it
		// hasn't changed. This would be particularly true for single cell
		// spaces.

		// The send may be delayed so that all space updates are sent in one
		// send.
		if (shouldSend)
		{
			(*iter)->cellApp().send();
		}

		++iter;
	}

}


/**
 *	This method changes the geometry of cells to balance the load.
 */
void Space::loadBalance()
{
#ifndef BW_EVALUATION
	if (isBalancing_)
	{
		WARNING_MSG( "Space::loadBalance( %d ): Called recursively.\n", id_ );
		return;
	}

	if (pRoot_ == NULL)
	{
		INFO_MSG( "Space::loadBalance( %d ): Called with pRoot_ == NULL.\n",
				id_ );
		return;
	}

	isBalancing_ = true;

	pRoot_->updateLoad();

	BW::Rect rect(
		-std::numeric_limits< float >::max(),
		-std::numeric_limits< float >::max(),
		std::numeric_limits< float >::max(),
		std::numeric_limits< float >::max() );

	// If a branch has a cell that is overloaded, we do not want to make
	// things worse. This is especially a cap when new cells are being
	// added.
	float loadSafetyBound = std::max( CellAppLoadConfig::safetyBound(),
			pRoot_->avgSmoothedLoad() * CellAppLoadConfig::safetyRatio() );

	bool wasLoaded = this->hasLoadedRequiredChunks();
	pRoot_->balance( rect, loadSafetyBound );

	pRoot_->updateLoad();
	if (wasLoaded && !this->hasLoadedRequiredChunks())
	{
		ERROR_MSG( "Space::loadBalance: "
			"Space %u has reverted to requiring chunks to load.\n", id_ );
	}

	// The send is delayed so that all space updates are sent in one
	// send.
	this->informCellAppsOfGeometry( /* shouldSend */ false );

	// TODO: I don't think this is needed. isBalancing_ enforces this is not
	// called recursively.

	// Delete cells from this space that are no longer included. Do this
	// after updateGeometry so that the cells being deleted know the latest
	// layout.
	{
		// Take a copy here because this method is called recursively.
		CellsToDelete copyOfCellsToDelete;
		copyOfCellsToDelete.swap( cellsToDelete_ );

		CellsToDelete::iterator iter = copyOfCellsToDelete.begin();

		while (iter != copyOfCellsToDelete.end())
		{
			(*iter)->removeSelf();

			++iter;
		}
	}

	this->checkCellsHaveLoadedMappedGeometry();

	isBalancing_ = false;
#endif
}


/**
 *	This method updates the ranges of nodes in the BSP tree.
 */
void Space::updateRanges()
{
	if (pRoot_ == NULL)
	{
		INFO_MSG( "Space::updateRanges( %d ): Called with pRoot_ == NULL.\n",
				id_ );
		return;
	}

	pRoot_->updateLoad();

	BW::Rect rect(
		-std::numeric_limits< float >::max(),
		-std::numeric_limits< float >::max(),
		std::numeric_limits< float >::max(),
		std::numeric_limits< float >::max() );

	pRoot_->updateRanges( rect );
}


/**
 *	This method adds this space to the input stream.
 *
 *	@param stream	The stream to add the space to.
 *	@param isForViewer Indicates whether the stream is being sent to CellApps or
 *		to SpaceViewer.
 */
void Space::addToStream( BinaryOStream & stream, bool isForViewer ) const
{
	stream << id_;
	if (pRoot_)
	{
		if (isForViewer)
		{
			stream << CellAppMgr::instance().numCellApps();
			stream << CellAppMgr::instance().numEntities();
		}
		pRoot_->addToStream( stream, isForViewer );
	}
}

/**
 *	This method adds all the geometry mappings in this space to the stream.
 *
 *	@param stream	The stream to add the geometry mapping data to.
 */
void Space::addGeometryMappingsToStream( BinaryOStream & stream ) const
{
	for ( DataEntries::const_iterator it = dataEntries_.begin();
			it != dataEntries_.end(); ++it )
	{
		if ((it->second.key == SPACE_DATA_MAPPING_KEY_CLIENT_SERVER) ||
			(it->second.key == SPACE_DATA_MAPPING_KEY_CLIENT_ONLY))
		{
			stream << it->second.key;
			const SpaceData_MappingData& data = *reinterpret_cast<
					const SpaceData_MappingData* >( it->second.data.data() );

			// Stream elements independently since Matrix is not packed.
			for ( int i = 0; i < 4; ++i )
				for ( int j = 0; j < 4; ++j )
					stream << data.matrix[i][j];

			stream << it->second.data.substr( sizeof( SpaceData_MappingData ) );
		}
	}
}

/**
 *	This method finds a cell corresponding to the input position.
 */
CellData * Space::findCell( float x, float z ) const
{
	return pRoot_ ? pRoot_->pCellAt( x, z ) : NULL;
}


/**
 *	This method finds a cell corresponding to the input address.
 */
CellData * Space::findCell( const Mercury::Address & addr ) const
{
	return cells_.findFromCellAppAddr( addr );
}


/**
 *	This method creates a cell and adds it to this space.
 */
CellData * Space::addCell( CellApp & cellApp, CellData * pCellToSplit )
{
	INFO_MSG( "Space::addCell: Space %u. CellApp %u (%s)\n",
			id_, cellApp.id(), cellApp.addr().c_str() );

	if (cellApp.isRetiring())
	{
		WARNING_MSG( "Space::addCell: Adding a cell to CellApp %u (%s) which "
			"is retiring.\n", cellApp.id(), cellApp.addr().c_str() );
	}

	CellData * pCellData = new CellData( cellApp, *this );

	if (pCellToSplit)
	{
		MF_ASSERT( pRoot_ != NULL );
		pRoot_ = pRoot_->addCellTo( pCellData, pCellToSplit );
		MF_ASSERT( pRoot_ != NULL );
	}
	else
	{
		pRoot_ = (pRoot_ ? pRoot_->addCell( pCellData ) : pCellData);
	}

	pRoot_->updateLoad();

	CellAppGroup * pSpaceGroup = this->cells().front()->cellApp().pGroup();
	CellAppGroup * pNewAppGroup = cellApp.pGroup();

	if (pNewAppGroup)
	{
		pNewAppGroup->join( pSpaceGroup );
	}
	else if (pSpaceGroup)
	{
		pSpaceGroup->insert( &cellApp );
	}

	Mercury::Bundle & bundle = cellApp.bundle();
	bundle.startRequest( CellAppInterface::addCell,
			new AddCellReplyHandler( cellApp.addr(), id_ ) );
	this->addToStream( bundle );

	bundle << isFirstCell_;
	isFirstCell_ = false;

	bundle << isFromDB_;

	//prepare space data for sending
	{
		// bundle.startMessage( CellAppInterface::allSpaceData );
		// bundle << id_;
		bundle << (uint32)dataEntries_.size();

		DataEntries::const_iterator iter = dataEntries_.begin();

		while (iter != dataEntries_.end())
		{
			bundle << iter->first <<
				iter->second.key << iter->second.data;
			++iter;
		}
	}

	cellApp.send();

	return pCellData;
}


/**
 *	This method adds a new cell to this Space. It chooses an appropriate
 *	CellApp.
 */
CellData * Space::addCell()
{
	CellAppGroup * pGroup = NULL;

	if (!cells_.empty())
	{
		pGroup = cells_.front()->cellApp().pGroup();
	}

	const CellApps & cellApps = CellAppMgr::instance().cellApps();
	CellApp * pCellApp = cellApps.findBestCellApp( this, pGroup );


	return pCellApp != NULL ? this->addCell( *pCellApp ) : NULL;
}


/**
 *	This method adds a new cell to the tree.
 */
CellData * Space::addCellTo( CellData * pCellToSplit )
{
	if (cells_.empty())
	{
		ERROR_MSG( "Space::addCellTo( %d ): cells_ is empty\n",
				this->id() );
		return NULL;
	}

	CellAppGroups appGroups( CellAppMgr::instance().cellApps() );

	CellAppGroup * pGroup = cells_.front()->cellApp().pGroup();

	MF_ASSERT( pGroup );

	const CellApps & cellApps = CellAppMgr::instance().cellApps();
	CellApp * pCellApp = cellApps.findBestCellApp( this, pGroup );


	return pCellApp != NULL ? this->addCell( *pCellApp, pCellToSplit ) : NULL;
}


/**
 *	This method is called from the CellData's constructor to add it to the
 *	space.
 */
void Space::addCell( CellData * pCell )
{
	cells_.add( pCell );
}


/**
 *	This method returns the number of unique IP addresses among the CellApps
 *	that handle cells on this space.
 */
uint Space::numUniqueIPs() const
{
	if (cells_.size() <= 1)
	{
		return cells_.size();
	}

	BW::set< uint32 > addresses;

	Cells::const_iterator cellIter = cells_.begin();

	while (cellIter != cells_.end())
	{
		addresses.insert( (*cellIter)->addr().ip );
		++cellIter;
	}

	return addresses.size();
}


/**
 *	This method returns the number of CellApps in this space on a particular IP
 *	address.
 */
int Space::numCellAppsOnIP( uint32 ip ) const
{
	int count = 0;
	Cells::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		count += ((*iter)->cellApp().addr().ip == ip);

		++iter;
	}

	return count;
}


/**
 *	This method recovers cell information from the input stream.
 */
void Space::recoverCells( BinaryIStream & data )
{
	if (pRoot_)
	{
		// TODO: Optimise and we should check that what is sent here
		// matches what we currently think the space looks like.

		// NOTE: We can't construct and delete pRoot_ multiple times because
		// when Space::recoverCell() gets called, it calls setCellApp() on one
		// of the items under pRoot_. If we deleted pRoot_ and reconstructed it,
		// we'd need to somehow call setCellApp() on the correct item again.

		// Need to just skip over the data.
		CellData::readTree( NULL, data );
	}
	else
	{
		pRoot_ = CellData::readTree( this, data );
	}
}


/**
 *	This method is used when a cell has been recovered.
 */
CellData * Space::recoverCell( CellApp & app, const Mercury::Address & addr )
{
	INFO_MSG( "Space::recoverCell: Addr %s. Space ID = %u\n",
			addr.c_str(), id_ );
	CellData * pCell = this->findCell( addr );

	if (pCell)
	{
		pCell->setCellApp( app );
		// Cells are put in the collection to indicate that they have not
		// been recovered.
		MF_VERIFY( cellsToDelete_.erase( pCell ) );
	}
	else
	{
		ERROR_MSG( "Space::recoverCell: No cell %s in space %u\n",
				addr.c_str(), id_ );
	}

	return pCell;
}


/**
 *	This static method cleans up things after a recovery.
 */
void Space::endRecovery()
{
	// TODO: Maybe we should treat unclaimed cells as though the cell
	// application has died unexpectedly.

	if (cellsToDelete_.empty())
	{
		INFO_MSG( "Space::endRecovery(): Clean recovery.\n" );
	}
	else
	{
		WARNING_MSG( "Space::endRecovery: Num cells not recovered = %"
				PRIzu "\n",	cellsToDelete_.size() );

		BW::set< CellData * >::iterator iter = cellsToDelete_.begin();
		while (iter != cellsToDelete_.end())
		{
			WARNING_MSG( "Space::endRecovery: %s in space %u\n",
					(*iter)->addr().c_str(), (*iter)->space().id() );

			++iter;
		}
	}
}


/**
 *	This method enables or disables offloading of entities on all cells in this
 *	space.
 */
void Space::shouldOffload( bool value )
{
    Cells::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		(*iter)->shouldOffload( value );

		iter++;
	}
}


/**
 *	This method returns whether this space is "large". This is currently defined
 *	as being a space that has multiple cells and at least one of those cells
 *	is the only one on its CellApp.
 */
bool Space::isLarge() const
{
	if (this->numCells() < 2)
	{
		return false;
	}

	Cells::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		if ((*iter)->cellApp().numCells() == 1)
		{
			return true;
		}

		++iter;
	}

	return false;
}


/**
 *	This method removes the input cell from this space.
 */
void Space::eraseCell( CellData * pCell, bool notifyCellApps )
{
	cells_.erase( pCell );

	if (notifyCellApps)
	{
		cells_.notifyOfCellRemoval( id_, *pCell );
	}

	if (pRoot_)
	{
		pRoot_ = (pRoot_ != pCell ) ? pRoot_->removeCell( pCell ) : NULL;
		this->updateRanges();
	}
}


/**
 *	This method stores data associated with the space.
 *
 *	@note This does not propagate data to the cells. It is called in response to
 *		data having been propagated already.
 */
void Space::addData( const SpaceEntryID & entryID,
					uint16 key, const BW::string & data, bool possibleRepeat,
					bool setLastMappedGeometry )
{
	DataEntry entry;
	entry.key = key;
	entry.data = data;

	if (!possibleRepeat)
	{
		INFO_MSG( "Space::addData: space = %d. key = %hu, size = %" PRIzu "\n",
				id_, key, data.size() );

		if (dataEntries_.find( entryID ) != dataEntries_.end())
		{
			WARNING_MSG( "Space::addData: "
					"space %u already has data entry (key = %d).\n",
				id_, key );
		}
	}

	dataEntries_[ entryID ] = entry;

	if (setLastMappedGeometry && (key == SPACE_DATA_MAPPING_KEY_CLIENT_SERVER))
	{
		// Remember this so that we can tell scripts what we think is the
		// "last" geometry mapping.
		lastMappedGeometry_ = data.substr( sizeof( SpaceData_MappingData ) );
		waitForChunkBoundUpdateCount_ = 1;
	}

	if (key == SPACE_DATA_ARTIFICIAL_LOAD)
	{
		if (data.size() >= sizeof( SpaceData_ArtificialLoad ) )
		{
			const SpaceData_ArtificialLoad * pLoadData =
					(SpaceData_ArtificialLoad *)data.data();
			artificialMinLoad_ = pLoadData->artificialMinLoad;
		}
		else
		{
			WARNING_MSG( "Space::addData: bad artificial load data "
							"for space %u (key = %d).\n",
						id_, key );
		}
	}

}


/**
 *	This method stores data associated with the space.
 *
 *	@note This does not propagate data to the cells. It is called in response to
 *		data having been propagated already.
 */
void Space::delData( const SpaceEntryID & entryID )
{
	DataEntries::iterator found = dataEntries_.find( entryID );
	if (found == dataEntries_.end())
	{
		WARNING_MSG( "Space::delData: Could not delete data entry.\n" );
		return;
	}

	if (found->second.key == SPACE_DATA_ARTIFICIAL_LOAD)
	{
		artificialMinLoad_ = 0.f;
	}

	dataEntries_.erase( found );
}


/**
 *	This method sends a space data update message to all the cells that are
 * 	part of the space, except the one specified by srcAddr.
 */
void Space::sendSpaceDataUpdate( const Mercury::Address& srcAddr,
		const SpaceEntryID& entryID, uint16 key, const BW::string* pValue )
{
	for (Cells::iterator it = cells_.begin(); it != cells_.end(); ++it)
	{
		CellApp & cellApp = (*it)->cellApp();

		if (cellApp.addr() != srcAddr)
		{
			Mercury::Bundle & bundle = cellApp.bundle();
			bundle.startMessage( CellAppInterface::spaceData );

			bundle << id_ << entryID << key;
			if (key != uint16(-1))
				bundle.addBlob( (void *)pValue->data(), pValue->length() );

			cellApp.send();
		}
	}
}


/**
 *	This method adds data to the input stream that will be sent to the database.
 */
void Space::sendToDB( BinaryOStream & stream ) const
{
	stream << id_;

	// Add the space data.
	stream << uint32( dataEntries_.size() );
	DataEntries::const_iterator iter = dataEntries_.begin();

	while (iter != dataEntries_.end())
	{
		stream << iter->first << iter->second.key << iter->second.data;
		++iter;
	}
}


/**
 *	This method updates space bounds information.
 */
void Space::updateBounds( BinaryIStream & data )
{
	data >> spaceBounds_ >> spaceGrid_;
}


/**
 * @return True if all cells in this space have loaded their
 * respective geometry.
 */
bool Space::cellsHaveLoadedMappedGeometry() const
{
	// Refer to Space::checkCellsHaveLoadedMappedGeometry() for
	// a description of waitForChunkBoundUpdateCount_.

	return (waitForChunkBoundUpdateCount_ <= 0)
		&& this->hasLoadedRequiredChunks();
}


/**
 *	This method checks whether our cells have finished loading the geometry
 *  in their range.
 */
void Space::checkCellsHaveLoadedMappedGeometry()
{
	if (lastMappedGeometry_.empty())
	{
		return;
	}

	if (!this->cellsHaveLoadedMappedGeometry())
	{
		// If we've just mapped a new geometry, allow time for cells
		// to tell us their new chunk bounds before deciding whether
		// they've finished loading their geometry.
		if (waitForChunkBoundUpdateCount_ > 0)
		{
			--waitForChunkBoundUpdateCount_;
		}
		return;
	}

	// Only first cell is bootstrap.
	uint8 flags = SPACE_GEOMETRY_LOADED_BOOTSTRAP_FLAG;

	Cells::iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		Mercury::Bundle & bundle = (*iter)->cellApp().bundle();

		bundle.startMessage( CellAppInterface::spaceGeometryLoaded );
		bundle << id_;
		bundle << flags;

		// Due to network delays a cell could've added another geometry
		// mapping by the time this message reaches them. That's why it's
		// important to tell them what we think is the last geometry
		// mapping.
		bundle << lastMappedGeometry_;

		// Just wait for regular send.
		// (*iter)->cellApp().send();

		flags = 0;	// subsequent cells not bootstrap

		++iter;
	}

	lastMappedGeometry_.clear();
}

/**
 *	This method is used to set the lastMappedGeometry_ of this space. Used
 * 	during recovery only.
 */
void Space::setLastMappedGeometry( const BW::string& lastMappedGeometry )
{
	if (!lastMappedGeometry_.empty())
	{
		// This is unusual. Only one Cell should have the lastMappedGeometry_
		// set. Not fatal for us, but possibly indicates the algorithm isn't
		// working properly.
		ERROR_MSG( "Space::setLastMappedGeometry: Confusion about "
			"lastMappedGeometry - ignoring previous value '%s'\n",
			lastMappedGeometry_.c_str() );
	}
	lastMappedGeometry_ = lastMappedGeometry;
	waitForChunkBoundUpdateCount_ = 1;
}


/**
 *	This method returns whether this space has loaded the chunks that it needs.
 */
bool Space::hasLoadedRequiredChunks() const
{
	return pRoot_ ? pRoot_->hasLoadedRequiredChunks() : true;
}


// -----------------------------------------------------------------------------
// Section: Static methods
// -----------------------------------------------------------------------------

/**
 *	This static method is used during load balancing. If a cell has been fully
 *	removed, it adds itself to a set so that it can be deleted later.
 */
void Space::addCellToDelete( CellData * pCell )
{
	cellsToDelete_.insert( pCell );
}


/**
 *	This method returns a watcher that can be used to watch Space objects.
 */
WatcherPtr Space::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		Space * pNull = NULL;

		pWatcher->addChild( "id", makeWatcher( &Space::id_ ) );

		pWatcher->addChild( "cells", Cells::pWatcher(), &pNull->cells_ );

		pWatcher->addChild( "areaNotLoaded",
			makeWatcher( &Space::areaNotLoaded ) );
		pWatcher->addChild( "numRetiringCells",
			makeWatcher( &Space::numRetiringCells ) );

		pWatcher->addChild( "loadMin", makeWatcher( &Space::minLoad ) );
		pWatcher->addChild( "loadMax", makeWatcher( &Space::maxLoad ) );
		pWatcher->addChild( "loadAvg", makeWatcher( &Space::avgLoad ) );
		pWatcher->addChild( "artificialMinLoad",
							makeWatcher( &Space::artificialMinLoad ) );
		pWatcher->addChild( "numCells", makeWatcher( &Space::numCells ) );

		pWatcher->addChild( "geometry", makeWatcher( &Space::geometryMappingAsString ) );

		pWatcher->addChild( "bsp", new PolymorphicWatcher(), &pNull->pRoot_ );
	}

	return pWatcher;
}


// -----------------------------------------------------------------------------
// Section: Watcher accessors
// -----------------------------------------------------------------------------


/**
 *	This method returns the area of the chunks that are not yet loaded.
 */
float Space::areaNotLoaded() const
{
	return pRoot_ ? pRoot_->areaNotLoaded() : 0.f;
}


/**
 *	This method returns the number of cells retiring from this space.
 */
int Space::numRetiringCells() const
{
	return pRoot_ ? pRoot_->numRetiring() : 0;
}


/**
 *	This method returns the minimum load of all CellApps handling this space.
 */
float Space::minLoad() const
{
	return pRoot_ ? pRoot_->minLoad() : 0.f;
}


/**
 *	This method returns the maximum load of all CellApps handling this space.
 */
float Space::maxLoad() const
{
	return pRoot_ ? pRoot_->maxLoad() : 0.f;
}


/**
 *	This method returns the average load of the cells in this space.
 */
float Space::avgLoad() const
{
	return pRoot_ ? pRoot_->avgLoad() : 0.f;
}


/**
 * This method return artificial minimal load associated with this space.
 * This may be used in load balancing.
 */
float Space::artificialMinLoad() const
{
	return artificialMinLoad_;
}


/**
 * This method returns a per-cell share of artificial minimal load.
 */
float Space::artificialMinLoadCellShare( float receivedCellLoad ) const
{
	const int numCells = this->numCells();
	if ( numCells == 0 )
	{
		return 0.f;
	}

	float totalLoad = 0.f;
	float maxLoad = 0.f;

	for (Cells::const_iterator it = cells_.begin();
				it != cells_.end();
				++it)
	{
		const CellData * pCellData = *it;
		float cellLoad = pCellData->lastReceivedLoad();

		maxLoad = std::max( cellLoad, maxLoad );

		totalLoad += cellLoad;
	}

	if (totalLoad >= artificialMinLoad_)
	{
		return 0.f;
	}

	const float addedLoad = artificialMinLoad_ - totalLoad;

	float sumOfDeltas = numCells * maxLoad - totalLoad;

	float artificialLoadShare = 0.f;
	const float thisDelta = maxLoad - receivedCellLoad;

	if (addedLoad >= sumOfDeltas)
	{
		artificialLoadShare =  thisDelta +
						(addedLoad - sumOfDeltas) / numCells;
	}
	else
	{
		artificialLoadShare = addedLoad * thisDelta / sumOfDeltas;
		// Just in case the float division makes calculated share infinite
		artificialLoadShare = std::min( artificialLoadShare, artificialMinLoad_ );
	}

	return artificialLoadShare;
}


/** 
 *	This method returns only the *first* geometry mapping name, 
 *	eg: "spaces/highlands". 
 */
BW::string Space::geometryMappingAsString() const
{
	for (DataEntries::const_iterator it = dataEntries_.begin(); 
		it != dataEntries_.end(); ++it)
	{
		if ((it->second.key == SPACE_DATA_MAPPING_KEY_CLIENT_SERVER) ||
			(it->second.key == SPACE_DATA_MAPPING_KEY_CLIENT_ONLY))
		{
			return it->second.data.substr( sizeof( SpaceData_MappingData ) );
		}
	}

	return "";
}


/**
 *	This method prints debug information about the space to the logs.
 */
void Space::debugPrint()
{
	pRoot_->debugPrint( 0 );
}

BW_END_NAMESPACE

// space.cpp
