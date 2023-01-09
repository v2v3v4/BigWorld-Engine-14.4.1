#ifndef RANGE_LIST_NODE_HPP
#define RANGE_LIST_NODE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/vectornodest.hpp"

#include <float.h>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class RangeTrigger;
class RangeListNode;

enum RangeListOrder
{
	RANGE_LIST_ORDER_HEAD = 0,
	RANGE_LIST_ORDER_ENTITY = 100,
	RANGE_LIST_ORDER_LOWER_BOUND = 190,
	RANGE_LIST_ORDER_UPPER_BOUND = 200,
	RANGE_LIST_ORDER_TAIL = 0xffff
};



/**
 * This class is the base of all range nodes. Range Nodes are used to keep track of
 * the order of entities and triggers relative to the x axis or the z axis.
 * This is used for AoI calculations and range queries.
 */
class RangeListNode
{
public:
	enum RangeListFlags
	{
		FLAG_NO_TRIGGERS		= 0,
		FLAG_ENTITY_TRIGGER		= 0x01,
		FLAG_LOWER_AOI_TRIGGER	= 0x02,
		FLAG_UPPER_AOI_TRIGGER	= 0x04,

		FLAG_IS_ENTITY			= 0x10,
		FLAG_IS_LOWER_BOUND		= 0x20
	};

	RangeListNode( RangeListFlags wantsFlags,
			RangeListFlags makesFlags,
			RangeListOrder order ) :
		pPrevX_( NULL ),
		pNextX_( NULL ),
		pPrevZ_( NULL ),
		pNextZ_( NULL ),
		wantsFlags_( wantsFlags ),
		makesFlags_( makesFlags ),
		order_( order )
	{ }
	virtual ~RangeListNode()	{}

	//oldZ is used to make the shuffle think entity is moving in orthogonal directions only
	void shuffleXThenZ( float oldX, float oldZ );

	void shuffleX( float oldX, float oldZ );
	void shuffleZ( float oldX, float oldZ );

	RangeListNode * prevX() const	{ return pPrevX_; }
	RangeListNode * prevZ() const	{ return pPrevZ_; }
	RangeListNode * nextX() const	{ return pNextX_; }
	RangeListNode * nextZ() const	{ return pNextZ_; }

	RangeListNode * getNeighbour( bool getNext, bool getZ ) const
	{
		return getNext ?
			(getZ ? pNextZ_ : pNextX_) :
			(getZ ? pPrevZ_ : pPrevX_);
	}

	void prevX( RangeListNode * pNode )	{ pPrevX_ = pNode; }
	void prevZ( RangeListNode * pNode )	{ pPrevZ_ = pNode; }
	void nextX( RangeListNode * pNode )	{ pNextX_ = pNode; }
	void nextZ( RangeListNode * pNode )	{ pNextZ_ = pNode; }

	RangeListOrder order() const 		{ return order_; }

	bool isEntity() const		{ return makesFlags_ & FLAG_IS_ENTITY; }
	bool isLowerBound() const	{ return makesFlags_ & FLAG_IS_LOWER_BOUND; }

	bool wantsCrossingWith( RangeListNode * pOther ) const
	{
		return (wantsFlags_ & pOther->makesFlags_);
	}

	void removeFromRangeList();
	void insertBeforeX( RangeListNode * entry );
	void insertBeforeZ( RangeListNode * entry );

	virtual void  debugRangeX() const;
	virtual void  debugRangeZ() const;
	virtual BW::string debugString() const	{ return BW::string( "Base" ); };

	float getCoord( bool getZ ) const { return getZ ? this->z() : this->x(); }

	virtual float x() const = 0;
	virtual float z() const = 0;

	virtual void crossedX( RangeListNode * /*node*/, bool /*positiveCrossing*/,
		float /*oldOthX*/, float /*oldOthZ*/ ) {}
	virtual void crossedZ( RangeListNode * /*node*/, bool /*positiveCrossing*/,
		float /*oldOthX*/, float /*oldOthZ*/ ) {}

protected:
	//pointers to the prev and next entities in the X and Z direction
	RangeListNode *	pPrevX_;
	RangeListNode *	pNextX_;
	RangeListNode *	pPrevZ_;
	RangeListNode *	pNextZ_;

	// Flags for type of crossings this node wants to receive
	RangeListFlags	wantsFlags_;

	// Flags for type of crossings this node wants to make
	RangeListFlags	makesFlags_;
	RangeListOrder	order_;
};


/**
 *	This class is used for the terminators of the range list. They are either
 *	the head or tail of the list. They always have a position of +/- FLT_MAX.
 */
class RangeListTerminator : public RangeListNode
{
public:
	RangeListTerminator( bool isHead ) :
		RangeListNode( RangeListFlags( 0 ), RangeListFlags( 0 ),
				isHead ? RANGE_LIST_ORDER_HEAD : RANGE_LIST_ORDER_TAIL) {}
	float x() const { return (order_ ? FLT_MAX : -FLT_MAX); }
	float z() const { return (order_ ? FLT_MAX : -FLT_MAX); }
	BW::string debugString() const { return order_ ? "Tail" : "Head"; }
};


#ifdef _WIN32
#pragma warning (disable:4355)
#endif

BW_END_NAMESPACE

#endif // RANGE_LIST_NODE_HPP
