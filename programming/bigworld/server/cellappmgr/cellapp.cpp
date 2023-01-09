#include "cellapp.hpp"

#include "cell_app_group.hpp"
#include "cellappmgr.hpp"
#include "cellappmgr_config.hpp"
#include "shutdown_handler.hpp"
#include "space.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "network/bundle.hpp"

#ifndef CODE_INLINE
#include "cellapp.ipp"
#endif


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CellApp::CellApp( const Mercury::Address & addr,
		uint16 viewerPort, CellAppID id ) :
	ChannelOwner( CellAppMgr::instance().interface(), addr ),
	currLoad_( 0.f ),
	lastReceivedLoad_( 0.f ),
	smoothedLoad_( 0.f ),
	estimatedLoad_( 0.f ),
	numEntities_( 0 ),
	viewerPort_( viewerPort ),
	id_( id ),
	isRetiring_( false ),
	numCellsRetiring_( 0 ),
	shutDownStage_( SHUTDOWN_NONE ),
	pGroup_( NULL )
{
	PROC_IP_INFO_MSG( "CellApp::CellApp: addr = %s\n", this->addr().c_str() );

	this->channel().isLocalRegular( false );
	this->channel().isRemoteRegular( true );
}


/**
 *	Destructor.
 */
CellApp::~CellApp()
{
	PROC_IP_INFO_MSG( "CellApp::~CellApp: addr = %s\n", this->addr().c_str() );

	MF_ASSERT( pGroup_ == NULL );

	cells_.deleteAll();
}


// -----------------------------------------------------------------------------
// Section: Message handlers
// -----------------------------------------------------------------------------

/**
 *	This method is called to inform us of the current load on a cell app.
 */
void CellApp::informOfLoad( const CellAppMgrInterface::informOfLoadArgs & args )
{
	lastReceivedLoad_ = args.load;

	float addedArtificialLoad = 0.f;
	for (Cells::const_iterator it = cells_.begin();
			it != cells_.end();
			++it)
	{
		addedArtificialLoad +=
				(*it)->space().artificialMinLoadCellShare( lastReceivedLoad_ );
	}

	currLoad_ = lastReceivedLoad_ + addedArtificialLoad;
	float bias = CellAppMgrConfig::loadSmoothingBias();
	smoothedLoad_ = ((1.f - bias) * smoothedLoad_) + (bias * currLoad_);
	estimatedLoad_ = smoothedLoad_;
	numEntities_ = args.numEntities;
}


/**
 *	This method handles a message from the associated cellapp. It is used to
 *	inform us about where the entities are on the cells of this application.
 */
void CellApp::updateBounds( BinaryIStream & data )
{
	while (data.remainingLength() != 0)
	{
		SpaceID spaceID;
		data >> spaceID;
		CellData * pCell = cells_.findFromSpaceID( spaceID );

		if (pCell != NULL)
		{
			pCell->updateBounds( data );
		}
		else
		{
			ERROR_MSG( "CellApp::updateBounds: "
					"CellApp %s has no cell in space %u\n",
				this->addr().c_str(), spaceID );
			// Just forget the remaining updates. It'll get it next time.
			data.finish();
			return;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------


/**
 *	This method updates the estimated load of the CellApp which may be
 *	influenced by actions that occur between informOfLoad() messages from
 *	the respective CellApp process. 
 */
void CellApp::updateEstimatedLoad( float loadIncrease )
{
	estimatedLoad_ += loadIncrease; 
}


/**
 *	This method handles a cell being added to the application.
 */
void CellApp::addCell( CellData * pCellData )
{
	cells_.add( pCellData );

	// Update the load estimate
	this->updateEstimatedLoad( CellAppMgrConfig::estimatedInitialCellLoad() );
}


/**
 *	This method erases the input cell from this application.
 */
void CellApp::eraseCell( CellData * pCellData )
{
	cells_.erase( pCellData );

	if (isRetiring_ && cells_.empty())
	{
		this->shutDown();
	}
}


/**
 *	This method returns the cell on this app that is part of the space with
 *	the input id.
 */
CellData * CellApp::findCell( SpaceID id ) const
{
	return cells_.findFromSpaceID( id );
}


/**
 *	This method starts the process of shutting down the application
 *	associated with this object.
 */
void CellApp::startRetiring()
{
	INFO_MSG( "CellApp::startRetiring: Retiring CellApp %u - %s\n",
			id_, this->addr().c_str() );

	isRetiring_ = true;

	if (!cells_.empty())
	{
		this->retireAllCells();
	}
	else
	{
		this->shutDown();
	}
}


/**
 *	This method starts retiring all cells from this app.
 */
void CellApp::retireAllCells()
{
	cells_.retireAll();
}


/**
 *	This method cancels the retiring state of all retiring cells from this app.
 */
void CellApp::cancelRetiringCells()
{
	MF_ASSERT( !this->isRetiring() );
	cells_.cancelRetiring();
}


/**
 *	This method handles setting a watcher. It starts to retire this application
 *	if true is passed.
 */
void CellApp::isRetiring( bool value )
{
	if (value != isRetiring_)
	{
		if (value)
		{
			this->startRetiring();
		}
		else
		{
			WARNING_MSG( "CellApp::isRetiring: Cannot stop retiring\n" );
		}
	}
}


/**
 *	This method shuts down the cell application associated with this object.
 */
void CellApp::shutDown()
{
	CellAppInterface::shutDownArgs args;
	args.isSigInt = false; // Not used.

	this->bundle() << args;
	this->send();
}


/**
 *	This method shuts down the cell application associated with this object in
 *	a controlled way.
 */
void CellApp::controlledShutDown( ShutDownStage stage, GameTime shutDownTime )
{
	CellAppInterface::controlledShutDownArgs args;
	args.stage = stage;
	args.shutDownTime = shutDownTime;

	this->bundle() << args;
	this->send();
}


/**
 *	This method lets the CellApp know that the space is being destroyed.
 */
void CellApp::shutDownSpace( SpaceID spaceID )
{
	Mercury::Bundle & bundle = this->bundle();
	bundle.startMessage( CellAppInterface::shutDownSpace );
	bundle << spaceID;

	this->send();
}


/**
 *	This method is called by the cell application when it wants to be shut down.
 */
void CellApp::retireApp()
{
	INFO_MSG( "CellApp::retireApp: CellApp %u is requesting to retire.\n",
			this->id() );
	this->startRetiring();
}


/**
 *	This method handles a message from the cell application acknowledging a
 *	stage of the controlled shutdown process.
 */
void CellApp::ackCellAppShutDown(
		const CellAppMgrInterface::ackCellAppShutDownArgs & args )
{
	//shutDownStage_ = args.stage;
	//CellAppMgr::instance().checkControlledShutDown();
	ShutDownHandler * pHandler = CellAppMgr::instance().pShutDownHandler();

	if (pHandler)
	{
		pHandler->ackCellApp( args.stage, *this );
	}
}


/**
 *	This method is called when the cell application associated with this object
 *	dies unexpectedly.
 *
 *	@param baseAppData	The stream that will receive the data that is to be sent
 *						to the base applications (via base app manager).
 */
void CellApp::handleUnexpectedDeath( BinaryOStream & baseAppData )
{
	WARNING_MSG( "CellApp::handleUnexpectedDeath: Cell App = %s\n",
		this->addr().c_str() );

	while (!cells_.empty())
	{
		CellData * pCell = cells_.front();
		Space & space = pCell->space();

		INFO_MSG( "CellApp::handleUnexpectedDeath: Cell in space %u is dead\n",
			space.id() );

		// Erase the dead cell first so it doesn't get involved in BSP
		// calculations.
		space.eraseCell( pCell );

		// Mark the space as not yet having had any entities, so that it will
		// not be shut down for being empty.
		space.hasHadEntities( false );

		// Do we have the last cell?
		if (space.numCells() < 1)
		{
			CellApp * pAltApp = CellAppMgr::instance().findAlternateApp( this );

			if (pAltApp)
			{
				INFO_MSG( "CellApp::handleUnexpectedDeath: "
						"Adding a new cell to space %u on app %s\n",
					space.id(), pAltApp->addr().c_str() );
				space.addCell( *pAltApp );
			}
			else
			{
				ERROR_MSG( "CellApp::handleUnexpectedDeath: "
					"No alternate cell app found\n" );
			}
		}

		CellData * pAlternate =
			space.findCell( pCell->range().xRange().midPoint(),
							pCell->range().yRange().midPoint()  );

		if (pAlternate)
		{
			// Try to get a cell from this space on a different machine if there
			// is one.
			if (pAlternate->cellApp().addr().ip == this->addr().ip)
			{
				INFO_MSG( "CellApp::handleUnexpectedDeath: "
						"first alternate CellApp %s is on same machine.\n",
					pAlternate->addr().c_str() );

				Cells::const_iterator iter = space.cells().begin();
				Cells::const_iterator endIter = space.cells().end();
				float bestLoad = 999999.f;

				while (iter != endIter)
				{
					CellData * pCurr = *iter;
					if (pCurr->cellApp().addr().ip != this->addr().ip)
					{
						float currLoad = pCurr->cellApp().smoothedLoad();

						if (currLoad < bestLoad)
						{
							pAlternate = pCurr;
							bestLoad = currLoad;
						}
					}

					++iter;
				}

				// There wasn't one currently in the space. Check if there is a
				// different CellApp machine available and add a cell to it.
				if (pAlternate->cellApp().addr().ip == this->addr().ip)
				{
					INFO_MSG( "CellApp::handleUnexpectedDeath: "
							"Entire space is on the same machine.\n" );
					CellApp * pBest =
						CellAppMgr::instance().cellApps().leastLoaded( this );

					if (pBest)
					{
						INFO_MSG( "CellApp::handleUnexpectedDeath: "
								"Adding a new cell to space %u on app %s\n",
							space.id(), pBest->addr().c_str() );
						pAlternate =
							space.addCell( *pBest );
					}
				}
			}

			INFO_MSG( "CellApp::handleUnexpectedDeath: "
				"alternate cell app = %s\n",
				pAlternate->addr().c_str() );

			baseAppData << pCell->space().id();		// Space id
			baseAppData << pAlternate->addr();		// New cell app
		}
		else
		{
			ERROR_MSG( "CellApp::handleUnexpectedDeath: "
					"Could not find for cell in space %u\n",
				space.id() );
		}

		space.informCellAppsOfGeometry( /* shouldSend */ true );

		delete pCell;
	}

	this->channel().setRemoteFailed();
}


/**
 *	This method returns whether or not any of the cells on this application are
 *	in the process of being retired. It is used when balancing load between cell
 *	applications.
 */
bool CellApp::hasAnyRetiringCells() const
{
	return numCellsRetiring_ > 0;
}


/**
 *	This method returns whether or not all of the cells on this application are
 *	in the process of being retired. It is used when balancing load between cell
 *	applications.
 */
bool CellApp::hasOnlyRetiringCells() const
{
	return numCellsRetiring_ >= this->numCells();
}


/**
 *	This method increases the number of cells on the application that are
 *	shutting down.
 */
void CellApp::incCellsRetiring()
{
	++numCellsRetiring_;
}


/**
 *	This method decreases the number of cells on the application that are
 *	shutting down.
 */
void CellApp::decCellsRetiring()
{
	--numCellsRetiring_;
	MF_ASSERT( numCellsRetiring_ >= 0 );
}


/**
 *	This method returns whether or not this application has a cell that belongs
 *	to a multicell space.
 */
bool CellApp::hasMulticellSpace() const
{
	Cells::const_iterator iter = cells_.begin();

	while (iter != cells_.end())
	{
		if ((*iter)->space().numCells() > 1)
		{
			return true;
		}

		++iter;
	}

	return false;
}


/**
 *	This method chooses a cell that should be offloaded when we want to reduce
 *	our load.
 */
CellData * CellApp::chooseCellToOffload() const
{
	// We may want a different system. We could choose randomly or choose the
	// least loaded, most loaded or smallest cell. We could also prefer cells
	// that own an entire space.
	return !cells_.empty() ? cells_.front() : NULL;
}


/**
 *	This method returns whether or not the Cell App Manager has heard from this
 *	from this Cell App in the timeout period.
 */
bool CellApp::hasTimedOut( uint64 currTime, uint64 timeoutPeriod ) const
{
	bool result = false;

	uint64 diff = currTime - this->channel().lastReceivedTime();
	result = (diff > timeoutPeriod);

	if (result)
	{
		PROC_IP_INFO_MSG( "CellApp::hasTimedOut: Timed out - %.2f (> %.2f) %s\n",
				double( (int64)diff )/stampsPerSecondD(),
				double( (int64)timeoutPeriod )/stampsPerSecondD(),
				this->addr().c_str() );
	}

	return result;
}


/**
 *	This method adds this CellApp to the input group and also any other CellApp
 *	that can be reached by traversing from this CellApp's spaces to all CellApps
 *	on those spaces and so on.
 */
void CellApp::markGroup( CellAppGroup * pGroup )
{
	pGroup->insert( this );

	// Flood fill all connected CellApps.
	// First, all Spaces of this CellApp.

	Cells::iterator fromAppIter = cells_.begin();

	while (fromAppIter != cells_.end())
	{
		Space * pSpace = (*fromAppIter)->getSpace();

		// Second, all CellApps covering those Spaces.
		Cells::const_iterator fromSpaceIter = pSpace->cells().begin();

		while (fromSpaceIter != pSpace->cells().end())
		{
			CellApp * pNewApp = (*fromSpaceIter)->pCellApp();

			if (pNewApp->pGroup() != pGroup)
			{
				MF_ASSERT( pNewApp->pGroup() == NULL );

				pNewApp->markGroup( pGroup );
			}

			++fromSpaceIter;
		}

		++fromAppIter;
	}
}


// -----------------------------------------------------------------------------
// Section: Watcher
// -----------------------------------------------------------------------------

/**
 *	This static method makes a watcher associated with this object type.
 */
WatcherPtr CellApp::pWatcher()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();
		CellApp * pNullCellApp = NULL;

		pWatcher->addChild( "id", makeWatcher( &CellApp::id_ ) );

		pWatcher->addChild( "channel", Mercury::ChannelOwner::pWatcher(),
				(ChannelOwner *)pNullCellApp );

		pWatcher->addChild( "viewer port", makeWatcher( &CellApp::viewerPort_ ) );
		pWatcher->addChild( "load", makeWatcher( &CellApp::currLoad_ ) );
		pWatcher->addChild( "smoothedLoad",
				makeWatcher( &CellApp::smoothedLoad_ ) );

		pWatcher->addChild( "numEntities",
			makeWatcher( &CellApp::numEntities_ ) );

		pWatcher->addChild( "numCells", makeWatcher( &CellApp::numCells ) );

		pWatcher->addChild( "numCellsRetiring",
				makeWatcher( &CellApp::numCellsRetiring_ ) );

		pWatcher->addChild( "isRetiring",
			makeNonRefWatcher(
				MF_ACCESSORS( bool, CellApp, isRetiring ) ) );

		pWatcher->addChild( "cells", Cells::pWatcher(), &pNullCellApp->cells_ );
	}

	return pWatcher;
}

BW_END_NAMESPACE

// cellapp.cpp
