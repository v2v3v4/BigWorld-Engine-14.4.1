
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/bwresource.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"

#include "xml_item_list.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
XmlItemList::XmlItemList() :
	sectionLock_( 0 ),
	section_( 0 ),
	rootSection_( 0 )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
XmlItemList::~XmlItemList()
{
	BW_GUARD;
}


/**
 *	This method changes the file path for the XML list file.
 *
 *	@param path	File path of the XML list file.
 */
void XmlItemList::setPath( const BW::wstring& path )
{
	BW_GUARD;

	path_ = path;
}


/**
 *	This method sets a data section for the XML list directly instead of a file
 *	path.
 *
 *	@param section	Data section that contains the XML list items.
 */
void XmlItemList::setDataSection( const DataSectionPtr section )
{
	BW_GUARD;

	rootSection_ = section;
	path_ = L"";
}


/**
 *	This method ensures that the XML list is being edited by only one at a
 *	time.
 *
 *	@return		The data section to the XML list items.
 */
DataSectionPtr XmlItemList::lockSection()
{
	BW_GUARD;

	ASSERT( sectionLock_ < 8 ); // too much lock nesting = not unlocking somewhere
	if ( !rootSection_ && path_.empty() )
		return 0;

	if ( !section_ )
	{
		if ( !rootSection_ )
		{
			BW::string npath;
			bw_wtoutf8( path_, npath );
			BWResource::instance().purge( npath );
			section_ = BWResource::openSection( npath, true );
			if ( !section_ )
				return 0;
		}
		else
			section_ = rootSection_;
	}
	sectionLock_++;
	return section_;
}


/**
 *	This method allows others to lock and use the XML list data section again.
 */
void XmlItemList::unlockSection()
{
	BW_GUARD;

	ASSERT( sectionLock_ );
	sectionLock_--;
	if ( sectionLock_ == 0 )
		section_ = 0;
}


/**
 *	This method returns the list of items on the XML list.
 *
 *	@param items	Return param, list of XML list items.
 */
void XmlItemList::getItems( XmlItemVec& items )
{
	BW_GUARD;

	DataSectionPtr section = lockSection();
	if ( !section )
		return;

	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		XmlItem::Position pos = XmlItem::TOP;
		if ( _stricmp( (*s)->readString( "position" ).c_str(), "top" ) == 0 )
			pos = XmlItem::TOP;
		else if ( ( _stricmp( (*s)->readString( "position" ).c_str(), "bottom" ) == 0 ) )
			pos = XmlItem::BOTTOM;
		items.push_back( 
			XmlItem(
				AssetInfo( 
					(*s)->readWideString( "type" ),
					(*s)->asWideString(),
					(*s)->readWideString( "longText" ),
					(*s)->readWideString( "thumbnail" ),
					(*s)->readWideString( "description" ) ),
				pos )
			);
	}

	unlockSection();
}


/**
 *	This method returns the data section corresponding to an XML list item.
 *
 *	@param item	XML list item we want get the data section for.
 *	@return		The data section to the XML list item.
 */
DataSectionPtr XmlItemList::getItem( const XmlItem& item )
{
	BW_GUARD;

	if ( item.assetInfo().text().empty() )
		return 0;

	DataSectionPtr section = lockSection();
	if ( !section )
		return 0;

	BW::wstring text = StringUtils::lowerCaseT( item.assetInfo().text() );
	BW::wstring longText = StringUtils::lowerCaseT( item.assetInfo().longText() );

	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		if ( (*s)->readWideString( "type" ) == item.assetInfo().type() &&
			StringUtils::lowerCaseT( (*s)->asWideString() ) == text &&
			StringUtils::lowerCaseT( (*s)->readWideString( "longText" ) ) == longText )
		{
			unlockSection();
			return (*s);
		}
	}

	unlockSection();
	return 0;
}


/**
 *	This method saves an XML list item to a data section.
 *
 *	@param section	Data section we want to save the item to.
 *	@param item		XML list item to be saved.
 */
void XmlItemList::dumpItem( DataSectionPtr section, const XmlItem& item )
{
	BW_GUARD;

	if ( !section )
		return;

	section->setWideString( item.assetInfo().text() );
	section->writeWideString( "type", item.assetInfo().type() );
	section->writeWideString( "longText", item.assetInfo().longText() );
	if ( !item.assetInfo().thumbnail().empty() )
		section->writeWideString( "thumbnail", item.assetInfo().thumbnail() );
	if ( !item.assetInfo().description().empty() )
		section->writeWideString( "description", item.assetInfo().description() );
}


/**
 *	This method adds an item to the back of the XML list.
 *
 *	@param item		XML list item to be added.
 *	@return		Data section of the newly added item.
 */
DataSectionPtr XmlItemList::add( const XmlItem& item )
{
	BW_GUARD;

	if ( item.assetInfo().text().empty() )
		return 0;

	DataSectionPtr section = lockSection();
	if ( !section )
		return 0;

	// Add it to the list and save
	DataSectionPtr newItem = section->newSection( "item" );
	if ( !newItem )
	{
		unlockSection();
		return 0;
	}
	dumpItem( newItem, item );
	section->save();
	unlockSection();
	return newItem;
}


/**
 *	This method adds an item to the XML list before "atItem".
 *
 *	@param item		XML list item to be added.
 *	@param atItem	Item where the "item" will be added, or an empty item to
 *					add "item" to the list.
 *	@return		Data section of the newly added item.
 */
DataSectionPtr XmlItemList::addAt( const XmlItem& item, const XmlItem& atItem )
{
	BW_GUARD;

	if ( item.assetInfo().text().empty() )
		return 0;

	DataSectionPtr section = lockSection();
	if ( !section )
		return 0;

	DataSectionPtr newItem = 0;

	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		DataSectionPtr dsitem;
		if ( (*s)->readWideString( "type" ) == atItem.assetInfo().type() &&
			(*s)->asWideString() == atItem.assetInfo().text() &&
			(*s)->readWideString( "longText" ) == atItem.assetInfo().longText() )
		{
			// add the new item in place
			newItem = section->newSection( "item" );
			dumpItem( newItem, item );
		}
		// add old item
		dsitem = section->newSection( "item" );
		dsitem->copy( *s );

		// delete old item
		section->delChild( *s );
	}
	if ( !newItem )
	{
		newItem = section->newSection( "item" );
		dumpItem( newItem, item );
	}

	section->save();
	unlockSection();
	return newItem;
}


/**
 *	This method removes an item from the XML list.
 *
 *	@param item		XML list item to be removed.
 */
void XmlItemList::remove( const XmlItem& item )
{
	BW_GUARD;

	DataSectionPtr section = lockSection();
	if ( !section )
		return;

	BW::wstring text = StringUtils::lowerCaseT( item.assetInfo().text() );
	BW::wstring longText = StringUtils::lowerCaseT( item.assetInfo().longText() );

	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		if ( (*s)->readWideString( "type" ) == item.assetInfo().type() &&
			StringUtils::lowerCaseT( (*s)->asWideString() ) == text &&
			StringUtils::lowerCaseT( (*s)->readWideString( "longText" ) ) == longText )
		{
			section->delChild( *s );
			section->save();
			break;
		}
	}
	unlockSection();
}


/**
 *	This method removes all items from the XML list.
 */
void XmlItemList::clear()
{
	BW_GUARD;

	DataSectionPtr section = lockSection();
	if ( !section )
		return;

	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	BW::vector<DataSectionPtr>::iterator oldestSection = sections.begin();
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
		section->delChild( *s );

	section->save();
	unlockSection();
}

BW_END_NAMESPACE


