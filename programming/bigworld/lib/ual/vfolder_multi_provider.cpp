
#include "pch.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "thumbnail_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "common/string_utils.hpp"
#include "vfolder_multi_provider.hpp"
#include "list_multi_provider.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
VFolderMultiProvider::VFolderMultiProvider() :
	parent_()
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
VFolderMultiProvider::~VFolderMultiProvider()
{
	BW_GUARD;
}


/**
 *	This method is called to prepare the enumerating of items in a VFolder
 *	or subfolder.
 *
 *	@param parent	Parent VFolder, if any.
 *	@return		True of there are items in it, false if empty.
 */
bool VFolderMultiProvider::startEnumChildren( const VFolderItemDataPtr parent )
{
	BW_GUARD;

	iter_ = providers_.begin();
	if ( iter_ == providers_.end() )
		return false;

	parent_ = parent.getObject();
	(*iter_)->startEnumChildren( parent_ );
	return true;
}


/**
 *	This method is called to iterate to and get the next item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param img		Returns the thumbnail for the item, if available.
 *	@return		Next item in the provider.
 */
VFolderItemDataPtr VFolderMultiProvider::getNextChild( ThumbnailManager& thumbnailManager, CImage& img )
{
	BW_GUARD;

	if ( iter_ == providers_.end() )
		return NULL;

	// loop until we find another item in a provider, or until there are
	// no items left in any of the providers.
	VFolderItemDataPtr data;
	while ( (data = (*iter_)->getNextChild( thumbnailManager, img )) == NULL )
	{
		// the current provider ran out of items, so go to the next provider.
		++iter_;
		if ( iter_ == providers_.end() )
			return NULL; // no more providers, break.
		// moved to the next provider. Start enumerating it's items
		(*iter_)->startEnumChildren( parent_ );
	}

	if ( data != NULL && img.IsNull() )
	{
		// an item was found, so try to load it's thumbnail
		(*iter_)->getThumbnail( thumbnailManager, data, img );
	}

	return data;
}


/**
 *	This method creates the thumbnail for an item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param data		Item data.
 *	@param img		Returns the thumbnail for the item, if available.
 */
void VFolderMultiProvider::getThumbnail( ThumbnailManager& thumbnailManager, VFolderItemDataPtr data, CImage& img )
{
	BW_GUARD;

	if ( !data || !data->getProvider() )
		return;

	// load the item's thumbnail from it's provider
	data->getProvider()->getThumbnail( thumbnailManager, data, img );
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
const BW::wstring VFolderMultiProvider::getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished )
{
	BW_GUARD;

	if ( !data )
		return L"";

	BW::wstring desc;
	if ( data->isVFolder() || !data->getExpandable() )
	{
		// if it's a folder or a vfolder, build summary info
		if ( data->assetInfo().description().empty() )
			desc = data->assetInfo().longText();
		else
			desc = data->assetInfo().description();

		if (finished)
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_MULTI_PROVIDER/DESCRIPTION", 
					desc,
					numItems
				);
		}
		else
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_MULTI_PROVIDER/DESCRIPTION_LOADING", 
					desc,
					numItems
				);
		}
	}
	else if ( data->getProvider() && data->getProvider()->getListProvider() )
	{
		// simple get the item's descriptive text directly from it's provider
		desc = data->getProvider()->getDescriptiveText( data, numItems, finished );
	}

	return desc;
}


/**
 *	This method returns all the information needed to initialise the
 *	appropriate ListProvider for this folder when the folder is clicked for
 *	example.
 *
 *	@param data		Item data.
 *	@param retInitIdString	Return param, used as an id/key for this provider
 *							and its configuration.
 *	@param retListProvider	Return param, list provider matching this VFolder
 *							provider.
 *	@param retItemClicked	Return param, true to call the click callback.
 *	@return		True if there is a valid list provider for this VFolder item.
 */
bool VFolderMultiProvider::getListProviderInfo(
	VFolderItemDataPtr data,
	BW::wstring& retInitIdString,
	ListProviderPtr& retListProvider,
	bool& retItemClicked )
{
	BW_GUARD;

	if ( !data || !listProvider_ )
		return false;

	if ( data->isVFolder() || !data->getExpandable() )
	{
		// if it's a folder or a vfolder, build summary info
		retItemClicked = false;
		retListProvider = listProvider_;
		// call this method in all the sub-providers, so they prepare their
		// list providers properly. Ignore their return values by using dummy
		// variables
		BW::wstring dummyInitId;
		ListProviderPtr dummyList;
		bool dummyClick;
		bool ret = false;
		for( ProvVec::iterator i = providers_.begin();
			i != providers_.end(); ++i )
		{
			ret |= (*i)->getListProviderInfo(
				data, dummyInitId, dummyList, dummyClick );
		}
		return ret;
	}
	else
	{
		// it's an item, so simply return the item's info from its provider
		retItemClicked = true;
		if ( data->getProvider() )
		{
			return data->getProvider()->getListProviderInfo(
				data, retInitIdString, retListProvider, retItemClicked );
		}
	}

	return false;
}


/**
 *	This method adds a provider to the internal list of providers.
 *
 *	@param provider	Provider to add.
 */
void VFolderMultiProvider::addProvider( VFolderProviderPtr provider )
{
	BW_GUARD;

	if ( !provider )
		return;

	providers_.push_back( provider );
}

BW_END_NAMESPACE

