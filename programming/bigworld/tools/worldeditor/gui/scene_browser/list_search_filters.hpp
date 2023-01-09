#ifndef LIST_SEARCH_FILTERS_HPP
#define LIST_SEARCH_FILTERS_HPP


#include "world/item_info_db.hpp"
#include "list_column.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class keeps track of the current filtering and handles the filters
 *	UI and user input.
 */
class ListSearchFilters
{
public:
	typedef BW::set< ItemInfoDB::Type > Types;

	ListSearchFilters( const ListColumns & columns );

	void doModal( const CWnd & pParent, const CPoint & pt );
	
	const Types & allowedTypes() const { return allowedTypes_; }

	void updateAllowedTypes();

	BW::wstring filterDesc() const;

private:
	const ListColumns & columns_;
	int currentFilter_;
	Types allowedTypes_;

	BW::wstring checkedPrefix( int filter ) const;

	BW::wstring descFromId( int filter ) const;
};

BW_END_NAMESPACE

#endif // LIST_SEARCH_FILTERS_HPP
