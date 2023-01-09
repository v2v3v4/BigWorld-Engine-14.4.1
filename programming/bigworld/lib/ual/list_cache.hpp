
#ifndef LIST_CACHE_HPP
#define LIST_CACHE_HPP


#include "atlimage.h"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class keeps a cache of list elements to improve performance in virtual
 *  lists. It manages the CImageList of the virtual to store the thumbnails.
 */
class ListCache
{
public:
	struct ListCacheElem
	{
		ListCacheElem() :
			image( 0 )
		{};
		BW::wstring key;
		int image;
	};

	ListCache();
	~ListCache();

	void init( CImageList* imgList, int imgFirstIndex );
	void clear();

	void setMaxItems( int maxItems );

	const ListCacheElem* cacheGet( const BW::wstring& text, const BW::wstring& longText );
	const ListCacheElem* cachePut( const BW::wstring& text, const BW::wstring& longText, const CImage& img );
	void cacheRemove( const BW::wstring& text, const BW::wstring& longText );
private:
	BW::list<ListCacheElem> listCache_;	// TODO: use a map instead.
	BW::vector<int> imgListFreeSpots_;
	CImageList* imgList_;
	int imgFirstIndex_;
	int maxItems_;
	typedef BW::list<ListCacheElem>::iterator ListCacheElemItr;
};


BW_END_NAMESPACE

#endif // LIST_CACHE_HPP
