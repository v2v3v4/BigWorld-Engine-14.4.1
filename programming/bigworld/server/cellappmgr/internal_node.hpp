#ifndef INTERNAL_NODE_HPP
#define INTERNAL_NODE_HPP

#include "bsp_node.hpp"


BW_BEGIN_NAMESPACE

namespace CM
{

/**
 *	This class is used for internal nodes of the BSP tree. They represent a
 *	partitioning line between cells. The leaves are the cells of a space.
 */
class InternalNode : public BSPNode
{
public:
	InternalNode( BSPNode * pLeft, BSPNode * pRight,
			bool isHorizontal, const BW::Rect & range, float position );
	InternalNode( Space & space, BinaryIStream & data );
	~InternalNode();

	virtual BSPNode * addCell( CellData * pCell, bool isHorizontal );
	virtual BSPNode * addCellTo( CellData * pNewCell, CellData * pCellToSplit,
		   bool isHorizontal );
	virtual CM::BSPNode * removeCell( CellData * pCell );

	virtual void visit( CellVisitor & visitor ) const;
	virtual CellData * pCellAt( float x, float y ) const;

	virtual float updateLoad();
	virtual float avgLoad() const;
	virtual float avgSmoothedLoad() const;
	virtual float totalSmoothedLoad() const;

	virtual float minLoad() const { return minLoad_; }
	virtual float maxLoad() const { return maxLoad_; }

	virtual int cellCount() const { return leftCount_ + rightCount_; }

	virtual void balance( const BW::Rect & range,
			float loadSafetyBound, bool isShrinking );

	virtual void updateRanges( const BW::Rect & range );

	virtual void addToStream( BinaryOStream & stream, bool isForViewer );

	virtual void debugPrint( int indent );

	// Methods used for setting the root watcher position.
	virtual float position() const						{ return position_; }
	virtual void position( float pos )					{ position_ = pos; }

	virtual Space * getSpace() const	{ return pLeft_->getSpace(); }

	virtual float calculateNewAreaNotLoaded(
			bool changeY, bool changeMax, bool moveLeft ) const;

	virtual WatcherPtr getWatcher();

	static void skipDataInStream( BinaryIStream & data );

private:
	InternalNode();
	void init();
	void readRecoveryData( BinaryIStream & data );

	int nonRetiringCellCount() const
	{
		return leftCount_ + rightCount_ - numRetiring_;
	}

	enum BalanceDirection
	{
		BALANCE_LEFT,
		BALANCE_NONE,
		BALANCE_RIGHT
	};

	void adjustAggression( InternalNode::BalanceDirection balanceDir );
	BalanceDirection balanceOnUnloadedChunks( float loadSafetyBound );
	void balanceChildren( float loadSafetyBound,
			BalanceDirection balanceDir );
	BalanceDirection doBalance( float loadSafetyBound );
	BalanceDirection snapToSpaceBounds();

	static BalanceDirection dirFromLoadDiff( float loadDiff );

	BSPNode * shrinkingChild( BalanceDirection direction ) const
	{
		return (direction == BALANCE_LEFT) ? pLeft_ : pRight_;
	}

	BSPNode * growingChild( BalanceDirection direction ) const
	{
		return (direction == BALANCE_LEFT) ? pRight_ : pLeft_;
	}

	/**
	 *	This helper method returns the limit to move to based on the direction
	 *	of movment.
	 */
	static float closestLimit( float position, float limit1, float limit2,
			BalanceDirection direction )
	{
		return (direction == BALANCE_LEFT) ?
			std::min( position, std::max( limit1, limit2 ) ) :
			std::max( position, std::min( limit1, limit2 ) );
	}

	float entityLimitInDirection( BalanceDirection direction,
			float loadDiff = FLT_MAX ) const;
	float chunkLimitInDirection( BalanceDirection direction ) const;

	void calculateChildRanges( BW::Rect & leftRange, BW::Rect & rightRight );

	bool addCellToChildTree( BSPNode *& rpChild, CellData * pNewCell,
		CellData * pCellToSplit );

	BSPNode *		pLeft_;
	BSPNode *		pRight_;

	int			leftCount_;
	int			rightCount_;

	// Whether the partition line is horizontal (true) or vertical
	bool		isHorizontal_;
	float		position_;

	// This faction of the balance to balance in a single balance.
	float				balanceAggression_;

	BalanceDirection	prevBalanceDir_;

	float		totalLoad_;
	float		totalSmoothedLoad_;
	float		minLoad_;
	float		maxLoad_;
};

} // namespace CM

BW_END_NAMESPACE

#endif // INTERNAL_NODE_HPP
