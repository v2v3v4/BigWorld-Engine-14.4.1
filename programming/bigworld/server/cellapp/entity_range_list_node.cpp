#include "script/first_include.hpp"

#include "entity_range_list_node.hpp"

#include "entity.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityRangeListNode
// -----------------------------------------------------------------------------

/**
 * This is the constructor for the EntityRangeListNode.
 * @param pEntity - entity that is associated with this node
 */
EntityRangeListNode::EntityRangeListNode( Entity * pEntity ) :
	RangeListNode( FLAG_NO_TRIGGERS,
				RangeListFlags(
					FLAG_ENTITY_TRIGGER |
					FLAG_LOWER_AOI_TRIGGER |
					FLAG_UPPER_AOI_TRIGGER |
					FLAG_IS_ENTITY ),
			RANGE_LIST_ORDER_ENTITY ),
	pEntity_( pEntity )
{}

/**
 * This method returns the x position of this node.
 * For entity nodes, this is the entities position
 *
 * @return x position of the node
 */
float EntityRangeListNode::x() const
{
	return pEntity_->position().x;
}

/**
 * This method returns the z position of this node.
 * For trigger nodes, this is the entities position
 *
 * @return z position of the node
 */
float EntityRangeListNode::z() const
{
	return pEntity_->position().z;
}

/**
 *	This method returns the identifier for the EntityRangeListNode.
 *
 *	@return The string identifier of the node.
 */
BW::string EntityRangeListNode::debugString() const
{
	char buf[80];
	bw_snprintf( buf, sizeof(buf), "Entity %d", (int)pEntity_->id() );

	return BW::string( buf );
}

/**
 *	This method returns the entity associated with this node.
 *
 *	@return The entity associated with this node.
 */
Entity* EntityRangeListNode::getEntity() const
{
	return pEntity_;
}


/**
 *	This method moves this node to the maximum z position.
 */
void EntityRangeListNode::remove()
{
	Entity::callbacksPermitted( false );
	Vector3 pos = pEntity_->position();
	float oldZ = pos.z;
	pos.z = FLT_MAX;
	pEntity_->globalPosition_ = pos;

	this->shuffleZ( this->x(), oldZ );
	this->removeFromRangeList();
	Entity::callbacksPermitted( true );
}


/**
 *	This method sets whether or not this node is an AoI trigger. An entity node
 *	transitions from being an AoI trigger to not being so if it gets
 *	AppealRadius triggers.
 */
void EntityRangeListNode::isAoITrigger( bool isAoITrigger )
{
	int changingFlags = FLAG_LOWER_AOI_TRIGGER | FLAG_UPPER_AOI_TRIGGER;

	if (isAoITrigger)
	{
		makesFlags_ = RangeListFlags( makesFlags_ | changingFlags );
	}
	else
	{
		makesFlags_ = RangeListFlags( makesFlags_ & ~changingFlags );
	}
}

BW_END_NAMESPACE

// entity_range_list_node.cpp
