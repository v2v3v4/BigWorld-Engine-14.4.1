
#include "pch.hpp"
#include "column_menu_item.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ColumnMenuItem::ColumnMenuItem( const BW::string & item,
		const BW::string & prefix, const BW::string & owner, int colIdx ) :
	item_( item ),
	prefix_( prefix ),
	owner_( owner ),
	colIdx_( colIdx )
{
	BW_GUARD;
}


/**
 *	This method compares two ColumnMenuItems for sorting.
 */
bool ColumnMenuItem::operator<( const ColumnMenuItem & other ) const
{
	BW_GUARD;

	if (owner_.empty() && !other.owner_.empty())
	{
		return true;
	}
	else if (!owner_.empty() && other.owner_.empty())
	{
		return false;
	}

	int ownerCmp = owner_.compare( other.owner_ );
	if (ownerCmp < 0)
	{
		return true;
	}
	else if (ownerCmp == 0)
	{
		return item_.compare( other.item_ ) < 0;
	}

	return false;
}
BW_END_NAMESPACE

