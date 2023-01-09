#ifndef QUAD_TREE_HPP
#define QUAD_TREE_HPP

/**
 *	This file implement the quadtree.
 *
 *	Possible memory reductions:
 *		- Reduce the depth of the tree that is being used.
 *		- In the calculateQTRange function, for large objects they can be scaled
 *			up so that they do not make as many leaf nodes.
 *		- When inserting objects, we do not need to go down to the maximum
 *			depth. We could stop when there are no other objects in the subtree
 *			(or a certain number).
 *		- We know that nodes at the maximum depth do not have any children so we
 *			could have a QuadTreeNode class without these pointers. This would
 *			involve casting (or the addition of virtual methods with the memory
 *			and small CPU costs).
 *		- The VC++7 implementation of BW::vector takes 16 bytes when empty. We
 *			could look at having internal nodes having a fixed length array. If
 *			they had four, this would use the same amount of memory if the
 *			vectors were empty. If they overflowed, you would have to put them
 *			in child nodes. (This would cost 4 times as much).
 *		- An alternative to the previous option is for (internal nodes) they
 *			could keep a pointer to a vector. They only create it when
 *			necessary. This would cost 4 bytes for those nodes that are empty
 *			and 20 bytes for those that aren't. Nodes at the maximum depth would
 *			always have a collection.
 *		- We could just insert the objects until they would be split over nodes.
 *			This could be bad for nodes that overlap the centre point but the
 *			benefits include that we could keep the elements in a quadtree node
 *			as a linked-list with the next pointer actually in the
 *			ChunkObstacle. This would also get rid of the need for the 4 byte
 *			mark in ChunkObstacles.
 *
 *	Thoughts on speed:
 *		- We should look at how memory is allocated and maybe look at doing a
 *			custom allocator. Could also do batch adds.
 *
 *	Thoughts on cleanup:
 *		- The functions that need to be implemented based on the member type
 *			could be static methods of the DESC class.
 *
 *	Other thoughts:
 *		- It would be possible to use this for the chunk's hull tree.
 *
 *  QuadTreeMemPool and related changes:
 *		- The QuadTree was a source of many memory allocations, showing
 *			second in the list of top 32. This was because the nodes 
 *			were being allocated using new and the vector of elements was
 *			growing from zero size, causing many allocations as it grew.
 *			These allocations were often scattered across address space
 *			and contributed to memory fragmentation. The performance
 *			during tree creation was not ideal either, taking
 *			up to 2ms just to add one new object in the tree.
 *		- To reduce the numbers of allocations a QuadTreeMemPool was
 *			introduced. Instead of pointers, nodes are
 *			now referenced by an index into this pool. 
 *			By doing this we can generate
 *			a single memory block of nodes that grows by 50% each time
 *			the space is exhausted. This relies on the nodes and the 
 *			elements to be plain old data (POD) so that the
 *			quad tree can realloc when it runs out of room.
 *			Unfortunately this has made the code harder to read.
 *		- I did try a version that did not rely on
 *			the nodes being POD and was quite successful when compiling
 *			with a C++11 compiler as move semantics reduced the amount
 *			of new being called. In the case of non C++11 there was
 *			potential to make the memory allocation problem even worse.
 *			I found in the case of BW::vector under MSVC C++11 compiler
 *			that it creates a proxy object on the heap even when the
 *			vector is empty and on the stack, so you are going to get
 *			a least two allocations associated with these, a
 *			reason I didn't continue to use them.
 *        
 *	TODO:	Need to implement deletion.
 */

#include "cstdmf/bw_vector.hpp"

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/bw_set.hpp"

#include <float.h>
#include <string.h>

BW_BEGIN_NAMESPACE

template <class MEMBER_TYPE> class QTTraversal;
template <class MEMBER_TYPE> struct QuadTreeNode;
template <class MEMBER_TYPE> class QTTraversal;

typedef int QTIndex;

/**
 *	This enumeration is used to specify a child of a quadtree node.
 */
enum Quad
{
	QUAD_BL = 0,
	QUAD_BR = 1,
	QUAD_TL = 2,
	QUAD_TR = 3
};


// -----------------------------------------------------------------------------
// Section: QTCoord
// -----------------------------------------------------------------------------

/**
 *	This is a helper class used to specify a coord in the quadtree.
 */
class QTCoord
{
public:
	/**
	 *	This method makes sure that the coordinate is not outside the bottom and
	 *	and left sides.
	 */
	void clipMin()
	{
		x_ = std::max( 0, x_ );
		y_ = std::max( 0, y_ );
	}

	/**
	 *	This method makes sure that the coordinate is not outside the right and
	 *	top sides.
	 */
	void clipMax( int depth )
	{
		const int MAX = (1 << depth) - 1;
		x_ = std::min( x_, MAX );
		y_ = std::min( y_, MAX );
	}

	/**
	 *	This method returns the quadrant that a child coord is in.
	 *
	 *	@param depth	The depth that the child is at.
	 */
	int findChild( int depth ) const
	{
		const int MASK = (1 << depth);

		return ((x_ & MASK) + 2 * (y_ & MASK)) >> depth;
	}

	/**
	 *	This method converts this coord to indicate a child of the current node.
	 *
	 *	@param quad		The desired child quadrant.
	 *	@param depth	The depth of the child node.
	 */
	void offset( int quad, int depth )
	{
		x_ |= ((quad & 0x1) << depth);
		y_ |= ((quad > 1) << depth);
	}

	int x_;
	int y_;
};


// -----------------------------------------------------------------------------
// Section: QTRange
// -----------------------------------------------------------------------------

/**
 *	This is a helper class used to specify a range in the quadtree.
 */
class QTRange
{
public:
	/**
	 *	This method returns whether or not this range fills a node at the input
	 *	depth.
	 */
	bool fills( int depth ) const
	{
		const int MAX = (1 << depth) - 1;
		return left_ == 0 && bottom_ == 0 &&
			right_ == MAX && top_ == MAX;
	}

	/**
	 *	This method returns whether or not this range is valid. It is only used
	 *	for debugging.
	 */
	bool isValid( int depth ) const
	{
		const int MAX = 1 << depth;
		return
			(0 <= left_) && (left_ <= right_) && (right_ < MAX) &&
			(0 <= bottom_) && (bottom_ <= top_) && (top_ < MAX) &&
			depth >= 0;
	}

	/**
	 *	This method returns whether or not this range is in the quadtree with
	 *	the input depth.
	 */
	bool inQuad( int depth ) const
	{
		const int MAX = 1 << depth;

		return 0 <= right_ && 0 <= top_ &&
			left_ < MAX && bottom_ < MAX;
	}

	/**
	 *	This method clips this range so that it is entirely in the range of a
	 *	quadtree with the input depth.
	 */
	void clip( int depth )
	{
		corner_[0].clipMin();
		corner_[1].clipMax( depth );
	}

	// ---- Data ----
	union
	{
		QTIndex	ranges_[4];
		QTCoord	corner_[2];
		struct
		{
			QTIndex	left_;
			QTIndex	bottom_;
			QTIndex	right_;
			QTIndex	top_;
		};
	};
};

// -----------------------------------------------------------------------------
// Section: QuadTreeMemPool
// -----------------------------------------------------------------------------
/**
 *	This class implements a special resizable memory pool for objects 
 *  in the quadtree.
 *  
 *  ***For use only with plain old data types (POD)***
 *
 *  If the objects are not POD then things will go bad because the pool
 *  may relocate them in memory.
 *
 *  Objects stored in the pool are not destroyed, no destructors are called.
 *
 *  Written specifically to store the nodes and and elements of a quadtree.
 *  The motivation was to reduce the number of memory allocations associated
 *  with quad trees and the reduce memory fragmentation.
 *
 *  Instead of nodes and elements being newed directly into the global
 *  heap, they are instead allocated inside QuadTreeMemPool.
 *  If the memory pool runs out of space then it reallocates itself,
 *  and does a bitwise copy from the old memory location to the new.
 *  Hence the reason that it only works with plain old data.
 */
template<typename MEM_TYPE>
struct QuadTreeMemPool
{
	void init();
	void destroy();

	size_t size() const;
	size_t capacity() const;
	void reserve( size_t num );

	MEM_TYPE & createNew();

	const MEM_TYPE & operator[] ( size_t index ) const;
	MEM_TYPE & operator[] ( size_t index );

	size_t capacity_;
	size_t size_;
	MEM_TYPE* data_;
};

// -----------------------------------------------------------------------------
// Section: QuadTreeNode
// -----------------------------------------------------------------------------

/**
 *	This class is used by QuadTree to implement its internal nodes.
 *  Important that this object remains plain old data (POD) because
 *  it is stored inside the QuadTreeMemPool
 */
template <class MEMBER_TYPE>
struct QuadTreeNode
{
	//The type of NodeRef and value of INVALID_NODE are linked
	typedef uint16 NodeRef;
	enum { INVALID_NODE = 0xffff };

	void init()
	{
		child_[0] = INVALID_NODE;
		child_[1] = INVALID_NODE;
		child_[2] = INVALID_NODE;
		child_[3] = INVALID_NODE;

		elements_.init();
	}

	void destroy()
	{
		child_[0] = INVALID_NODE;
		child_[1] = INVALID_NODE;
		child_[2] = INVALID_NODE;
		child_[3] = INVALID_NODE;

		elements_.destroy();
	}

	bool del( const MEMBER_TYPE * element, const QTRange & range, int depth );

	void addElement( const MEMBER_TYPE * element )
	{		
		//Reserving a few elements can halve the number of
		//allocations that occur as new elements are added.
		//Reserve here instead of inside init(),
		//if we have a node it does not imply it will have
		//elements.
		const int numToReserve = 16;
		elements_.reserve(numToReserve);

		elements_.createNew() = element;
	}

	size_t getNumElements() const
	{
		return elements_.size();
	}

	const MEMBER_TYPE * getElement( size_t index ) const
	{		
		return elements_[index];
	}

	void swapLastRemove( size_t index );

	void removeElement( size_t index )
	{
		this->swapLastRemove(index);
	}

	size_t getElementsCapacity() const
	{
		return elements_.capacity();
	}
	
	NodeRef getChildRef( Quad i ) const
	{
		return child_[i];
	}

	void setChild( NodeRef nodeRef, Quad i )
	{
		child_[i] = nodeRef;
	}

	typedef QuadTreeMemPool<const MEMBER_TYPE*> ElementPool;
	NodeRef child_[4];
	ElementPool	elements_;
};

// -----------------------------------------------------------------------------
// Section: QuadTree
// -----------------------------------------------------------------------------

/**
 *	This class implements a quadtree. It is used to store the collision objects
 *	in a column.
 *
 *	MEMBER_TYPE indicates the type of object that is to be stored in the
 *	QuadTree. There needs to be a function matching the following prototype:
 *
 *	QTRange calculateQTRange( const MEMBER_TYPE & input, const Vector2 & origin,
 *						 float range, int depth )
 *
 *	DESC is a class that describes the QuadTree. It should have two static
 *	members. An integer DEPTH indicates the maximum depth of the tree and a
 *	floating point value, RANGE, that indicates the range that the QuadTree
 *	covers. It is assumed to be square.
 */
template <class MEMBER_TYPE>
class QuadTree
{
public:
	typedef QTTraversal< MEMBER_TYPE > Traversal;
	typedef QuadTreeNode< MEMBER_TYPE > Node;
	typedef typename QuadTreeNode< MEMBER_TYPE >::NodeRef NodeRef;
	typedef QuadTreeMemPool<Node> NodePool;

	QuadTree( float x, float z, int depth, float range );
	~QuadTree();

	void add( const MEMBER_TYPE & element );
	void del( const MEMBER_TYPE & element );

	void addToRoot( const MEMBER_TYPE & element );
	bool removeFromRoot( const MEMBER_TYPE & element );

	const MEMBER_TYPE* testPoint( const Vector3 & point ) const;

	NodeRef root() const;	
	NodeRef getChildRef( NodeRef nodeRef, Quad i ) const;

	size_t getNumElements( NodeRef nodeRef ) const;
	const MEMBER_TYPE* getElement( NodeRef nodeRef, size_t elementIndex ) const;

	Traversal traverse( const Vector3 & source, const Vector3 & extent,
		float radius = 0.f ) const;

	int depth() const	{ return depth_; }
	float range() const	{ return range_; }

	//Debugging functions
	void print() const;
	long size() const;
	int countNodes() const;
	int countElements() const;
	void getUniqueElements( BW::set<const MEMBER_TYPE*>& elements ) const;

private:

	QuadTree();
	QuadTree( const QuadTree& );
	QuadTree& operator=( const QuadTree& );

	NodeRef createNewNode();
	NodeRef getOrCreateChild( NodeRef nodeRef, Quad i );

	void addElement( NodeRef nodeRef, const MEMBER_TYPE * element );
	void addElement( const MEMBER_TYPE * element, 
		NodeRef node, const QTRange & range, int depth );

	const MEMBER_TYPE * testPoint( const Vector3 & point,
		NodeRef nodeRef, QTCoord coord, int depth ) const;

	//Debugging functions
	int size( NodeRef nodeRef ) const;
	void print( NodeRef nodeRef, int depth ) const;
	int countAt( NodeRef nodeRef, QTCoord coord, int depth ) const;
	int countElements( NodeRef nodeRef ) const;
	int countNodes( NodeRef nodeRef ) const;

	void getUniqueElements( BW::set<const MEMBER_TYPE*>& elements, 
		NodeRef nodeRef ) const;

	NodePool nodes_;

	const Vector2 origin_;	/// The coordinate of the bottom-left corner.
	const int depth_;
	const float range_;
};

// -----------------------------------------------------------------------------
// Section: QTTraversal
// -----------------------------------------------------------------------------

/**
 *	This class is used to traverse a QuadTree.
 */
template <class MEMBER_TYPE>
class QTTraversal
{
	static const int Q0 = 1 << 0;	// Bottom left
	static const int Q1 = 1 << 1;	// Bottom right
	static const int Q2 = 1 << 2;	// Top left
	static const int Q3 = 1 << 3;	// Top right

	typedef QuadTree<MEMBER_TYPE> Tree;
	typedef typename Tree::NodeRef NodeRef;

public:
	const MEMBER_TYPE * next();
	~QTTraversal() { pTree_ = NULL; }

private:
	QTTraversal() { pTree_ = NULL; }

	QTTraversal( const Tree * pTree, const Vector3 & src,
			const Vector3 & dst, float radius,
			const Vector2 & origin );

	struct StackElement
	{
		NodeRef nodeRef;
		QTCoord coord;
		int depth;
	};

	inline void pushChild( const StackElement & curr, int quad );
	void push( NodeRef node, QTCoord coord, int depth );
	StackElement pop();

	const MEMBER_TYPE * processHead();

	// TODO: Should not hard-code the stack size. It should be something like:
	//	3 * depth_ + 1
	static const int STACK_SIZE = 20;
	StackElement stack_[ STACK_SIZE ];
	int size_;

	size_t			headIndex_;

	// The following members are used to describe the query. We think of the
	// query interval parameterised as: src + t*dir, for t in [0,1]. Since the
	// query line has width, we end have a high side of the line and the low
	// side of the line.
	float			xLow0_;		///< Stores the t where the low line cuts x=0
	float			yLow0_;		///< Stores the t where the low line cuts y=0

	float			xHigh0_;	///< Stores the t where the high line cuts x=0
	float			yHigh0_;	///< Stores the t where the high line cuts y=0

	// The following store how much t changes for each grid square movement at
	// the lowest level along their relevant axis. The reason we keep separate
	// deltas for the low line and the high line is for the degenerate case when
	// the query line is along an axis.
	float			xLowDelta_;
	float			yLowDelta_;

	float			xHighDelta_;
	float			yHighDelta_;

	/**
	 *	Stores the direction of the traversal.
	 *	0 - Bottom-left to top-right
	 *	1 - Bottom-right to top-left
	 *	2 - Top-left to bottom-right
	 *	3 -	Top-right to bottom-left
	 */
	int				dirType_;

	const Tree*		pTree_;

	friend class QuadTree< MEMBER_TYPE >;
};

#include "quad_tree.ipp"

BW_END_NAMESPACE

#endif // QUAD_TREE_HPP
