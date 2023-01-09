// quad_tree.ipp

BW_END_NAMESPACE

#include <limits.h>
#include "cstdmf/dprintf.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: QuadTreeMemPool
// -----------------------------------------------------------------------------

template<typename MEM_TYPE>
void QuadTreeMemPool<MEM_TYPE>::init()
{
	capacity_ = 0;
	size_ = 0;
	data_ = NULL;
}

template<typename MEM_TYPE>
void QuadTreeMemPool<MEM_TYPE>::destroy()
{
	bw_free( data_ );
	capacity_ = 0;
	size_ = 0;
	data_ = NULL;
}

template<typename MEM_TYPE>
inline size_t QuadTreeMemPool<MEM_TYPE>::size() const
{
	return size_;
}

template<typename MEM_TYPE>
inline size_t QuadTreeMemPool<MEM_TYPE>::capacity() const
{
	return capacity_;
}

template<typename MEM_TYPE>
inline void QuadTreeMemPool<MEM_TYPE>::reserve( size_t num )
{
	MF_ASSERT( data_ ? capacity_ > 0 : capacity_ == 0 );

	if (num <= capacity_) return;

	data_ = (MEM_TYPE*)bw_realloc( data_, sizeof(MEM_TYPE)*num);

	capacity_ = num;
}

/**
 *  Create a new item in the pool and return a reference
 *  to it. Do not hold onto this reference as it may
 *  become invalid if the pool has to realloc on
 *  subsequent calls to createNew
 */
template<typename MEM_TYPE>
inline MEM_TYPE & QuadTreeMemPool<MEM_TYPE>::createNew()
{
	if (size_ == capacity_)
	{
		reserve( capacity_ + (capacity_ / 2) + 1 );			
	}

	MF_ASSERT( data_ );
	MEM_TYPE* newItem = data_ + size_++;
	return *newItem;
}

template<typename MEM_TYPE>
inline const MEM_TYPE & QuadTreeMemPool<MEM_TYPE>::operator[] ( size_t index ) const
{
	MF_ASSERT( data_ && (index < size_) );
	return *(data_ + index);
}

template<typename MEM_TYPE>
inline MEM_TYPE & QuadTreeMemPool<MEM_TYPE>::operator[] ( size_t index )
{
	MF_ASSERT( data_ && (index < size_) );
	return *(data_ + index);
}

// -----------------------------------------------------------------------------
// Section: QuadTree
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param x	The x-coordinate of the left of the quadtree range.
 *	@param z	The z-coordinate of the bottom of the quadtree range.
 *	@param depth The maximum depth of the tree.
 *	@param range The range that the QuadTree covers. It is assumed to be square.
 */
template <class MEMBER_TYPE>
inline QuadTree<MEMBER_TYPE>::QuadTree( float x, float z,
		int depth, float range ) :
	origin_( x, z ),
	depth_( depth ),
	range_( range )
{
	int maxNodes = 0;
	for (int i = 0, levelNodes = 1; i <= depth_; ++i)
	{
		maxNodes += levelNodes;
		levelNodes *= 4;
	}

	//If this is a problem, need to change the nodes to 
	//store more than just 16 bit integers
	MF_ASSERT( maxNodes < Node::INVALID_NODE-1 );

	//Start the node pool.
	nodes_.init();
	//Reserve space for a few nodes. This saves doing
	//some memory allocations when the quad tree starts
	//to grow
	const int numToReserve = 64;
	nodes_.reserve(numToReserve);
	//Add the root node
	createNewNode();
	MF_ASSERT( nodes_.size() == 1 );
}


/**
 *	Destructor.
 */
template <class MEMBER_TYPE>
inline QuadTree<MEMBER_TYPE>::~QuadTree()
{
	//Destroy all the nodes in the node pool
	for( size_t i = 0; i < nodes_.size(); ++i )
	{
		nodes_[i].destroy();
	}

	//destroy the node pool itself
	nodes_.destroy();
}


/**
 *	This method	adds the input element to the quadtree.
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::add( const MEMBER_TYPE & element )
{
	QTRange range = calculateQTRange( element, origin_,
		range_, depth_ );

	if (!range.inQuad( depth_ ))
	{
		// This warning is known to be triggered by the current
		// incomplete system for handling very large objects.
		dprintf("QuadTree::add: Skipping - Out of range (%d, %d) - (%d, %d)\n",
			range.left_, range.bottom_, range.right_, range.top_ );
		return;
	}

	range.clip( depth_ );

	this->addElement( &element, this->root(), range, depth_ );
}


/**
 *	This method removes the input element from this quadtree.
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::del( const MEMBER_TYPE & element )
{
	QTRange range = calculateQTRange( element, range_, depth_ );
	nodes_[this->root()].del( &element, range, depth_ );
}

/**
 *	This method adds the element to the root node only
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::addToRoot( const MEMBER_TYPE & element )
{
	this->addElement( this->root(), &element );
}

/**
 *	This method removes the element from the root node only
 */
template <class MEMBER_TYPE>
inline bool QuadTree<MEMBER_TYPE>::removeFromRoot( const MEMBER_TYPE & element )
{
	NodeRef rootRef = this->root();
	const size_t numElements = this->getNumElements( rootRef );
	for (size_t i = 0; i < numElements; ++i)
	{
		const MEMBER_TYPE * existing = this->getElement( rootRef, i);
		if (existing == &element)
		{			
			nodes_[rootRef].removeElement(i);
			return true;
		}
	}

	return false;
}

/**
 *	This method returns member that contains the input point.
 */
template <class MEMBER_TYPE>
inline const MEMBER_TYPE *
QuadTree<MEMBER_TYPE>::testPoint( const Vector3 & point ) const
{
	QTCoord rootCoord;
	rootCoord.x_ = rootCoord.y_ = 0;

	NodeRef rootRef = this->root();
	return nodes_[rootRef].testPoint( point, rootCoord, depth_ );
}

/**
 *	This method returns the root marker
 */
template <class MEMBER_TYPE>
inline typename QuadTree<MEMBER_TYPE>::NodeRef 
QuadTree<MEMBER_TYPE>::root() const 
{ 
	return 0; 
}

/**
 *	Create a new node, return the node reference of the new node.
 *  New nodes are created at the end of the node pool
 *  
 */
template <class MEMBER_TYPE>
inline typename QuadTree<MEMBER_TYPE>::NodeRef 
QuadTree<MEMBER_TYPE>::createNewNode()
{
	MF_ASSERT( nodes_.size() < Node::INVALID_NODE );
	NodeRef nodeRef = static_cast<NodeRef>(nodes_.size());
	Node& newNode = nodes_.createNew();
	newNode.init();
	return nodeRef;
}

/**
 *	This method returns a child and creates one at the
 *  given node and child index if the child does not exist
 */
template <class MEMBER_TYPE>
inline typename QuadTree<MEMBER_TYPE>::NodeRef 
QuadTree<MEMBER_TYPE>::getOrCreateChild( NodeRef nodeRef, Quad i )
{
	NodeRef child = this->getChildRef( nodeRef, i );
	if (child == Node::INVALID_NODE)
	{
		child = this->createNewNode();
		nodes_[nodeRef].setChild( child, i );
	}

	return child;
}

/**
 *	This method returns a child without trying to create one
 */
template <class MEMBER_TYPE>
typename QuadTree<MEMBER_TYPE>::NodeRef 
inline QuadTree<MEMBER_TYPE>::getChildRef( NodeRef nodeRef, Quad i ) const
{
	NodeRef child = nodes_[nodeRef].getChildRef( i );
	return child;
}

/**
 *	This method adds a new element to the given node
 */
template <class MEMBER_TYPE>
inline void 
QuadTree<MEMBER_TYPE>::addElement( NodeRef nodeRef, const MEMBER_TYPE * element )
{
	nodes_[nodeRef].addElement( element );
}

template <class MEMBER_TYPE>
inline size_t 
QuadTree<MEMBER_TYPE>::getNumElements( NodeRef nodeRef ) const
{
	return nodes_[nodeRef].getNumElements();
}

template <class MEMBER_TYPE>
inline const MEMBER_TYPE *
QuadTree<MEMBER_TYPE>::getElement( NodeRef nodeRef, size_t elementIndex ) const
{
	return nodes_[nodeRef].getElement( elementIndex );
}


/**
 *	For debugging. This method prints out this tree.
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::print() const
{
	dprintf( "sizeof(QT) = %" PRIzu "\n", sizeof( *this ) );
	dprintf( "sizeof(QTN) = %" PRIzu "\n",
		sizeof( QuadTreeNode<MEMBER_TYPE> ) );
	dprintf( "sizeof(elements) = %" PRIzu "\n", sizeof( nodes_ ) );
	dprintf( "Range = %f, Depth = %d\n", range_, depth_ );
	this->print( this->root(), 0 );

	const int width = 1 << depth_;
	QTCoord coord;

	for (coord.y_ = width - 1; coord.y_ >= 0; coord.y_--)
	{
		for (coord.x_ = 0; coord.x_ < width; coord.x_++)
		{
			dprintf( "%d", this->countAt( this->root(), coord, depth_ ) );
		}
		dprintf( "\n" );
	}
}


/**
 *	This method returns the number of bytes that this tree uses.
 */
template <class MEMBER_TYPE>
inline long QuadTree<MEMBER_TYPE>::size() const
{
	size_t size = sizeof( *this ) + this->size(this->root());
	MF_ASSERT( size <= LONG_MAX );
	return ( long ) size;
}

/**
 *	This method returns the number of nodes in the quadtree.
 *
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::countNodes() const
{
	return this->countNodes(this->root());
}

/**
 *	This method returns the number of elements in the quadtree.
 *
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::countElements() const
{
	return this->countElements(this->root());
}

/**
 *	Returns a list of the unique elements in the quadtree.
 *
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::getUniqueElements(
	BW::set<const MEMBER_TYPE*>& elements) const
{
	this->getUniqueElements(elements, this->root());
}

/**
 *	This method returns a traversal object that can be used to traverse the
 *	quadtree and visit the members they may have been collided.
 */
template <class MEMBER_TYPE>
inline QTTraversal<MEMBER_TYPE> QuadTree<MEMBER_TYPE>::traverse(
	const Vector3 & src, const Vector3 & dst,
	float radius ) const
{
	// TODO: This method could be generalised so the we don't have to always
	// take Vector3s. We could have a conversion method to get Vector2s.
	return QTTraversal<MEMBER_TYPE>( this, src, dst, radius, origin_ );
}


// -----------------------------------------------------------------------------
// Section: QTTraversal
// -----------------------------------------------------------------------------

/**
 *	Constructor. It creates a traversal that 'points' to the first element.
 */
template <class MEMBER_TYPE>
inline QTTraversal<MEMBER_TYPE>::QTTraversal(
		const Tree * pTree,
		const Vector3 & src, const Vector3 & dst, float radius,
		const Vector2 & origin ) :
	size_( 0 ),
	headIndex_( 0 )
{
	pTree_ = pTree;

	const int DEPTH = pTree->depth();
	const float RANGE = pTree->range();
	QTCoord root;
	root.x_ = root.y_ = 0;

	// The root is the first node that we are going to consider.
	this->push( pTree->root(), root, DEPTH );

	// This stores the interval from the src to dst. We can think of the line as
	// being: src + t * dir, for t in [0, 1]
	Vector2 dir( dst.x - src.x, dst.z - src.z );

	// dirType_ stores whether the line points up or down, left or right.
	const bool pointsLeft = (dir.x < 0);
	const bool pointsDown = (dir.y < 0);
	dirType_ = 2 * pointsDown + pointsLeft;

	// Since the line has width, we calculate a high line and a low line.
	Vector2 dirNorm = dir;
	dirNorm.normalise();
	dirNorm *= radius;
	Vector2 perp( -dirNorm.y, dirNorm.x );

	// Directions are flipped so that all calculations are done assuming the
	// query line points towards to top-right.

	// If the line points to top-left or bottom-right, the perpendicular is
	// actually in the opposite direction.
	if (pointsDown != pointsLeft)
	{
		perp *= -1.f;
	}

	dir += 2.f * dirNorm;

	Vector2 srcLow( src.x, src.z );
	srcLow -= dirNorm;
	srcLow -= perp;

	Vector2 srcHigh( src.x, src.z );
	srcHigh -= dirNorm;
	srcHigh += perp;

	Vector2 dstLow( dst.x, dst.z );
	dstLow += dirNorm;
	dstLow -= perp;
	Vector2 dstHigh( dst.x, dst.z );
	dstHigh += dirNorm;
	dstHigh += perp;

	const int WIDTH = 1 << DEPTH;

	// Needs to be big but when you divide by 0.5 (i.e. double) it must not go
	// to infinity.
	const float BIG_FLOAT = FLT_MAX / 4.f;

	if (!isZero( dir.x ))
	{
		// Solving 0 = (srcLow.x - origin.x) + t * dir.x
		xLow0_	= (origin.x + pointsLeft * RANGE - srcLow.x) / dir.x;
		xHigh0_	= (origin.x + pointsLeft * RANGE - srcHigh.x) / dir.x;

		// How much t changes for each step in the grid
		xLowDelta_	= RANGE / WIDTH / fabsf( dir.x );
		xHighDelta_ = xLowDelta_;
	}
	else
	{
		// For the degenerate case, where we travel parallel to the y-axis, we
		// want t to be a big negative for coordinates less than the
		// y-coordinate and a big positive for coordinates greater.
		// Note: The degenerate case for the y-axis is slightly different
		//       to the x-axis as the high crossing has a smaller value
		//		 than the low crossing in the x case
		const float lowCrossing =
			floorf( WIDTH * (srcLow.x - origin.x) / RANGE ) + 0.5f;
		xLow0_ = -BIG_FLOAT;
		xLowDelta_ = -xLow0_/lowCrossing;

		// Note: The min here is due to a strange optimisation in VC+2005.
		// Even though srcLow.x >= srcHigh.x, the crossings could be the other
		// way around.
		const float highCrossing = std::min( lowCrossing,
			floorf( WIDTH * (srcHigh.x - origin.x) / RANGE ) + 0.5f );
		xHigh0_ = (highCrossing > 0.f) ? -BIG_FLOAT : BIG_FLOAT;
		xHighDelta_ = -xHigh0_/highCrossing;
	}
	
	// The y-coordinate done like the x-coordinate
	if (!isZero( dir.y ))
	{
		yLow0_	= (origin.y + pointsDown * RANGE - srcLow.y) / dir.y;
		yHigh0_	= (origin.y + pointsDown * RANGE - srcHigh.y) / dir.y;
		yLowDelta_ = RANGE / WIDTH / fabsf( dir.y );
		yHighDelta_ = yLowDelta_;
	}
	else
	{
		const float lowCrossing =
			floorf( WIDTH * (srcLow.y - origin.y) / RANGE ) + 0.5f;
		yLow0_ = (lowCrossing > 0.f) ? -BIG_FLOAT : BIG_FLOAT;
		yLowDelta_ = -yLow0_/lowCrossing;

		// Note: The max here is due to a strange optimisation in VC+2005.
		// Even though srcLow.y <= srcHigh.y, the crossings could be the other
		// way around.
		const float highCrossing = std::max( lowCrossing,
			floorf( WIDTH * (srcHigh.y - origin.y) / RANGE ) + 0.5f );
		yHigh0_ = -BIG_FLOAT;
		yHighDelta_ = -yHigh0_/highCrossing;
	}
}


/**
 *	This method pushes a child onto the stack so that it will later be visited
 *	by the traversal.
 */
template <class MEMBER_TYPE>
inline void QTTraversal<MEMBER_TYPE>::pushChild(
	const StackElement & curr, int quad )
{
	// This is a bit tricky. We do the calculations assuming the interval points
	// from the bottom-left to the top-right. Here we calculate the actual
	// quadrant that we are talking about. It turns out that we can XOR the
	// virtual quadrant with the dirType.
	NodeRef nodeRef = pTree_->getChildRef( curr.nodeRef, (Quad)(quad ^ dirType_) );

	if (nodeRef != QuadTreeNode<MEMBER_TYPE>::INVALID_NODE)
	{
		QTCoord newCoord = curr.coord;

		// We still pretend it's in the flipped coord system
		newCoord.offset( quad, curr.depth - 1 );
		this->push( nodeRef, newCoord, curr.depth - 1 );
	}
}


/**
 *	This method returns the current elements in the traversal and moves on the
 *	the next element.
 */
template <class MEMBER_TYPE>
inline const MEMBER_TYPE * QTTraversal<MEMBER_TYPE>::next()
{
	if (size_ <= 0)
	{
		return NULL;
	}

#ifdef TIME_QUADTREE
	dwQTTraverse.start();
#endif

	while (size_ > 0)
	{
		// Check if the head has things to traverse.
		const MEMBER_TYPE * pResult = this->processHead();

		if (pResult)
		{
#ifdef TIME_QUADTREE
			dwQTTraverse.stop();
#endif
			return pResult;
		}

		// Pop the stack
		StackElement curr = this->pop();

		if (curr.depth > 0)
		{
			QTCoord & coord = curr.coord;
			// The number of grid square in half of this current node.
			const int halfWidth = 1 << (curr.depth - 1);

			// The following stores how much t changes across any of the child
			// nodes. Low and high are only different in the degenerate case
			// where the line is parallel to an axis.
			const float currLowXDelta = halfWidth * xLowDelta_;
			const float currLowYDelta = halfWidth * yLowDelta_;
			const float currHighXDelta = halfWidth * xHighDelta_;
			const float currHighYDelta = halfWidth * yHighDelta_;

			// Stores the t-value for the intersection of the lines with each of
			// the lines in the node.
			// (That is x=min, x=mid, x=max, y=min, y=mid, y=max).
			const float xMinLow = xLow0_ + coord.x_ * xLowDelta_;
			const float xMidLow = xMinLow + currLowXDelta;
//			const float xMaxLow = xMidLow + currLowXDelta;

			const float yMinLow = yLow0_ + coord.y_ * yLowDelta_;
			const float yMidLow = yMinLow + currLowYDelta;
			const float yMaxLow = yMidLow + currLowYDelta;

			const float xMinHigh = xHigh0_ + coord.x_ * xHighDelta_;
			const float xMidHigh = xMinHigh + currHighXDelta;
			const float xMaxHigh = xMidHigh + currHighXDelta;

			const float yMinHigh = yHigh0_ + coord.y_ * yHighDelta_;
			const float yMidHigh = yMinHigh + currHighYDelta;
//			const float yMaxHigh = yMidHigh + currHighYDelta;

			// The line goes above the mid-point?
			const bool aboveMidpoint = (yMidHigh <= xMidHigh);
			const bool belowMidpoint = (xMidLow <= yMidLow);

			const bool aboveQ0 = (yMidLow < xMinLow);
			const bool belowQ0 = (xMidHigh < yMinHigh);

			const bool aboveQ3 = (yMaxLow < xMidLow);
			const bool belowQ3 = (xMaxHigh < yMidHigh);

			// Is the interval entirely in 1 half of the region?
			const bool toLeft	= (xMidLow > 1.f);
			const bool toRight	= (xMidHigh < 0.f);
			const bool isAbove	= (yMidLow < 0.f);
			const bool isBelow	= (yMidHigh > 1.f);

			// Push the child that we intersect.
			// Note: Quad 3 is pushed first, so done last and Quad 0 pushed last
			//	so that it is done first.
			if (!aboveQ3 && !belowQ3 && !toLeft && !isBelow)
				this->pushChild( curr, 3 );

			if (belowMidpoint && !toLeft && !isAbove)
				this->pushChild( curr, 1 );

			if (aboveMidpoint && !toRight && !isBelow)
				this->pushChild( curr, 2 );

			if (!aboveQ0 && !belowQ0 && !toRight && !isAbove)
				this->pushChild( curr, 0 );
		}
	}

#ifdef TIME_QUADTREE
	dwQTTraverse.stop();
#endif
	return NULL;
}


/**
 *	This method pushs an entry onto the stack.
 */
template <class MEMBER_TYPE>
inline void QTTraversal<MEMBER_TYPE>::push(
	const NodeRef node, QTCoord coord, int depth )
{
	MF_ASSERT( size_ < STACK_SIZE );
	MF_ASSERT( headIndex_ == 0 );

	StackElement & curr = stack_[ size_ ];
	curr.nodeRef = node;
	curr.coord = coord;
	curr.depth = depth;
	size_++;
}


/**
 *	This method pushs an entry onto the stack.
 */
template <class MEMBER_TYPE>
inline typename QTTraversal<MEMBER_TYPE>::StackElement
	QTTraversal<MEMBER_TYPE>::pop()
{
	headIndex_ = 0;

	return stack_[ --size_ ];
}


/**
 *	This method looks at the current head of the stack. If there are still
 *	elements that have not yet been visited, visit them.
 *
 *	@return The next member to visit. If this node has none, NULL is returned.
 */
template <class MEMBER_TYPE>
inline const MEMBER_TYPE * QTTraversal<MEMBER_TYPE>::processHead()
{
	StackElement & head = stack_[ size_ - 1 ];

	size_t numElements = pTree_->getNumElements( head.nodeRef );
	while (headIndex_ < numElements)
	{
		const MEMBER_TYPE * next = pTree_->getElement( head.nodeRef, headIndex_ );
		headIndex_++;

		// TODO: Test whether the height is correct.
		const bool shouldConsider = true;

		if (shouldConsider)
		{
			return next;
		}
	}

	return NULL;
}


// -----------------------------------------------------------------------------
// Section: QuadTreeNode
// -----------------------------------------------------------------------------

/**
 *	This method add a member to this node.
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::addElement( const MEMBER_TYPE * element, 
	typename QuadTree<MEMBER_TYPE>::NodeRef node, const QTRange & range, int depth )
{
	MF_ASSERT( range.isValid( depth ) );

	if (range.fills( depth ))
	{
		this->addElement( node, element );
	}
	else
	{
		const int newDepth = depth - 1;
		const int MASK = (1 << newDepth);
		const int CLEAR = ~(1 << newDepth);
		const int MAX = (1 << newDepth) - 1;

		int inLeft		= !(range.left_		& MASK);
		int inBottom	= !(range.bottom_	& MASK);
		int inRight		= range.right_	& MASK;
		int inTop		= range.top_	& MASK;

		QTRange origRange = range;
		for (int i = 0; i < 4; i++)
		{
			origRange.ranges_[i] &= CLEAR;
		}
		QTRange newRange = origRange;

		if (inLeft)
		{
			if (inRight) newRange.right_ = MAX; // Right clipped

			if (inTop)
			{
				if (inBottom)
				{
					newRange.bottom_ = 0; // Left and bottom clipped
				}
				MF_ASSERT( newRange.isValid( newDepth ) );
				NodeRef child = this->getOrCreateChild( node, QUAD_TL );
				this->addElement( element, child, newRange, newDepth );
				newRange.bottom_ = origRange.bottom_;
				newRange.top_ = MAX;
				// Now right and top clipped
			}

			if (inBottom)
			{
				MF_ASSERT( newRange.isValid( newDepth ) );
				NodeRef child = this->getOrCreateChild( node, QUAD_BL );
				this->addElement( element, child, newRange, newDepth );
			}

			newRange.right_ = origRange.right_; // Now just top clipped
		}

		if (inRight)
		{
			if (inLeft) newRange.left_ = 0;
			if (inTop)	newRange.top_ = MAX;
			// Left and top clipped

			if (inBottom)
			{
				MF_ASSERT( newRange.isValid( newDepth ) );
				NodeRef child = this->getOrCreateChild( node, QUAD_BR );
				this->addElement( element, child, newRange, newDepth );
				newRange.bottom_ = 0;
				// Left, top and bottom clipped
			}

			if (inTop)
			{
				newRange.top_ = origRange.top_;
				// Left and bottom clipped.
				MF_ASSERT( newRange.isValid( newDepth ) );
				NodeRef child = this->getOrCreateChild( node, QUAD_TR );
				this->addElement( element, child, newRange, newDepth );
			}
		}
	}
}


/**
 *	This method deletes an element from this node.
 */
template <class MEMBER_TYPE>
inline bool QuadTreeNode<MEMBER_TYPE>::del( const MEMBER_TYPE * element,
	const QTRange & range,
	int depth )
{
	// TODO: Implement this!
	return false;
}

/**
 *	Removes the object at the given index. It does this by
 *	calling the destructor for the object to be removed
 *	and then copying the last object in the pool over the
 *	removed object
 *
 *	@param index	Index of the object to remove
 */
template<typename MEMBER_TYPE>
inline void QuadTreeNode<MEMBER_TYPE>::swapLastRemove( size_t index )
{
	MF_ASSERT( elements_.size_ );
	MF_ASSERT( index < elements_.size_ );

	//Move the last element over the deleted one
	--elements_.size_;
	if (elements_.size_ && (index < elements_.size_))
	{
		*(elements_.data_ + index) = *(elements_.data_ + elements_.size_);
	}	
}

/**
 *	This method returns a member in this subtree that contains the input point.
 *	If no such member exists, NULL is returned.
 */
template <class MEMBER_TYPE>
inline const MEMBER_TYPE *
	QuadTree<MEMBER_TYPE>::testPoint(
		const Vector3 & point, NodeRef nodeRef, QTCoord coord, int depth ) const
{
	const MEMBER_TYPE * pResult = NULL;

	if (depth > 0)
	{
		const int quad = coord.findChild( depth -1 );
		NodeRef child = this->getChildRef( quad );

		if (child != Node::INVALID_NODE)
		{
			pResult = this->testPoint( point, child, coord, depth );
		}
	}

	if (pResult == NULL)
	{
		size_t numElements = getNumElements( nodeRef );

		for( size_t i = 0; i < numElements; ++i )
		{
			const MEMBER_TYPE * pCurr = getElement(i);

			if (containsTestPoint( *pCurr, point ))
			{
				return pCurr;
			}
		}
	}

	return pResult;
}


/**
 *	This method returns the number of bytes used by the quadtree rooted at this
 *	node.
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::size( NodeRef nodeRef ) const
{
	int childrenSize = 0;

	for (int i = 0; i < 4; i++)
	{
		NodeRef childRef = this->getChildRef( nodeRef, (Quad)i );

		if (childRef != Node::INVALID_NODE)
		{
			childrenSize += this->size( childRef );
		}		
	}

	size_t size = childrenSize + sizeof( *this ) +
		sizeof( const MEMBER_TYPE * ) * 
		nodes_[nodeRef].getElementsCapacity();

	MF_ASSERT( size <= INT_MAX );
	return ( int ) size;
}


/**
 *	This function is used for debugging. It prints out info about this node.
 */
template <class NODE>
void printQTNode( const NODE & node, const char * prefix )
{
	dprintf( "%s%d\n", prefix, node.elements().size() );
}


/**
 *	This method is used for debugging. It prints out the quadtree rooted at this
 *	node.
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::print( NodeRef nodeRef, int depth ) const
{
	char prefix[128];
	memset( prefix, ' ', 2 * depth );
	prefix[ 2 * depth ] = '\0';
	printQTNode( this->getNode(nodeRef), prefix );

	for (int i = 0; i < 4; i++)
	{
		NodeRef childRef = this->getChildRef( nodeRef, i );
		if (childRef != Node::INVALID_NODE)
		{
			dprintf( "%s Child %d\n", prefix, i );
			print( childRef, depth + 1 );
		}
	}
}


/**
 *	This method is used for debugging. It returns the number of elements that
 *	are at the input coordinate.
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::countAt( NodeRef nodeRef, QTCoord coord, int depth ) const
{
	int count = this->getNumElements( nodeRef );

	if (depth > 0)
	{
		int quad = coord.findChild( depth - 1 );

		NodeRef childRef = this->getChildRef( nodeRef, quad );

		if (childRef != Node::INVALID_NODE)
		{
			count += countAt( childRef, coord, depth -1 );
		}
	}

	return count;
}

/**
 *	This method is used for debugging. Counts number of nodes associated
 *	with this node. ie this node and all its children.
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::countNodes( NodeRef nodeRef ) const
{
	int count = 1;
	for( int i = 0; i < 4; ++i )
	{		
		NodeRef childRef = this->getChildRef(nodeRef, static_cast<Quad>(i));
		if (childRef != Node::INVALID_NODE)
		{
			count += this->countNodes( childRef );
		}		
	}

	return count;
}

/**
 *	This method is used for debugging. It returns the number of elements in
 *	this node and all it's child nodes
 */
template <class MEMBER_TYPE>
inline int QuadTree<MEMBER_TYPE>::countElements( NodeRef nodeRef ) const
{
	size_t count = this->getNumElements( nodeRef );
	for( int i = 0; i < 4; ++i )
	{		
		NodeRef childRef = this->getChildRef(nodeRef, static_cast<Quad>(i));
		if (childRef != Node::INVALID_NODE)
		{
			count += countElements( childRef );
		}		
	}

	return static_cast<int>(count);
}

/**
 *	Debugging. Returns a unique set of the elements added
 */
template <class MEMBER_TYPE>
inline void QuadTree<MEMBER_TYPE>::getUniqueElements( 
	BW::set<const MEMBER_TYPE*>& elements, NodeRef nodeRef ) const
{
	//Go through elements at this node
	size_t numElements = this->getNumElements( nodeRef );
	for( size_t i = 0; i < numElements; ++i )
	{
		elements.insert( this->getElement(nodeRef, i) );
	}

	//Go through the child nodes.
	for( int i = 0; i < 4; ++i )
	{		
		NodeRef childRef = this->getChildRef(nodeRef, static_cast<Quad>(i));
		if (childRef != Node::INVALID_NODE)
		{
			getUniqueElements( elements, childRef );
		}		
	}
}

// quad_tree.ipp
