
#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "list_multi_provider.hpp"
#include "thumbnail_manager.hpp"

#include "common/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	Section: ListMultiProvider::ListItem
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor.
 *
 *	@param provider	Source provider for the item.
 *	@param index	Index of the item in the list.
 */
ListMultiProvider::ListItem::ListItem( ListProviderPtr provider, int index ) :
	provider_( provider ),
	index_( index ),
	inited_( false )
{
	BW_GUARD;
}


/**
 *	This method returns the cached text for the item, or asks the provider for
 *	it, caches it and then returns it.
 *
 *	@return		Text corresponding to this list item.
 */
const wchar_t* ListMultiProvider::ListItem::text() const
{
	BW_GUARD;

	if ( !inited_ )
	{
		// The asset info is not yet cached, so cache it
		text_ = provider_->getAssetInfo( index_ ).text();
		inited_ = true;
	}
	return text_.c_str();
}


///////////////////////////////////////////////////////////////////////////////
//	Section: ListMultiProvider
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor.
 */
ListMultiProvider::ListMultiProvider() :
	lastNumItems_( 0 )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
ListMultiProvider::~ListMultiProvider()
{
	BW_GUARD;
}


/**
 *	This method relays the "refresh" call to the subproviders.
 */
void ListMultiProvider::refresh()
{
	BW_GUARD;

	// refresh all providers
	for( ProvVec::iterator i = providers_.begin();
		i != providers_.end(); ++i )
	{
		(*i)->refresh();
	}
	// and refresh the items_ vector with the new data
	fillItems();
}


/**
 *	This method relays the "finished" call to the subproviders.
 *
 *	@return	True of all subproviders have finished, false otherwise.
 */
bool ListMultiProvider::finished()
{
	BW_GUARD;

	for( ProvVec::iterator i = providers_.begin();
		i != providers_.end(); ++i )
	{
		if ( !(*i)->finished() )
		{
			// at least one provider hasen't finished, so return false
			return false;
		}
	}
	return true;
}


/**
 *	This method relays the "getNumItems" call to the subproviders.
 *
 *	@return	Sum of the number of items of all subproviders.
 */
int ListMultiProvider::getNumItems()
{
	BW_GUARD;

	int total = 0;
	for( ProvVec::iterator i = providers_.begin();
		i != providers_.end(); ++i )
	{
		total += (*i)->getNumItems();
	}
	// return the sum of the num of items of each provider
	return total;
}


/**
 *	This method returns the item's info by asking the item's provider.
 *
 *	@param index	Index of the item in the list.
 *	@return	Item information, or an empty AssetInfo struct if not found.
 */
const AssetInfo ListMultiProvider::getAssetInfo( int index )
{
	BW_GUARD;

	updateItems();
	if ( index < 0 || index >= (int)items_.size() )
		return AssetInfo();

	// gets the item directly from the provider, using the provider_ and 
	// index_ members of the element 'index' of the items_ vector.
	return items_[ index ].provider()->getAssetInfo( items_[ index ].index() );
}


/**
 *	This method returns the item's thumbnail by asking the item's provider.
 *
 *	@param manager	Reference to the thumbnail manager object in use.
 *	@param index	Index of the item in the list.
 *	@param img		Returns here the thumbnail for the item.
 *	@param w		Desired width for the thumbnail.
 *	@param h		Desired height for the thumbnail.
 *	@param updater	Thumbnail creation callback object.
 */
void ListMultiProvider::getThumbnail( ThumbnailManager& manager,
										int index, CImage& img, int w, int h,
										ThumbnailUpdater* updater )
{
	BW_GUARD;

	updateItems();
	if ( index < 0 || index >= (int)items_.size() )
		return;

	// gets the thumb directly from the provider, using the provider_ and 
	// index_ members of the element 'index' of the items_ vector.
	items_[ index ].provider()->getThumbnail( manager,
		items_[ index ].index(),
		img, w, h, updater );
}


/**
 *	This method filters items by asking each subprovider to filter its items.
 */
void ListMultiProvider::filterItems()
{
	BW_GUARD;

	// filter all providers
	for( ProvVec::iterator i = providers_.begin();
		i != providers_.end(); ++i )
	{
		(*i)->filterItems();
	}
	// and fill the items_ vector
	fillItems();
}


/**
 *	This method adds a list subprovider to the providers_ vector.
 *
 *	@param provider	Subprovider to add.
 */
void ListMultiProvider::addProvider( ListProviderPtr provider )
{
	BW_GUARD;

	if ( !provider )
		return;

	providers_.push_back( provider );
}


/**
 *	This method updates the items_ vector if the number of items has changed.
 */
void ListMultiProvider::updateItems()
{
	BW_GUARD;

	if ( getNumItems() != lastNumItems_ )
	{
		fillItems();
		lastNumItems_ = getNumItems();
	}
}


/**
 *	This static method is used as a sorting callback for std::sort.
 *	
 *	@param a	First item to compare.
 *	@param b	The other item to compare against.
 *	@return		True if a is less than b, false otherwise.
 */
bool ListMultiProvider::s_comparator( const ListItem& a, const ListItem& b )
{
	BW_GUARD;

	// If both items are in the same provider, compare by index because items
	// are sorted in each provider.
	if ( a.provider() == b.provider() )
		return a.index() < b.index();

	// different providers, so compare the filenames
	return wcsicmp( a.text(), b.text() ) < 0;
}


/**
 *	This method fills this provider's items_ vector with the items of each of
 *	its subproviders and sorts them.
 */
void ListMultiProvider::fillItems()
{
	BW_GUARD;

	items_.clear();
	lastNumItems_ = getNumItems();
	if ( !lastNumItems_ || !providers_.size() )
		return;

	// reserve to optimise performance.
	items_.reserve( lastNumItems_ );

	for( ProvVec::iterator p = providers_.begin();
		p != providers_.end(); ++p )
	{
		// loop through the providers
		int numItems = (*p)->getNumItems();
		ListProviderPtr listProv = *p;
		for( int i = 0; i < numItems; ++i )
		{
			// push back all the items from the provider
			items_.push_back( ListItem( listProv, i ) );
		}
	}
	// finally, sort the vector
	std::sort< BW::vector<ListItem>::iterator >(
		items_.begin(), items_.end(), s_comparator );
}

BW_END_NAMESPACE

