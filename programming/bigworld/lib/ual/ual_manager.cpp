
#include "pch.hpp"
#include "ual_dialog.hpp"
#include "ual_favourites.hpp"
#include "ual_history.hpp"
#include "ual_manager.hpp"

#include "common/string_utils.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

/// Asset Browser manager singleton instance.
BW_SINGLETON_STORAGE( UalManager )


/**
 *	Constructor.
 */
UalManager::UalManager()
	: GUI::ActionMaker<UalManager>( "UalActionRefresh|UalActionLayout", &UalManager::handleGUIAction )
	, thumbnailManager_( new ThumbnailManager() )
	, timerID_( 0 )
	, itemClickCallback_( 0 )
	, itemDblClickCallback_( 0 )
	, startPopupMenuCallback_( 0 )
	, endPopupMenuCallback_( 0 )
	, startDragCallback_( 0 )
	, updateDragCallback_( 0 )
	, endDragCallback_( 0 )
	, focusCallback_( 0 )
	, errorCallback_( 0 )
{
	BW_GUARD;

	favourites_.setChangedCallback( new UalFunctor0< UalManager >( this, &UalManager::favouritesCallback ) );
	history_.setChangedCallback( new UalFunctor0< UalManager >( this, &UalManager::historyCallback ) );

	timerID_ = SetTimer( 0, 0, 100, onTimer );

	BWResource::instance().addModificationListener( this );
}


/**
 *	Destructor.
 */
UalManager::~UalManager()
{
	BW_GUARD;

	BWResource::instance().removeModificationListener( this );

	KillTimer( 0, timerID_ );
}


/**
 *	This method tells all asset browsers that the favourites list has changed.
 */
void UalManager::favouritesCallback()
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		(*i)->favouritesChanged();
}


/**
 *	This method tells all asset browsers that the history list has changed.
 */
void UalManager::historyCallback()
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		(*i)->historyChanged();
}


/**
 *	This method adds a search path to the asset browser.
 *
 *	@param path		Path to add to the list of search paths.
 */
void UalManager::addPath( const BW::wstring& path )
{
	BW_GUARD;

	if ( !path.length() )
		return;

	BW::wstring pathL = path;
	std::replace( pathL.begin(), pathL.end(), L'/', L'\\' );
	if ( std::find( paths_.begin(), paths_.end(), pathL ) != paths_.end() )
		return;
	paths_.push_back( pathL );
}


/**
 *	This method returns the search path at the specified index.
 *
 *	@param i	Index to the desired path.
 *	@return		Desired path, or the empty string if the path was not found.
 */
const BW::wstring UalManager::getPath( int i )
{
	BW_GUARD;

	if ( i < 0 )
		return L"";

	for( BW::vector<BW::wstring>::iterator p = paths_.begin(); p != paths_.end(); ++p )
		if ( i-- == 0 )
			return *p;

	return L"";
}


/**
 *	This method returns the number of registered search paths.
 *
 *	@return	The number of registered search paths.
 */
int UalManager::getNumPaths()
{
	BW_GUARD;

	return static_cast<int>(paths_.size());
}


/**
 *	This method sets the Asset Browser configuration file path.
 *
 *	@param config	Asset Browser configuration file path.
 */
void UalManager::setConfigFile( BW::wstring config )
{
	BW_GUARD;

	configFile_ = config;
	configFileAbsolute_ = config;
	BWResource::resolveToAbsolutePath( configFileAbsolute_ );
}


/**
 *	This method stops any threads and releases any static resources.
 */
void UalManager::fini()
{
	BW_GUARD;

	INFO_MSG( "UAL Manager - Waiting for the Thumbnail Manager to stop ...\n" );
	thumbnailManager_->stop();
	INFO_MSG( "UAL Manager - ... Thumbnail Manager stopped\n" );
	UalDialog::fini();
}


void UalManager::onResourceModified( 
	const BW::StringRef& basePath, const BW::StringRef& resourceID,
	ResourceModificationListener::Action action )
{
	BW_GUARD;

	updateItem( bw_utf8tow( basePath + resourceID ) );
}


/**
 *	This method is used by Asset Browser dialogs to register themselves to the
 *	manager.
 *
 *	@param dialog	Asset Browser dialog pointer.
 */
void UalManager::registerDialog( UalDialog* dialog )
{
	BW_GUARD;

	dialogs_.push_back( dialog );
}


/**
 *	This method is used by Asset Browser dialogs to deregister themselves from
 *	the manager.
 *
 *	@param dialog	Asset Browser dialog pointer.
 */
void UalManager::unregisterDialog( UalDialog* dialog )
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
	{
		if ( *i == dialog )
		{
			dialogs_.erase( i );
			return;
		}
	}
}


/**
 *	This static method is a callback for when our thumbnail manager timer goes
 *	off.
 *
 *	@param hwnd		Ignored, window handle.
 *	@param nMsg		Ignored, message id.
 *	@param nIDEvent	Ignored, timer id.
 *	@param dwtime	Ignored, time.
 */
/*static*/ void UalManager::onTimer(HWND hwnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime )
{
	BW_GUARD;

	instance().thumbnailManager().tick();
}


/**
 *	This method returns the config file path.
 *
 *	@return The config file path.
 */
const BW::wstring UalManager::getConfigFile()
{
	BW_GUARD;

	return configFileAbsolute_;
}


/**
 *	This method returns the currently focused Asset Browser dialog.
 *
 *	@return Currently focused Asset Browser dialog.
 */
UalDialog* UalManager::getActiveDialog()
{
	BW_GUARD;

	// if there's only one, return it
	if ( dialogs_.size() == 1 )
		return *dialogs_.begin();

	// more than one, find the focused control and find the parent dialog,
	HWND fw = GetFocus();
	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		if ( (*i)->GetSafeHwnd() == fw || IsChild( (*i)->GetSafeHwnd(), fw ) )
			return (*i);

	// last resort hack: use rectangles, because when the first thing clicked
	// is a toolbar button, then nothing is focused
	CPoint pt;
	GetCursorPos( &pt );
	HWND hwnd = ::WindowFromPoint( pt );
	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
	{
		if ( IsChild( (*i)->GetSafeHwnd(), hwnd ) )
			return (*i);
	}
	return 0;
}


/**
 *	This method updates an item on the tree view and list of all Asset Browser 
 *	dialogs.
 *
 *	@param longText		Full text description of the item, usually a file path.
 */
void UalManager::updateItem( const BW::wstring& longText )
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		(*i)->updateItem( longText );
}


/**
 *	This method forces a refresh all of the Asset Browser dialogs.
 */
void UalManager::refreshAllDialogs()
{
	BW_GUARD;

	for (DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i)
	{
		(*i)->guiActionRefresh();
	}
}


/**
 *	This method ensures an item is visible on the list and it's virtual folder
 *	visible on the tree view, on all Asset Browser dialogs.
 *
 *	@param vfolder		Virtual folder the item belongs too.
 *	@param longText		Full text description of the item, usually a file path.
 */
void UalManager::showItem( const BW::wstring& vfolder, const BW::wstring& longText )
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		(*i)->showItem( vfolder, longText );
}


/**
 *	This method distributes the GUI action calls to individual handlers
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool UalManager::handleGUIAction( GUI::ItemPtr item )
{
	BW_GUARD;

	if (item->action() == "UalActionRefresh")	return	guiActionRefresh( item );
	else if (item->action() == "UalActionLayout")	return	guiActionLayout( item );

	return false;
}


/**
 *	This method refreshes the contents of the tree view and the list, usually
 *	in response to the user clicking the refresh button on the dialog's
 *	toolbar.
 *
 *	@param item		Ignored, GUIMANAGER item.
 *	@return			True if successful.
 */
bool UalManager::guiActionRefresh( GUI::ItemPtr item )
{
	BW_GUARD;

	UalDialog* dlg = getActiveDialog();
	if ( dlg )
		dlg->guiActionRefresh();
	return true;
}


/**
 *	This method toggles the layout of the internals of the dialog, usually
 *	in response to the user clicking the layout button on the dialog's
 *	toolbar.
 *
 *	@return		True if successful.
 */
bool UalManager::guiActionLayout( GUI::ItemPtr item )
{
	BW_GUARD;

	UalDialog* dlg = getActiveDialog();
	if ( dlg )
		dlg->guiActionLayout();
	return true;
}


/**
 *	This method is called when the drag & drop operation is canceled.
 */
void UalManager::cancelDrag()
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		(*i)->resetDragDropTargets();
}


/**
 *	This method updates a drag and drop operation, finds out if the mouse is
 *	hovering on top of the list or the tree view, and if so, it lets them know.
 *
 *	@param itemInfo	Item being dragged.
 *	@param endDrag	True if this is the last update, false otherwise.
 *	@return		The Asset Browser under the mouse, or NULL if none.
 */
UalDialog* UalManager::updateDrag( const UalItemInfo& itemInfo, bool endDrag )
{
	BW_GUARD;

	for( DialogsItr i = dialogs_.begin(); i != dialogs_.end(); ++i )
		if ( (*i)->updateDrag( itemInfo, endDrag ) )
			return *i;
	return 0;
}


/**
 *	This method copies a virtual folder from an Asset Browser dialog and puts
 *	it into another Asset Browser dialog.
 *
 *	@param srcUal	Dialog where the virtual folder was dragged from.
 *	@param dstUal	Dialog where the dragged folder has been dropped.
 *	@param ii		Item information corresponding to the virtual folder.
 */
void UalManager::copyVFolder( UalDialog* srcUal, UalDialog* dstUal, const UalItemInfo& ii )
{
	BW_GUARD;

	if ( !srcUal || !dstUal )
		return;

	if ( ii.assetInfo().longText().empty() )
	{
		// It's a VFolder
		int oldCount = dstUal->folderTree_.getLevelCount();
		// it's a VFolder, so try to create it from the custom folders
		dstUal->loadCustomVFolders( srcUal->customVFolders_, ii.assetInfo().text() );
		if ( oldCount < dstUal->folderTree_.getLevelCount() )
		{
			// it was created from a custom vfolder, so add it to the new dialog's custom vfolders
			BW::vector<DataSectionPtr> customVFolders;
			srcUal->customVFolders_->openSections( "customVFolder", customVFolders );
			for( BW::vector<DataSectionPtr>::iterator s = customVFolders.begin();
				s != customVFolders.end(); ++s )
			{
				if ( ii.assetInfo().text() == (*s)->asWideString() )
				{
					if ( !dstUal->customVFolders_ )
						dstUal->customVFolders_ = new XMLSection( "customVFolders" );
					DataSectionPtr section = dstUal->customVFolders_->newSection( "customVFolder" );
					section->copy( *s );
					break;
				}
			}
		}

		// if there's a vfolder named the same, load it too. If not, this call
		// is still needed in order to build the excludeVFolders_ vector properly
		BW::string nconfigFile;
		bw_wtoutf8( configFile_, nconfigFile );
		DataSectionPtr root = BWResource::openSection( nconfigFile );
		if ( root )
			dstUal->loadVFolders( root->openSection( "VFolders" ), ii.assetInfo().text() );
	}
	else if ( ii.folderExtraData_ )
	{
		// it's not a VFolder, so create a custom folder from scratch.
		// For now, only Files VFolder items are clonable, so only managing custom Files-derived VFolders
		if ( !dstUal->customVFolders_ )
			dstUal->customVFolders_ = new XMLSection( "customVFolders" );
		DataSectionPtr section = dstUal->customVFolders_->newSection( "customVFolder" );
		section->setWideString( ii.assetInfo().text() );

		// find out if it inherits from a customVFolder or a VFolder
		BW::wstring inheritName = ((VFolder*)ii.folderExtraData_)->getName();
		if ( srcUal->customVFolders_ )
		{
			BW::vector<DataSectionPtr> customVFolders;
			srcUal->customVFolders_->openSections( "customVFolder", customVFolders );
			for( BW::vector<DataSectionPtr>::iterator s = customVFolders.begin();
				s != customVFolders.end(); ++s )
			{
				if ( inheritName == (*s)->asWideString() )
				{
					inheritName = (*s)->readWideString( "inheritsFrom" );
					break;
				}
			}
		}
		section->writeWideString( "inheritsFrom", inheritName );

		section->writeWideString( "path", ii.assetInfo().longText() );
		dstUal->loadCustomVFolders( dstUal->customVFolders_, ii.assetInfo().text() );
		// build the excludeVFolders_ vector properly (using a special label to exclude all default vfolders)
		BW::string nconfigFile;
		bw_wtoutf8( configFile_, nconfigFile );
		DataSectionPtr root = BWResource::openSection( nconfigFile );
		if ( root )
			dstUal->loadVFolders( root->openSection( "VFolders" ), L"***EXCLUDE_ALL***" );
	}
	// set folder custom info
	VFolderPtr srcVFolder = srcUal->folderTree_.getVFolder( ii.assetInfo().text(), false );
	VFolderPtr dstVFolder = dstUal->folderTree_.getVFolder( ii.assetInfo().text() );
	if ( srcVFolder && dstVFolder )
	{
		UalFolderData* srcData = (UalFolderData*)srcVFolder->getData();
		UalFolderData* dstData = (UalFolderData*)dstVFolder->getData();
		if ( srcData && dstData )
			dstData->thumbSize_ = srcData->thumbSize_;
	}
}

BW_END_NAMESPACE

