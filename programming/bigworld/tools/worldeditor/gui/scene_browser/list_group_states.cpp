
#include "pch.hpp"
#include "list_group_states.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ListGroupStates::ListGroupStates() :
	pGroupBy_( NULL )
{
	BW_GUARD;
}


/**
 *	Destructor
 */
ListGroupStates::~ListGroupStates()
{
	BW_GUARD;

	bw_safe_delete(pGroupBy_);
}


/**
 *	This method is used to set the current group for grouping items.
 *
 *	@param pGroup	Group info to use for grouping.
 */
void ListGroupStates::groupBy( ListGroup * pGroup )
{
	BW_GUARD;

	bw_safe_delete(pGroupBy_);
	if (pGroup)
	{
		pGroupBy_ = new ListGroup( *pGroup );
	}
}


/**
 *	This method finds out if a group is expanded or collapsed.
 *
 *	@param group	Group name.
 *	@return			True if the group is expanded, false otherwise.
 */
bool ListGroupStates::groupExpanded( const BW::string & group ) const
{
	BW_GUARD;

	GroupStateMap::const_iterator it = groupStates_.find( group );
	bool expanded = true;
	if (it != groupStates_.end())
	{
		expanded = (*it).second;
	}
	return expanded;
}


/**
 *	This method finds out if a list item is a group start, and therefore a 
 *	GroupItem item. It relies in the fact that groups are always sorted.
 *
 *	@param items	Items that we are grouping.
 *	@param index	Item's index.
 *	@return			True if the index is a group item, false otherwise.
 */
bool ListGroupStates::isGroupStart( const ItemIndex & items, int index ) const
{
	BW_GUARD;

	if (!pGroupBy_ || index == -1)
	{
		return false;
	}
	else if (index == 0)
	{
		return true;
	}

	ItemInfoDB::ItemPtr pPrevItem = items[ index - 1 ];
	ItemInfoDB::ItemPtr pItem = items[ index ];

	return pPrevItem->propertyAsString( pGroupBy_->second ) !=
								pItem->propertyAsString( pGroupBy_->second );
}


/**
 *	This helper method expands or collapses a groups.
 *
 *	@param group		Group name.
 *	@param doExpand		True to expand all groups, false to collapse them.
 */
void ListGroupStates::expandCollapse( const BW::string & group,
																bool doExpand )
{
	BW_GUARD;

	if (pGroupBy_)
	{
		GroupStateMap::iterator itGrp = groupStates_.find( group );
		if (itGrp != groupStates_.end())
		{
			(*itGrp).second = doExpand;
		}
		else
		{
			groupStates_.insert(
				GroupStatePair( group, doExpand ) );
		}
	}
}


/**
 *	This helper method expands or collapses all groups.
 *
 *	@param items		Items that we are grouping.
 *	@param doExpand		True to expand all groups, false to collapse them.
 */
void ListGroupStates::expandCollapseAll( const ItemIndex & items,
											bool doExpand )
{
	BW_GUARD;

	if (pGroupBy_)
	{
		BW::string prevGroup;
		for (ItemIndex::const_iterator itItem = items.begin();
			itItem != items.end(); ++itItem)
		{
			BW::string group =
							(*itItem)->propertyAsString( pGroupBy_->second );
			if (itItem == items.begin() || prevGroup != group)
			{
				// It's a group item, so set its state.
				GroupStateMap::iterator itGrp = groupStates_.find( group );
				if (itGrp != groupStates_.end())
				{
					(*itGrp).second = doExpand;
				}
				else
				{
					groupStates_.insert(
						GroupStatePair( group, doExpand ) );
				}
			}
			prevGroup = group;
		}
	}
}


/**
 *	This method toggles the expanded/collapsed state of a group.
 *
 *	@param items	Items that we are grouping.
 *	@param group	Group to toggle.
 */
void ListGroupStates::handleGroupClick( const ItemIndex & items,
													const BW::string & group )
{
	BW_GUARD;

	if (!group.empty())
	{
		GroupStateMap::iterator itGrp = groupStates_.find( group );

		if (GetAsyncKeyState( VK_SHIFT ) < 0 ||
			GetAsyncKeyState( VK_CONTROL ) < 0)
		{
			// modifier key pressed, expand/collapse all.
			bool newIsExpanded = false;
			if (itGrp != groupStates_.end())
			{
				newIsExpanded = !(*itGrp).second;
			}

			expandCollapseAll( items, newIsExpanded );
		}
		else
		{
			// modifier key not pressed, expand/collapse just one.
			if (itGrp != groupStates_.end())
			{
				(*itGrp).second = !(*itGrp).second;
			}
			else
			{
				groupStates_.insert(
								GroupStatePair( group, false ) );
			}
		}
	}
}

BW_END_NAMESPACE

