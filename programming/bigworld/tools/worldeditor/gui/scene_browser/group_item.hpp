#ifndef GROUP_ITEM_HPP
#define GROUP_ITEM_HPP


#include "world/item_info_db.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This subclass stores info for a group of items, and returns the
 *	appropriate group-level info for the list row it occupies.
 */
class GroupItem : public ItemInfoDB::Item
{
public:
	GroupItem( const BW::string & group, ItemInfoDB::Type type );

	BW::string propertyAsString( const ItemInfoDB::Type & type ) const;

	void numItems( int numItems );

	void numTris( int numTris );

	void numPrimitives( int numPrims );

	static ItemInfoDB::Type groupNumItemsType();

	static ItemInfoDB::Type groupNameType();

private:
	BW::string group_;
	ItemInfoDB::Type type_;
	int numItems_;
};

BW_END_NAMESPACE

#endif //GROUP_ITEM_HPP