#include "bsp_node.hpp"

#include "server/balance_config.hpp"


BW_BEGIN_NAMESPACE

namespace CM
{

/**
 *	Constructor.
 */
BSPNode::BSPNode( const BW::Rect & range ) :
	range_( range ),
	entityBoundLevels_( BalanceConfig::numCPUOffloadLevels() ),
	// Set chunk bounds so initially the cell cannot move.
	chunkBounds_( FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX ),
	numRetiring_( 0 ),
	areaNotLoaded_( 0.f )
{
}


/**
 *	This method creates the initial watcher that is used by InternalNode and
 *	CellData.
 */
WatcherPtr BSPNode::createWatcher()
{
	Watcher * pWatcher = new DirectoryWatcher;

	pWatcher->addChild( "numRetiring", makeWatcher( &BSPNode::numRetiring ) );
	pWatcher->addChild( "areaNotLoaded", makeWatcher( &BSPNode::areaNotLoaded ) );

	return pWatcher;
}

} // namespace CM

BW_END_NAMESPACE

// bsp_node.cpp
