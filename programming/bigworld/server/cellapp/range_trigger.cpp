#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "range_trigger.hpp"

#include <cstddef>
#include <math.h>

#include "entity.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Cell", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RangeTriggerNode
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param pRangeTrigger The node that this trigger is offset from.
 *	@param range   The range of the AoI for this entity.
 *	@param wantsFlags
 *	@param makesFlags
 */
RangeTriggerNode::RangeTriggerNode( RangeTrigger * pRangeTrigger,
		float range,
		RangeListFlags wantsFlags,
		RangeListFlags makesFlags ) :
	RangeListNode( wantsFlags, makesFlags,
			(makesFlags & FLAG_IS_LOWER_BOUND) ?
				RANGE_LIST_ORDER_LOWER_BOUND :
				RANGE_LIST_ORDER_UPPER_BOUND ),
	pRange_( pRangeTrigger ),
	range_( range ),
	oldRange_( 0.f )
{
}


/**
 * This method returns the x position of this node.
 * For trigger nodes, this is the entities position + the range
 *
 * @return x position of the node
 */
float RangeTriggerNode::x() const
{
	// It is important that this is volatile. If this is not done, a call to
	// this may pass back the value in a floating point register that has more
	// precision than we expect. This can later cause problems when this value
	// is assumed to have less precision in isInXRange.
	volatile float x = pRange_->pCentralNode()->x() + range_;

	return x;
}


/**
 * This method returns the z position of this node.
 * For trigger nodes, this is the entities position + the range
 *
 * @return z position of the node
 */
float RangeTriggerNode::z() const
{
	// It is important that this is volatile. If this is not done, a call to
	// this may pass back the value in a floating point register that has more
	// precision than we expect. This can later cause problems when this value
	// is assumed to have less precision in wasInZRange.
	volatile float z = pRange_->pCentralNode()->z() + range_;

	return z;
}


/**
 * This method returns the x position of this node.
 * For trigger nodes, this is the entities position + the range
 *
 * @return x position of the node
 */
float RangeTriggerNode::oldX() const
{
	// It is important that this is volatile. If this is not done, a call to
	// this may pass back the value in a floating point register that has more
	// precision than we expect. This can later cause problems when this value
	// is assumed to have less precision in isInXRange.
	volatile float x = pRange_->oldEntityX_ + oldRange_;

	return x;
}


/**
 * This method returns the z position of this node.
 * For trigger nodes, this is the entities position + the range
 *
 * @return z position of the node
 */
float RangeTriggerNode::oldZ() const
{
	// It is important that this is volatile. If this is not done, a call to
	// this may pass back the value in a floating point register that has more
	// precision than we expect. This can later cause problems when this value
	// is assumed to have less precision in wasInZRange.
	volatile float z = pRange_->oldEntityZ_ + oldRange_;

	return z;
}



/**
 * This method returns the identifier for the RangeTriggerNode
 *
 * @return the string identifier of the node
 */
BW::string RangeTriggerNode::debugString() const
{
	char buf[256];
	bw_snprintf( buf, sizeof(buf), "%s trigger of %s range %6.3f Flags: %x %x",
		(range_ > 0.f) ? "Upper" : "Lower",
		pRange_->debugString().c_str(),
		fabsf(range_),
		wantsFlags_,
		makesFlags_ );

	return buf;
}


/**
 *	This method is called whenever there is a shuffle in the X direction.
 *	The range trigger node then decides whether or not the entity triggers or
 *	untriggers it.
 *
 *	@param pNode Other entity that has crossed this trigger node.
 *	@param positiveCrossing Which direction the shuffle occurred.
 *	@param oldOthX The old x co-ordinate of the other node.
 *	@param oldOthZ The old z co-ordinate of the other node.
 */
void RangeTriggerNode::crossedX( RangeListNode * pNode, bool positiveCrossing,
	float oldOthX, float oldOthZ )
{
	if (pNode->isEntity())
	{
		this->crossedXEntity( pNode, positiveCrossing, oldOthX, oldOthZ );
	}
	else
	{
		this->crossedXEntityRange( pNode, positiveCrossing );
	}
}


/**
 *	This method is called whenever there is a shuffle in the X direction.
 *	The range trigger node then decides whether or not the entity triggers or
 *	untriggers it.
 *
 *	@param pNode Other entity that has crossed this trigger node.
 *	@param positiveCrossing Which direction the shuffle occurred.
 *	@param oldOthX The old x co-ordinate of the other node.
 *	@param oldOthZ The old z co-ordinate of the other node.
 */
void RangeTriggerNode::crossedXEntity( RangeListNode * pNode,
		bool positiveCrossing, float oldOthX, float oldOthZ )
{
	if (pNode == pRange_->pCentralNode()) return;

	// x is shuffled first so the old z position is checked.
	const bool wasInZ = pRange_->wasInZRange( oldOthZ, fabsf( oldRange_ ) );

	if (!wasInZ)
	{
		return;
	}

	Entity * pOtherEntity = EntityRangeListNode::getEntity( pNode );

	const bool isEntering = (this->isLowerBound() == positiveCrossing);

	if (isEntering)
	{
		const bool isInX = pRange_->isInXRange( pNode->x(), fabsf( range_ ) );
		const bool isInZ = pRange_->isInZRange( pNode->z(), fabsf( range_ ) );

		if (isInX && isInZ)
		{
			pRange_->triggerEnter( *pOtherEntity );
		}
	}
	else
	{
		const bool wasInX = pRange_->wasInXRange( oldOthX, fabsf( oldRange_ ) );

		if (wasInX)
		{
			pRange_->triggerLeave( *pOtherEntity );
		}
	}
}


/**
 *	This method handles the case where this range trigger node crossed an
 *	entity's appeal range in the X direction.
 */
void RangeTriggerNode::crossedXEntityRange( RangeListNode * pNode,
		bool positiveCrossing )
{
	RangeTriggerNode * pOtherNode = static_cast< RangeTriggerNode * >( pNode );
	const RangeTrigger & otherRangeTrigger = pOtherNode->rangeTrigger();

	RangeTrigger & thisRangeTrigger = *pRange_;

	// x is shuffled first so the old z position is checked.
	const bool wasInZ =
		thisRangeTrigger.wasInZRange(
				otherRangeTrigger.pLowerTrigger()->oldZ(),
				otherRangeTrigger.pUpperTrigger()->oldZ(),
				fabsf( oldRange_ ) );

	if (!wasInZ)
	{
		return;
	}

	Entity * pOtherEntity = otherRangeTrigger.pEntity();

	const bool isEntering = (this->isLowerBound() == positiveCrossing);

	if (isEntering)
	{
		const bool isInX = thisRangeTrigger.isInXRange(
				otherRangeTrigger.pLowerTrigger()->x(),
				otherRangeTrigger.pUpperTrigger()->x(),
				fabsf( range_ ) );
		const bool isInZ = thisRangeTrigger.isInZRange(
				otherRangeTrigger.pLowerTrigger()->z(),
				otherRangeTrigger.pUpperTrigger()->z(),
				fabsf( range_ ) );

		if (isInX && isInZ)
		{
			thisRangeTrigger.triggerEnter( *pOtherEntity );
		}
	}
	else
	{
		const bool wasInX =
			thisRangeTrigger.wasInXRange(
					otherRangeTrigger.pLowerTrigger()->oldX(),
					otherRangeTrigger.pUpperTrigger()->oldX(),
					fabsf( oldRange_ ) );

		if (wasInX)
		{
			thisRangeTrigger.triggerLeave( *pOtherEntity );
		}
	}
}


/**
 *	This method is called whenever, there is a shuffle in the Z direction. The
 *	range trigger node then decides whether or not the entity triggers or
 *	untriggers it.
 *
 *	@param pNode Other entity that has crossed this trigger node.
 *	@param positiveCrossing Which direction the shuffle occurred.
 *	@param oldOthX The old x co-ordinate of the other node.
 *	@param oldOthZ The old z co-ordinate of the other node.
 */
void RangeTriggerNode::crossedZ( RangeListNode * pNode, bool positiveCrossing,
	float oldOthX, float oldOthZ )
{
	if (pNode->isEntity())
	{
		this->crossedZEntity( pNode, positiveCrossing, oldOthX, oldOthZ );
	}
	else
	{
		this->crossedZEntityRange( pNode, positiveCrossing );
	}
}


/**
 *	This method is called whenever, there is a shuffle in the Z direction. The
 *	range trigger node then decides whether or not the entity triggers or
 *	untriggers it.
 *
 *	@param pNode Other entity that has crossed this trigger node.
 *	@param positiveCrossing Which direction the shuffle occurred.
 *	@param oldOthX The old x co-ordinate of the other node.
 *	@param oldOthZ The old z co-ordinate of the other node.
 */
void RangeTriggerNode::crossedZEntity( RangeListNode * pNode,
		bool positiveCrossing, float oldOthX, float oldOthZ )
{
	if (pNode == pRange_->pCentralNode()) return;

	// z is shuffled second so the new x position is checked.
	const bool isInX = pRange_->isInXRange( pNode->x(), fabsf( range_ ) );

	if (!isInX)
	{
		return;
	}

	Entity * pOtherEntity = EntityRangeListNode::getEntity( pNode );

	const bool isEntering = (this->isLowerBound() == positiveCrossing);

	if (isEntering)
	{
		const bool isInZ = pRange_->isInZRange( pNode->z(), fabsf( range_ ) );

		if (isInZ)
		{
			pRange_->triggerEnter( *pOtherEntity );
		}
	}
	else
	{
		const bool wasInX = pRange_->wasInXRange( oldOthX, fabsf( oldRange_ ) );
		const bool wasInZ = pRange_->wasInZRange( oldOthZ, fabsf( oldRange_ ) );

		if (wasInX && wasInZ)
		{
			pRange_->triggerLeave( *pOtherEntity );
		}
	}
}


/**
 *	This method handles the case where this range trigger node crossed an
 *	entity's appeal range in the Z direction.
 */
void RangeTriggerNode::crossedZEntityRange( RangeListNode * pNode,
		bool positiveCrossing )
{
	RangeTriggerNode * pOtherNode = static_cast< RangeTriggerNode * >( pNode );
	const RangeTrigger & otherRangeTrigger = pOtherNode->rangeTrigger();

	RangeTrigger & thisRangeTrigger = *pRange_;

	// x is shuffled first so the new x position is checked.
	const bool isInX = thisRangeTrigger.isInXRange(
			otherRangeTrigger.pLowerTrigger()->x(),
			otherRangeTrigger.pUpperTrigger()->x(),
			fabsf( range_ ) );

	if (!isInX)
	{
		return;
	}

	Entity * pOtherEntity = otherRangeTrigger.pEntity();

	const bool isEntering = (this->isLowerBound() == positiveCrossing);

	if (isEntering)
	{
		const bool isInZ = pRange_->isInZRange(
				otherRangeTrigger.pLowerTrigger()->z(),
				otherRangeTrigger.pUpperTrigger()->z(),
				fabsf( range_ ) );

		if (isInZ)
		{
			pRange_->triggerEnter( *pOtherEntity );
		}
	}
	else
	{
		const bool wasInX =
			pRange_->wasInXRange(
					otherRangeTrigger.pLowerTrigger()->oldX(),
					otherRangeTrigger.pUpperTrigger()->oldX(),
					fabsf( oldRange_ ) );
		const bool wasInZ =
			pRange_->wasInZRange(
					otherRangeTrigger.pLowerTrigger()->oldZ(),
					otherRangeTrigger.pUpperTrigger()->oldZ(),
					fabsf( oldRange_ ) );

		if (wasInX && wasInZ)
		{
			pRange_->triggerLeave( *pOtherEntity );
		}
	}
}


/**
 * This method sets the range of the trigger. It does a reshuffle to shrink the
 *	range and any entities which are no longer in the range are untriggered.
 *
 * @param range - the range of the node from its entity
 */
void RangeTriggerNode::setRange( float range )
{
	oldRange_ = range_;
	float oldX = this->x();
	float oldZ = this->z();
	range_ = range;
	this->shuffleXThenZ( oldX, oldZ );
	oldRange_ = range_;
}


// -----------------------------------------------------------------------------
// Section: RangeTrigger
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param pCentralNode	The node around which this trigger is based
 *	@param range		The range (in metres) of this trigger.
 *	@param wantsFlagsLower
 *	@param wantsFlagsUpper
 *	@param makesFlagsLower
 *	@param makesFlagsUpeer
 */
RangeTrigger::RangeTrigger( RangeListNode * pCentralNode,
		float range,
		RangeListNode::RangeListFlags wantsFlagsLower,
		RangeListNode::RangeListFlags wantsFlagsUpper,
		RangeListNode::RangeListFlags makesFlagsLower,
		RangeListNode::RangeListFlags makesFlagsUpper ) :
	pCentralNode_( pCentralNode ),
	upperBound_( this, range, wantsFlagsUpper, makesFlagsUpper ),
	lowerBound_( this, -range, wantsFlagsLower,
			RangeListNode::RangeListFlags(
				makesFlagsLower | RangeListNode::FLAG_IS_LOWER_BOUND ) ),
	oldEntityX_( pCentralNode->x() ),
	oldEntityZ_( pCentralNode->z() )
{
}


/**
 *	Destructor.
 */
RangeTrigger::~RangeTrigger()
{
}


/**
 *	This method performs the initial shuffle after range triggers have
 *	been created. It cannot be called from our constructor since the
 *	constructor of our derived class (which implements the trigger methods)
 *	has not yet been called (and it crashes if you try :)
 */
void RangeTrigger::insert()
{
	/*
	pCentralNode_->insertBeforeX( &lowerBound_ );
	pCentralNode_->insertBeforeZ( &lowerBound_ );
	pCentralNode_->nextX()->insertBeforeX( &upperBound_ );
	pCentralNode_->nextZ()->insertBeforeZ( &upperBound_ );
	*/
	// The code above does not work in the following case:
	// 1. the range of the trigger is less than the floating pt epsilon
	// 2. there is >1 entity type node at the same posn as pCentralNode
	// In this case, the initial shuffle will generate spurious leaves
	// for nodes that happened to get sorted after pCentralNode.
	// e.g. triggers are x, entities are o, new triggers are |:
	// x x x x o o o o o x x x
	//            | |
	// the triggers encapsulate the entity. But if say the range is zero,
	// then the inital shuffle will move them to this:
	// x x x x o o o o o | | x x x
	// which generates two triggerLeaves for entities never entered.
	// There is already code to ignore the when the trigger crosses its
	// own entity, required for the setRange(0) call used to remove them.
	// And there might be a problem with seeing those co-located entities
	// in the first place too ... anyway, this solution is much saner for
	// all concerned:

	uint16 lowerOrder = lowerBound_.order();
	uint16 upperOrder = upperBound_.order();

	MF_ASSERT(
		lowerOrder < upperOrder &&
		lowerOrder > pCentralNode_->order() &&
		upperOrder > pCentralNode_->order() );

	RangeListNode * pCursor = pCentralNode_->nextX();

	while (isEqual( pCursor->x(), pCentralNode_->x() ) &&
		(pCursor->order() < upperOrder))
	{
		pCursor = pCursor->nextX();
	}
	pCursor->insertBeforeX( &lowerBound_ );
	pCursor->insertBeforeX( &upperBound_ );

	pCursor = pCentralNode_->nextZ();

	while (isEqual( pCursor->z(), pCentralNode_->z() ) &&
		(pCursor->order() < upperOrder))
	{
		pCursor = pCursor->nextZ();
	}
	pCursor->insertBeforeZ( &lowerBound_ );
	pCursor->insertBeforeZ( &upperBound_ );

	oldEntityX_ = pCentralNode_->x();
	oldEntityZ_ = pCentralNode_->z();

	// now perform the initial shuffle from this location, which is
	// the same spot that setRange(0) would put us in
	upperBound_.oldRange( 0.f );
	lowerBound_.oldRange( 0.f );
	
	upperBound_.shuffleX( oldEntityX_, oldEntityZ_ );
	lowerBound_.shuffleX( oldEntityX_, oldEntityZ_ );
	
	upperBound_.shuffleZ( oldEntityX_, oldEntityZ_ );
	lowerBound_.shuffleZ( oldEntityX_, oldEntityZ_ );
	
	upperBound_.updateOldRange();
	lowerBound_.updateOldRange();
}


/**
 *	This method removes the triggers from the range list, first contracting them
 *	to a point (i.e. radius of 0)
 */
void RangeTrigger::remove()
{
	float our = upperBound_.range();
	float olr = lowerBound_.range();
	upperBound_.setRange( 0 );
	lowerBound_.setRange( 0 );
	this->removeWithoutContracting();
	upperBound_.range( our );
	lowerBound_.range( olr );
}


/**
 *	This method removes the triggers from the range list, without contracting
 *	the nodes.
 */
void RangeTrigger::removeWithoutContracting()
{
	upperBound_.removeFromRangeList();
	lowerBound_.removeFromRangeList();
}


/**
 *	This method shuffles the upper and lower bound triggers.
 *
 *	@param oldX The old value of X (oldX is the central (subject's) oldX).
 *	@param oldZ The old value of Z (oldZ is the central (subject's) oldZ).
 */
void RangeTrigger::shuffleXThenZ( float oldX, float oldZ )
{
	// TODO: Remove this assert after a while
	MF_ASSERT( isEqual( oldEntityX_, oldX ) );
	MF_ASSERT( isEqual( oldEntityZ_, oldZ ) );
	upperBound_.shuffleXThenZ(
			oldX + upperBound_.range(), oldZ + upperBound_.range() );
	lowerBound_.shuffleXThenZ(
			oldX + lowerBound_.range(), oldZ + lowerBound_.range() );
	oldEntityX_ = pCentralNode_->x();
	oldEntityZ_ = pCentralNode_->z();
}


/**
 *	This method shuffles only the trigger nodes that would expand the area
 *	encompassed by this trigger, for a movement in the direction indicated.
 */
void RangeTrigger::shuffleXThenZExpand( bool xInc, bool zInc,
		float oldX, float oldZ )
{
	// TODO: Remove this assert after a while
	MF_ASSERT( isEqual( oldEntityX_, oldX ) );
	MF_ASSERT( isEqual( oldEntityZ_, oldZ ) );
	// It does really matter what arguments are passed here for shuffleX and
	// shuffleZ as they are not used since the triggers do not have
	// FLAG_MAKES_CROSSINGS set.
	RangeTriggerNode* xTrigger = xInc ? &upperBound_ : &lowerBound_;
	xTrigger->shuffleX( oldX + xTrigger->range(), oldZ + xTrigger->range() );
	RangeTriggerNode* zTrigger = zInc ? &upperBound_ : &lowerBound_;
	zTrigger->shuffleZ( oldX + zTrigger->range(), oldZ + zTrigger->range() );
}


/**
 *	This method shuffles only the trigger nodes that would contract the area
 *	encompassed by this trigger, for a movement in the direction indicated.
 */
void RangeTrigger::shuffleXThenZContract( bool xInc, bool zInc,
		float oldX, float oldZ )
{
	// TODO: Remove this assert after a while
	MF_ASSERT( isEqual( oldEntityX_, oldX ) );
	MF_ASSERT( isEqual( oldEntityZ_, oldZ ) );

	// It does really matter what arguments are passed here for shuffleX and
	// shuffleZ as they are not used since the triggers do not have
	// FLAG_MAKES_CROSSINGS set.
	RangeTriggerNode* xTrigger = xInc ? &lowerBound_ : &upperBound_;
	xTrigger->shuffleX( oldX + xTrigger->range(), oldZ + xTrigger->range() );
	RangeTriggerNode* zTrigger = zInc ? & lowerBound_ : &upperBound_;
	zTrigger->shuffleZ( oldX + zTrigger->range(), oldZ + zTrigger->range() );
	oldEntityX_ = pCentralNode_->x();
	oldEntityZ_ = pCentralNode_->z();
}


/**
 *	This method sets the ranges of the upper and lower bound triggers
 *
 *	@param range - range of the param
 */
void RangeTrigger::setRange( float range )
{
	float r = range;
	if (r < 0.001f) r = 0.001f;

	upperBound_.setRange(  r );
	lowerBound_.setRange( -r );
}


/**
 *	Return what kind of triggers these are
 */
BW::string RangeTrigger::debugString() const
{
	return "RangeTrigger";
}


/**
 *	This method determines whether or not the given node is contained
 *	within this trigger.
 */
bool RangeTrigger::contains( RangeListNode * pQuery ) const
{
	float qx = pQuery->x();
	float qz = pQuery->z();
	uint16 qo = pQuery->order();

	float lx = lowerBound_.x();
	float lz = lowerBound_.z();
	uint16 lo = lowerBound_.order();

	float ux = upperBound_.x();
	float uz = upperBound_.z();
	uint16 uo = upperBound_.order();

	return
		(lx < qx || (isEqual( lx, qx ) && lo < qo)) &&
		(qx < ux || (isEqual( qx, ux ) && qo < uo)) &&
		(lz < qz || (isEqual( lz, qz ) && lo < qo)) &&
		(qz < uz || (isEqual( qz, uz ) && qo < uo));
}


/**
 *	This method determines whether or not the given node is contained within
 *	the Z-range of this trigger.
 */
bool RangeTrigger::containsInZ( RangeListNode * pQuery ) const
{
	float qz = pQuery->z();
	uint16 qo = pQuery->order();

	float lz = lowerBound_.z();
	uint16 lo = lowerBound_.order();

	float uz = upperBound_.z();
	uint16 uo = upperBound_.order();

	return
		(lz < qz || (isEqual( lz, qz ) && lo < qo)) &&
		(qz < uz || (isEqual( qz, uz ) && qo < uo));
}

BW_END_NAMESPACE

// range_trigger.cpp
