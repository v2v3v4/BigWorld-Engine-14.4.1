#ifndef ENTITY_RANGE_LIST_NODE_HPP
#define ENTITY_RANGE_LIST_NODE_HPP

#include "range_list_node.hpp"


BW_BEGIN_NAMESPACE

class Entity;

/**
 *	This class is used as an entity's entry into the range list. The position
 *	of this node is the same as the entity's position. When the entity moves,
 *	this node may also move along the x/z lists.
 */
class EntityRangeListNode : public RangeListNode
{
public:
	EntityRangeListNode( Entity * entity );

	float x() const;
	float z() const;

	BW::string debugString() const;
	Entity * getEntity() const;

	void remove();

	void isAoITrigger( bool isAoITrigger );

	static Entity * getEntity( RangeListNode * pNode )
	{
		MF_ASSERT( pNode->isEntity() );
		return static_cast< EntityRangeListNode * >( pNode )->getEntity();
	}

	static const Entity * getEntity( const RangeListNode * pNode )
	{
		MF_ASSERT( pNode->isEntity() );
		return static_cast< const EntityRangeListNode * >( pNode )->getEntity();
	}

protected:
	Entity * pEntity_;
};

BW_END_NAMESPACE

#endif // ENTITY_RANGE_LIST_NODE_HPP
