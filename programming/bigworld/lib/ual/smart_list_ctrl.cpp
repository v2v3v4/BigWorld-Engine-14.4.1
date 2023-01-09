
#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "time.h"
#include "ual_resource.h"
#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "filter_holder.hpp"
#include "smart_list_ctrl.hpp"
#include "resmgr/string_provider.hpp"
#include "common/string_utils.hpp"
#include "controls/drag_image.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param thumbnailManager		Class that provides the thumbnail images for
 *								the list's items.
 */
SmartListCtrl::SmartListCtrl( ThumbnailManagerPtr thumbnailManager ) :
	style_( SmartListCtrl::BIGICONS ),
	provider_( 0 ),
	thumbnailManager_( thumbnailManager ),
	listCache_( &listCacheBig_ ),
	dragImgList_( 0 ),
	dragImage_( NULL ),
	dragging_( false ),
	generateDragListEndItem_( false ),
	lastListDropItem_( -1 ),
	lastItemChanged_( -1 ),
	ignoreSelMessages_( false ),
	listViewIcons_( true ),
	thumbWidthSmall_( 16 ),
	thumbHeightSmall_( 16 ),
	thumbWidthCur_( 0 ),
	thumbHeightCur_( 0 ),
	customItems_( 0 ),
	eventHandler_( 0 ),
	maxSelUpdateMsec_( 50 ),
	delayedSelectionPending_( false ),
	redrawPending_( false ),
	maxItems_( 200 )
{
	BW_GUARD;

	MF_ASSERT( thumbnailManager_ != NULL );
	thumbWidth_ = thumbnailManager_->size();
	thumbHeight_ = thumbnailManager_->size();
}


/**
 *	Destructor.
 */
SmartListCtrl::~SmartListCtrl()
{
	BW_GUARD;

	thumbnailManager_->resetPendingRequests( this );

	bw_safe_delete(dragImgList_);
	bw_safe_delete(dragImage_);
}


/**
 *	This method returns the current display style for items.
 *
 *	@return	Current style, see SmartListCtrl::ViewStyle for more info.
 */
SmartListCtrl::ViewStyle SmartListCtrl::getStyle()
{
	BW_GUARD;

	return style_;
}


/**
 *	This method changes the current display style for items.
 *
 *	@param style	New style, see SmartListCtrl::ViewStyle for more info.
 */
void SmartListCtrl::setStyle( ViewStyle style )
{
	BW_GUARD;

	style_ = style;
	static CImageList dummyImgList;
	static const int IMGLIST_FORMAT = ILC_COLOR24|ILC_MASK;

	thumbnailManager_->resetPendingRequests( this );

	if ( !dummyImgList.GetSafeHandle() )
	{
		dummyImgList.Create( 1, 1, IMGLIST_FORMAT, 0, 0 );
		dummyImgList.SetBkColor( GetBkColor() );
	}

	// delete previous image list
	SetImageList( &dummyImgList, LVSIL_NORMAL );
	SetImageList( &dummyImgList, LVSIL_SMALL );
	SetImageList( &dummyImgList, LVSIL_STATE );

	CImageList* imgListPtr = 0;
	// set thumbnail size according to list style
	DWORD wstyle = GetWindowLong( GetSafeHwnd(), GWL_STYLE );
	// hack: have to force change the list view style so tooltip cache resets.
	SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_REPORT );
	wstyle = GetWindowLong( GetSafeHwnd(), GWL_STYLE );
	if ( style_ == BIGICONS )
	{
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_ICON );
		listViewIcons_ = true;
		thumbWidthCur_ = thumbWidth_;
		thumbHeightCur_ = thumbHeight_;
		listCache_ = &listCacheBig_;
		listCache_->setMaxItems( maxItems_ );
		imgListPtr = &imgListBig_;
	}
	else if ( style_ == SMALLICONS )
	{
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_LIST );
		listViewIcons_ = true;
		thumbWidthCur_ = thumbWidthSmall_;
		thumbHeightCur_ = thumbHeightSmall_;
		// Since small icons take less space, up the max cache items (by 16 if
		// big thumbs are 64x64 and small thumbs are 16x16, for example) to
		// take advantage of the same memory space.
		int memoryMultiplier =
			( thumbWidth_ * thumbHeight_ ) /
			( thumbWidthSmall_ * thumbHeightSmall_ );
		listCache_ = &listCacheSmall_;
		listCache_->setMaxItems( maxItems_ * memoryMultiplier );
		imgListPtr = &imgListSmall_;
	}
	else
	{
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_LIST );
		listViewIcons_ = false;
		thumbWidthCur_ = 0;
		thumbHeightCur_ = 0;
		imgListPtr = &dummyImgList;
	}

	// create image list if the style requires it
	if ( style_ != LIST )
	{
		if ( !imgListPtr->GetSafeHandle() )
		{
			imgListPtr->Create( thumbWidthCur_, thumbHeightCur_, IMGLIST_FORMAT, 0, 32 );
			imgListPtr->SetBkColor( GetBkColor() );
			imgListPtr->Add( AfxGetApp()->LoadIcon( IDI_UALFILE ) );
			// clear cache
			listCache_->init( imgListPtr, 1 );
		}
	}

	// set the image list
	if ( style_ == BIGICONS )
		SetImageList( imgListPtr, LVSIL_NORMAL );
	else if ( style_ == SMALLICONS )
		SetImageList( imgListPtr, LVSIL_SMALL );

	// clear and start
	SetItemCount( 0 );
	if ( provider_ )
		SetTimer( SMARTLIST_LOADTIMER_ID, SMARTLIST_LOADTIMER_MSEC, 0 );
}


/**
 *	This MFC method is overriden to make sure we set the list control's tooltip
 *	and double-buffer flags.
 */
void SmartListCtrl::PreSubclassWindow()
{
	BW_GUARD;

	SetExtendedStyle( GetExtendedStyle() | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER );
}


/**
 *	This method is called to initialise or reset the list to show items from a
 *	list provider.
 *
 *	@param provider		Object that provider the list with items.
 *	@param customItems	Application-defined custom items to add to the list.
 *	@param clearSelection	True to clear the selection if the list is being 
 *							reset to a new provider, false to keep the
 *							selection if the list is just being refreshed.
 */
void SmartListCtrl::init( ListProviderPtr provider, XmlItemVec* customItems, bool clearSelection )
{
	BW_GUARD;

	provider_ = provider;
	customItems_ = customItems;

	if ( clearSelection )
	{
		bool oldIgnore = ignoreSelMessages_;
		ignoreSelMessages_ = true;
		SetItemState( -1, 0, LVIS_SELECTED );
		selItems_.clear();
		ignoreSelMessages_ = oldIgnore;
	}

	setStyle( getStyle() );
}


/**
 *	This method sets the maximum number of items in the list image cache.
 *
 *	@param maxItems		Maximum number of items in the list image cache.
 */
void SmartListCtrl::setMaxCache( int maxItems )
{
	BW_GUARD;

	maxItems_ = maxItems;
}


/**
 *	This method returns whether or not the list is displaying icons for items.
 *
 *	@return		True if icons are being shown for items, false if not.
 */
bool SmartListCtrl::getListViewIcons()
{
	BW_GUARD;

	return listViewIcons_;
}


/**
 *	This method sets whether or not the list will display icons for items.
 *
 *	@param listViewIcons	True so icons are shown for items, false if not.
 */
void SmartListCtrl::setListViewIcons( bool listViewIcons )
{
	BW_GUARD;

	listViewIcons_ = listViewIcons;
}


/**
 *	This method reloads items from the provider.
 */
void SmartListCtrl::refresh()
{
	BW_GUARD;

	if ( !provider_ )
		return;

	listCacheBig_.clear();
	listCacheSmall_.clear();
	provider_->refresh();
	init( provider_, customItems_, false );
}


/**
 *	This method returns the current list item provider.
 *
 *	@return		The current list item provider.
 */
ListProviderPtr SmartListCtrl::getProvider()
{
	BW_GUARD;

	return provider_;
}


/**
 *	This method returns whether or not all items for the list have been loaded
 *	by asking the provider.
 *
 *	@return		True if the list is finished loading items, false otherwise.
 */
bool SmartListCtrl::finished()
{
	BW_GUARD;

	if ( provider_ )
		return provider_->finished();
	return false;
}


/**
 *	This method returns the custom item at the position position.
 *
 *	@param index	Position of the desired custom item in the list.
 *	@return		The desired custom item, or NULL if not found.
 */
XmlItem* SmartListCtrl::getCustomItem( int& index )
{
	BW_GUARD;

	if ( !customItems_ )
		return 0;

	int topIndex = 0;
	for( XmlItemVec::iterator i = customItems_->begin();
		i != customItems_->end();
		++i )
	{
		if ( (*i).position() != XmlItem::TOP )
			continue;
		if ( topIndex == index )
			return &(*i);
		++topIndex;
	}
	int bottomIndex = topIndex + provider_->getNumItems();
	for( XmlItemVec::iterator i = customItems_->begin();
		i != customItems_->end();
		++i )
	{
		if ( (*i).position() != XmlItem::BOTTOM )
			continue;
		if ( bottomIndex == index )
			return &(*i);
		++bottomIndex;
	}
	index -= topIndex;
	return 0;
}


/**
 *	This method returns whether or not the item at the specified position is
 *	a custom item.
 *
 *	@param index	Position of the item in the list.
 *	@return		True if the item is custom app-defined, false if it comes from
 *				the provider.
 */
bool SmartListCtrl::isCustomItem( int index )
{
	BW_GUARD;

	return getCustomItem( index ) != 0;
}


/**
 *	This method returns all the information about the item at the specified
 *	position.
 *
 *	@param index	Position of the item in the list.
 *	@return		Information about the item, or an empty AssetInfo if the index
 *				is out of bounds.
 */
const AssetInfo SmartListCtrl::getAssetInfo( int index )
{
	BW_GUARD;

	XmlItem* item = getCustomItem( index );
	if ( item )
		return item->assetInfo();

	if ( !provider_ || index >= provider_->getNumItems() )
		return AssetInfo();

	return provider_->getAssetInfo( index );
}


/**
 *	This method is used to internally update an item's image.
 *
 *	@param index	Position of the item in the list.
 *	@param inf		Item's information.
 *	@param removeFromCache	True to remove the items previous images from the
 *							list's image cache.
 */
void SmartListCtrl::updateItemInternal( int index, const AssetInfo& inf, bool removeFromCache )
{
	BW_GUARD;

	if ( !provider_ || index < 0 )
		return;

	if ( removeFromCache )
	{
		listCacheBig_.cacheRemove( inf.text(), inf.longText() );
		listCacheSmall_.cacheRemove( inf.text(), inf.longText() );
	}
	CRect clRect;
	GetClientRect( &clRect );
	CRect rect;
	GetItemRect( index, &rect, LVIR_BOUNDS );
	if ( rect.right >= 0 && rect.bottom >= 0 &&
		rect.left <= clRect.right && rect.top <= clRect.bottom )
	{
		RedrawItems( index, index );
		RedrawWindow( rect, NULL, 0 );
	}
}


/**
 *	This method updates an item's image by index.
 *
 *	@param index	Position of the item in the list.
 *	@param removeFromCache	True to remove the items previous images from the
 *							list's image cache.
 */
void SmartListCtrl::updateItem( int index, bool removeFromCache )
{
	BW_GUARD;

	updateItemInternal( index, getAssetInfo( index ), removeFromCache );
}


/**
 *	This method updates an item's image by the item's information.
 *
 *	@param assetInfo		Item's information.
 *	@param removeFromCache	True to remove the items previous images from the
 *							list's image cache.
 */
void SmartListCtrl::updateItem( const AssetInfo& assetInfo, bool removeFromCache )
{
	BW_GUARD;

	// remove the item from the cache and schedule a redraw
	if ( removeFromCache )
	{
		listCacheBig_.cacheRemove( assetInfo.text(), assetInfo.longText() );
		listCacheSmall_.cacheRemove( assetInfo.text(), assetInfo.longText() );
	}
	if ( !redrawPending_ )
	{
		// only schedule a redraw if one hasn't been scheduled yet
		redrawPending_ = true;
		SetTimer( SMARTLIST_REDRAWTIMER_ID, SMARTLIST_REDRAWTIMER_MSEC, 0 );
	}
	return;
}


/**
 *	This method scrolls the list to the specified item and selects it.
 *
 *	@param assetInfo		Item's information.
 *	@return	True if the item was found, false if not.
 */
bool SmartListCtrl::showItem( const AssetInfo& assetInfo )
{
	BW_GUARD;

	int n = GetItemCount();
	int begin = binSearch( n, 0, n-1, assetInfo ); 
	if ( begin == -1 )
	{
		// binSearch didn't find it, so try from the beginning
		begin = 0;	
	}
	for( int i = begin; i < n; ++i )
	{
		AssetInfo inf = getAssetInfo( i );
		if ( wcsicmp( inf.longText().c_str(), assetInfo.longText().c_str() ) == 0 )
		{
			SetItemState( -1, 0, LVIS_SELECTED );
			SetItemState( i, LVIS_SELECTED, LVIS_SELECTED );
			EnsureVisible( i, FALSE );
			return true;
		}
	}
	return false;
}


/**
 *	This method receives an object that wishes to receive notifications from
 *	this list control.
 *
 *	@param eventHandler		Object wishing to handle notifications.
 */
void SmartListCtrl::setEventHandler( SmartListCtrlEventHandler* eventHandler )
{
	BW_GUARD;

	eventHandler_ = eventHandler;
}


/**
 *	This method sets the default image for items that don't have their own
 *	thumbnail.
 *
 *	@param icon		Image to become the default image for items.
 */
void SmartListCtrl::setDefaultIcon( HICON icon )
{
	BW_GUARD;

	if ( icon && imgListBig_.GetSafeHandle() )
		imgListBig_.Replace( 0, icon );
	if ( icon && imgListSmall_.GetSafeHandle() )
		imgListSmall_.Replace( 0, icon );
}


/**
 *	This method is used to update the list's contents when filters or search
 *	text changes.
 */
void SmartListCtrl::updateFilters()
{
	BW_GUARD;

	if ( !provider_ )
		return;

	provider_->filterItems();
	// hack to force reset of the tooltips
	DWORD wstyle = GetWindowLong( GetSafeHwnd(), GWL_STYLE );
	SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_REPORT );
	wstyle = GetWindowLong( GetSafeHwnd(), GWL_STYLE );
	if ( style_ == BIGICONS )
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_ICON );
	else
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, (wstyle & ~LVS_TYPEMASK) | LVS_LIST );
	// do the actual change
	changeItemCount( provider_->getNumItems() );
}


/**
 *	This method performs a binary search in the list for the specified item,
 *	taking advantage of the fact that the list's items are sorted.
 *
 *	@param size		Number of items on the list.
 *	@param begin	Recurse param, first item index of the segment to search.
 *	@param end		Recurse param, last item index of the segment to search.
 *	@param assetInfo Item we are searching for.
 *	@return		The position of the item on the list, or -1 if not found.
 */
int SmartListCtrl::binSearch( int size, int begin, int end, const AssetInfo& assetInfo )
{
	BW_GUARD;

	if ( size == 0 || end == begin - 1 )
		return -1;	// this values can happen under normal circumstances

	if ( size < 0 || begin < 0 || end < 0 || end < begin - 1
		|| begin >= size || end >= size )
	{
		// border cases that should not happen
		WARNING_MSG( "SmartListCtrl::binSearch: bad parameters size (%d),"
			" begin (%d) and/or end (%d), searching for %s (%s)\n",
			size, begin, end, assetInfo.text().c_str(),
			assetInfo.longText().c_str() );
		return -1;
	}

	int index = (begin + end) / 2;

	if ( index < 0 || index >= size )
	{
		// this should never happen at this stage
		WARNING_MSG( "SmartListCtrl::binSearch: bad index %d searching for %s (%s)\n",
			index, assetInfo.text().c_str(), assetInfo.longText().c_str() );
		return -1;
	}

	AssetInfo inf = getAssetInfo( index );
	int cmp = wcsicmp( inf.text().c_str(), assetInfo.text().c_str() );
	if ( cmp == 0 )
	{
		// found. loop backwards to find the first in case of duplicates
		int i = index;
		while( --i > 0 )
		{
			AssetInfo inf = getAssetInfo( i );
			if ( wcsicmp( inf.text().c_str(), assetInfo.text().c_str() ) == 0 )
				--index;
			else
				break;
		}
		return index;
	}
	else if ( begin < end )
	{
		if ( cmp < 0 )
		{
			return binSearch( size, index+1, end, assetInfo );
		}
		else
		{
			return binSearch( size, begin, index-1, assetInfo );
		}
	}
	return -1;
}


/**
 *	This method obtains the information and image for an item.
 *
 *	@param index	Position of the desired item on the list.
 *	@param text		Return param, display text for the item.
 *	@param image	Return param, index of the item's image on the image list.
 *	@param textOnly	True if the caller only wants the text, true to get the
 *					both the text and the image.
 */
void SmartListCtrl::getData( int index, BW::wstring& text, int& image, bool textOnly /*false*/ )
{
	BW_GUARD;

	if ( !provider_ )
		return;

	if ( generateDragListEndItem_ )
	{
		text = Localise(L"UAL/SMART_LIST_CTRL/MORE");
		image = -1;
		return;
	}

	XmlItem* item = getCustomItem( index );
	if ( item )
	{
		text = item->assetInfo().text();
		if ( !textOnly && thumbWidthCur_ && thumbHeightCur_ )
		{
			image = 0;
			const ListCache::ListCacheElem* elem =
				listCache_->cacheGet( text, item->assetInfo().longText() );
			if ( elem )
			{
				// cache hit
				image = elem->image;
				return;
			}

			// cache miss
			CImage img;
			thumbnailManager_->create( item->assetInfo().thumbnail(),
				img, thumbWidthCur_, thumbHeightCur_, this, true );
			if ( !img.IsNull() )
			{
				elem = listCache_->cachePut( text, item->assetInfo().longText(), img );
				if ( elem )
					image = elem->image;
			}
		}
		return;
	}

	const AssetInfo assetInfo = provider_->getAssetInfo( index );
	text = assetInfo.text();

	if ( !textOnly && thumbWidthCur_ && thumbHeightCur_ )
	{
		image = 0;
		const ListCache::ListCacheElem* elem = 
			listCache_->cacheGet( text, assetInfo.longText() );
		if ( elem )
		{
			// cache hit
			image = elem->image;
			return;
		}

		// cache miss
		CImage img;
		provider_->getThumbnail( *thumbnailManager_, index, img, thumbWidthCur_, thumbHeightCur_, this );

		if ( !img.IsNull() )
		{
			elem = listCache_->cachePut( text, assetInfo.longText(), img );
			if ( elem )
				image = elem->image;
		}
	}
	else
		image = -1;
}


/**
 *	This method changes the number of items on the list, which triggers a
 *	refresh of the list's contents.
 *
 *	@param numItems	New item count.
 */
void SmartListCtrl::changeItemCount( int numItems )
{
	BW_GUARD;

	// flag to avoid sending callback messages when manually selecting items
	bool oldIgnore = ignoreSelMessages_;
	ignoreSelMessages_ = true;
	
	// deselect all
	SetItemState( -1, 0, LVIS_SELECTED );

	// change item count
	int numCustomItems = 0;
	if ( customItems_ )
		numCustomItems = static_cast<int>(customItems_->size());
	SetItemCountEx( numItems + numCustomItems, LVSICF_NOSCROLL );

	// restore selected items
	if ( provider_ )
	{
		clock_t start = clock();
		for( SelectedItemItr s = selItems_.begin();
			s != selItems_.end() && ((clock()-start)*1000/CLOCKS_PER_SEC) < maxSelUpdateMsec_;
			++s )
		{
			for( int item = 0;
				item < numItems && ((clock()-start)*1000/CLOCKS_PER_SEC) < maxSelUpdateMsec_;
				++item )
			{
				if ( (*s).equalTo( provider_->getAssetInfo( item ) ) )
				{
					SetItemState( item, LVIS_SELECTED, LVIS_SELECTED );
					break;
				}
			}
		}
	}
	ignoreSelMessages_ = oldIgnore;
}


/**
 *	This method records the current selection so the selection is preserved as
 *	the item count grows from the provider loading items asynchronously.
 */
void SmartListCtrl::updateSelection()
{
	BW_GUARD;

	if ( !provider_ )
		return;

	// save selected items
	int numSel = GetSelectedCount();
	selItems_.clear();
	selItems_.reserve( numSel );
	int item = -1;
	clock_t start = clock();
	for( int i = 0; i < numSel && ((clock()-start)*1000/CLOCKS_PER_SEC) < maxSelUpdateMsec_; i++ )
	{
		item = GetNextItem( item, LVNI_SELECTED );
		selItems_.push_back( provider_->getAssetInfo( item ) );
	}
}


/**
 *	This method returns whether or not the user is dragging a list item.
 *
 *	@return		True if an item is being dragged by the user, false if not.
 */
bool SmartListCtrl::isDragging()
{
	BW_GUARD;

	return dragging_;
}


/**
 *	This method updates the temporary image created for giving feedback to the 
 *	user for the item he is dragging.
 *
 *	@param x	Current X mouse position.
 *	@param y	Current Y mouse position.
 */
void SmartListCtrl::updateDrag( int x, int y )
{
	BW_GUARD;

	POINT pt = { x, y };
	dragImage_->update( pt );
}


/**
 *	This method is called when the drag and drop operation finishes to hide the
 *	drag image.
 */
void SmartListCtrl::endDrag()
{
	BW_GUARD;

	bw_safe_delete(dragImage_);
	bw_safe_delete(dragImgList_);
	dragging_ = false;
}


/**
 *	This method highlights an item and sets it as a drop target for when
 *	dragging list items into the list itself.  This can be useful for some
 *	kinds of lists that allow the user to move items around (the Asset
 *	Browser's favourite list for example).
 *
 *	@param index	Position of the desired item on the list.
 */
void SmartListCtrl::setDropTarget( int index )
{
	BW_GUARD;

	bool oldIgnore = ignoreSelMessages_;
	ignoreSelMessages_ = true;
	SetItemState( index, LVIS_DROPHILITED, LVIS_DROPHILITED );
	if ( index != lastListDropItem_ )
	{
		RedrawItems( index, index );
		if ( lastListDropItem_ != -1 ) 
		{
			SetItemState( lastListDropItem_, 0, LVIS_DROPHILITED );
			RedrawItems( lastListDropItem_, lastListDropItem_ );
		}
		UpdateWindow();
		lastListDropItem_ = index;
	}
	ignoreSelMessages_ = oldIgnore;
}


/**
 *	This method removes any highlighting and stops having an item as a drop
 *	target.
 */
void SmartListCtrl::clearDropTarget()
{
	BW_GUARD;

	if ( lastListDropItem_ != -1 )
	{
		bool oldIgnore = ignoreSelMessages_;
		ignoreSelMessages_ = true;
		SetItemState( lastListDropItem_, 0, LVIS_DROPHILITED );
		RedrawItems( lastListDropItem_, lastListDropItem_ );
		UpdateWindow();
		lastListDropItem_ = -1;
		ignoreSelMessages_ = oldIgnore;
	}
}


/**
 *	This method enables or disables allowing mutliple selections in the list.
 *	An example of a list with multiple selections is Model Editor's Animations
 *	folder in its Asset Browser.
 *
 *	@param allow	True to allow the user to select more than one item.
 */
void SmartListCtrl::allowMultiSelect( bool allow )
{
	BW_GUARD;

	DWORD wstyle = GetWindowLong( GetSafeHwnd(), GWL_STYLE );
	if ( allow )
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, wstyle & ~LVS_SINGLESEL );
	else
		SetWindowLong( GetSafeHwnd(), GWL_STYLE, wstyle | LVS_SINGLESEL );
}


/**
 *	This method is used to receiv notification from the thumbnail manager when
 *	an item's thumbnail is ready for display.
 *
 *	@param longText	Full text for the item, usually a file path.
 */
void SmartListCtrl::thumbManagerUpdate( const BW::wstring& longText )
{
	BW_GUARD;

	if ( !GetSafeHwnd() || longText.empty() )
		return;

	BW::wstring longTextTmp = longText;
	std::replace( longTextTmp.begin(), longTextTmp.end(), L'/', L'\\' );
	BW::wstring textTmp = longTextTmp.c_str() + longTextTmp.find_last_of( L'\\' ) + 1;

	updateItem(
		AssetInfo( L"", textTmp, longTextTmp ),
		false /* dont try to remove from cache, it's not there */ );
}


// MFC Messages
BEGIN_MESSAGE_MAP(SmartListCtrl, CListCtrl)
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_NOTIFY_REFLECT( NM_RCLICK, OnRightClick )
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM, OnOdFindItem)
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(LVN_ODSTATECHANGED, OnOdStateChanged)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChanged)
	ON_NOTIFY_REFLECT(NM_CLICK, OnItemClick)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
	ON_WM_LBUTTONDBLCLK()
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnToolTipText)
END_MESSAGE_MAP()


/**
 *	This MFC method handles resizing to ensure that items are redrawn, to avoid
 *	visual artifacts on the control.
 *
 *	@param nType	MFC resize type.
 *	@param cx		MFC new width of the control.
 *	@param cy		MFC new height of the control.
 */
void SmartListCtrl::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CListCtrl::OnSize( nType, cx, cy );
	RedrawWindow( NULL, NULL, RDW_INVALIDATE );
}


/**
 *	This MFC method is overriden to manually handle a few key presses, such as
 *	'Ctrl+A' for selecting all items on the list, and to relay 'Delete' events
 *	to the handler to, for example, delete an item from a list that allows
 *	deletion (such as the History and Favourites lists of the Asset Browser).
 *
 *	@param nChar	MFC key code.
 *	@param nRepCnt	MFC repetition count.
 *	@param nFlags	MFC event flags.
 */
void SmartListCtrl::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	BW_GUARD;

	if ( nChar == 'A' && GetAsyncKeyState( VK_CONTROL ) < 0 )
	{
		// select all
		SetItemState( -1, LVIS_SELECTED, LVIS_SELECTED );
		updateSelection();
		return;
	}
	else if ( nChar == VK_DELETE && eventHandler_ )
	{
		eventHandler_->listItemDelete();
		return;
	}
	CListCtrl::OnKeyDown( nChar, nRepCnt, nFlags );
}


/**
 *	This MFC method relays right-click events to the event handler.
 *
 *	@param pNotifyStruct MFC notify struct.
 *	@param result	MFC result.
 */
void SmartListCtrl::OnRightClick( NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	if ( eventHandler_ )
	{
		int item = GetNextItem( -1, LVNI_FOCUSED );
/*		ignoreSelMessages_ = true;
		SetItemState( -1, 0, LVIS_SELECTED );
		if ( item >= 0 )
			SetItemState( item, LVIS_SELECTED, LVIS_SELECTED );
		updateSelection();
		ignoreSelMessages_ = false;*/
		eventHandler_->listItemRightClick( item );
		return;
	}
}


/**
 *	This MFC method is called on an MFC virtual list when an item is redrawn,
 *	so we can provide MFC with all it needs to display the item.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnGetDispInfo( NMHDR* pNMHDR, LRESULT* pResult )
{
	BW_GUARD;

    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* pItem = &(pDispInfo)->item;

	BW::wstring text;
	int iImage;
	getData( pItem->iItem, text, iImage, !(pItem->mask & LVIF_IMAGE) );

	if ( pItem->mask & LVIF_TEXT )
	{
		wcsncpy( pItem->pszText, text.c_str(), pItem->cchTextMax );
		pItem->pszText[ pItem->cchTextMax-1 ] = 0;
	}
	if ( pItem->mask & LVIF_IMAGE )
		pItem->iImage = iImage;
}


/**
 *	This MFC method is called on an MFC virtual list when a search is being
 *	performed, for example, when the user types a letter it will try to find
 *	the first item that starts with that letter. See LVN_ODFINDITEM for more
 *	info.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnOdFindItem(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = -1;

	if ( !provider_ )
		return;

	NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;

    //Is search NOT based on string?
    if( !(pFindInfo->lvfi.flags & LVFI_STRING) )
        return;

	BW::wstring search( pFindInfo->lvfi.psz );

	int numItems = provider_->getNumItems();
	for( int i = 0; i < numItems; ++i )
	{
		const AssetInfo assetInfo = provider_->getAssetInfo( i );
		const BW::wstring & text = assetInfo.text();
		// we need the 'n'-version to do only partial matches
		if ( wcsnicmp( search.c_str(), text.c_str(), search.length() ) == 0 )
		{
			*pResult = i;
			break;
		}

	}
}


/**
 *	This method is used to notify the event handler that the selection has been
 *	changed, but it's done in a delayed fashion to avoid spamming the handler.
 *	For example, if a user selects 100 items, we get 100 individual MFC events,
 *	one for each item as they get selected, so instead of sending 100
 *	notifications, we just set a flag and a timer that eventually result in a 
 *	call to this method.
 */
void SmartListCtrl::delayedSelectionNotify()
{
	BW_GUARD;

	KillTimer( SMARTLIST_SELTIMER_ID );

	if ( eventHandler_ )
		eventHandler_->listItemSelect();
	
	delayedSelectionPending_ = false;
}


/**
 *	This MFC method handles timer events, and it deals with the different kinds
 *	of timers started by the list control.
 *
 *	@param id	Timer id of the timer that triggered this event.
 */
void SmartListCtrl::OnTimer( UINT_PTR id )
{
	BW_GUARD;

	if ( id == SMARTLIST_SELTIMER_ID )
	{
		// The selection has changed.
		delayedSelectionNotify();
	}
	else if ( id == SMARTLIST_LOADTIMER_ID )
	{
		// Time to check whether or not the provider has new items for us.
		KillTimer( SMARTLIST_LOADTIMER_ID );

		if ( !provider_ )
			return;

		bool finished = provider_->finished();
		int numItems = provider_->getNumItems();
		int numCustomItems = 0;
		if ( customItems_ )
			numCustomItems = static_cast<int>(customItems_->size());

		if ( numItems + numCustomItems != GetItemCount() )
			changeItemCount( numItems );

		if ( eventHandler_ )
			eventHandler_->listLoadingUpdate();

		if ( finished )
		{
			if ( eventHandler_ )
				eventHandler_->listLoadingFinished();
		}
		else
			SetTimer( SMARTLIST_LOADTIMER_ID, SMARTLIST_LOADTIMER_MSEC, 0 );
	}
	else if ( id == SMARTLIST_REDRAWTIMER_ID )
	{
		// Handle a delayed redraw.
		KillTimer( SMARTLIST_REDRAWTIMER_ID );
		redrawPending_ = false;
		RedrawWindow( NULL, NULL, RDW_INVALIDATE );
	}
}


/**
 *	This MFC method handles when a range of item's have been selected all at
 *	once.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnOdStateChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = 0;

	if ( !eventHandler_ || ignoreSelMessages_ )
		return;

	LPNMLVODSTATECHANGE state = (LPNMLVODSTATECHANGE)pNMHDR;

	updateSelection();

	if ( provider_ )
	{
		KillTimer( SMARTLIST_SELTIMER_ID );
		delayedSelectionPending_ = true;
		SetTimer( SMARTLIST_SELTIMER_ID, SMARTLIST_SELTIMER_MSEC, 0 );
	}
}


/**
 *	This MFC method handles when a single item's selection state has changed.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = 0;

	if ( !eventHandler_ || ignoreSelMessages_ )
		return;

	LPNMLISTVIEW state = (LPNMLISTVIEW)pNMHDR;

	updateSelection();

	if ( provider_ )
	{
		KillTimer( SMARTLIST_SELTIMER_ID );
		delayedSelectionPending_ = true;
		SetTimer( SMARTLIST_SELTIMER_ID, SMARTLIST_SELTIMER_MSEC, 0 );
	}
	lastItemChanged_ = state->iItem;
}


/**
 *	This MFC method relays the item click event to the event handler.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnItemClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	if ( !eventHandler_  )
		return;

	int item = GetNextItem( -1, LVNI_FOCUSED );
	if ( GetItemState( item, LVIS_SELECTED ) != LVIS_SELECTED )
		item = -1;
	if ( item != -1 && item != lastItemChanged_ )
		eventHandler_->listItemSelect();
	lastItemChanged_ = -1;
}


/**
 *	This MFC method handles when the user starts draggin an item.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = 0;

	if ( !eventHandler_ )
		return;

	LPNMLISTVIEW info = (LPNMLISTVIEW)pNMHDR;
	BW::wstring text;
	int image;
	getData( info->iItem, text, image );

	POINT pt;
	GetCursorPos( &pt );

	bw_safe_delete(dragImgList_);

	int pos = GetNextItem( -1, LVNI_SELECTED );
	bool isFirst = true;
	int xoff = 0;
	int yoff = 0;
	int xstep = 0;
	int ystep = 0;
	IMAGEINFO imf;
	int maxDragWidth = 400;
	int maxDragHeight = 350;
	while ( pos != -1 ) {
		if ( isFirst ) {
			dragImgList_ = CreateDragImage( pos, &pt );
			dragImgList_->GetImageInfo( 0, &imf );
			xstep = imf.rcImage.right - imf.rcImage.left;
			ystep = imf.rcImage.bottom - imf.rcImage.top;
			yoff = imf.rcImage.bottom;
			isFirst = false;
		}
		else
		{
			if ( yoff + ystep > maxDragHeight && xoff + xstep > maxDragWidth )
				generateDragListEndItem_ = true; // reached the max, so generate a 'more...' item in GetData
			CImageList* oneImgList = CreateDragImage( pos, &pt );
			generateDragListEndItem_ = false;
			CImageList* tempImgList = new CImageList();
			tempImgList->Attach(
				ImageList_Merge( 
					dragImgList_->GetSafeHandle(),
					0, oneImgList->GetSafeHandle(), 0, xoff, yoff ) );
			bw_safe_delete(dragImgList_);
			bw_safe_delete(oneImgList);
			dragImgList_ = tempImgList;
			dragImgList_->GetImageInfo( 0, &imf );
			yoff += ystep;
			if ( yoff > maxDragHeight )
			{
				xoff += xstep;
				if ( xoff > maxDragWidth )
					break;
				yoff = 0;
			}
		}
		pos = GetNextItem( pos, LVNI_SELECTED );
	}

	if ( dragImgList_ ) 
	{
		CPoint offset( thumbWidthCur_ + 16 , max( 16, thumbHeightCur_ - 14 ) );
		dragImgList_->SetBkColor( GetBkColor() );
		dragImage_ = new controls::DragImage( *dragImgList_, pt, offset );
	}

	if ( delayedSelectionPending_ )
	{
		// if a selection timer is pending, force it
		delayedSelectionNotify();
	}

	dragging_ = true;
	eventHandler_->listStartDrag( info->iItem );

}


/**
 *	This MFC method relays double-click events to the event handler.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	BW_GUARD;

	if ( !eventHandler_ )
		return;

	int item = GetNextItem( -1, LVNI_FOCUSED );
	if ( GetItemState( item, LVIS_SELECTED ) != LVIS_SELECTED )
		item = -1;
	eventHandler_->listDoubleClick( item );
}


/**
 *	This MFC method relays tooltip requests events to the event handler.
 *
 *	@param pNMHDR	MFC notify header.
 *	@param pResult	MFC result.
 */
void SmartListCtrl::OnToolTipText(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = 0;

	LPNMLVGETINFOTIP it = (LPNMLVGETINFOTIP)pNMHDR;
	int item = it->iItem;

	BW::wstring text;
	if ( eventHandler_ )
		eventHandler_->listItemToolTip( item, text );
	else if ( provider_ )
		text = provider_->getAssetInfo( item ).text();
	
	text = text.substr( 0, it->cchTextMax - 1 );

	lstrcpyn( it->pszText, text.c_str(), it->cchTextMax );
}

BW_END_NAMESPACE

