
#include "pch.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "thumbnail_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "common/string_utils.hpp"
#include "vfolder_xml_provider.hpp"
#include "list_xml_provider.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
VFolderXmlProvider::VFolderXmlProvider()
{
	BW_GUARD;

	init( L"" );
}


/**
 *	Constructor.
 *
 *	@param path		File path for the XML file containing the items.
 */
VFolderXmlProvider::VFolderXmlProvider(	const BW::wstring& path )
{
	BW_GUARD;

	init( path );
}


/**
 *	Destructor.
 */
VFolderXmlProvider::~VFolderXmlProvider()
{
	BW_GUARD;
}


/**
 *	This method initialises the provider to a new XML list.
 *
 *	@param path		File path for the XML file containing the items.
 */
void VFolderXmlProvider::init( const BW::wstring& path )
{
	BW_GUARD;

	path_ = path;
	StringUtils::toLowerCaseT( path_ );
	std::replace( path_.begin(), path_.end(), L'/', L'\\' );

	items_.clear();

	sort_ = false;

	DataSectionPtr dataSection;

	BW::string npath;
	bw_wtoutf8( path_, npath );
	dataSection = BWResource::openSection( npath );

	if ( !dataSection )
		return;
	
	sort_ = dataSection->readBool( "sort", sort_ );
}


/**
 *	This method is called to prepare the enumerating of items in a VFolder
 *	or subfolder.
 *
 *	@param parent	Parent VFolder, if any.
 *	@return		True of there are items in it, false if empty.
 */
bool VFolderXmlProvider::startEnumChildren( const VFolderItemDataPtr parent )
{
	BW_GUARD;

	DataSectionPtr dataSection;

	items_.clear();

	BW::string npath;
	bw_wtoutf8( path_, npath );
	BWResource::instance().purge( npath );
	dataSection = BWResource::openSection( npath );

	if ( !dataSection )
		return false;

	BW::vector<DataSectionPtr> sections;
	dataSection->openSections( "item", sections );
	filterHolder_->enableSearchText( false );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		AssetInfoPtr item = new AssetInfo( *s );
		if ( !filterHolder_ || filterHolder_->filter( item->text(), item->longText() ) )
			items_.push_back( item );
	}
	filterHolder_->enableSearchText( true );

	itemsItr_ = items_.begin();
	return true;
}


/**
 *	This method is called to iterate to and get the next item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param img		Returns the thumbnail for the item, if available.
 *	@return		Next item in the provider.
 */
VFolderItemDataPtr VFolderXmlProvider::getNextChild( ThumbnailManager& thumbnailManager, CImage& img )
{
	BW_GUARD;

	if ( itemsItr_ == items_.end() )
		return 0;

	if ( !(*itemsItr_)->thumbnail().empty() )
		thumbnailManager.create( (*itemsItr_)->thumbnail(), img, 16, 16, folderTree_ );
	else
		thumbnailManager.create( (*itemsItr_)->longText(), img, 16, 16, folderTree_ );

	VFolderXmlItemData* newItem = new VFolderXmlItemData( this,
		*(*itemsItr_), XMLGROUP_ITEM, false, (*itemsItr_)->thumbnail() );

	itemsItr_++;

	return newItem;
}


/**
 *	This method creates the thumbnail for an item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param data		Item data.
 *	@param img		Returns the thumbnail for the item, if available.
 */
void VFolderXmlProvider::getThumbnail( ThumbnailManager& thumbnailManager, VFolderItemDataPtr data, CImage& img )
{
	BW_GUARD;

	if ( !data )
		return;

	VFolderXmlItemData* xmlData = (VFolderXmlItemData*)data.getObject();

	if ( !xmlData->thumb().empty() )
		thumbnailManager.create( xmlData->thumb(), img, 16, 16, folderTree_ );
	else
		thumbnailManager.create( xmlData->assetInfo().longText(), img, 16, 16, folderTree_ );
}


/**
 *	This method returns a text description for the item, good for the dialog's
 *	status bar.
 *
 *	@param data		Item data.
 *	@param numItems	Total number of items, to include in the descriptive text.
 *	@param finished	True if loading of items has finished, false if not, in
 *					which case it is noted in the descriptive text.
 *	@return		Descriptive text for the item.
 */
const BW::wstring VFolderXmlProvider::getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished )
{
	BW_GUARD;

	if ( !data )
		return L"";

	BW::wstring desc;
	if ( data->assetInfo().description().empty() )
		desc = data->assetInfo().longText();
	else
		desc = data->assetInfo().description();

	if ( data->isVFolder() )
	{
		desc = Localise( L"UAL/VFOLDER_XML_PROVIDER/NUM_ITEMS", getPath(), numItems );
	}

	return desc;
}

bool VFolderXmlProvider::getListProviderInfo(
	VFolderItemDataPtr data,
	BW::wstring& retInitIdString,
	ListProviderPtr& retListProvider,
	bool& retItemClicked )
{
	BW_GUARD;

	if ( !data || !listProvider_ )
		return false;

	retItemClicked = !data->isVFolder();

	if ( retInitIdString == getPath() && retListProvider == listProvider_ )
		return false;

	retListProvider = listProvider_;
	((ListXmlProvider*)listProvider_.getObject())->init( getPath() );
	retInitIdString = getPath();

	return true;
}


/**
 *	This method returns the path to the XML list file.
 *
 *	@return		The path to the XML list file.
 */
BW::wstring VFolderXmlProvider::getPath()
{
	BW_GUARD;

	return path_;
}


/**
 *	This method returns true if the list is sorted, false if not.
 *
 *	@return		True if the list is sorted, false if not.
 */
bool VFolderXmlProvider::getSort()
{
	BW_GUARD;

	return sort_;
}

BW_END_NAMESPACE

