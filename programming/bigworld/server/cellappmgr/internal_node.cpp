#include "internal_node.hpp"
#include "cellappmgr_config.hpp"
#include "cell_data.hpp"
#include "space.hpp"

#include "math/mathdef.hpp"

#include "server/balance_config.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: InternalNode
// -----------------------------------------------------------------------------

namespace CM
{

/**
 *	Constructor.
 */
InternalNode::InternalNode( BSPNode * pLeft, BSPNode * pRight,
		bool isHorizontal, const BW::Rect & range, float position ) :
	// Note: There are three constructors.
	BSPNode( range )
{
	this->init();
	pLeft_ = pLeft;
	pRight_ = pRight;
	isHorizontal_ = isHorizontal;
	position_ = position;
}


/**
 *	Constructor used while recovering cell data.
 */
InternalNode::InternalNode( Space & space, BinaryIStream & data ) :
	// Note: There are three constructors.
	BSPNode( BW::Rect( 0, 0, 0, 0 ) )
{
	this->init();
	this->readRecoveryData( data );
	pLeft_ = CellData::readTree( &space, data );
	pRight_ = CellData::readTree( &space, data );
}


/**
 *	Private default constructor only used by skipDataInStream().
 */
InternalNode::InternalNode() :
	BSPNode( BW::Rect( 0, 0, 0, 0 ) )
{
	this->init();
}


/**
 *	This is used by all constructors to initialise the data.
 */
void InternalNode::init()
{
	pLeft_ = NULL;
	pRight_ = NULL;
	leftCount_ = 1;
	rightCount_ = 1;
	isHorizontal_ = false;
	position_ = 0.f;

	balanceAggression_ = 1.f;

	prevBalanceDir_ = BALANCE_NONE;

	totalLoad_ = 0.f;
	totalSmoothedLoad_ = 0.f;
	minLoad_ = 0.f;
	maxLoad_ = 0.f;
}


/**
 *	Destructor.
 */
InternalNode::~InternalNode()
{
	delete pLeft_;
	delete pRight_;
}


/**
 *	This method adds a new cell to this subtree.
 *
 *	@return The new subtree.
 */
BSPNode * InternalNode::addCell( CellData * pCell, bool isHorizontal )
{
	bool addToLeft = false;

	if (leftCount_ < rightCount_)
	{
		addToLeft = true;
	}
	else if (leftCount_ == rightCount_)
	{
		float value = range_.range1D( isHorizontal_ ).midPoint();

		addToLeft = (position_ < value);
	}

	if (addToLeft && !pLeft_->isRetiring())
	{
		pLeft_ = pLeft_->addCell( pCell, !isHorizontal_ );
	}
	else
	{
		pRight_ = pRight_->addCell( pCell, !isHorizontal_ );
	}

	return this;
}


/**
 *	This method is a helper that attempts to add a new cell to a subtree if the
 *	subtree contains the indicated cell.
 *
 *	@param rpChild The child subtree to attempt adding. If successful, the
 *		child tree pointer is updated to the new subtree.
 *	@param pNewCell The new cell to be added.
 *	@param pCellToSplit The cell that should be split.
 *
 *	@return True if the new cell was added to the child subtree otherwise false.
 */
bool InternalNode::addCellToChildTree( BSPNode *& rpChild, CellData * pNewCell,
		CellData * pCellToSplit )
{
	BSPNode * pNewSubtree =
		rpChild->addCellTo( pNewCell, pCellToSplit, !isHorizontal_ );

	if (!pNewSubtree)
	{
		return false;
	}

	rpChild = pNewSubtree;

	return true;
}


/**
 *	This method adds a cell to a subtree by splitting the given cell.
 *
 *	@return The new subtree if pCellToSplit is in this subtree, otherwise NULL.
 */
BSPNode * InternalNode::addCellTo( CellData * pNewCell, CellData * pCellToSplit,
		bool isHorizontal )
{
	if (this->addCellToChildTree( pLeft_, pNewCell, pCellToSplit ) ||
		this->addCellToChildTree( pRight_, pNewCell, pCellToSplit ))
	{
		return this;
	}

	return NULL;
}


/**
 *	This method is used to remove a cell from the tree.
 */
CM::BSPNode * InternalNode::removeCell( CellData * pCell )
{
	CM::BSPNode * pResult = this;

	if (pLeft_ == pCell)
	{
		pResult = pRight_;
		pLeft_ = pRight_ = NULL;
		delete this;
	}
	else if (pRight_ == pCell)
	{
		pResult = pLeft_;
		pLeft_ = pRight_ = NULL;
		delete this;
	}
	else
	{
		// Note: This may promote an internal node so that we have two
		// consecutive partitions that are in the same direction.
		pLeft_ = pLeft_->removeCell( pCell );
		pRight_ = pRight_->removeCell( pCell );
	}

	return pResult;
}


/**
 *	This method is used to implement the Visitor pattern.
 */
void InternalNode::visit( CellVisitor & visitor ) const
{
	visitor.visitPartition( *this );

	pLeft_->visit( visitor );
	pRight_->visit( visitor );
}


/**
 *	This method returns the cell at the input point.
 */
CellData * InternalNode::pCellAt( float x, float y ) const
{
	bool isLeft = isHorizontal_ ? (y < position_) : (x < position_);

	return isLeft ? pLeft_->pCellAt( x, y ) : pRight_->pCellAt( x, y );
}


/**
 *	This method returns the average load of cells that are children of this
 *	node.
 */
float InternalNode::avgLoad() const
{
	return float( totalLoad_ )/std::max( this->nonRetiringCellCount(), 1 );
}


/**
 *	This method returns the average smoothed load of cells that are children
 *	of this node.
 */
float InternalNode::avgSmoothedLoad() const
{
	return float( totalSmoothedLoad_ )/std::max( this->nonRetiringCellCount(), 1 );
}


/**
 *	This method returns the total smoothed load of cells that are children
 *	of this node.
 */
float InternalNode::totalSmoothedLoad() const
{
	return totalSmoothedLoad_;
}


/**
 *	This method updates the load associated with nodes in this sub-tree.
 */
float InternalNode::updateLoad()
{
	totalLoad_ = pLeft_->updateLoad() + pRight_->updateLoad();

	totalSmoothedLoad_ =
		pLeft_->totalSmoothedLoad() + pRight_->totalSmoothedLoad();

	numRetiring_ = pLeft_->numRetiring() + pRight_->numRetiring();

	areaNotLoaded_ = pLeft_->areaNotLoaded() + pRight_->areaNotLoaded();

	minLoad_ = std::min( pLeft_->minLoad(), pRight_->minLoad() );
	maxLoad_ = std::max( pLeft_->maxLoad(), pRight_->maxLoad() );


	entityBoundLevels_.merge( pLeft_->entityBoundLevels(),
							pRight_->entityBoundLevels(),
							isHorizontal_, position_ );

	const BW::Rect & lBounds = pLeft_->balanceChunkBounds();
	const BW::Rect & rBounds = pRight_->balanceChunkBounds();

	if (isHorizontal_)
	{
		chunkBounds_.yMin_ = lBounds.yMin_;
		chunkBounds_.yMax_ = rBounds.yMax_;

		chunkBounds_.xMin_ = std::max( lBounds.xMin_, rBounds.xMin_ );
		chunkBounds_.xMax_ = std::min( lBounds.xMax_, rBounds.xMax_ );
	}
	else
	{
		chunkBounds_.xMin_ = lBounds.xMin_;
		chunkBounds_.xMax_ = rBounds.xMax_;

		chunkBounds_.yMin_ = std::max( lBounds.yMin_, rBounds.yMin_ );
		chunkBounds_.yMax_ = std::min( lBounds.yMax_, rBounds.yMax_ );
	}

	leftCount_ = pLeft_->cellCount();
	rightCount_ = pRight_->cellCount();

	return totalLoad_;
}


/**
 *	This method is used to balance the load between the cells.
 */
void InternalNode::balance( const BW::Rect & range,
	   float loadSafetyBound, bool isShrinking )
{
	range_ = range;

	BalanceDirection balanceDir = this->doBalance( loadSafetyBound );

	this->balanceChildren( loadSafetyBound, balanceDir );
}


/**
 *	This method updates this and the child ranges.
 */
void InternalNode::updateRanges( const BW::Rect & range )
{
	range_ = range;

	// Update child ranges
	BW::Rect leftRange;
	BW::Rect rightRange;

	this->calculateChildRanges( leftRange, rightRange );

	pLeft_->updateRanges( leftRange );
	pRight_->updateRanges( rightRange );
}


/**
 *	This method calculates the range of the child nodes.
 */
void InternalNode::calculateChildRanges( BW::Rect & leftRange,
		BW::Rect & rightRange )
{
	leftRange = range_;
	rightRange = range_;

	leftRange.range1D( isHorizontal_ ).max_ =
		std::max( position_, range_.range1D( isHorizontal_ ).min_ );
	rightRange.range1D( isHorizontal_ ).min_ =
		std::min( position_, range_.range1D( isHorizontal_ ).max_ );
}


/**
 *	This method does that actual work of balancing.
 */
InternalNode::BalanceDirection InternalNode::doBalance( float loadSafetyBound )
{
	bool shouldLimitToChunks =
		CellAppMgrConfig::shouldLimitBalanceToChunks();
	const bool isRetiring = pLeft_->isRetiring() || pRight_->isRetiring();

	float loadDiff = 0.f;

	// Do not move if we do not know if the CellApp has created the Cell.
	// This avoids problems with other Cells offloading before the new Cell
	// exists.
	const bool childrenCreated = pLeft_->hasBeenCreated() &&
		pRight_->hasBeenCreated();

	// Check whether we should balance based on unloaded chunks

	if (!isRetiring)
	{
		if (!this->hasLoadedRequiredChunks())
		{
			shouldLimitToChunks = false;

			if ((this->maxLoad() < loadSafetyBound) &&
				CellAppMgrConfig::shouldBalanceUnloadedChunks())
			{
				return childrenCreated ?
					this->balanceOnUnloadedChunks( loadSafetyBound ) :
					BALANCE_NONE;
			}
		}

		// Difference from average.
		const float leftAvgLoad = pLeft_->avgLoad();
		const float rightAvgLoad = pRight_->avgLoad();
		const float nodeAvgLoad = this->avgLoad();

		if (leftAvgLoad > rightAvgLoad)
		{
			loadDiff = fabs( leftAvgLoad - nodeAvgLoad );
		}
		else
		{
			loadDiff = -fabs( rightAvgLoad - nodeAvgLoad );
		}
	}
	else
	{
		loadDiff = pLeft_->isRetiring() ? 1.f : -1.f;
	}

	BalanceDirection balanceDir = this->dirFromLoadDiff( loadDiff );

	BSPNode * pFromNode = this->growingChild( balanceDir );

	const bool shouldMove = (balanceDir != BALANCE_NONE) &&
		childrenCreated &&
		(pFromNode->maxLoad() < loadSafetyBound);

	float newPos = position_;

	if (shouldMove)
	{
		float entityLimit = this->entityLimitInDirection( balanceDir,
				loadDiff * balanceAggression_ );
		float chunkLimit = shouldLimitToChunks ?
			this->chunkLimitInDirection( balanceDir ) : entityLimit;

		newPos = this->closestLimit( position_,
					entityLimit, chunkLimit, balanceDir );
	}
	else
	{
		balanceDir = BALANCE_NONE;
	}

	position_ = newPos;

	this->adjustAggression( balanceDir );

	return balanceDir;
}


/**
 *	This method adjusts the balance aggression. If the balance direction
 *	changes, the balance aggression is reduced.
 */
void InternalNode::adjustAggression( InternalNode::BalanceDirection balanceDir )
{
	const int hasReversedDirection =
		((balanceDir == BALANCE_LEFT) && (prevBalanceDir_ == BALANCE_RIGHT)) ||
		((balanceDir == BALANCE_RIGHT) && (prevBalanceDir_ == BALANCE_LEFT));

	if (hasReversedDirection)
	{
		// Changed direction. Descrease aggression
		balanceAggression_ *= BalanceConfig::aggressionDecrease();
	}
	else
	{
		// If heading in the same direction or still, increase aggression.
		balanceAggression_ *= BalanceConfig::aggressionIncrease();
	}

	balanceAggression_ = BalanceConfig::clampedAggression( balanceAggression_ );
	prevBalanceDir_ = balanceDir;
}


/**
 *	This method balances the children.
 */
void InternalNode::balanceChildren( float loadSafetyBound,
		InternalNode::BalanceDirection balanceDir )
{
	// Update child ranges, and trigger load balancing for children.
	BW::Rect leftRange;
	BW::Rect rightRange;

	this->calculateChildRanges( leftRange, rightRange );

	pLeft_->balance( leftRange, loadSafetyBound,
			/* isShrinking */ balanceDir == BALANCE_LEFT );
	pRight_->balance( rightRange, loadSafetyBound,
			/* isShrinking */ balanceDir == BALANCE_RIGHT );
}


/**
 *	This method returns the direction to balance based on unloaded chunks.
 */
InternalNode::BalanceDirection
	InternalNode::balanceOnUnloadedChunks( float loadSafetyBound )
{
	BalanceDirection balanceDir = this->snapToSpaceBounds();

	if (balanceDir != BALANCE_NONE)
	{
		return balanceDir;
	}

	int leftCount = pLeft_->cellCount();
	int rightCount = pRight_->cellCount();

	float leftUnloaded = pLeft_->areaNotLoaded() / leftCount;
	float rightUnloaded = pRight_->areaNotLoaded() / rightCount;

	balanceDir = this->dirFromLoadDiff( leftUnloaded - rightUnloaded );

	const bool shouldMoveLeft = (balanceDir == BALANCE_LEFT);

	// NOTE: This calculation is currently incorrect when there are many
	// children. The boundary of all children is expanded, not just the edge
	// children.
	float newLeftUnloaded =
		pLeft_->calculateNewAreaNotLoaded( /* changeY: */isHorizontal_,
				/* changeMin:*/ true,
				/* moveLeft: */ shouldMoveLeft ) / leftCount;
	float newRightUnloaded =
		pRight_->calculateNewAreaNotLoaded( /* changeY: */isHorizontal_,
				/* changeMin:*/ false,
				/* moveLeft: */ shouldMoveLeft ) / rightCount;

	// Things don't get better if we move this way

	if (std::max( newLeftUnloaded, newRightUnloaded ) >=
			std::max( leftUnloaded, rightUnloaded ) )
	{
		return BALANCE_NONE;
	}

	// If it could make the CPU balance too bad. Stop balancing.
	const float MAX_CPU_OFFLOAD = BalanceConfig::maxCPUOffload();
	const float lowerSafetyBound = loadSafetyBound - MAX_CPU_OFFLOAD;

	BSPNode * pFromNode = this->growingChild( balanceDir );

	if (pFromNode->maxLoad() > lowerSafetyBound)
	{
		return BALANCE_NONE;
	}

	float entityLimit = this->entityLimitInDirection( balanceDir );
	float gridSize = this->getSpace()->spaceGrid();
	float chunkLimit =
		(pFromNode == pLeft_) ?
			position_ + gridSize :
			position_ - gridSize;

	position_ = this->closestLimit( position_,
			entityLimit, chunkLimit, balanceDir );

	return balanceDir;
}


/**
 *	This method returns a balance direction given left minus right loads.
 */
InternalNode::BalanceDirection InternalNode::dirFromLoadDiff( float loadDiff )
{
	return
		(loadDiff > 0.f) ? BALANCE_LEFT :
		(loadDiff < 0.f) ? BALANCE_RIGHT : BALANCE_NONE;
}


/**
 *	This method returns the limit of movement based on the entity bounds in the
 *	direction of movement.
 */
float InternalNode::entityLimitInDirection( BalanceDirection direction,
	   float loadDiff ) const
{
	// If less than the minimum level, don't move.
	if (fabs( loadDiff ) < BalanceConfig::minCPUOffload())
	{
		return position_;
	}

	BSPNode * pToNode = this->shrinkingChild( direction );

	// This also works for BALANCE_NONE case.
	bool shouldGetMax = (pToNode == pLeft_);

	return pToNode->entityBoundLevels().entityBoundForLoadDiff(
										loadDiff,
										isHorizontal_,
										shouldGetMax,
										position_ );
}


/**
 *	This method returns the limit of movement based on the entity bounds in the
 *	direction of movement.
 */
float InternalNode::chunkLimitInDirection( BalanceDirection direction ) const
{
	BSPNode * pFromNode = this->growingChild( direction );

	// This also works for BALANCE_NONE case.
	bool shouldGetMax = (pFromNode == pLeft_);

	float ghostDistance = CellAppMgrConfig::ghostDistance();

	if (shouldGetMax)
	{
		ghostDistance = -ghostDistance;
	}

	const Rect & chunkBounds = pFromNode->balanceChunkBounds();

	return chunkBounds.range1D( isHorizontal_ )[ shouldGetMax ] + ghostDistance;
}


/**
 *	This method snaps the partition point to the nearest space boundary.
 *
 *	@return The direction of movement. BALANCE_NONE if it is currently within
 *		the boundary.
 */
InternalNode::BalanceDirection InternalNode::snapToSpaceBounds()
{
	// Snap the position to the space bounds if we are currently outside it.
	const BW::Rect & spaceBounds = this->getSpace()->spaceBounds();

	float leftSpaceBound = spaceBounds.range1D( isHorizontal_ ).min_;

	if (position_ < leftSpaceBound)
	{
		position_ = leftSpaceBound;
		return BALANCE_RIGHT;
	}

	float rightSpaceBound = spaceBounds.range1D( isHorizontal_ ).max_;

	if (position_ > rightSpaceBound)
	{
		position_ = rightSpaceBound;
		return BALANCE_LEFT;
	}

	return BALANCE_NONE;
}


/**
 *	This method adds this subtree to the input stream.
 */
void InternalNode::addToStream( BinaryOStream & stream, bool isForViewer )
{
	stream << uint8( isHorizontal_ ? BSP_NODE_HORIZONTAL : BSP_NODE_VERTICAL );
	stream << position_;
	if (isForViewer)
	{
		stream << this->avgLoad() << balanceAggression_;
	}
	pLeft_->addToStream( stream, isForViewer );
	pRight_->addToStream( stream, isForViewer );
}


/**
 *	This static method skips over the data for an InternalNode in the stream.
 */
void InternalNode::skipDataInStream( BinaryIStream & data )
{
	{
		InternalNode dummy;
		dummy.readRecoveryData( data );
	}
	CellData::readTree( NULL, data );	// left
	CellData::readTree( NULL, data );	// right
}


#if 0
void InternalNode::setIsLeaf( bool isLeaf )
{
	// TODO
}
#endif


/**
 *	This method sets the members of this class (non-recursively) from streamed
 * 	recovery data.
 */
inline void InternalNode::readRecoveryData( BinaryIStream & data )
{
	data >> position_ >> isHorizontal_;
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
float InternalNode::calculateNewAreaNotLoaded(
		bool changeY, bool changeMax, bool moveLeft ) const
{
	// TODO: Fix this
	// NOTE: This calculation is currently incorrect when there are many
	// children. The boundary of all children is expanded, not just the edge
	// children.
	return pLeft_->calculateNewAreaNotLoaded( changeY, changeMax, moveLeft ) +
		pRight_->calculateNewAreaNotLoaded( changeY, changeMax, moveLeft );
}


/**
 *	This method is used to dump debug information about the BSP tree.
 */
void InternalNode::debugPrint( int indent )
{
	DEBUG_MSG( "%*sInternal(%p): %f (%d). (%f,%f)->(%f,%f). (%f,%f)->(%f,%f)\n",
			2 * indent + 1, "+",
			this,
			position_, isHorizontal_,
			chunkBounds_.xMin(), chunkBounds_.yMin(),
			chunkBounds_.xMax(), chunkBounds_.yMax(),
			range_.xMin(), range_.yMin(),
			range_.xMax(), range_.yMax() );

	pLeft_->debugPrint( indent + 1 );
	pRight_->debugPrint( indent + 1 );
}


/*
 *	This method overrides WatcherProvider to support polymorphic watchers.
 */
WatcherPtr InternalNode::getWatcher()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = BSPNode::createWatcher();

		InternalNode * pNull = NULL;

		pWatcher->addChild( "left",
				new PolymorphicWatcher(), &pNull->pLeft_ );
		pWatcher->addChild( "right",
				new PolymorphicWatcher(), &pNull->pRight_ );

		pWatcher->addChild( "position",
			makeWatcher( &InternalNode::position_, Watcher::WT_READ_WRITE ) );
		pWatcher->addChild( "isHorizontal",
				makeWatcher( &InternalNode::isHorizontal_ ) );
	}

	return pWatcher;
}

} // namespace CM

BW_END_NAMESPACE

// cell_data.cpp
