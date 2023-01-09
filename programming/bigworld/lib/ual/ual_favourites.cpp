
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"

#include "ual_favourites.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UalFavourites::UalFavourites()
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
UalFavourites::~UalFavourites()
{
	BW_GUARD;
}


/**
 *	This method adds an item to the favourites at the back of the list.
 *
 *	@param item	Item information to add to the favourites.
 *	@return		Data section corresponding to the new item, or NULL on failure.
 */
DataSectionPtr UalFavourites::add( const XmlItem& item )
{
	BW_GUARD;

	if ( !path_.length() || item.empty() )
		return 0;

	// See if it's already in the favourites
	DataSectionPtr dsitem = getItem( item );
	if ( !!dsitem )
		return dsitem;

	// Add it to the favourites
	dsitem = XmlItemList::add( item );
	if ( changedCallback_ )
		(*changedCallback_)();
	return dsitem;
}


/**
 *	This method adds an item to the favourites at a specific position.
 *
 *	@param item	Item information to add to the favourites.
 *	@param atItem	Place "item" before "atItem".
 *	@return		Data section corresponding to the new item, or NULL on failure.
 */
DataSectionPtr UalFavourites::addAt(
	const XmlItem& item,
	const XmlItem& atItem )
{
	BW_GUARD;

	DataSectionPtr dsitem = XmlItemList::addAt( item, atItem );
	if ( changedCallback_ )
		(*changedCallback_)();
	return dsitem;
}


/**
 *	This method removes an item from the favourites.
 *
 *	@param item	Item information to remove from the favourites.
 *	@param callCallback True if the registered callback should be called.
 */
void UalFavourites::remove( const XmlItem& item, bool callCallback )
{
	BW_GUARD;

	XmlItemList::remove( item );
	if ( callCallback && changedCallback_ )
		(*changedCallback_)();
}


/**
 *	This method removes all items from the favourites.
 */
void UalFavourites::clear()
{
	BW_GUARD;

	XmlItemList::clear();
	if ( changedCallback_ )
		(*changedCallback_)();
}

BW_END_NAMESPACE

