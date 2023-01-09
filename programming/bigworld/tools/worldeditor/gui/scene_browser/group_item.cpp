
#include "pch.hpp"
#include "group_item.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 *
 *	@param group	Group name, matching the type (chunk id, filename, etc).
 *	@param type		Type of the grouping property.
 */
GroupItem::GroupItem(	const BW::string & group,
										ItemInfoDB::Type type ) :
	ItemInfoDB::Item( "", NULL, 0, 0, "", "", "",
		ItemInfoDB::ItemProperties() ),
	group_( group ),
	type_( type ),
	numItems_( 0 )
{
	BW_GUARD;
}


/**
 *	This method handles returning group-level property strings depending on the
 *	requested type.
 *
 *	@param type		Type of the desired property.
 *	@return		The property string if it's a group-level property, "" if not.
 */
BW::string GroupItem::propertyAsString( const ItemInfoDB::Type & type ) const
{
	BW_GUARD;

	static ValueType s_valueTypeInt( ValueTypeDesc::INT );

	BW::string property;
	if (type == type_)
	{
		return group_;
	}
	else if (type == ItemInfoDB::Type::builtinType(
										ItemInfoDB::Type::TYPEID_NUMTRIS ))
	{
		s_valueTypeInt.toString( numTris_, property );
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_NUMPRIMS ))
	{
		s_valueTypeInt.toString( numPrimitives_, property );
	}
	else if (type == groupNumItemsType())
	{
		s_valueTypeInt.toString( numItems_, property );
	}
	else if (type == groupNameType())
	{
		return group_;
	}

	return property;
}


/**
 *	This method sets the number of items in the group.
 */
void GroupItem::numItems( int numItems )
{
	BW_GUARD;

	numItems_ = numItems;
}


/**
 *	This method sets the number of triangles in the group.
 */
void GroupItem::numTris( int numTris )
{
	BW_GUARD;

	numTris_ = numTris;
}


/**
 *	This method sets the number of primitives in the group.
 */
void GroupItem::numPrimitives( int numPrims )
{
	BW_GUARD;

	numPrimitives_ = numPrims;
}


/**
 *	This static member returns the type of the group-only property that stores
 *	the number of items in the group.
 *
 *	@return	Type of the property for the number of items in the group.
 */
/*static*/
ItemInfoDB::Type GroupItem::groupNumItemsType()
{
	BW_GUARD;

	return ItemInfoDB::Type( "builtin_groupNumItems", ValueTypeDesc::STRING );
}



/**
 *	This static member returns the type of the group-only property that stores
 *	the name of the group.
 *
 *	@return	Type of the property for the name of the group.
 */
/*static*/
ItemInfoDB::Type GroupItem::groupNameType()
{
	BW_GUARD;

	return ItemInfoDB::Type( "builtin_groupName", ValueTypeDesc::STRING );
}

BW_END_NAMESPACE

