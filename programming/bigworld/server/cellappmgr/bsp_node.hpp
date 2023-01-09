#ifndef BSP_NODE_HPP
#define BSP_NODE_HPP

#include "entity_bound_levels.hpp"

#include "math/math_extra.hpp"

#include "cstdmf/polymorphic_watcher.hpp"
#include "cstdmf/watcher.hpp"

#include <float.h>


BW_BEGIN_NAMESPACE

class CellData;
class Space;

namespace CM
{
class InternalNode;

/**
 *	This interface is used as a base class of objects that will be used in the
 *	BSPNode::visit method.
 */
class CellVisitor
{
public:
	virtual ~CellVisitor() {};
	virtual void visitPartition( const InternalNode & node ) {};
	virtual void visitCell( const CellData & cell ) {};
};

enum BSPNodeType
{
	BSP_NODE_HORIZONTAL,
	BSP_NODE_VERTICAL,
	BSP_NODE_LEAF
};


/**
 *	This class is the base class for nodes in the BSP tree. The leaves of the
 *	tree are the cells of a space. The internal nodes are the partitioning lines
 *	between the cells.
 */
class BSPNode : public WatcherProvider
{
public:
	BSPNode( const BW::Rect & range );
	virtual ~BSPNode() {};

	virtual BSPNode * addCell( CellData * pCell, bool isHorizontal = true ) = 0;
	virtual BSPNode * addCellTo( CellData * pNewCell, CellData * pCellToSplit,
		   bool isHorizontal = true ) = 0;
	virtual CM::BSPNode * removeCell( CellData * pCell ) = 0;

	virtual void visit( CellVisitor & visitor ) const = 0;
	virtual CellData * pCellAt( float x, float y ) const = 0;

	virtual bool hasBeenCreated() const		{ return true; }

	virtual float updateLoad() = 0;
	virtual float avgLoad() const = 0;
	virtual float avgSmoothedLoad() const = 0;
	virtual float totalSmoothedLoad() const = 0;

	virtual float minLoad() const = 0;
	virtual float maxLoad() const = 0;

	virtual int cellCount() const = 0;

	virtual void balance( const BW::Rect & range,
			float loadSafetyBound,
			bool isShrinking = false ) = 0;
	virtual void updateRanges( const BW::Rect & range ) = 0;

	virtual void addToStream( BinaryOStream & stream, bool isForViewer ) = 0;

	virtual bool isRetiring() const		{ return false; }
	int numRetiring() const				{ return numRetiring_; }

	// Methods used for setting the root watcher position.
	virtual float position() const						{ return 0.f; }
	virtual void position( float pos )					{};

	const BW::Rect & range() const				{ return range_; }
	void setRange( const BW::Rect & range )		{ range_ = range; }

	const EntityBoundLevels & entityBoundLevels() const { return entityBoundLevels_; }

	virtual const BW::Rect & balanceChunkBounds() const	{ return chunkBounds_; }

	bool hasLoadedRequiredChunks() const	{ return areaNotLoaded_ <= 0.f; }
	float areaNotLoaded() const				{ return areaNotLoaded_; }

	virtual void debugPrint( int indent ) = 0;

	virtual Space * getSpace() const = 0;

	virtual float calculateNewAreaNotLoaded(
			bool changeY, bool changeMax, bool moveLeft ) const = 0;

	static WatcherPtr createWatcher();

protected:
	BW::Rect range_;
	EntityBoundLevels entityBoundLevels_;
	BW::Rect chunkBounds_;

	int numRetiring_;

	float areaNotLoaded_;
};

} // namespace CM

BW_END_NAMESPACE

#endif // BSP_NODE_HPP
