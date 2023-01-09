#include "cell_data.hpp"

#ifndef CODE_INLINE
#include "cell_data.ipp"
#endif

#include "cellapp.hpp"
#include "cellappmgr.hpp"
#include "cellappmgr_config.hpp"
#include "internal_node.hpp"
#include "space.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "cstdmf/watcher.hpp"

#include "network/bundle.hpp"

#include <limits>


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class CellData
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CellData::CellData( CellApp & cellApp,
		Space & space ) :
	CM::BSPNode(
		BW::Rect(
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max() ) ),
	pCellApp_( &cellApp ),
	pSpace_( &space ),
	hasBeenCreated_( false ),
	isOverloaded_( false ),
	hasHadLoadedArea_( false ),
	recoveringAddr_( 0, 0 )
{
	space.addCell( this );
	cellApp.addCell( this );
}


/**
 *	This constructor is used when we are recovering the CellAppMgr.
 */
CellData::CellData( Space & space, BinaryIStream & data ) :
	CM::BSPNode(
		BW::Rect(
			-std::numeric_limits< float >::max(),
			-std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max(),
			std::numeric_limits< float >::max() ) ),
	pCellApp_( NULL ),
	pSpace_( &space ),
	hasBeenCreated_( true ),
	isOverloaded_( false ),
	hasHadLoadedArea_( false ),
	recoveringAddr_( 0, 0 )
{
	CellData::readRecoveryData( recoveringAddr_, data );
	space.addCell( this );

	// We keep the Cells that are waiting to be recovered in the space's
	// cellsToDelete_ collection. These are removed as we are told to
	// recover them.
	space.addCellToDelete( this );
}


/**
 *	Destructor.
 */
CellData::~CellData()
{
	if (pSpace_)
	{
		pSpace_->eraseCell( this );
	}

	if (pCellApp_)
	{
		pCellApp_->eraseCell( this );

		if (numRetiring_)
		{
			pCellApp_->decCellsRetiring();
		}
	}
}


/**
 *	This method updates the bounds associated with this cell based on
 *	information sent from the CellApp.
 *	inform us about where the entities are on the cells of this application.
 */
void CellData::updateBounds( BinaryIStream & data )
{
	this->updateEntityBounds( data );

	data >> chunkBounds_;

	this->space().updateBounds( data );

	data >> numEntities_;

	if (numEntities_ > 0)
	{
		this->space().hasHadEntities( true );
	}
}


/**
 *	This method reads in the entity bounds from a stream.
 */
void CellData::updateEntityBounds( BinaryIStream & data )
{
	entityBoundLevels_.updateFromStream( data );
}


/**
 *  This method enables or disables offloading of entities from this cell.
 */
void CellData::shouldOffload( bool enable )
{
	Mercury::Bundle & bundle = this->cellApp().bundle();
	bundle.startMessage( CellAppInterface::shouldOffload );
	bundle << this->space().id() << enable;
	this->cellApp().send();
}


/**
 *	This method sets the application that this cell is in. It is used during
 *	recovery.
 */
void CellData::setCellApp( CellApp & cellApp )
{
	MF_ASSERT( pCellApp_ == NULL );
	MF_ASSERT( cellApp.addr() == recoveringAddr_ );

	pCellApp_ = &cellApp;
	cellApp.addCell( this );
}


/**
 *	This method returns the address of the application that this cell is on.
 */
const Mercury::Address & CellData::addr() const
{
	return pCellApp_ ? pCellApp_->addr() : recoveringAddr_;
}


/**
 *	This method sends a retireCell message to the app.
 */
void CellData::sendRetireCell( bool isRetiring ) const
{
	INFO_MSG( "CellData::sendRetireCell: "
			"Retiring cell %u from space %u. isRetiring = %d.\n",
		pCellApp_ ? pCellApp_->id() : 0, pSpace_->id(), isRetiring );

	Mercury::Bundle & bundle = this->cellApp().bundle();
	bundle.startMessage( CellAppInterface::retireCell );
	bundle << pSpace_->id();
	bundle << isRetiring;

	// Don't send immediately so that messages are aggregated.
	this->cellApp().channel().delayedSend();
}


/**
 *	This method removes this Cell from the system. By the end of this method,
 *	this object has been deleted.
 */
void CellData::removeSelf()
{
	INFO_MSG( "CellData::removeSelf: Removed %u from space %u.\n",
			pCellApp_ ? pCellApp_->id() : 0, pSpace_->id() );

	pSpace_->eraseCell( this, true );
	pSpace_ = NULL;

	delete this;
}


/**
 *	This method starts the process of removing this cell.
 */
void CellData::startRetiring()
{
	if (!numRetiring_)
	{
		INFO_MSG( "CellData::startRetiring: cell %u from space %u\n",
				pCellApp_ ? pCellApp_->id() : 0,
				pSpace_->id() );

		if (pCellApp_)
		{
			pCellApp_->incCellsRetiring();
		}

		this->sendRetireCell( /*isRetiring:*/true );
		numRetiring_ = 1;
	}
}


/**
 *	This method cancels the process of retiring this cell.
 */
void CellData::cancelRetiring()
{
	if (numRetiring_)
	{
		INFO_MSG( "CellData::cancelRetiring: cell %u from space %u\n",
				pCellApp_ ? pCellApp_->id() : 0,
				pSpace_->id() );

		if (pCellApp_)
		{
			pCellApp_->decCellsRetiring();
		}

		this->sendRetireCell( /*isRetiring:*/false );

		numRetiring_ = 0;
	}
}


/**
 *	This static method reads a subtree from the input stream.
 */
CM::BSPNode * CellData::readTree( Space * pSpace, BinaryIStream & data )
{
	if (data.remainingLength() == 0)
	{
		return NULL;
	}

	bool isLeaf;
	data >> isLeaf;

	if (pSpace)
	{
		if (isLeaf)
		{
			return new CellData( *pSpace, data );
		}
		else
		{
			return new CM::InternalNode( *pSpace, data );
		}
	}
	else
	{
		if (isLeaf)
			CellData::skipDataInStream( data );
		else
			CM::InternalNode::skipDataInStream( data );

		return NULL;
	}
}


/**
 *	This method sets the members of this class (non-recursively) from the
 * 	recovery data in the stream.
 */
inline void CellData::skipDataInStream( BinaryIStream & data )
{
	Mercury::Address addr;
	CellData::readRecoveryData( addr, data );
}


/**
 *	This method extracts recovery data from the stream.
 */
inline void CellData::readRecoveryData( Mercury::Address& addr,
	BinaryIStream & data )
{
	data >> addr;
}


/**
 *	This is a static method that gets a generic watcher for classes of type
 *	CellData.
 */
WatcherPtr CellData::pWatcher()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = BSPNode::createWatcher();

		CellData	* pNull = NULL;

		pWatcher->addChild( "range", makeWatcher( &CellData::range_ ) );

		// pWatcher->addChild( "entityBounds",
		//	makeWatcher( &CellData::entityBounds_ ) );

		pWatcher->addChild( "chunkBounds",
			makeWatcher( &CellData::chunkBounds_ ) );

		pWatcher->addChild( "isRetiring", makeWatcher(
			MF_ACCESSORS_EX( bool, CellData, isRetiring, setIsRetiring ) ) );

		pWatcher->addChild( "app",
				new BaseDereferenceWatcher( CellApp::pWatcher() ),
				&pNull->pCellApp_ );

		pWatcher->addChild( "space",
				new BaseDereferenceWatcher( Space::pWatcher() ),
				&pNull->pSpace_ );

		pWatcher->addChild( "isLeaf", makeWatcher(
				MF_ACCESSORS_EX( bool, CellData, isLeaf, setIsLeaf ) ) );

	}

	return pWatcher;
}


// -----------------------------------------------------------------------------
// Section: BSPNode overrides
// -----------------------------------------------------------------------------

/**
 *	This method adds a cell that will be paired with this cell.
 */
CM::BSPNode * CellData::addCell( CellData * pCell, bool isHorizontal )
{
	const float partitionPt = range_.range1D( isHorizontal ).max_;
	BW::Rect newRange = range_;
	newRange.range1D( isHorizontal ).min_ = partitionPt;
	newRange.range1D( isHorizontal ).max_ = partitionPt;
	pCell->setRange( newRange );

	// TODO: At the moment, the new cell is always added to the right or top. It
	// may be better to choose the side based on which side is unbounded. A
	// simple test might be to check if fabs( min_ ) < fabs( max_ ) of
	// range_.range1D( isHorizontal ).

	return new CM::InternalNode( this, pCell,
			isHorizontal, range_, partitionPt );
}


/**
 *	This method is used to remove a cell from the tree.
 */
CM::BSPNode * CellData::removeCell( CellData * pCell )
{
	return this;
}


/**
 *
 */
void CellData::visit( CM::CellVisitor & visitor ) const
{
	visitor.visitCell( *this );
}


/**
 *	This method returns the area of unloaded chunks.
 */
inline
float CellData::calculateAreaNotLoaded() const
{
	const bool shouldDebugIfNonZero =
		hasHadLoadedArea_ && isZero( areaNotLoaded_ );

	return this->calculateAreaNotLoaded( range_, shouldDebugIfNonZero );
}


/**
 *	This method calculates the required area that would not be loaded if the
 *	node's range where changed. This is used when "load balancing" based on the
 *	number of unloaded chunks.
 *
 *	@param changeY	If true, a horizontal boundary is changed.
 *	@param changeMax	If true, the maximum boundary is changed.
 *	@param moveLeft	If true, the boundary is increased, otherwise decreased.
 */
float CellData::calculateNewAreaNotLoaded( bool changeY,
		bool changeMax, bool moveLeft ) const
{
	BW::Rect range( range_ );

	range.range1D( changeY )[ changeMax ] +=
		moveLeft ? -pSpace_->spaceGrid() : pSpace_->spaceGrid();

	return this->calculateAreaNotLoaded( range );
}


/**
 *	This helper method is used by calculateNewAreaNotLoaded.
 */
float CellData::calculateAreaNotLoaded( const BW::Rect & range,
		bool shouldDebugIfNonZero ) const
{
	if ((range.xRange().length() <= 0.f) ||
		(range.yRange().length() <= 0.f))
	{
		return 0.f;
	}

	const BW::Rect & spaceBounds = pSpace_->spaceBounds();

	// Need to load chunks within ghost distance of our boundaries as well
	BW::Rect desiredRect( range );
	const float FLOATING_POINT_TOLERANCE = 1.f; // Avoid floating point issues
	desiredRect.safeInflateBy(
			CellAppMgrConfig::ghostDistance() - FLOATING_POINT_TOLERANCE );

	BW::Rect clippedDesiredRect;
	clippedDesiredRect.setToIntersection( spaceBounds, desiredRect );

	BW::Rect clippedChunkRect;
	clippedChunkRect.setToIntersection( chunkBounds_, clippedDesiredRect );

	float innerArea = clippedChunkRect.area();
	float outerArea = clippedDesiredRect.area();

	float areaNotLoaded = outerArea - innerArea;

	if (shouldDebugIfNonZero && (areaNotLoaded > 0.f))
	{
		// This is here to debug the case where this cell reverts from having
		// its area loaded to not (which should not really happen).
		WARNING_MSG( "CellData::calculateAreaNotLoaded: Cell %d in Space %d reverted to not having chunks loaded.\n"
				"\tareaNotLoaded = %f\n"
				"\tspace.hasLoadedRequiredChunks = %d\n"
				"\trange: (%f, %f) -> (%f, %f)\n"
				"\tdesiredRect: (%f, %f) -> (%f, %f)\n"
				"\tspaceBounds: (%f, %f) -> (%f, %f)\n"
				"\tclippedDesiredRect: (%f, %f) -> (%f, %f)\n"
				"\tchunkBounds_: (%f, %f) -> (%f, %f)\n"
				"\tclippedChunkRect: (%f, %f) -> (%f, %f)\n"
				"\touterArea: %f\n"
				"\tinnerArea: %f\n",
			this->cellApp().id(),
			pSpace_->id(),
			areaNotLoaded,
			pSpace_->hasLoadedRequiredChunks(),
			range.xMin(), range.yMin(), range.xMax(), range.yMax(),
			desiredRect.xMin(), desiredRect.yMin(), desiredRect.xMax(), desiredRect.yMax(),
			spaceBounds.xMin(), spaceBounds.yMin(), spaceBounds.xMax(), spaceBounds.yMax(),
			clippedDesiredRect.xMin(), clippedDesiredRect.yMin(), clippedDesiredRect.xMax(), clippedDesiredRect.yMax(),
			chunkBounds_.xMin(), chunkBounds_.yMin(), chunkBounds_.xMax(), chunkBounds_.yMax(),
			clippedChunkRect.xMin(), clippedChunkRect.yMin(), clippedChunkRect.xMax(), clippedChunkRect.yMax(),
			outerArea,
			innerArea );

		// pSpace_->debugPrint();
	}

	return areaNotLoaded;
}


/**
 *	This method handles the isRetiring watcher being changed.
 */
void CellData::setIsRetiring( bool value )
{
	if (value)
	{
		this->startRetiring();
	}
	else
	{
		this->cancelRetiring();
	}
}


/**
 *	This method handles the isLeaf watcher being changed, and splits the cell
 *	if no longer a leaf.
 */
void CellData::setIsLeaf( bool isLeaf )
{
	if (!isLeaf)
	{
		pSpace_->addCellTo( this );
	}
}


/**
 *	This method splits this cell, creating a new cell with this one as the
 *	new internal node.
 */
CM::BSPNode * CellData::addCellTo( CellData * pNewCell, CellData * pCellToSplit,
	   bool isHorizontal )
{
	if (pCellToSplit != this)
	{
		return NULL;
	}

	return this->addCell( pNewCell, isHorizontal );
}


/*
 *	Override from BSPNode.
 */
float CellData::updateLoad()
{
	// We also calculate the area not loaded
	areaNotLoaded_ = this->calculateAreaNotLoaded();

	if (!hasHadLoadedArea_)
	{
		const Rect & spaceBounds = this->space().spaceBounds();

		hasHadLoadedArea_ =
			(spaceBounds.area() > 0.f) && spaceBounds.intersects( range_ );
	}

	return pCellApp_ ? pCellApp_->currLoad() : 0.f;
}


/*
 *	Override from BSPNode.
 */
float CellData::avgLoad() const
{
	return this->cellApp().currLoad();
}


/*
 *	Override from BSPNode.
 */
float CellData::avgSmoothedLoad() const
{
	return this->cellApp().smoothedLoad();
}


/*
 *	Override from BSPNode.
 */
float CellData::totalSmoothedLoad() const
{
	return this->cellApp().smoothedLoad();
}


/*
 *	Override from BSPNode.
 */
float CellData::minLoad() const
{
	// Should this be smoothed or not?
	return this->cellApp().smoothedLoad();
}


/*
 *	Override from BSPNode.
 */
float CellData::maxLoad() const
{
	// Should this be smoothed or not?
	return this->cellApp().smoothedLoad();
}


/*
 *	Returns last received load on the associated CellApp
 */
float CellData::lastReceivedLoad() const
{
	/* TODO decide whether we want the check of pCellApp_ against NULL
	 * to be in a form of IF_NOT_MF_ASSERT_DEV( pCellApp_ ) */
	// Should this be smoothed or not?
	return pCellApp_ ? pCellApp_->lastReceivedLoad() : 0.f;
}


/*
 *	Override from BSPNode.
 */
void CellData::balance( const BW::Rect & range,
		float loadSafetyBound, bool isShrinking )
{
	// Should not shrink if not yet created.
	MF_ASSERT( hasBeenCreated_ || !isShrinking );

	range_ = range;
	isOverloaded_ = (this->cellApp().smoothedLoad() > loadSafetyBound);

	bool hasArea = (range.xMin() < range.xMax()) &&
			(range.yMin() < range.yMax());

	// When a cell has no area, it should be removed if load balancing is trying
	// to make it smaller.
	if (!hasArea && (isShrinking || numRetiring_) && (this->numEntities() == 0))
	{
		Space::addCellToDelete( this );
	}
}


/*
 *	Override from BSPNode.
 */
void CellData::updateRanges( const BW::Rect & range )
{
	range_ = range;
}


/*
 *	Override from BSPNode.
 */
void CellData::addToStream( BinaryOStream & stream, bool isForViewer )
{
	float smoothedLoad =
		pCellApp_ ? pCellApp_->smoothedLoad() : 0.f;
	stream << uint8( CM::BSP_NODE_LEAF ) << this->addr() << smoothedLoad;

	if (!isForViewer)
	{
		// Preserve backwards compatibility for SpaceViewer.
		stream << uint8( this->hasBeenCreated() );
	}
	else
	{
		if (pCellApp_)
		{
			stream << this->cellApp().id();
			stream << this->cellApp().viewerPort();
		}
		else
		{
			stream << CellAppID( 0 ) << uint16( 0 );
		}
		entityBoundLevels_.addToStream( stream );
		stream << chunkBounds_;
		stream << (int8)this->isRetiring();
		stream << isOverloaded_;
	}
}


/**
 *	This method returns the chunk bounds that should be considered when load
 *	balancing.
 */
const BW::Rect & CellData::balanceChunkBounds() const
{
	const float FLOATING_POINT_TOLERANCE = 1.f; // Avoid floating point issues
	BW::Rect desiredRect( range_ );
	desiredRect.safeInflateBy(
			CellAppMgrConfig::ghostDistance() - FLOATING_POINT_TOLERANCE );

	return chunkBounds_.contains( desiredRect ) ?
		chunkBounds_ : range_;
}


/**
 *	This method is used to dump debug information about the BSP tree.
 */
void CellData::debugPrint( int indent )
{
	const Rect & rect = this->balanceChunkBounds();

	DEBUG_MSG( "%*sCell: %d. (%f,%f)->(%f,%f). (%f,%f)->(%f,%f). (%f,%f)->(%f,%f)\n",
			2 * indent + 1, "+",
			this->cellApp().id(),
			chunkBounds_.xMin(), chunkBounds_.yMin(),
			chunkBounds_.xMax(), chunkBounds_.yMax(),
			range_.xMin(), range_.yMin(),
			range_.xMax(), range_.yMax(),
			rect.xMin(), rect.yMin(),
			rect.xMax(), rect.yMax() );
}


/*
 *	This method overrides WatcherProvider to support polymorphic watchers.
 */
WatcherPtr CellData::getWatcher()
{
	return CellData::pWatcher();
}

BW_END_NAMESPACE

// cell_data.cpp
