
#include "pch.hpp"
#include "list_search_filters.hpp"
#include "common/popup_menu.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	const int FILTER_VISIBLE = 1;
	const int FILTER_ALL = 2;
	const int FILTER_NAME = 3;
	const int FILTER_TYPE = 4;
	const int FILTER_CHUNK = 5;
	const int FILTER_PATH = 6;
} // anonymous namespace


/**
 *	Constructor.
 *
 *	@param columns	A reference to the active columns. Will hold onto it.
 */
ListSearchFilters::ListSearchFilters( const ListColumns & columns ) :
	columns_( columns ),
	currentFilter_( FILTER_VISIBLE )
{
	BW_GUARD;
}


/**
 *	This method shows the filter selection UI, and handles user input.
 *
 *	@param pParent	Parent window of the filter's UI.
 *	@param pt		Point specifying the top-left corner for the UI.
 */
void ListSearchFilters::doModal( const CWnd & pParent, const CPoint & pt )
{
	BW_GUARD;

	PopupMenu menu;

	menu.addItem( checkedPrefix( FILTER_VISIBLE ) +
							descFromId( FILTER_VISIBLE ), FILTER_VISIBLE );

	menu.addItem( checkedPrefix( FILTER_ALL ) +
							descFromId( FILTER_ALL ), FILTER_ALL );

	menu.addItem( checkedPrefix( FILTER_NAME ) +
							descFromId( FILTER_NAME ), FILTER_NAME );

	menu.addItem( checkedPrefix( FILTER_TYPE ) +
							descFromId( FILTER_TYPE ), FILTER_TYPE );

	menu.addItem( checkedPrefix( FILTER_CHUNK ) +
							descFromId( FILTER_CHUNK ), FILTER_CHUNK );

	menu.addItem( checkedPrefix( FILTER_PATH ) +
							descFromId( FILTER_PATH ), FILTER_PATH );

	int result = menu.doModal( pParent, pt );

	if (result == FILTER_VISIBLE)
	{
		currentFilter_ = result;
		updateAllowedTypes();
	}
	else if (result == FILTER_ALL)
	{
		allowedTypes_.clear();
		currentFilter_ = result;
	}
	else if (result == FILTER_NAME)
	{
		allowedTypes_.clear();
		allowedTypes_.insert( ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_ASSETNAME ) );
		currentFilter_ = result;
	}
	else if (result == FILTER_TYPE)
	{
		allowedTypes_.clear();
		allowedTypes_.insert( ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_ASSETTYPE ) );
		currentFilter_ = result;
	}
	else if (result == FILTER_CHUNK)
	{
		allowedTypes_.clear();
		allowedTypes_.insert( ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_CHUNKID ) );
		currentFilter_ = result;
	}
	else if (result == FILTER_PATH)
	{
		allowedTypes_.clear();
		allowedTypes_.insert( ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_FILEPATH ) );
		currentFilter_ = result;
	}
}


/**
 *	This method updates the filters when the columns change, if necessary.
 */
void ListSearchFilters::updateAllowedTypes()
{
	BW_GUARD;

	if (currentFilter_ == FILTER_VISIBLE)
	{
		allowedTypes_.clear();
		for (ListColumns::const_iterator it = columns_.begin();
			it != columns_.end(); ++it)
		{
			if ((*it).visible())
			{
				allowedTypes_.insert( (*it).type() );
			}
		}
	}
}


/**
 *	This method returns the text description for the current filter.
 *
 *	@return Text description for the current filter.
 */
BW::wstring ListSearchFilters::filterDesc() const
{
	BW_GUARD;

	return descFromId( currentFilter_ );
}


/**
 *	This method returns the popup menu "checked" prefix if the specified filter
 *	is the current filter, or the empty string if it isn't.
 *
 *	@param filter	Desired filter.
 *	@return The popup menu "checked" prefix if the specified filter is the
 *			current filter, or the empty string if it isn't.
 */
BW::wstring ListSearchFilters::checkedPrefix( int filter ) const
{
	BW_GUARD;

	return BW::wstring( currentFilter_ == filter ? L"##" : L"" );
}


/**
 *	This method returns the text description for the specified filter.
 *
 *	@param filter	Desired filter.
 *	@return Text description for the specified filter.
 */
BW::wstring ListSearchFilters::descFromId( int filter ) const
{
	BW_GUARD;

	if (filter == FILTER_VISIBLE )
	{
		return Localise( L"SCENEBROWSER/FILTER_VISIBLE" );
	}
	else if (filter == FILTER_ALL )
	{
		return Localise( L"SCENEBROWSER/FILTER_ALL" );
	}
	else if (filter == FILTER_NAME )
	{
		return Localise( L"SCENEBROWSER/FILTER_ASSETNAME" );
	}
	else if (filter == FILTER_TYPE )
	{
		return Localise( L"SCENEBROWSER/FILTER_ASSETTYPE" );
	}
	else if (filter == FILTER_CHUNK )
	{
		return Localise( L"SCENEBROWSER/FILTER_CHUNK" );
	}
	else if (filter == FILTER_PATH )
	{
		return Localise( L"SCENEBROWSER/FILTER_PATH" );
	}

	return L"";
}

BW_END_NAMESPACE

