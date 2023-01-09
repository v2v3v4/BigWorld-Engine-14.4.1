
#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "list_xml_provider.hpp"
#include "thumbnail_manager.hpp"

#include "common/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ListXmlProvider::ListXmlProvider() :
	errorLoading_( false )
{
	BW_GUARD;

	init( L"" );
}


/**
 *	Destructor.
 */
ListXmlProvider::~ListXmlProvider()
{
	BW_GUARD;

	clearItems();
}


/**
 *	This method initialises the provider from an XML file path.
 *
 *	@param path		Path to the xml file containing the items.
 */
void ListXmlProvider::init( const BW::wstring& path )
{
	BW_GUARD;

	path_ = path;
	StringUtils::toLowerCaseT( path_ );
	std::replace( path_.begin(), path_.end(), L'/', L'\\' );

	refreshPurge( true );
}


/**
 *	This method refreshes the items in the provider.
 */
void ListXmlProvider::refresh()
{
	BW_GUARD;

	refreshPurge( true );
}


/**
 *	This static method is used as a sorting callback for std::sort.
 *	
 *	@param a	First item to compare.
 *	@param b	The other item to compare against.
 *	@return		True if a is less than b, false otherwise.
 */
bool ListXmlProvider::s_comparator( const ListXmlProvider::ListItemPtr& a, const ListXmlProvider::ListItemPtr& b )
{
	BW_GUARD;

	return wcsicmp( a->text().c_str(), b->text().c_str() ) < 0;
}


/**
 *	This method reads the contents of the provider's XML file and filters them.
 *	
 *	@param purge	True to purge the XML file from the resource manager's
 *					cache.
 */
void ListXmlProvider::refreshPurge( bool purge )
{
	BW_GUARD;

	errorLoading_ = false;

	clearItems();

	if ( !path_.length() )
		return;

	DataSectionPtr dataSection;

	BW::string npath;
	bw_wtoutf8( path_, npath );
	if ( purge )
		BWResource::instance().purge( npath );

	dataSection = BWResource::openSection( npath );

	if ( !dataSection )
	{
		errorLoading_ = true;
		return;
	}

	BW::vector<DataSectionPtr> sections;
	dataSection->openSections( "item", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		ListItemPtr item = new AssetInfo(
			(*s)->readWideString( "type" ),
			(*s)->asWideString(),
			(*s)->readWideString( "longText" ),
			(*s)->readWideString( "thumbnail" ),
			(*s)->readWideString( "description" )
			);
		items_.push_back( item );
	}

	if ( dataSection->readBool( "sort", false ) )
		std::sort< ItemsItr >( items_.begin(), items_.end(), s_comparator );

	filterItems();
}


/**
 *	This method clears all items.
 */
void ListXmlProvider::clearItems()
{
	BW_GUARD;

	items_.clear();
	searchResults_.clear();
}


/**
 *	This method always returns true for XML providers because they do all
 *	processing in the main thread when inited.
 *	
 *	@return		True, it always finishes as soon as it's inited.
 */
bool ListXmlProvider::finished()
{
	return true; // it's not asyncronous
}


/**
 *	This method returns the number of items in the provider (after filtering).
 *	
 *	@return		The number of items in the provider (after filtering).
 */
int ListXmlProvider::getNumItems()
{
	BW_GUARD;

	return (int)searchResults_.size();
}


/**
 *	This method returns info object for the item at the given position.
 *
 *	@param index	Index of the item in the list.
 *	@return		Asset info object corresponding to the item.
 */
const AssetInfo ListXmlProvider::getAssetInfo( int index )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return AssetInfo();

	return *searchResults_[ index ];
}



/**
 *	This method returns the appropriate thumbnail for the item.
 *
 *	@param manager	Reference to the thumbnail manager object in use.
 *	@param index	Index of the item in the list.
 *	@param img		Returns here the thumbnail for the item.
 *	@param w		Desired width for the thumbnail.
 *	@param h		Desired height for the thumbnail.
 *	@param updater	Thumbnail creation callback object.
 */
void ListXmlProvider::getThumbnail( ThumbnailManager& manager,
								   int index, CImage& img, int w, int h,
								   ThumbnailUpdater* updater )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return;

	BW::wstring thumb;
	thumb = searchResults_[ index ]->thumbnail();

	if ( !thumb.length() )
		thumb = searchResults_[ index ]->longText();

	BW::string nthumb;
	bw_wtoutf8( thumb, nthumb);
	BW::string fname;
	fname = BWResource::findFile( nthumb );

	BW::wstring wfname;
	bw_utf8tow( fname, wfname );
	manager.create( wfname, img, w, h, updater );
}


/**
 *	This method filters the list of items found in the XML file by the filters
 *	if the filters and/or search text are active.
 */
void ListXmlProvider::filterItems()
{
	BW_GUARD;

	searchResults_.clear();

	for( ItemsItr i = items_.begin(); i != items_.end(); ++i )
	{
		// filters filtering
		if ( !filterHolder_ ||
			filterHolder_ && filterHolder_->filter( (*i)->text(), (*i)->longText() ) )
		{
			if ( (searchResults_.size() % VECTOR_BLOCK_SIZE) == 0 )
				searchResults_.reserve( searchResults_.size() + VECTOR_BLOCK_SIZE );
			searchResults_.push_back( (*i) );
		}
	}
}

/**
 *	This method initialises the provider from an XML file path.
 *
 *	@param path		Path to the xml file containing the items.
 */
void ListXmlSectionProvider::init( const BW::wstring& path )
{
	BW_GUARD;

	path_ = path;
	refreshPurge( true );
}

BW_END_NAMESPACE

