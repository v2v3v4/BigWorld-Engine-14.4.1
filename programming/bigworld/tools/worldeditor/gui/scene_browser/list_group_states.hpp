#ifndef LIST_GROUP_STATES_HPP
#define LIST_GROUP_STATES_HPP


#include "world/item_info_db.hpp"

BW_BEGIN_NAMESPACE

typedef std::pair< BW::string, ItemInfoDB::Type > ListGroup;
typedef BW::vector< ListGroup > ListGroups;


/**
 *	This helper class deals with list groups and group-related operations.
 */
class ListGroupStates
{
public:
	typedef BW::vector< ItemInfoDB::ItemPtr > ItemIndex;

	ListGroupStates();
	~ListGroupStates();

	void groupBy( ListGroup * group );
	const ListGroup * groupBy() const { return pGroupBy_; }
	void groups( ListGroups & retGroups ) const;

	ItemInfoDB::Type groupByType() const
				{ return pGroupBy_ ? pGroupBy_->second : ItemInfoDB::Type(); }

	bool groupExpanded( const BW::string & group ) const;

	bool isGroupStart( const ItemIndex & items, int index ) const;

	void expandCollapse( const BW::string & group, bool doExpand );

	void expandCollapseAll( const ItemIndex & items, bool doExpand );
	
	void handleGroupClick( const ItemIndex & items,
												const BW::string & group );

private:
	typedef std::pair< BW::string, bool > GroupStatePair;
	typedef BW::map< BW::string, bool > GroupStateMap;

	ListGroup * pGroupBy_;
	GroupStateMap groupStates_;
};

BW_END_NAMESPACE

#endif // LIST_GROUP_STATES_HPP
