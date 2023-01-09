#include "script/first_include.hpp"

#include "mobile_range_list_node.hpp"

#include "entity.hpp"
#include "range_trigger.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MobileRangeListNode
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for the MobileRangeListNode.
 *
 *	@param x	the initial X position of this node
 *	@param z	the initial Z position of this node
 *	@param wantsFlags	the flags which must match for this node to be crossed
 *	@param makesFlags	the flags which must match for this node to cross another
 *	@param order	the order of updating of this node.
 */
MobileRangeListNode::MobileRangeListNode( float x, float z,
		RangeListFlags wantsFlags, RangeListFlags makesFlags,
		RangeListOrder order ) :
	RangeListNode( wantsFlags, makesFlags, order ),
	x_( x ),
	z_( z )
{
}


/**
 *	This method returns the x position of this node.
 *
 *	@return	x position of the node
 *	@see	RangeListNode::x()
 */
float MobileRangeListNode::x() const
{
	return x_;
}


/**
 *	This method returns the z position of this node.
 *
 *	@return	z position of the node
 *	@see	RangeListNode::z()
 */
float MobileRangeListNode::z() const
{
	return z_;
}


/**
 *	This method returns the identifier for the MobileRangeListNode.
 *
 *	@return	The string identifier of the node.
 *	@see	RangeListNode::debugString()
 */
BW::string MobileRangeListNode::debugString() const
{
	char buf[80];
	bw_snprintf( buf, sizeof(buf), "Mobile (%f,%f)", x_, z_ );

	return BW::string( buf );
}


/**
 *	This method updates the X position of this node.
 *
 *	@param newX	The new X position of this node
 *	@param newZ	The new Z position of this node
 */
void MobileRangeListNode::setPosition( float newX, float newZ )
{
	float oldX = x_;
	float oldZ = z_;

	// Make sure that no controllers are cancelled while doing this.
	// (And that no triggers are added/deleted/modified!)
	Entity::callbacksPermitted( false );

	x_ = newX;
	z_ = newZ;

	// check if upper triggers should move first or lower ones
	bool didIncreaseX = (oldX < newX);
	bool didIncreaseZ = (oldZ < newZ);

	// shuffle the leading triggers
	for (Triggers::iterator it = triggers_.begin(); it != triggers_.end(); it++)
	{
		(*it)->shuffleXThenZExpand( didIncreaseX, didIncreaseZ, oldX, oldZ );
	}

	// shuffle ourself
	this->shuffleXThenZ( oldX, oldZ );

	// shuffle the trailing triggers
	for (Triggers::reverse_iterator it = triggers_.rbegin();
			it != triggers_.rend(); it++)
	{
		(*it)->shuffleXThenZContract( didIncreaseX, didIncreaseZ, oldX, oldZ );
	}

	// TODO: Even with this sorting this is broken if there is >1 trigger
	// with the same range :(

	Entity::callbacksPermitted( true );
}


/**
 *	This method moves this node to the maximum z position, and then removes
 *	itself from its RangeLists.
 */
void MobileRangeListNode::remove()
{
	if ((wantsFlags_ && 0xf) == FLAG_NO_TRIGGERS &&
		(makesFlags_ && 0xf) == FLAG_NO_TRIGGERS)
	{
		// Fulfill the method description...
		z_ = FLT_MAX;
		// No need to shuffle, we don't trigger anyway.
		this->removeFromRangeList();
		return;
	}

	Entity::callbacksPermitted( false );

	float oldZ = z_;
	z_ = FLT_MAX;

	// Don't bother shuffling X, moving to maximum Z should cross all possible
	// nodes.
	this->shuffleZ( x_, oldZ );
	this->removeFromRangeList();

	Entity::callbacksPermitted( true );
}


/**
 *	This methods adds the given trigger to those that move with us.
 *
 *	@param pTrigger	The RangeTrigger to add to our collection
 */
void MobileRangeListNode::addTrigger( RangeTrigger * pTrigger )
{
	Triggers::iterator it;

	for (it = triggers_.begin(); it != triggers_.end(); ++it)
	{
		if ((*it)->range() <= pTrigger->range())
		{
			break;
		}
	}

	triggers_.insert( it, pTrigger );
}


/**
 *	Notify us that the given trigger has had its range modified
 *
 *	@param pTrigger	The RangeTrigger in our collection to modify
 */
void MobileRangeListNode::modTrigger( RangeTrigger * pTrigger )
{
	// ideally would do a bubble-sort kind of operation
	this->delTrigger( pTrigger );
	this->addTrigger( pTrigger );
}


/**
 *	Remove the given trigger from those that move with us
 *
 *	@param pTrigger	The RangeTrigger in our collection to remove
 */
void MobileRangeListNode::delTrigger( RangeTrigger * pTrigger )
{
	Triggers::iterator found = std::find(
		triggers_.begin(), triggers_.end(), pTrigger );
	MF_ASSERT( found != triggers_.end() );
	triggers_.erase( found );
}


BW_END_NAMESPACE

// entity_range_list_node.cpp
