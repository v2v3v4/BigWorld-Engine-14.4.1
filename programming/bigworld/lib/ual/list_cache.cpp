
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "list_cache.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/guard.hpp"
#include <algorithm>


// List Cache Key
#define BUILD_LISTCACHE_KEY( A, B ) ((A) + L"|" + (B))

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ListCache::ListCache() :
	imgList_( 0 ),
	imgFirstIndex_( 0 )
{
	BW_GUARD;

	setMaxItems( 200 );
}


/**
 *	Destructor
 */
ListCache::~ListCache()
{
	BW_GUARD;
}


/**
 *	This method is used to initialise the cache.
 *
 *	@param imgList	Image list of the list control that wants to be cached.
 *	@param imgFirstIndex	Minimum index of images to cache, allowing list
 *							controls to store their non-cached images first.
 */
void ListCache::init( CImageList* imgList, int imgFirstIndex )
{
	BW_GUARD;

	imgList_ = imgList;
	imgFirstIndex_ = imgFirstIndex;

	clear();
}


/**
 *	This method cleares the whole cache.
 */
void ListCache::clear()
{
	BW_GUARD;

	if ( !imgList_ || !imgList_->GetSafeHandle() )
		return;

	listCache_.clear();
	imgListFreeSpots_.clear();
	if ( maxItems_ > 0 )
		imgListFreeSpots_.reserve( maxItems_ );

	int index = imgFirstIndex_;
	while ( index < imgList_->GetImageCount() )
		imgListFreeSpots_.push_back( index++ );
}


/**
 *	This method is used to set the size of the cache.
 *
 *	@param maxItems	Maximum number of items to store in the cache.
 */
void ListCache::setMaxItems( int maxItems )
{
	BW_GUARD;

	maxItems_ = maxItems;
	if ( maxItems_ > 0 )
		imgListFreeSpots_.reserve( maxItems_ );
}


/**
 *	This method tries to find an image in the cache and return it.
 *
 *	@param text	Short name of the item.
 *	@param longText	Full description of the item, usually a file path.
 *	@return	List cache item if found, NULL if not in the cache.
 */
const ListCache::ListCacheElem* ListCache::cacheGet( const BW::wstring& text, const BW::wstring& longText )
{
	BW_GUARD;

	if ( !imgList_ || !imgList_->GetSafeHandle() )
		return 0;

	if ( maxItems_ <= 0 )
		return 0;

	BW::wstring key = BUILD_LISTCACHE_KEY( text, longText );

	for( ListCacheElemItr i = listCache_.begin(); i != listCache_.end(); ++i )
	{
		if ( wcsicmp( (*i).key.c_str(), key.c_str() ) == 0 )
		{
			// cache hit. Put first on the list
			ListCacheElem tempElem = (*i);
			listCache_.erase( i );
			listCache_.push_front( tempElem );
			return &(*listCache_.begin());
		}
	}

	// cache miss
	return 0;
}


/**
 *	This method inserts a new cache element, matching an item with its image.
 *
 *	@param text	Short name of the item.
 *	@param longText	Full description of the item, usually a file path.
 *	@param img	Corresponding thumbnail image for the item.
 *	@return	List cache item just created, or NULL if an error occurred.
 */
const ListCache::ListCacheElem* ListCache::cachePut( const BW::wstring& text, const BW::wstring& longText, const CImage& img )
{
	BW_GUARD;

	if ( !imgList_ || !imgList_->GetSafeHandle() )
		return 0;

	ListCacheElem newElem;

	newElem.key = BUILD_LISTCACHE_KEY( text, longText );

	StringUtils::toLowerCaseT( newElem.key );

	int image = 0;

	if ( maxItems_ <= 0 )
	{
		// cache only one item
		listCache_.clear();
		imgList_->Remove( imgFirstIndex_ );
		if ( !img.IsNull() )
			image = imgList_->Add( CBitmap::FromHandle( (HBITMAP)img ), (CBitmap*)0 );

		newElem.image = image;
		listCache_.push_back( newElem );
		return &(*listCache_.rbegin());
	}

	if ( (int)listCache_.size() >= maxItems_ )
	{
		// cache full, replace oldest (last in the list)
		int oldestImg = listCache_.back().image;

		if ( !img.IsNull() )
		{
			// find out if an unused spot is available in the image list
			int replaceImg = -1;
			if ( oldestImg >= imgFirstIndex_ )
			{
				// replace the oldest image
				replaceImg = oldestImg;
			}
			else if ( !imgListFreeSpots_.empty() )
			{
				// replace an available free spot
				replaceImg = imgListFreeSpots_.back();
				imgListFreeSpots_.pop_back();
			}

			// add image
			if ( replaceImg >= imgFirstIndex_ )
			{
				// oldest used image, so replace it
				image = oldestImg;
				imgList_->Replace( image, CBitmap::FromHandle( (HBITMAP)img ), (CBitmap*)0 );
			}
			else
			{
				// oldest didn't use image, so add one
				image = imgList_->Add( CBitmap::FromHandle( (HBITMAP)img ), (CBitmap*)0 );
			}
		}
		else if ( oldestImg >= imgFirstIndex_ )
		{
			// oldest used image, and current doesn't, so add it as a free spot
			imgListFreeSpots_.push_back( oldestImg );
		}

		newElem.image = image;

		// remove the oldest (last in the list)
		listCache_.pop_back();
		// and insert new one
		listCache_.push_front( newElem );
		return &(*listCache_.begin());
	}
	else
	{
		if ( !img.IsNull() )
		{
			if ( !imgListFreeSpots_.empty() )
			{
				// replace an available free spot
				image = imgListFreeSpots_.back();
				imgListFreeSpots_.pop_back();

				imgList_->Replace( image,
					CBitmap::FromHandle( (HBITMAP)img ), (CBitmap*)0 );
			}
			else
			{
				// no free space, so add it.
				image = imgList_->Add( CBitmap::FromHandle( (HBITMAP)img ), (CBitmap*)0 );
			}
		}

		newElem.image = image;

		// cache has free space, add to cache
		listCache_.push_front( newElem );
		return &(*listCache_.begin());
	}
}


/**
 *	This method takes an item's cache element out of the cache.
 *
 *	@param text	Short name of the item.
 *	@param longText	Full description of the item, usually a file path.
 */
void ListCache::cacheRemove( const BW::wstring& text, const BW::wstring& longText )
{
	BW_GUARD;

	BW::wstring key = BUILD_LISTCACHE_KEY( text, longText );
	StringUtils::toLowerCaseT( key );

	for( ListCacheElemItr i = listCache_.begin(); i != listCache_.end(); )
		if ( (*i).key == key )
		{
			if ( (*i).image >= imgFirstIndex_ )
			{
				// add it as a free spot
				imgListFreeSpots_.push_back( (*i).image );
			}
			// erase the actual cache entry
			i = listCache_.erase( i );
		}
		else
			 ++i;
}

BW_END_NAMESPACE


