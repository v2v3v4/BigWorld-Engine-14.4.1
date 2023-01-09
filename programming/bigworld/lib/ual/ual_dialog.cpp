// UalDialog.cpp : implementation file
//

#include "pch.hpp"
#include "filter_holder.hpp"
#include "ual_dialog.hpp"
#include "ual_manager.hpp"
#include "ual_history.hpp"
#include "ual_favourites.hpp"

#include "ual_name_dlg.hpp"

#include "common/string_utils.hpp"

#include "controls/user_messages.hpp"

#include "thumbnail_manager.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/debug.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_toolbar.hpp"


DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

// Internal type for passing custom data to FolderTree::selectVFolderCustom.
typedef std::pair<UalDialog*,const wchar_t*> vfolderTagTestData;


// Minimum size of a splitter pane (either the tree view or the list).
static const int MIN_SPLITTER_PANE_SIZE = 16;


// Implementation for the Asset Browser's dialog GUITABS id.
const BW::wstring UalDialog::contentID = L"UAL";


/**
 *	Constructor.
 *
 *	@param configFile	Path of the configuration file. See the ual_config.xml
 *						file in the file grammar guide for more info.
 */
UalDialog::UalDialog( const BW::wstring& configFile )
	: CDialog(UalDialog::IDD, 0)
	, configFile_( configFile )
	, fileListProvider_( new ListFileProvider( UalManager::instance().thumbnailManager().postfix() ) )
	, xmlListProvider_( new ListXmlProvider() )
	, historyListProvider_( new ListXmlProvider() )
	, favouritesListProvider_( new ListXmlProvider() )
	, splitterBar_( 0 )
	, dlgShortCaption_( Localise(L"UAL/UAL_DIALOG/SHORT_CAPTION") )
	, dlgLongCaption_( Localise(L"UAL/UAL_DIALOG/LONG_CAPTION") )
	, preferredWidth_( 290 ) , preferredHeight_( 380 )
	, hicon_( 0 )
	, layoutVertical_( true )
	, layoutLastRowSize_( 0 ) , layoutLastColSize_( 0 )
	, defaultSize_( 100 )
	, folderTree_( &UalManager::instance().thumbnailManager() )
	, smartList_( &UalManager::instance().thumbnailManager() )
	, showFilters_( false )
	, lastFocus_( 0 )
	, customVFolders_( 0 )
{
	BW_GUARD;

	lastLanguage_ =
		StringProvider::instance().currentLanguage()->getIsoLangName() + L"_" +
		StringProvider::instance().currentLanguage()->getIsoCountryName();

	UalManager::instance().registerDialog( this );
}


/**
 *	Destructor.
 */
UalDialog::~UalDialog()
{
	BW_GUARD;

	if( hicon_ )	DeleteObject( hicon_ );
	bw_safe_delete(splitterBar_);
	UalManager::instance().unregisterDialog( this );
}


/**
 *	This static method registers virtual folder loaders in the loader registry.
 *
 *	@param loader	Virtual folder loader to register.
 */
/*static*/ void UalDialog::registerVFolderLoader( UalVFolderLoaderPtr loader )
{
	BW_GUARD;

	LoaderRegistry::loaders().push_back( loader );
}


/**
 *	This static method clears any static resources allocated by this dialog.
 */
/*static*/ void UalDialog::fini()
{
	BW_GUARD;

	LoaderRegistry::loaders().clear();
}


/**
 *	This MFC method initialises the dialog's controls into their instances.
 *
 *	@param pDX	MFC data exchange struct.
 */
void UalDialog::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UALTREE, folderTree_);
	DDX_Control(pDX, IDC_UALLIST, smartList_);
	DDX_Control(pDX, IDC_UALSEARCHBK, search_);
	DDX_Control(pDX, IDC_UALSTATUS, statusBar_);
}


/**
 *	This method handles GUITABS right-click events, such as right-clicking on
 *	the panel's caption bar or tab.
 *
 *	@param loader	Virtual folder loader to register.
 */
void UalDialog::handleRightClick( int x, int y )
{
	BW_GUARD;

	showContextMenu( 0 );
}


/**
 *	This method loads the dialog from a GUITABS layout section.
 *
 *	@param section	Data section containing the panel's layout.
 *	@return		True if loaded successfully, false if it failed.
 */
bool UalDialog::load( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
	{
		error( "Problems loading from guitabs layout file." );
		return false;
	}

	// load basic layout info from the guitabs layout Content section

	if ( lastLanguage_ == section->readWideString( "lastLanguage", lastLanguage_ ) )
	{
		// only read the custom names if the language is the same.
		dlgShortCaption_ = section->readWideString( "shortCaption", dlgShortCaption_ );
		dlgLongCaption_ = section->readWideString( "longCaption", dlgLongCaption_ );
	}
	int size = section->readInt( "initialTreeSize", defaultSize_ );
	if ( size < 0 )
		error( "invalid defaultSize. Should be greater or equal to zero." );
	else
		defaultSize_ = size;
	setLayout( section->readBool( "layoutVertical", layoutVertical_ ), true );
	showFilters_ = section->readBool( "filtersVisible", showFilters_ );

	customVFolders_ = new XMLSection( "customVFolders" );
	customVFolders_->copy( section );
	loadCustomVFolders( customVFolders_ );

	loadVFolderExcludeInfo( section );

	BW::vector<DataSectionPtr> sections;
	section->openSections( "VFolderData", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		VFolderPtr vfolder = folderTree_.getVFolder( (*s)->asWideString() );
		if ( vfolder )
		{
			UalFolderData* data = (UalFolderData*)vfolder->getData();
			if ( data )
				data->thumbSize_ = (*s)->readInt( "thumbSize" );
		}
	}

	folderTree_.setVFolderOrder( section->readWideString( "vfolderOrder" ) );
	folderTree_.selectVFolder( section->readWideString( "lastVFolder" ) );

	return true;
}


/**
 *	This method saves the dialog to a GUITABS layout section.
 *
 *	@param section	Data section to save the panel's layout to.
 *	@return		True if saved successfully, false if it failed.
 */
bool UalDialog::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
	{
		error( "Problems saving to guitabs layout file." );
		return false;
	}

	// save basic layout info in the guitabs layout Content section
	section->writeWideString( "lastLanguage", lastLanguage_ );
	section->writeWideString( "shortCaption", dlgShortCaption_ );
	section->writeWideString( "longCaption", dlgLongCaption_ );
	if ( splitterBar_->GetSafeHwnd() )
	{
		int size;
		int min;		
		if ( layoutVertical_ )
			splitterBar_->GetRowInfo( 0, size, min );
		else
			splitterBar_->GetColumnInfo( 0, size, min );
		if ( size < MIN_SPLITTER_PANE_SIZE )
			size = MIN_SPLITTER_PANE_SIZE;
		section->writeInt( "initialTreeSize", size );
	}
	section->writeBool( "layoutVertical", layoutVertical_ );
	section->writeBool( "filtersVisible", showFilters_ );

	// save vfolder extra data, such as thumbSize
	BW::vector<HTREEITEM> treeItems;
	folderTree_.getVFolders( treeItems );
	for( BW::vector<HTREEITEM>::iterator i = treeItems.begin();
		i != treeItems.end(); ++i )
	{
		VFolderItemData* itemData = (VFolderItemData*)folderTree_.GetItemData( *i );
		if ( !itemData->isVFolder() && !itemData->getVFolder() )
			continue;
		UalFolderData* data = (UalFolderData*)(itemData->getVFolder()->getData());
		if ( data && data->thumbSize_ != data->originalThumbSize_ )
		{
			DataSectionPtr folderSection = section->newSection( "VFolderData" );
			BW::string nitemtext;
			bw_wtoutf8( (LPCTSTR)folderTree_.GetItemText( *i ), nitemtext );
			folderSection->setString( nitemtext );
			folderSection->writeInt( "thumbSize", data->thumbSize_ );
		}
	}

	// save excludeVFolders data
	BW::wstring excluded;
	for( BW::vector<BW::wstring>::iterator i = excludeVFolders_.begin();
		i != excludeVFolders_.end(); ++i )
	{
		if ( !excluded.empty() )
			excluded += L";";
		excluded += (*i);
	}
	if ( !excluded.empty() )
		section->writeWideString( "excludeVFolder", excluded );

	// save customVFolders
	if ( customVFolders_ )
	{
		BW::vector<DataSectionPtr> sections;
		customVFolders_->openSections( "customVFolder", sections );
		for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
		{
			DataSectionPtr customVFolder = section->newSection( "customVFolder" );
			customVFolder->copy( *s );
		}
	}

	// save vfolder order
	section->writeWideString( "vfolderOrder", folderTree_.getVFolderOrder() );

	// save last selected item
	HTREEITEM item = folderTree_.GetSelectedItem();
	VFolderItemData* data = 0;
	if ( item )
		data = (VFolderItemData*)folderTree_.GetItemData( item );
	if ( data )
	{
		VFolderPtr lastVFolder = folderTree_.getVFolder( data );
		if ( lastVFolder )
			section->writeWideString( "lastVFolder", lastVFolder->getName() );
	}

	return true;
}


/**
 *	This method handles clicking on the "clone" button of the GUITABS panel's
 *	caption bar, by creating a new UalDialog with the same characteristics as
 *	the one the click occurred in.
 *
 *	@return		New UalDialog cloned from "this".
 */
GUITABS::ContentPtr UalDialog::clone()
{
	BW_GUARD;

	UalDialogFactory ualFactory;
	UalDialog* newUal = ualFactory.createUal( configFile_ );

	// copy settings to the new UAL
	int min;
	if ( layoutVertical_ )
	{
		splitterBar_->GetRowInfo( 0, newUal->layoutLastRowSize_, min );
		newUal->layoutLastColSize_ = layoutLastColSize_;
	}
	else
	{
		splitterBar_->GetColumnInfo( 0, newUal->layoutLastColSize_, min );
		newUal->layoutLastRowSize_ = layoutLastRowSize_;
	}
	newUal->defaultSize_ = defaultSize_;
	newUal->setLayout( layoutVertical_ );
	newUal->showFilters_ = showFilters_;

	if ( lastItemInfo_.isFolder_ && lastItemInfo_.dialog_ )
	{
		// is the result of dragging and dropping a folder, so clone using that info
		newUal->folderTree_.clear();

		UalManager::instance().copyVFolder( this, newUal, lastItemInfo_ );

		if ( newUal->folderTree_.GetCount() )
			newUal->folderTree_.SelectItem( newUal->folderTree_.GetChildItem( TVI_ROOT ) );
	}
	else
	{
		// it's not being clone because of a drag&drop operation, so do standard stuff
		newUal->customVFolders_ = new XMLSection( "customVFolders" );
		if ( !!customVFolders_ )
			newUal->customVFolders_->copy( customVFolders_ );
		newUal->loadCustomVFolders( newUal->customVFolders_ );

		for( BW::vector<BW::wstring>::iterator i = excludeVFolders_.begin();
			i != excludeVFolders_.end(); ++i )
		{
			newUal->folderTree_.removeVFolder( *i );
			newUal->excludeVFolders_.push_back( *i );
		}

		newUal->folderTree_.setVFolderOrder( folderTree_.getVFolderOrder() );

		// set folder custom info
		BW::vector<HTREEITEM> treeItems;
		folderTree_.getVFolders( treeItems );
		for( BW::vector<HTREEITEM>::iterator i = treeItems.begin();
			i != treeItems.end(); ++i )
		{
			if ( !(*i) )
				continue;
			VFolderPtr srcVFolder = folderTree_.getVFolder(
				(VFolderItemData*)folderTree_.GetItemData( *i ) );
			VFolderPtr dstVFolder = newUal->folderTree_.getVFolder( (LPCTSTR)folderTree_.GetItemText( *i ) );
			if ( srcVFolder && dstVFolder )
			{
				UalFolderData* srcData = (UalFolderData*)srcVFolder->getData();
				UalFolderData* dstData = (UalFolderData*)dstVFolder->getData();
				if ( srcData && dstData )
					dstData->thumbSize_ = srcData->thumbSize_;
			}
		}
	}

	// just in case, reset some key values
	lastItemInfo_.dialog_ = 0;
	lastItemInfo_.folderExtraData_ = 0;

	return newUal;
}


/**
 *	This method saves the dialog's layout to the config file.
 *	TODO: This method seems like it's now longer use.  If so, remove.
 *
 *	@return		New UalDialog cloned from "this".
 */
void UalDialog::saveConfig()
{
	BW_GUARD;

	if ( configFile_.empty() )
	{
		error( "No config file specified." );
		return;
	}

	BW::string nconfigFile;
	bw_wtoutf8( configFile_, nconfigFile );
	DataSectionPtr root = BWResource::openSection( nconfigFile );
	if ( !root )
	{
		error( "Couldn't save config file." );
		return;
	}
	DataSectionPtr config = root->openSection( "Config" );
	if ( !config )
	{
		error( "Couldn't create Config section. Couldn't save config file." );
		return;
	}

	save( config );

	root->save();
}


/**
 *	This method loads the dialog's configuration from the config file.
 *
 *	@param fname	Optional override of the config file path passed in during
 *					construction.
 *	@return		True if successful, false if not.
 */
bool UalDialog::loadConfig( const BW::wstring fname )
{
	BW_GUARD;

	if ( !fname.empty() )
		configFile_ = fname;

	if ( configFile_.empty() )
	{
		error( "No config file specified." );
		return false;
	}

	BW::string nconfigFile;
	bw_wtoutf8( configFile_, nconfigFile );
	BWResource::instance().purge( nconfigFile );
	DataSectionPtr root = BWResource::openSection( nconfigFile );

	if ( !root )
	{
		error( "Couldn't load config file." );
		return false;
	}

	loadMain( root->openSection( "Config" ) );
	loadToolbar( root->openSection( "Toolbar" ) );
	loadFilters( root->openSection( "Filters" ) );
	loadVFolders( root->openSection( "VFolders" ) );
	return true;
}


/**
 *	This method creates an icon according to who it's specified in an XML
 *	section.
 *
 *	@param section	XML section containing the icon creation parameters.
 *	@param item		Tag name of the XML element containing the parameters.
 *	@return		The desired icon if successful, NULL if not.
 */
HICON UalDialog::iconFromXml( DataSectionPtr section, BW::string item )
{
	BW_GUARD;

	BW::string icon = section->readString( item );
	if ( icon.empty() )
		return 0;

	int iconNum = atoi( icon.c_str() );

	HICON ret = 0;
	if ( iconNum != 0 )
	{
		ret = AfxGetApp()->LoadIcon( section->readInt( "icon" ) );
		if ( !ret )
			error( BW::string( "Couldn't load icon resource for VFolder " ) + section->asString() );
	}
	else
	{
		icon = BWResource::findFile( icon );
		BW::wstring wicon;
		bw_utf8tow( icon, wicon );
		ret = (HICON)LoadImage( AfxGetInstanceHandle(),
			wicon.c_str(), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_LOADFROMFILE );
		if ( !ret )
			error( BW::string( "Couldn't load icon file for VFolder " ) + section->asString() );
	}

	return ret;
}


/**
 *	This method loads the configuration parameters that are global to the
 *	dialog, and not directly related to the assets.
 *
 *	@param section	Data section corresponding to the "Config" section.
 */
void UalDialog::loadMain( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return;

	dlgShortCaption_ = Localise( section->readWideString( "shortCaption", dlgShortCaption_ ).c_str() );
	dlgLongCaption_ = Localise( section->readWideString( "longCaption", dlgLongCaption_ ).c_str() );
	hicon_ = iconFromXml( section, "icon" );
	int width = section->readInt( "preferredWidth", preferredWidth_ );
	if ( width < 1 )
		error( "invalid preferredWidth. Should be greater than zero." );
	else
		preferredWidth_ = width;

	int height = section->readInt( "preferredHeight", preferredHeight_ );
	if ( height < 1 )
		error( "invalid preferredHeight. Should be greater than zero." );
	else
		preferredHeight_ = height;

	int size = section->readInt( "initialTreeSize", defaultSize_ );
	if ( size < 0 )
		error( "invalid defaultSize. Should be greater or equal to zero." );
	else
		defaultSize_ = size;
	
	fileListProvider_->setThreadYieldMsec(
		section->readInt(
			"threadYieldMsec",
			fileListProvider_->getThreadYieldMsec() ) );

	fileListProvider_->setThreadPriority(
		section->readInt(
			"threadPriority",
			fileListProvider_->getThreadPriority() ) );

	setLayout( section->readBool( "layoutVertical", layoutVertical_ ), true );
	showFilters_ = section->readBool( "filtersVisible", showFilters_ );
	folderTree_.setSortVFolders( section->readBool( "sortVFolders", true ) );
	folderTree_.setSortSubFolders( section->readBool( "sortSubFolders", true ) );
	int maxCache = section->readInt( "maxCacheItems", 200 );
	if ( maxCache < 0 )
		error( "invalid maxCacheItems. Should be greater or equal to zero." );
	else
		smartList_.setMaxCache( maxCache );
	smartList_.SetIconSpacing(
			section->readInt( "iconSpacingX", 90 ),
			section->readInt( "iconSpacingY", 100 )
		);
	filtersCtrl_.setPushlike( section->readBool( "pushlikeFilters", false ) );
	search_.idleText( 
		section->readWideString( "searchIdleText", Localise(L"UAL/UAL_DIALOG/DEFAULT_SEARCH_IDLE_TEXT") ) );
}


/**
 *	This method loads the GUIMANAGER toolbar used in the dialog for the refresh
 *	and layout buttons.
 *
 *	@param section	Data section corresponding to the "Toolbar" section.
 */
void UalDialog::loadToolbar( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section || !section->countChildren() )
		return;

	for( int i = 0; i < section->countChildren(); ++i )
		GUI::Manager::instance().add( new GUI::Item( section->openChild( i ) ) );

	toolbar_.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,1,1), this, 0 );
	toolbar_.SetBitmapSize( CSize( 16, 16 ) );
	toolbar_.SetButtonSize( CSize( 24, 22 ) );

	CToolTipCtrl* tc = toolbar_.GetToolTips();
	if ( tc )
		tc->SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	GUI::Toolbar* guiTB = new GUI::Toolbar( "UalToolbar", toolbar_ );
	GUI::Manager::instance().add( guiTB );

	SIZE tbSize = guiTB->minimumSize();
	toolbar_.SetWindowPos( 0, 0, 0, tbSize.cx, tbSize.cy, SWP_NOMOVE | SWP_NOZORDER );
}


/**
 *	This method loads filters that are available when clicking on the
 *	magnifying glass to the left of the search text box.
 *
 *	@param section	Data section corresponding to the "Filters" section.
 */
void UalDialog::loadFilters( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return;

	BW::vector<DataSectionPtr> filters;
	section->openSections( "Filter", filters );
	for( BW::vector<DataSectionPtr>::iterator s = filters.begin(); s != filters.end(); ++s )
	{
		FilterSpecPtr filterSpec;
		if ( (*s)->readBool( "separator", false ) )
		{
			filterSpec = new FilterSpec( L"" );
		}
		else
		{
			BW::wstring name = (*s)->asWideString();
			BW::wstring group = (*s)->readWideString( "group", L"" );

			BW::wstring str[2];
			BW::string secstrs[2] = { "include", "exclude" };
			for( int i = 0; i < 2; ++i )
			{
				BW::vector<DataSectionPtr> sections;
				(*s)->openSections( secstrs[i], sections );
				for( BW::vector<DataSectionPtr>::iterator ss = sections.begin(); ss != sections.end(); ++ss )
				{
					if ( !str[i].empty() )
						str[i] += L";";
					str[i] += (*ss)->asWideString();
				}
				sections.clear();
			}
			filterSpec = new FilterSpec( name, false, str[0], str[1], group );
			if ( str[0].empty() && str[1].empty() )
			{
				BW::string nname;
				bw_wtoutf8( name, nname );
				error( BW::string( "Filter " ) + nname + " has no include nor exclude tags." );
			}
		}
		filterHolder_.addFilter( filterSpec );
	}
}


/**
 *	This method loads the actual content of the dialog, which is, the virtual
 *	folders that will load and display the different asset types.
 *
 *	@param section	Data section corresponding to the "VFolders" section.
 *	@param loadOneName	If not empty, only the VFolder named as the contents of
 *						loadOneName is loaded.
 *	@param parent	Loaded folders will be placed under this parent VFolder.	
 */
void UalDialog::loadVFolders( DataSectionPtr section, const BW::wstring& loadOneName, VFolderPtr parent )
{
	BW_GUARD;

	if ( !section )
		return;

	for( int i = 0; i < section->countChildren(); ++i )
	{
		DataSectionPtr child = section->openChild( i );
		VFolderPtr vfolder = loadVFolder( child, loadOneName, parent );
		if ( vfolder != NULL )
		{
			if ( vfolder->subVFolders() )
			{
				// look and load nested vfolders
				loadVFolders( child, L"", vfolder );
			}
		}
		else if ( !loadOneName.empty() )
		{
			// Check to see if this vfolder has subVFolders.
			UalVFolderLoaderPtr loader = LoaderRegistry::loader( child->sectionName() );
			if ( loader != NULL && loader->subVFolders() )
			{
				// look for loadOneName in the nested folders and load it at the parent's level
				loadVFolders( child, loadOneName, parent );
			}
		}
	}
}


/**
 *	This method loads a single virtual folder and returns it.
 *
 *	@param section	Data section corresponding to the "VFolders" section.
 *	@param loadOneName	If empty, it loads the VFolder in the data section. If
 *						not empty, it loads the VFolder only if the section's
 *						name is the same as the contents of loadOneName.
 *	@param parent	The loaded folder will be placed under this parent VFolder.	
 *	@return			The virtual folder just loaded, or NULL if unsuccessful.
 */
VFolderPtr UalDialog::loadVFolder( DataSectionPtr section, const BW::wstring& loadOneName, VFolderPtr parent, DataSectionPtr customData )
{
	BW_GUARD;

	if ( !section )
		return 0;

	if ( section->asString().empty() )
	{
		error( BW::string("A VFolder of type '") + section->sectionName() + "' has no name in the XML config file." );
		return 0;
	}

	if ( loadOneName != L"***EXCLUDE_ALL***" &&
		( loadOneName.empty() || loadOneName == section->asWideString() ) )
	{
		UalVFolderLoaderPtr loader = LoaderRegistry::loader( section->sectionName() );

		if ( loader == NULL )
		{
			// it's not a recognized vfolder section, so return.
			// Note: This early error doesn't seem to get caught by WE at the moment,
			// probably because WE registers it's error callback after this.
			error( "VFolder type '" + section->sectionName() + "' could not be loaded" );
			return 0; 
		}

		VFolderPtr vfolder = loader->load( this, section, parent, customData, true/*addToFolderTree*/ );

		if ( !vfolder )
			return 0; // test passed but load failed.

		// remove it from the exclude list, if it's in
		for( BW::vector<BW::wstring>::iterator i = excludeVFolders_.begin();
			i != excludeVFolders_.end(); )
			if ( (*i) == section->asWideString() )
				i = excludeVFolders_.erase( i );
			else
				++i;
		return vfolder;
	}
	else
	{
		// if not created already, exclude it
		if ( !folderTree_.getVFolder( section->asWideString() )  &&
			std::find(
				excludeVFolders_.begin(),
				excludeVFolders_.end(),
				section->asWideString() ) == excludeVFolders_.end() )
			excludeVFolders_.push_back( section->asWideString() );
	}

	return 0;
}


/**
 *	This method loads the "excludeVFolder" information, which marks virtual
 *	folders that the user has removed in the UI.  Since the VFolders stay in
 *	the config file in case the user wants to revert his actions, when delete
 *	a virtual folder is simply added to this list.
 *
 *	@param section	Data section corresponding to the dialog's GUITABS section.
 */
void UalDialog::loadVFolderExcludeInfo( DataSectionPtr section )
{
	BW_GUARD;

	excludeVFolders_.clear();
	BW::vector<BW::wstring> excluded;
	BW::vector<DataSectionPtr> excludeVFolders;
	section->openSections( "excludeVFolder", excludeVFolders );
	for( BW::vector<DataSectionPtr>::iterator s = excludeVFolders.begin();
		s != excludeVFolders.end(); ++s )
	{
		excluded.clear();
		bw_tokenise( (*s)->asWideString(), L";,", excluded );
		for( BW::vector<BW::wstring>::iterator i = excluded.begin();
			i != excluded.end();
			++i )
		{
			if ( !(*i).empty() )
			{
				folderTree_.removeVFolder( (*i) );
				if ( std::find(
								excludeVFolders_.begin(),
								excludeVFolders_.end(),
								*i ) == excludeVFolders_.end() )
					excludeVFolders_.push_back( *i );
			}
		}
	}
}


/**
 *	This method loads the custom virtual folders from the GUITABS section.
 *	These are subfolders of a virtual folder that have been dragged out into
 *	another top-level folder or dialog.
 *
 *	@param section	Data section corresponding to the dialog's GUITABS section.
 *	@param loadOneName	If empty, it loads the VFolder in the data section. If
 *						not empty, it loads the VFolder only if the section's
 *						name is the same as the contents of loadOneName.
 */
void UalDialog::loadCustomVFolders( DataSectionPtr section, const BW::wstring& loadOneName )
{
	BW_GUARD;

	if ( !section )
		return;

	BW::vector<DataSectionPtr> customVFolders;
	section->openSections( "customVFolder", customVFolders );
	if ( customVFolders.empty() )
		return;

	BW::string nconfigFile;
	bw_wtoutf8( configFile_, nconfigFile );
	DataSectionPtr root = BWResource::openSection( nconfigFile );
	if ( !root )
		return;
	DataSectionPtr vfolders = root->openSection( "VFolders" );
	if ( !vfolders )
		return;

	for( BW::vector<DataSectionPtr>::iterator s = customVFolders.begin();
		s != customVFolders.end(); ++s )
	{
		BW::wstring inheritsFrom = (*s)->readWideString( "inheritsFrom" );
		if ( inheritsFrom.empty() )
			continue;

		if ( loadOneName.empty() || loadOneName == (*s)->asWideString() )
			VFolderPtr vfolder = loadFromBaseVFolder( vfolders, inheritsFrom, (*s) );
	}
}


/**
 *	This method loads a sub folder (or custom folder) by usings the the
 *	original VFolder it was dragged out from.
 *	These are subfolders of a virtual folder that have been dragged out into
 *	another top-level folder or dialog, and use the "customData" to override
 *	some of the loading info in the parent VFolder.
 *
 *	@param section	Data section corresponding to the dialog's GUITABS section.
 *	@param baseName	Name of the VFolder in the config file where this custom
 *					folder came from originaly.
 *	@param customData	Subfolder-specific data that will override the original
 *						generic config in the base VFolder.
 *	@param parent	Parent virtual folder to place it under in the dialog.
 *	@return		The custom virtual folder just created.
 */
VFolderPtr UalDialog::loadFromBaseVFolder( DataSectionPtr section, const BW::wstring& baseName, DataSectionPtr customData, VFolderPtr parent )
{
	BW_GUARD;

	if ( !section )
		return 0;

	for( int i = 0; i < section->countChildren(); ++i )
	{
		DataSectionPtr child = section->openChild( i );
		if ( baseName == child->asWideString() )
		{
			VFolderPtr vfolder = loadVFolder( child, L"", parent, customData );
			return vfolder;
		}
		// look for nested vfolders, but gonna load it at the root level
		VFolderPtr vfolder = loadFromBaseVFolder( child, baseName, customData, parent );
		if ( vfolder )
			return vfolder;
	}
	return 0;
}


/**
 *	This method allows for updating an asset in the dialog, both in the tree
 *	view and in the list.
 *
 *	@param longText	Full text describing the item, usually a file path.
 */
void UalDialog::updateItem( const BW::wstring& longText )
{
	BW_GUARD;

	if ( !GetSafeHwnd() || longText.empty() )
		return;

	BW::wstring longTextTmp = longText;
	std::replace( longTextTmp.begin(), longTextTmp.end(), L'/', L'\\' );
	BW::wstring textTmp = longTextTmp.c_str() + longTextTmp.find_last_of( L'\\' ) + 1;

	if ( folderTree_.GetSafeHwnd() )
		folderTree_.updateItem( AssetInfo( L"", textTmp, longTextTmp ) );
	if ( smartList_.GetSafeHwnd() )
		smartList_.updateItem( AssetInfo( L"", textTmp, longTextTmp ) );
}


/**
 *	This static method is used as a FolderTree::ItemTextCB for finding an item
 *	in the tree view.
 *
 *	@param item		Tree view item handle currently being tested.
 *	@param testData	Custom data passed to the callback, in this case it's a
 *					"vfolderTagTestData".
 */
/*static*/ bool UalDialog::vfolderFindByTag( HTREEITEM item, void* testData )
{
	BW_GUARD;

	if ( !testData )
		return false;

	vfolderTagTestData dlgInfo = *(vfolderTagTestData*)testData;
	UalDialog* dlg = dlgInfo.first;
	const wchar_t* vfolderName = dlgInfo.second;

	VFolderItemDataPtr data = (VFolderItemData*)dlg->folderTree_.GetItemData( item );
	if ( !vfolderName || !data || !data->isVFolder() )
		return false;

	VFolderPtr vfolder = data->getVFolder();
	if ( !vfolder )
		return false;
	
	UalFolderData* folderData = (UalFolderData*)vfolder->getData();
	if ( !folderData )
		return false;

	return folderData->internalTag_ == vfolderName;
}


/**
 *	This method ensures an item is visible in the list, and that its
 *	corresponding VFolder is visible in the tree view.
 *
 *	@param vfolder	Name of the virtual folder containing the item.
 *	@param longText	Full text description of the item, usualy a file path.	
 */
void UalDialog::showItem( const BW::wstring& vfolder, const BW::wstring& longText )
{
	BW_GUARD;

	if ( !GetSafeHwnd() || vfolder.empty() || longText.empty() )
		return;

	BW::wstring longTextTmp = longText;
	std::replace( longTextTmp.begin(), longTextTmp.end(), L'/', L'\\' );

	if ( folderTree_.GetSafeHwnd() )
	{
		vfolderTagTestData testData( this, vfolder.c_str() );
		folderTree_.selectVFolderCustom( vfolderFindByTag, &testData );
	}

	if ( smartList_.GetSafeHwnd() )
	{
		BW::wstring textTmp = longTextTmp.c_str() + longTextTmp.find_last_of( L'\\' ) + 1;
		if ( !smartList_.showItem( AssetInfo( L"", textTmp, longTextTmp ) ) )
			delayedListShowItem_ = longTextTmp;
	}
}


/**
 *	This method initialises the filters control.
 */
void UalDialog::buildFiltersCtrl()
{
	BW_GUARD;

	filtersCtrl_.Create(
		AfxRegisterWndClass( 0, 0, GetSysColorBrush( COLOR_BTNFACE ), 0 ),
		L"", WS_VISIBLE | WS_CHILD, CRect( 0, 0, 1, 1 ), this, 0 );
	filtersCtrl_.setEventHandler( this );
}


/**
 *	This method initialises the tree view control.
 */
void UalDialog::buildFolderTree()
{
	BW_GUARD;

	folderTree_.init();

	folderTree_.setEventHandler( this );
}


/**
 *	This method initialises the list control.
 */
void UalDialog::buildSmartList()
{
	BW_GUARD;

	xmlListProvider_->setFilterHolder( &filterHolder_ );
	historyListProvider_->setFilterHolder( &filterHolder_ );
	favouritesListProvider_->setFilterHolder( &filterHolder_ );
	fileListProvider_->setFilterHolder( &filterHolder_ );

	smartList_.SetIconSpacing( 90, 90 );

	smartList_.init( 0, 0 );
	smartList_.setEventHandler( this );
}


/**
 *	This MFC method is called on dialog initialisation, and creates all the 
 *	dialog's UI elements and configures them from XML.
 *
 *	@return		Always return TRUE.
 */
BOOL UalDialog::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	setLayout( layoutVertical_, true );

	search_.init( MAKEINTRESOURCE( IDB_UALHIDEFILTERS ),
					MAKEINTRESOURCE( IDB_UALSEARCHCLOSE ), L"",
					Localise(L"UAL/UAL_DIALOG/TOOLTIP_SEARCH_FILTERS"),
					Localise(L"UAL/UAL_DIALOG/TOOLTIP_SEARCH") );
	if( toolTip_.CreateEx( this, 0, WS_EX_TOPMOST ) )
	{
		toolTip_.SetMaxTipWidth( SHRT_MAX );
		toolTip_.AddTool( &statusBar_, L"" );
		toolTip_.SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
		toolTip_.Activate( TRUE );
	}
	setStatusText( L"" );

	buildFolderTree();
	buildSmartList();
	buildFiltersCtrl();

	if ( configFile_.length() )
		loadConfig();

	buildSmartListFilters();

	return TRUE;
}


/**
 *	This method refreshes the contents of the tree view and the list, usually
 *	in response to the user clicking the refresh button on the dialog's
 *	toolbar.
 *
 *	@return		True if successful.
 */
bool UalDialog::guiActionRefresh()
{
	BW_GUARD;

	HTREEITEM sel = folderTree_.GetSelectedItem();
	if ( sel )
	{
		VFolderItemData* data = (VFolderItemData*)folderTree_.GetItemData( sel );
		if ( data )
		{
			// save search text in case selection changes
			BW::wstring oldSearch = search_.searchText();

			HTREEITEM oldSel = folderTree_.GetSelectedItem();
			folderTree_.refreshVFolder( folderTree_.getVFolder( data ) );
			HTREEITEM sel = folderTree_.GetSelectedItem();
			if ( oldSel != sel && sel )
			{
				folderTreeSelect( (VFolderItemData*)folderTree_.GetItemData( sel ) );
				search_.searchText( oldSearch );
				return true;
			}
			else
			{
				search_.searchText( oldSearch );
			}
		}
	}
	smartList_.refresh();
	return true;
}


/**
 *	This method toggles the layout of the internals of the dialog, usually
 *	in response to the user clicking the layout button on the dialog's
 *	toolbar.
 *
 *	@return		True if successful.
 */
bool UalDialog::guiActionLayout()
{
	BW_GUARD;

	setLayout( !layoutVertical_ );
	return true;
}


/**
 *	This method aids in adjusting the size and placement of the search text
 *	box and the dialog's toolbar when the dialog is resized.
 *
 *	@param width	New width for the dialog.
 *	@param height	New height for the dialog.
 */
void UalDialog::adjustSearchSize( int width, int height )
{
	BW_GUARD;

	const int xmargin = 4;
	const int ymargin = 6;
	const int ysearch = 19;
	const int gap = 2;
	const int minSearchX = 90;

	if (search_.GetSafeHwnd())
	{
		CRect trect( 0, 0, 0, 0 );
		if (toolbar_.GetSafeHwnd())
		{
			toolbar_.GetWindowRect( &trect );
		}
		CRect rect;
		if (width - trect.Width() < minSearchX)
		{
			if (toolbar_.GetSafeHwnd())
			{
				toolbar_.SetWindowPos( 0, xmargin, ymargin, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
			}
			search_.SetWindowPos( &CWnd::wndBottom,
				xmargin, ymargin + trect.Height() + gap*2,
				width - xmargin*2, ysearch, 0 );
			search_.GetWindowRect( &rect );
			ScreenToClient( &rect );
		}
		else
		{
			search_.SetWindowPos( &CWnd::wndBottom,
				xmargin, ymargin,
				width - trect.Width() - xmargin*2 - gap*2, ysearch, 0 );
			search_.GetWindowRect( &rect );
			ScreenToClient( &rect );
			if (toolbar_.GetSafeHwnd())
			{
				toolbar_.SetWindowPos( 0, rect.right + gap*2, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
			}
		}
	}
}


/**
 *	This method sets the correct image for the magnifying glass icon to the
 *	left of the search bar depending on the visibility and state of the
 *	filters.
 */
void UalDialog::updateFiltersImage()
{
	BW_GUARD;

	int res = 0;
	if ( filtersCtrl_.empty() )
		res = IDB_UALMAGNIFIER;
	else
	{
		if ( filterHolder_.hasActiveFilters() )
			if ( showFilters_ )
				res = IDB_UALHIDEFILTERSA;
			else
				res = IDB_UALSHOWFILTERSA;
		else
			if ( showFilters_ )
				res = IDB_UALHIDEFILTERS;
			else
				res = IDB_UALSHOWFILTERS;
	}
	search_.filtersImg( MAKEINTRESOURCE( res ) );
}


/**
 *	This method aids in adjusting the size and placement of filters when the
 *	dialog is resized.
 *
 *	@param width	New width for the dialog.
 *	@param height	New height for the dialog.
 */
void UalDialog::adjustFiltersSize( int width, int height )
{
	BW_GUARD;

	if ( filtersCtrl_.GetSafeHwnd() )
	{
		updateFiltersImage();

		if ( !showFilters_ || filtersCtrl_.empty() )
		{
			filtersCtrl_.ShowWindow( SW_HIDE );
		}
		else
		{
			filtersCtrl_.ShowWindow( SW_SHOW );
			filtersCtrl_.recalcWidth( width - 8 );
			int top = 0;
			if (search_.GetSafeHwnd())
			{
				CRect rect;
				search_.GetWindowRect( &rect );
				ScreenToClient( &rect );
				top = rect.bottom + 6;
			}
			filtersCtrl_.SetWindowPos( 0, 4, top, width - 8, filtersCtrl_.getHeight(), SWP_NOZORDER );
		}
	}	
}


/**
 *	This method aids in adjusting the size of the splitter bar when the dialog
 *	is resized, in order to avoid completely collapsing a splitter pane.
 *	This method also resizes and repositions the dialog's status bar.
 *
 *	@param width	New width for the dialog.
 *	@param height	New height for the dialog.
 */
void UalDialog::adjustSplitterSize( int width, int height )
{
	BW_GUARD;

	if ( splitterBar_ && splitterBar_->GetSafeHwnd() )
	{
		int top = 0;
		if (search_.GetSafeHwnd())
		{
			CRect rect;
			search_.GetWindowRect( &rect );
			ScreenToClient( &rect );
			top = rect.bottom + 4;
		}
		if ( showFilters_ && !filtersCtrl_.empty() )
			top += filtersCtrl_.getHeight() + 2;
		splitterBar_->SetWindowPos( 0, 3, top, width - 6, height - top - 15, SWP_NOZORDER );
		folderTree_.RedrawWindow();
		smartList_.RedrawWindow();
	}

	if ( statusBar_.GetSafeHwnd() )
	{
		CRect rect;
		splitterBar_->GetWindowRect( &rect );
		ScreenToClient( &rect );
		statusBar_.SetWindowPos( 0, rect.left, rect.bottom, rect.right, 17, SWP_NOZORDER );
		statusBar_.RedrawWindow();
	}
}


/**
 *	This method updates the text in the status bar.
 */
void UalDialog::refreshStatusBar()
{
	BW_GUARD;

	HTREEITEM item = folderTree_.GetSelectedItem();
	if ( item )
		setFolderTreeStatusBar( (VFolderItemData*)folderTree_.GetItemData( item ) );
}


/**
 *	This method allows changing the view style of the list.  For more info see
 *	"SmartListCtrl::ViewStyle".
 *
 *	@param style	New list view style.
 */
void UalDialog::setListStyle( SmartListCtrl::ViewStyle style )
{
	BW_GUARD;

	smartList_.setStyle( style );
	HTREEITEM sel = folderTree_.GetSelectedItem();
	if ( !sel )
		return;
	VFolderPtr vfolder = folderTree_.getVFolder(
		(VFolderItemData*)folderTree_.GetItemData( sel ) );
	if ( !vfolder )
		return;

	UalFolderData* folderData = (UalFolderData*)vfolder->getData();
	if ( !folderData || !folderData->showInList_ )
		return;

	if ( style == SmartListCtrl::BIGICONS )
		folderData->thumbSize_ = 2;
	else if ( style == SmartListCtrl::SMALLICONS )
		folderData->thumbSize_ = 1;
	else
		folderData->thumbSize_ = 0;
}


/**
 *	This method allows changing the internal layout of the dialog to either
 *	vertical (tree view on top of list) or horizontal (tree view to the left of
 *	the list).  Vertical suits to a tall, narrow dialog, while horizontal is
 *	best for a wide dialog.
 *
 *	@param vertical		True to set vertical layout, false for horizontal.
 *	@param resetLastSize	True to ignore the last splitter sizes (on load),
 *							or false to use it (when the user changes it).
 */
void UalDialog::setLayout( bool vertical, bool resetLastSize )
{
	BW_GUARD;

	// if a previous splitter exists, save last pane sizes and delete
	if ( splitterBar_->GetSafeHwnd() )
	{
		folderTree_.SetParent( this );
		smartList_.SetParent( this );

		if ( resetLastSize )
		{
			layoutLastRowSize_ = 0;
			layoutLastColSize_ = 0;
		}
		else if ( layoutVertical_ != vertical )
		{
			int min;
			if ( layoutVertical_ )
				splitterBar_->GetRowInfo( 0, layoutLastRowSize_, min );
			else
				splitterBar_->GetColumnInfo( 0, layoutLastColSize_, min );
		}
		splitterBar_->DestroyWindow();
		bw_safe_delete(splitterBar_);
	}

	// update flag and button state
	layoutVertical_ = vertical;

	// create new splitter
	int id2;

	splitterBar_ = new SplitterBarType();
	splitterBar_->setMinRowSize( MIN_SPLITTER_PANE_SIZE );
	splitterBar_->setMinColSize( MIN_SPLITTER_PANE_SIZE );

	if ( layoutVertical_ )
	{
		splitterBar_->CreateStatic( this, 2, 1, WS_CHILD );
		id2 = splitterBar_->IdFromRowCol( 1, 0 );
	}
	else
	{
		splitterBar_->CreateStatic( this, 1, 2, WS_CHILD );
		id2 = splitterBar_->IdFromRowCol( 0, 1 );
	}

	// set parents properly
	folderTree_.SetDlgCtrlID( splitterBar_->IdFromRowCol( 0, 0 ) );
	folderTree_.SetParent( splitterBar_ );

	smartList_.SetDlgCtrlID( id2 );
	smartList_.SetParent( splitterBar_ );

	splitterBar_->ShowWindow( SW_SHOW );

	// restore last saved pane sizes
	int size = defaultSize_;
	if ( layoutVertical_ )
	{
		if ( layoutLastRowSize_ > 0 )
			size = layoutLastRowSize_;
		if ( size < MIN_SPLITTER_PANE_SIZE ) // limit minimum splitter size
			size = MIN_SPLITTER_PANE_SIZE;
		splitterBar_->SetRowInfo( 0, size, 1 );
		splitterBar_->SetRowInfo( 1, 10, 1 );
	}
	else
	{
		if ( layoutLastColSize_ > 0 )
			size = layoutLastColSize_;
		if ( size < MIN_SPLITTER_PANE_SIZE ) // limit minimum splitter size
			size = MIN_SPLITTER_PANE_SIZE;
		splitterBar_->SetColumnInfo( 0, size, 1 );
		splitterBar_->SetColumnInfo( 1, 10, 1 );
	}

	// recalc layout and update
	splitterBar_->RecalcLayout();
	CRect rect;
	GetClientRect( &rect );
	adjustSplitterSize( rect.Width(), rect.Height() );
}


/**
 *	This method initialises the filters.
 */
void UalDialog::buildSmartListFilters()
{
	BW_GUARD;

	int i = 0;
	int pos = 6;
	FilterSpecPtr filter = 0;

	filtersCtrl_.clear();

	while ( filter = filterHolder_.getFilter( i++ ) )
	{
		if ( filter->getName().length() )
			filtersCtrl_.add( filter->getName().c_str(), filter->getActive(), (void*)filter.getObject() );
		else
			filtersCtrl_.addSeparator();
	} 
	
	CRect rect;
	GetClientRect( &rect );
	adjustFiltersSize( rect.Width(), rect.Height() );
	adjustSplitterSize( rect.Width(), rect.Height() );
}


// MFC message map
BEGIN_MESSAGE_MAP(UalDialog, CDialog)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND_RANGE( GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand )
	ON_MESSAGE( WM_SEARCHFIELD_CHANGE, OnSearchFieldChanged )
	ON_MESSAGE( WM_SEARCHFIELD_FILTERS, OnSearchFieldFilters )
END_MESSAGE_MAP()


/**
 *	This MFC method relays focus events to the focus functor.
 *
 *	@param pOldWnd		MFC previously focused window.
 */
void UalDialog::OnSetFocus( CWnd* pOldWnd )
{
	BW_GUARD;

	if ( UalManager::instance().focusCallback() )
		(*UalManager::instance().focusCallback())( this, true );
}


/**
 *	This MFC method relays focus events to the focus functor.
 *
 *	@param pNewWnd		MFC newly focused window.
 */
void UalDialog::OnKillFocus( CWnd* pNewWnd )
{
	BW_GUARD;

	if ( UalManager::instance().focusCallback() )
		(*UalManager::instance().focusCallback())( this, false );
}


/**
 *	This MFC method is overriden in order to relay events to the tooltips 
 *	helper control. It was also used to keep track of the currently focused
 *	control and to steal the focus if the mouse was over it.
 *
 *	@param msg	MFC message info.
 *	@return		FALSE to keep processing messages.
 */
BOOL UalDialog::PreTranslateMessage( MSG* msg )
{
	BW_GUARD;

	if ( msg->message == WM_LBUTTONDOWN )
	{
		// Save the las control that had the focus in the UAL
		if ( msg->hwnd == search_.GetSafeHwnd() ||
			msg->hwnd == folderTree_.GetSafeHwnd() ||
			msg->hwnd == smartList_.GetSafeHwnd() )
			lastFocus_ = msg->hwnd;
	}
	else if ( msg->message == WM_MOUSEMOVE )
	{
		// Steal back the focus to the UAL
//		if ( !::IsChild( this->GetSafeHwnd(), ::GetFocus() ) )
//			::SetFocus( lastFocus_ );
	}

    if ( toolTip_.GetSafeHwnd() )
        toolTip_.RelayEvent( msg );

	return 0;
}


/**
 *	This MFC method simply calls the base class' OnDestroy method.
 */
void UalDialog::OnDestroy()
{
	BW_GUARD;

	CDialog::OnDestroy();
}


/**
 *	This MFC method is overriden in order to resize the dialog's internal
 *	elements.
 *
 *	@param nType	MFC resize type.
 *	@param cx		MFC width.
 *	@param cy		MFC height.
 */
void UalDialog::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	adjustSearchSize( cx, cy );
	adjustFiltersSize( cx, cy );
	adjustSplitterSize( cx, cy );	
}


/**
 *	This event handler simply forwards toolbar events to the GUIMANAGER.
 *
 *	@param nID	Toolbar button id.
 */
void UalDialog::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	GUI::Manager::instance().act( nID );
}


/**
 *	This method sets the dialog's status bar text from a virtual folder item.
 *
 *	@param data		Virtual folder item that will provide the status bar text.
 */
void UalDialog::setFolderTreeStatusBar( VFolderItemData* data )
{
	BW_GUARD;

	if ( data && !!data->getProvider() )
		setStatusText(
			data->getProvider()->getDescriptiveText(
				data, smartList_.GetItemCount(), smartList_.finished() ) );
	else
		setStatusText( L"" );
}


/**
 *	This method simply relays a tree view item click to the item click functor.
 *
 *	@param data		Tree view item the user clicked.
 */
void UalDialog::callbackVFolderSelect( VFolderItemData* data )
{
	BW_GUARD;

	if ( !data || data->isVFolder() || !UalManager::instance().itemClickCallback() )
		return;

	POINT pt;
	GetCursorPos( &pt );

	UalItemInfo ii( this, data->assetInfo(), pt.x, pt.y );
	(*UalManager::instance().itemClickCallback())( &ii );
}


/**
 *	This method is called whenever the favourites virtual folder changes, in
 *	which case the tree view and list are refreshed if necessary.
 */
void UalDialog::favouritesChanged()
{
	BW_GUARD;

	folderTree_.refreshVFolders( favouritesFolderProvider_ );
	if ( smartList_.getProvider() == favouritesListProvider_.getObject() )
		smartList_.refresh();
}


/**
 *	This method is called whenever the history virtual folder changes, in
 *	which case the tree view and list are refreshed if necessary.
 */
void UalDialog::historyChanged()
{
	BW_GUARD;

	folderTree_.refreshVFolders( historyFolderProvider_ );
	if ( smartList_.getProvider() == historyListProvider_.getObject() )
		smartList_.refresh();
}


/**
 *	This method is called when an item in the tree view is selected. If it's a
 *	folder, then the list is updated with the corresponding list provider so it
 *	displays the contents pointed to by the folder.
 *
 *	@param data		Tree view item that was selected.
 */
void UalDialog::folderTreeSelect( VFolderItemData* data )
{
	BW_GUARD;

	if ( !data )
		return;

	bool showInList = false;

	// get the parent vfolder to get subtree extra info
	VFolderPtr vfolder = folderTree_.getVFolder( data );
	XmlItemVec* customItems = 0;
	if ( !!vfolder )
	{
		UalFolderData* folderData = (UalFolderData*)vfolder->getData();
		customItems = vfolder->getCustomItems();
		if ( folderData )
		{
			search_.idleText( folderData->idleText_ );
			if ( folderData->showInList_ )
			{
				// set the thumbnail size / list style
				if ( folderData->thumbSize_ == 2 )
					setListStyle( SmartListCtrl::BIGICONS );
				else if ( folderData->thumbSize_ == 1 )
					setListStyle( SmartListCtrl::SMALLICONS );
				else
					setListStyle( SmartListCtrl::LIST );
				// set filter state disabled/enabled
				filtersCtrl_.enableAll( true );
				filterHolder_.enableAll( true );
				for( BW::vector<BW::wstring>::iterator i = folderData->disabledFilters_.begin();
					i != folderData->disabledFilters_.end(); ++i )
				{
					filtersCtrl_.enable( (*i), false );
					filterHolder_.enable( (*i), false );
				}
				showInList = true;
				smartList_.allowMultiSelect( folderData->multiItemDrag_ );
			}
		}
	}

	if ( !!data->getProvider() )
	{
		// see if it's the favourites provider
		CWaitCursor wait;
		ListProviderPtr listProvider;
		bool itemClicked = false;
		if ( showInList &&
			data->getProvider()->getListProviderInfo(
				data, lastListInit_, listProvider, itemClicked ) )
		{
			smartList_.init( listProvider, customItems );
		}
		if ( itemClicked )
			callbackVFolderSelect( data );
		setFolderTreeStatusBar( data );
	}
	else
	{
		// it's a plain vfolder
		smartList_.init( 0, customItems );
		setFolderTreeStatusBar( data );
		lastListInit_ = L"";
	}
	updateFiltersImage();
	return;
}


/**
 *	This method is called when an item starts to get dragged by the user from
 *	the tree view.
 *
 *	@param data		Tree view item being dragged.
 */
void UalDialog::folderTreeStartDrag( VFolderItemData* data )
{
	BW_GUARD;

	if ( !data )
		return;

	// hack: using the getExpandable flag to see if its a folder type,
	// so all expandable items can be cloned (not sure if conceptually correct)
	VFolderPtr vfolder = folderTree_.getVFolder( data );
	BW::vector<AssetInfo> assets;
	assets.push_back( data->assetInfo() );
	dragLoop( assets, data->getExpandable(), vfolder.getObject() );
}


/**
 *	This method is called when the user presses the Delete key while a tree
 *	view item is selected.  If it's the history or favourites folder, then we
 *	go ahead and try to deleted the selected item.
 *
 *	@param data		Tree view item about to be deleted.
 */
void UalDialog::folderTreeItemDelete( VFolderItemData* data )
{
	BW_GUARD;

	if ( !data )
		return;

	if ( data->getProvider().getObject() == historyFolderProvider_.getObject() )
	{
		if ( data->isVFolder() )
		{
			if ( MessageBox(
					Localise(L"UAL/UAL_DIALOG/CLEAR_HISTORY_TEXT"),
					Localise(L"UAL/UAL_DIALOG/CLEAR_HISTORY_TITLE"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) != IDYES )
			{
				folderTree_.SetFocus();
				return;
			}
			folderTree_.SetFocus();

			UalManager::instance().history().clear();
		}
		else
			UalManager::instance().history().remove( data->assetInfo() );
	}
	else if ( data->getProvider().getObject() == favouritesFolderProvider_.getObject() )
	{
		if ( data->isVFolder() )
		{
			if ( MessageBox(
					Localise(L"UAL/UAL_DIALOG/CLEAR_FAVOURITES_TEXT"),
					Localise(L"UAL/UAL_DIALOG/CLEAR_FAVOURITES_TITLE"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) != IDYES )
			{
				folderTree_.SetFocus();
				return;
			}
			folderTree_.SetFocus();

			UalManager::instance().favourites().clear();
		}
		else
			UalManager::instance().favourites().remove( data->assetInfo() );
	}
}


/**
 *	This method shows a context menu for when the user right-clicks on top of
 *	an item, be it in the tree view or the list.
 *
 *	@param ii	Item information.
 */
void UalDialog::showItemContextMenu( UalItemInfo* ii )
{
	BW_GUARD;

	// build the popup menu
	int openExplorerCmd =		0xFF00;
	int openExplorerCmdRange =	0x0020;	// up to 32 paths
	int copyPathCmd =			0xFF20;
	int copyPathCmdRange =		0x0020;	// up to 32 paths
	int addToFavCmd =			0xFF40;
	int removeFromFavCmd =		0xFF41;
	int removeFromHistCmd =		0xFF42;
	int bigViewCmd =			0xFF43;
	int smallViewCmd =			0xFF44;
	int listViewCmd =			0xFF45;

	PopupMenu menu;

	PopupMenu::Items appItems;
	if ( UalManager::instance().startPopupMenuCallback() )
		(*UalManager::instance().startPopupMenuCallback())( ii, appItems );

	// List Styles submenu
	menu.startSubmenu( Localise(L"UAL/UAL_DIALOG/LIST_VIEW_STYLES") );

	BW::wstring check;
	check = smartList_.getStyle() == SmartListCtrl::LIST ? L"##" : L"";
	menu.addItem( check + Localise(L"UAL/UAL_DIALOG/LIST"), listViewCmd );

	check = smartList_.getStyle() == SmartListCtrl::SMALLICONS ? L"##" : L"";
	menu.addItem( check + Localise(L"UAL/UAL_DIALOG/SMALL_ICONS"), smallViewCmd );

	check = smartList_.getStyle() == SmartListCtrl::BIGICONS ? L"##" : L"";
	menu.addItem( check + Localise(L"UAL/UAL_DIALOG/BIG_ICONS"), bigViewCmd );

	menu.endSubmenu();

	// add item paths
	BW::vector<BW::wstring> paths;
	if ( ii )
	{
		if ( !ii->isFolder() )
		{
			if ( smartList_.getProvider() == favouritesListProvider_.getObject() )
				menu.addItem( Localise(L"UAL/UAL_DIALOG/REMOVE_FROM_FAVOURITES"), removeFromFavCmd );
			else if ( smartList_.getProvider() == historyListProvider_.getObject() )
				menu.addItem( Localise(L"UAL/UAL_DIALOG/REMOVE_FROM_HISTORY"), removeFromHistCmd );

			if ( smartList_.getProvider() != favouritesListProvider_.getObject() )
				menu.addItem( Localise(L"UAL/UAL_DIALOG/ADD_TO_FAVOURITES"), addToFavCmd );
		}

		if ( !ii->getNext() )
		{
			// allow open in explorer and copy path if only one item is selected
			bw_tokenise( ii->longText(), L",;", paths );
			//StringUtils::vectorFromString( ii->longText(), paths );
			if (paths.size() == 1)
			{
				if (PathFileExists( paths[0].c_str() ))
				{
					menu.addItem( Localise(L"UAL/UAL_DIALOG/OPEN_FOLDER_IN_EXPLORER"), openExplorerCmd );
					menu.addItem( Localise(L"UAL/UAL_DIALOG/COPY_PATH_TO_CLIPBOARD"), copyPathCmd );
				}
			}
			else
			{
				for( int i = 0; i < (int)paths.size() && i < openExplorerCmdRange; ++i )
				{
					if ( PathFileExists( paths[ i ].c_str() ) )
					{
						menu.addItem( 
								Localise(L"UAL/UAL_DIALOG/OPEN_X_IN_EXPLORER", paths[i]),
								openExplorerCmd + i );
					}
				}

				for( int i = 0; i < (int)paths.size() && i < copyPathCmdRange; ++i )
				{
					if ( PathFileExists( paths[ i ].c_str() ) )
					{
						menu.addItem( 
								Localise(L"UAL/UAL_DIALOG/COPY_X_TO_CLIPBOARD", paths[i]),
								copyPathCmd + i );
					}
				}
			}
		}
	}

	if ( !appItems.empty() ) 
		menu.addSeparator(); // separator

	menu.addItems( appItems );

	// run the menu
	int result = menu.doModal( GetSafeHwnd() );

	if ( result >= openExplorerCmd && result < openExplorerCmd + openExplorerCmdRange )
	{
		BW::wstring path = paths[ result - openExplorerCmd ];
		BW::wstring cmd = L"explorer ";
		if ( !PathIsDirectory( path.c_str() ) )
			cmd += L"/select,\"";
		else
			cmd += L"\"";
		cmd += path + L"\"";

		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		GetStartupInfo( &si );

		if( CreateProcess( NULL, (LPTSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, 0,
			&si, &pi ) )
		{
			CloseHandle( pi.hThread );
			CloseHandle( pi.hProcess );
		}
	}
	else if ( result >= copyPathCmd && result < copyPathCmd + copyPathCmdRange )
	{
		if ( OpenClipboard() )
		{
			BW::wstring path = paths[ result - copyPathCmd ];
			HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, (path.length() + 1) * sizeof(TCHAR)); 
			if ( data && EmptyClipboard() )
			{ 
				LPTSTR str = (LPTSTR)GlobalLock( data );
				memcpy( str, path.c_str(), path.length() * sizeof(TCHAR));
				str[ path.length() ] = (TCHAR) 0;
				GlobalUnlock( data );

				SetClipboardData( CF_UNICODETEXT, data );
			} 
			CloseClipboard();
		}
	}
	else if ( result == bigViewCmd )
	{
		setListStyle( SmartListCtrl::BIGICONS );
	}
	else if ( result == smallViewCmd )
	{
		setListStyle( SmartListCtrl::SMALLICONS );
	}
	else if ( result == listViewCmd )
	{
		setListStyle( SmartListCtrl::LIST );
	}
	else if ( result == addToFavCmd ||
		result == removeFromFavCmd ||
		result == removeFromHistCmd )
	{
		// multi-items actions
		CWaitCursor wait;
		while ( ii )
		{
			if ( result == addToFavCmd )
			{
				UalManager::instance().favourites().add(
					ii->assetInfo() );
			}
			else if ( result == removeFromFavCmd )
			{
				UalManager::instance().favourites().remove(
					ii->assetInfo() );
			}
			else if ( result == removeFromHistCmd )
			{
				UalManager::instance().history().remove(
					ii->assetInfo() );
			}
			ii = ii->getNext();
		}
	}
	else if ( UalManager::instance().endPopupMenuCallback() ) 
	{
		(*UalManager::instance().endPopupMenuCallback())( ii, result );
	}
}


/**
 *	This method shows a context menu for when the user right-clicks on top of
 *	an item or folder in the tree view.
 *
 *	@param data		Tree view item clicked.
 */
void UalDialog::showContextMenu( VFolderItemData* data )
{
	BW_GUARD;

	if ( !data || data->isVFolder() )
	{
		bool plainVFolder = true;
		if ( data )
			plainVFolder = 
				data->getProvider().getObject() != favouritesFolderProvider_.getObject() &&
				data->getProvider().getObject() != historyFolderProvider_.getObject();

		// build menu items
		int bigViewCmd =			0xFF43;
		int smallViewCmd =			0xFF44;
		int listViewCmd =			0xFF45;
		int renameCmd =				0xFF50;
		int defaultFoldersCmd =		0xFF51;
		int removeFolderCmd =		0xFF52;
		PopupMenu menu;

		// List Styles submenu
		menu.startSubmenu( Localise(L"UAL/UAL_DIALOG/LIST_VIEW_STYLES") );

		BW::wstring check;
		check = smartList_.getStyle() == SmartListCtrl::LIST ? L"##" : L"";
		menu.addItem( check + Localise(L"UAL/UAL_DIALOG/LIST"), listViewCmd );

		check = smartList_.getStyle() == SmartListCtrl::SMALLICONS ? L"##" : L"";
		menu.addItem( check + Localise(L"UAL/UAL_DIALOG/SMALL_ICONS"), smallViewCmd );

		check = smartList_.getStyle() == SmartListCtrl::BIGICONS ? L"##" : L"";
		menu.addItem( check + Localise(L"UAL/UAL_DIALOG/BIG_ICONS"), bigViewCmd );

		menu.endSubmenu();

		// common menu items
		menu.addItem( Localise(L"UAL/UAL_DIALOG/CHANGE_PANEL_TITLE"), renameCmd );
		menu.addItem( Localise(L"UAL/UAL_DIALOG/RELOAD_DEFAULT_FOLDERS"), defaultFoldersCmd );

		if ( data )
		{
			BW::wstring remove = Localise(L"UAL/UAL_DIALOG/REMOVE_X", data->assetInfo().text());
			menu.addItem( remove, removeFolderCmd );
		}
		if ( !plainVFolder )
			menu.addItem( Localise(L"UAL/UAL_DIALOG/CLEAR_CONTENTS"), 100 );

		// run the menu
		int result = menu.doModal( GetSafeHwnd() );

		if ( result == removeFolderCmd && data )
		{
			excludeVFolders_.push_back( data->assetInfo().text() );
			folderTree_.removeVFolder( data->getTreeItem() );
			if ( customVFolders_ )
			{
				BW::vector<DataSectionPtr> sections;
				customVFolders_->openSections( "customVFolder", sections );
				for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
					if ( (*s)->asWideString() == data->assetInfo().text() )
					{
						customVFolders_->delChild( *s );
						break;
					}
			}
			if ( folderTree_.GetCount() == 0 )
			{
				UalManager::instance().thumbnailManager().resetPendingRequests( &folderTree_ );
				// resetPendingRequests on the SmartList is done in its init
				smartList_.init( 0, 0 );
				setFolderTreeStatusBar( 0 );
				updateFiltersImage();
			}
		}
		else if ( result == defaultFoldersCmd )
		{
			if ( MessageBox( Localise(L"UAL/UAL_DIALOG/RELOAD_TEXT"),
				Localise(L"UAL/UAL_DIALOG/RELOAD_TITLE"),
				MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION ) == IDYES )
			{
				excludeVFolders_.clear();
				folderTree_.clear();
				UalManager::instance().thumbnailManager().resetPendingRequests( &folderTree_ );
				// resetPendingRequests on the SmartList is done in its init
				smartList_.init( 0, 0 );
				setFolderTreeStatusBar( 0 );
				updateFiltersImage();
				if ( !configFile_.empty() )
				{
					customVFolders_ = 0;
					BW::string nconfigFile;
					bw_wtoutf8( configFile_, nconfigFile );
					BWResource::instance().purge( nconfigFile );
					DataSectionPtr root = BWResource::openSection( nconfigFile );
					if ( root )
						loadVFolders( root->openSection( "VFolders" ) );
				}
			}
		}
		else if ( result == renameCmd )
		{
			UalNameDlg dlg;
			dlg.setNames( dlgShortCaption_, dlgLongCaption_ );
			if ( dlg.DoModal() == IDOK )
			{
				dlg.getNames( dlgShortCaption_, dlgLongCaption_ );
				// Ugly hack: repaint all windows just to get the new panel title repainted :S  Instead, should implement a notification mecanism so the appropriate panel gets the repaint message
				if ( GetDesktopWindow() )
					GetDesktopWindow()->RedrawWindow( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASENOW | RDW_ALLCHILDREN );
			}
		}
		else if ( result == listViewCmd )
		{
			setListStyle( SmartListCtrl::LIST );
		}
		else if ( result == smallViewCmd )
		{
			setListStyle( SmartListCtrl::SMALLICONS );
		}
		else if ( result == bigViewCmd )
		{
			setListStyle( SmartListCtrl::BIGICONS );
		}
		else if ( result == 100 && data )
			folderTreeItemDelete( data );
	}
	else if ( data )
	{
		// create a popup menu for the item and call the app to fill it
		CPoint pt;
		GetCursorPos( &pt );
		UalItemInfo ii( this, data->assetInfo(), pt.x, pt.y );
		ii.isFolder_ = data->getExpandable();

		showItemContextMenu( &ii );
	}
}


/**
 *	This method fills in a vector with all the assets selected in the list.
 *
 *	@param assets	Return vector that will contain the selected assets.
 */
void UalDialog::fillAssetsVectorFromList( BW::vector<AssetInfo>& assets )
{
	BW_GUARD;

	int numSel = smartList_.GetSelectedCount();
	if ( numSel > 500 )
	{
		numSel = 500;
		error( "Dragging too many items, only taking the first 500." );
	}
	assets.reserve( numSel );
	int item = smartList_.GetNextItem( -1, LVNI_SELECTED );
	while( item > -1 && numSel > 0 )
	{
		assets.push_back( smartList_.getAssetInfo( item ) );
		item = smartList_.GetNextItem( item, LVNI_SELECTED );
		numSel--;
	}
}

BW::vector<BW::wstring> UalDialog::getSelectedAssetPaths()
{
	BW::vector<AssetInfo> assets;
	fillAssetsVectorFromList(assets);

	BW::vector<BW::wstring> ret;
	for (size_t i = 0; i < assets.size(); i++)
	{
		// replace backslash with forwardslash
		// convert full path into relative path usable with ResMgr
		BW::wstring wpath = assets[i].longText();
		std::replace(wpath.begin(), wpath.end(), L'\\', L'/');
		wpath = BWResource::dissolveFilenameW(wpath);
		ret.push_back(wpath);
	}

	return ret;
}

/**
 *	This method handles a right-click event on an item in the tree view.
 *
 *	@param data		Tree view item clicked.
 */
void UalDialog::folderTreeRightClick( VFolderItemData* data )
{
	BW_GUARD;

	showContextMenu( data );
}


/**
 *	This method relays a double-click event on an item in the tree view to the
 *	appropriate functor.
 *
 *	@param data		Tree view item clicked.
 */
void UalDialog::folderTreeDoubleClick( VFolderItemData* data )
{
	BW_GUARD;

	if ( !UalManager::instance().itemDblClickCallback() || !data )
		return;

	if ( data->isVFolder() )
		return;

	CPoint pt;
	GetCursorPos( &pt );
	UalItemInfo ii( this, data->assetInfo(), pt.x, pt.y );

	ii.isFolder_ = data->getExpandable();

	(*UalManager::instance().itemDblClickCallback())( &ii );
}


/**
 *	This method is called when the list performs an update while loading items
 *	and updates the status bar as the list gets loaded with items.  It also
 *	shows an item on the list previously tagged as needing being shown.
 */
void UalDialog::listLoadingUpdate()
{
	BW_GUARD;

	if ( !delayedListShowItem_.empty() )
	{
		BW::wstring textTmp = delayedListShowItem_.c_str() + delayedListShowItem_.find_last_of( L'\\' ) + 1;
		if ( smartList_.showItem( AssetInfo( L"", textTmp, delayedListShowItem_ ) ) )
			delayedListShowItem_ = L"";
	}

	refreshStatusBar();
}


/**
 *	This method is called when the list has finished loading items.
 */
void UalDialog::listLoadingFinished()
{
	BW_GUARD;

	delayedListShowItem_ = L"";
	refreshStatusBar();
}


/**
 *	This method is called when an item on the list gets selected, relaying the
 *	event to the appropriate functor and updating the status bar.
 */
void UalDialog::listItemSelect()
{
	BW_GUARD;

	// notify
	if ( UalManager::instance().itemClickCallback() )
	{
		int focusItem = smartList_.GetNextItem( -1, LVNI_FOCUSED );
		if ( focusItem >=0 && smartList_.GetItemState( focusItem, LVIS_SELECTED ) == LVIS_SELECTED )
		{
			POINT pt;
			GetCursorPos( &pt );
			AssetInfo assetInfo = smartList_.getAssetInfo( focusItem );
			UalItemInfo ii( this, assetInfo, pt.x, pt.y );
			(*UalManager::instance().itemClickCallback())( &ii );
		}
	}

	int numSel = smartList_.GetSelectedCount();

	if ( !numSel )
		refreshStatusBar();
	else
	{
		// update status bar
		BW::wstring txt;

		txt = 
			Localise
			(
				L"UAL/UAL_DIALOG/SELECTED_ITEMS", 
				numSel, 
				smartList_.GetItemCount()
			);

		if ( numSel > 10 )
			txt += Localise(L"UAL/UAL_DIALOG/MANY_ITEMS");
		else
		{
			txt += L" : ";
			int item = -1;
			for( int i = 0; i < numSel; ++i )
			{
				item = smartList_.GetNextItem( item, LVNI_SELECTED );
				if ( i != 0 )
					txt += L", ";
				if ( smartList_.getAssetInfo( item ).description().empty() )
					txt += smartList_.getAssetInfo( item ).longText();
				else
					txt += smartList_.getAssetInfo( item ).description();
			}
		}
		setStatusText( txt );
	}
}


/**
 *	This method handles deleting an item from the list, which is only allowed
 *	on the favourites and history lists.
 */
void UalDialog::listItemDelete()
{
	BW_GUARD;

	if ( smartList_.getProvider() == historyListProvider_.getObject() ||
		smartList_.getProvider() == favouritesListProvider_.getObject() )
	{
		// delete from the history or favourites, depending on the current list provider
		int item = -1;
		int numSel = smartList_.GetSelectedCount();
		if ( numSel > 1 )
		{
			if ( MessageBox(
					Localise(L"UAL/UAL_DIALOG/MULTI_DELETE_TEXT"),
					Localise(L"UAL/UAL_DIALOG/MULTI_DELETE_TITLE"),
					MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) != IDYES )
			{
				smartList_.SetFocus();
				return;
			}
			smartList_.SetFocus();
		}

		for( int i = 0; i < numSel; i++ )
		{
			item = smartList_.GetNextItem( item, LVNI_SELECTED );
			if ( item >= 0 )
			{
				if ( smartList_.getProvider() == historyListProvider_.getObject() )
					UalManager::instance().history().remove(
						smartList_.getAssetInfo( item ),
						i == numSel - 1 );
				else
					UalManager::instance().favourites().remove(
						smartList_.getAssetInfo( item ),
						i == numSel - 1 );
			}
		}
	}
}


/**
 *	This method relays list double-click events to the appropriate functor.
 *
 *	@param index	Index of the clicked item on the list.
 */
void UalDialog::listDoubleClick( int index )
{
	BW_GUARD;

	if ( !UalManager::instance().itemDblClickCallback() )
		return;

	CPoint pt;
	GetCursorPos( &pt );
	AssetInfo assetInfo;
	if ( index >= 0 )
		assetInfo = smartList_.getAssetInfo( index );

	UalItemInfo ii( this, assetInfo, pt.x, pt.y );
	(*UalManager::instance().itemDblClickCallback())( &ii );
}


/**
 *	This method is called when the user starts dragging an item from the list.
 *
 *	@param index	Index of the dragged item on the list.
 */
void UalDialog::listStartDrag( int index )
{
	BW_GUARD;

	if ( index < 0 || index >= smartList_.GetItemCount() )
		return;

	BW::vector<AssetInfo> assets;
	fillAssetsVectorFromList( assets );
	dragLoop( assets );
}


/**
 *	This method shows a context menu for when the user right-clicks on top of
 *	an item in the list.
 *
 *	@param index	Index of the clicked item on the list.
 */
void UalDialog::listItemRightClick( int index )
{
	BW_GUARD;

	BW::vector<AssetInfo> assets;
	fillAssetsVectorFromList( assets );

	if ( index < 0 || index >= smartList_.GetItemCount() || assets.empty() )
	{
		showItemContextMenu( 0 );
		return;
	}

	CPoint pt;
	GetCursorPos( &pt );

	BW::vector<AssetInfo>::iterator a = assets.begin();
	UalItemInfo ii( this, *a++, pt.x, pt.y );
	UalItemInfo* iip = &ii;
	while( a != assets.end() )
	{
		iip->setNext(
			new UalItemInfo( this, *a++, pt.x, pt.y ) );
		iip = iip->getNext();
	}
	
	showItemContextMenu( &ii );
}


/**
 *	This method is called when the tooltip for a list item is needed.
 *
 *	@param index	Index of the clicked item on the list.
 *	@param info		Return string that will contain the tooltip for the item.
 */
void UalDialog::listItemToolTip( int index, BW::wstring& info )
{
	BW_GUARD;

	if ( index < 0 )
		return;

	AssetInfo assetInfo = smartList_.getAssetInfo( index );
	info = assetInfo.text();
	if ( !assetInfo.longText().empty() )
	{
		BW::string nlongText;
		bw_wtoutf8( assetInfo.longText(), nlongText );
		BW::wstring path;
		bw_utf8tow( BWResource::getFilePath(	BWResource::dissolveFilename( nlongText ) ), path );
		if ( !path.empty() )
		{
			info += Localise(L"UAL/UAL_DIALOG/NL_PATH");
			info += path;
		}
	}
	if ( !assetInfo.description().empty() )
	{
		info += L"\n";
		info += assetInfo.description();
	}
}


/**
 *	This method is called frequently from the dragLoop to keep track of the
 *	dragging item's image and to relay drag updates to the appropriate functor.
 *
 *	@param ii		Item information.
 *	@param srcPt	Mouse position.
 *	@param isScreenCoords	True if the mouse position is in screen coordinates
 *							or false if it's in client area coordinates.
 */
void UalDialog::handleDragMouseMove( UalItemInfo& ii, const CPoint& srcPt, bool isScreenCoords /*= false*/ )
{
	BW_GUARD;

	CPoint pt = srcPt;
	if (!isScreenCoords)
	{
		ClientToScreen( &pt );
	}
	if ( smartList_.isDragging() )
	{
		smartList_.updateDrag( pt.x, pt.y );
	}
	else if ( folderTree_.isDragging() )
	{
		folderTree_.updateDrag( pt.x, pt.y );
	}
	ii.x_ = pt.x;
	ii.y_ = pt.y;
	if ( !UalManager::instance().updateDrag( ii, false ) &&
		UalManager::instance().updateDragCallback() )
		(*UalManager::instance().updateDragCallback())( &ii );
}


/**
 *	This method keeps looping as long as the user is still dragging an item,
 *	updating the item's drag image and checking for drop locations.
 *
 *	@param assetsInfo	List of items being dragged.
 *	@param isFolder		True if the user is dragging a folder, false otherwise.
 *	@param folderExtraData	A pointer to a VFolder object if isFolder is true,
 *							NULL otherwise.
 */
void UalDialog::dragLoop( BW::vector<AssetInfo>& assetsInfo, bool isFolder, void* folderExtraData )
{
	BW_GUARD;

	if ( assetsInfo.empty() )
		return;

	POINT pt;
	GetCursorPos( &pt );

	BW::vector<AssetInfo>::iterator a = assetsInfo.begin();
	UalItemInfo ii( this, *a++, pt.x, pt.y, isFolder, folderExtraData );
	UalItemInfo* iip = &ii;
	while( a != assetsInfo.end() )
	{
		iip->setNext(
			new UalItemInfo( this, *a++, pt.x, pt.y, isFolder, folderExtraData ) );
		iip = iip->getNext();
	}

	if ( ii.isFolder_ )
		lastItemInfo_ = ii; // used when cloneRequired, to know last item dragged to be cloned

	if ( UalManager::instance().startDragCallback() )
		(*UalManager::instance().startDragCallback())( &ii );

	UpdateWindow();
	SetCapture();

	// send at least one update drag message
	handleDragMouseMove( ii, pt, true );

	while ( CWnd::GetCapture() == this )
	{
		MSG msg;
		if ( !::GetMessage( &msg, NULL, 0, 0 ) )
		{
			AfxPostQuitMessage( (int)msg.wParam );
			break;
		}

		if ( msg.message == WM_LBUTTONUP )
		{
			// END DRAG
			POINT pt = { (short)LOWORD( msg.lParam ), (short)HIWORD( msg.lParam ) };
			ClientToScreen( &pt );
			ii.x_ = pt.x;
			ii.y_ = pt.y;
			UalItemInfo* info = 0;
			UalDialog* endDialog = 0;
			if ( !( endDialog = UalManager::instance().updateDrag( ii, true ) ) )
				info = &ii; // if it's not an UAL to UAL drag, call the callback with the item info
			stopDrag();

			if ( UalManager::instance().endDragCallback() )
				(*UalManager::instance().endDragCallback())( info );
			if ( endDialog )
				endDialog->folderTree_.RedrawWindow();
			lastItemInfo_ = UalItemInfo();
			return;
		}
		else if ( msg.message == WM_MOUSEMOVE )
		{
			// UPDATE DRAG
			POINT pt = { (short)LOWORD( msg.lParam ), (short)HIWORD( msg.lParam ) };
			handleDragMouseMove( ii, pt );
		}
		else if ( msg.message == WM_KEYUP || msg.message == WM_KEYDOWN )
		{
			if ( msg.wParam == VK_ESCAPE )
				break; // CANCEL DRAG

			if ( msg.message == WM_KEYUP || !(msg.lParam & 0x40000000) )
			{
				// send update messages, but not if being repeated
				if ( !UalManager::instance().updateDrag( ii, false ) &&
					UalManager::instance().updateDragCallback() )
					(*UalManager::instance().updateDragCallback())( &ii );
			}
		}
		else if ( msg.message == WM_RBUTTONDOWN )
			break; // CANCEL DRAG
		else
			DispatchMessage( &msg );
	}

	cancelDrag();
}


/**
 *	This method is called when the drag & drop operation completes successfully
 *	in which case temporary resources such as drag images are cleared.
 */
void UalDialog::stopDrag()
{
	BW_GUARD;

	if ( smartList_.isDragging() )
		smartList_.endDrag();
	else if ( folderTree_.isDragging() )
		folderTree_.endDrag();
	UalManager::instance().cancelDrag();
	ReleaseCapture();
}


/**
 *	This method is called when the drag & drop operation is canceled.
 */
void UalDialog::cancelDrag()
{
	BW_GUARD;

	stopDrag();
	if ( UalManager::instance().endDragCallback() )
		(*UalManager::instance().endDragCallback())( 0 );
	lastItemInfo_ = UalItemInfo();
}


/**
 *	This method resets any visual feedback elements used during a drag
 *	operation such as highlight rectangles for drop targets.
 */
void UalDialog::resetDragDropTargets()
{
	BW_GUARD;

	folderTree_.SelectDropTarget( 0 );
	folderTree_.SetInsertMark( 0 );
	folderTree_.UpdateWindow();
	smartList_.clearDropTarget();
}


/**
 *	This method is called when either the tree view or the list need to be
 *	scrolled as the result of the user dragging an item near the edges of the
 *	control.
 *
 *	@param wnd	Window that needs to be scrolled.
 *	@param pt	Mouse position in client area coordinates.
 */
void UalDialog::scrollWindow( CWnd* wnd, CPoint pt )
{
	BW_GUARD;

	int scrollZone = 20;

	if ( wnd == &smartList_ )
	{
		CRect rect;
		smartList_.GetClientRect( &rect );
		bool vertical = ((GetWindowLong( smartList_.GetSafeHwnd(), GWL_STYLE ) & LVS_TYPEMASK) == LVS_ICON);
		int size = (vertical?rect.Height():rect.Width());
		int scrollArea = min( scrollZone, size / 4 );
		int coord = vertical?pt.y:pt.x;
		int speedx = vertical?0:1;
		int speedy = vertical?10:0;
		if ( coord < scrollArea ) 
		{
			smartList_.Scroll( CSize( -speedx, -speedy ) );
			smartList_.UpdateWindow();
		}
		else if ( coord >= size - scrollArea && coord < size )
		{
			smartList_.Scroll( CSize( speedx, speedy ) );
			smartList_.UpdateWindow();
		}
	}
	else if ( wnd == &folderTree_ )
	{
		static int speedDamping = 0;
		int speedDampingK = 3;
		CRect rect;
		folderTree_.GetClientRect( &rect );
		int pos = folderTree_.GetScrollPos( SB_VERT );
		int scrollAreaHeight = min( scrollZone, rect.Height() / 4 );
		if ( speedDamping == 0 )
			if ( pt.y < scrollAreaHeight && pos > 0 ) 
				folderTree_.SendMessage( WM_VSCROLL, SB_THUMBPOSITION | ((pos-1)<<16), 0 );
			else if ( pt.y >= rect.Height() - scrollAreaHeight && pt.y < rect.Height() )
				folderTree_.SendMessage( WM_VSCROLL, SB_THUMBPOSITION | ((pos+1)<<16), 0 );
		speedDamping++;
		if ( speedDamping > speedDampingK )
			speedDamping = 0;
	}
}


/**
 *	This method updates a drag and drop operation when the mouse is hovering
 *	over the list.
 *
 *	@param itemInfo	Item being dragged.
 *	@param endDrag	True if this is the last update, false otherwise.
 */
void UalDialog::updateSmartListDrag( const UalItemInfo& itemInfo, bool endDrag )
{
	BW_GUARD;

	CPoint pt( itemInfo.x_, itemInfo.y_ );
	if ( smartList_.getProvider() == favouritesListProvider_.getObject() &&
		!itemInfo.isFolder_ )
	{
		// managing favourites items by drag/drop to the list
		smartList_.ScreenToClient( &pt );
		UINT flags;
		int dropItemL = smartList_.HitTest( pt, &flags );

		if ( !endDrag )
		{
			// update
			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
			if ( dropItemL > -1 )
				smartList_.setDropTarget( dropItemL );
			else
				smartList_.clearDropTarget();
			scrollWindow( &smartList_, pt );
		}
		else
		{
			// end drag
			AssetInfo dropAssetInfo;
			if ( dropItemL > -1 )
				dropAssetInfo = smartList_.getAssetInfo( dropItemL );

			bool doAdd = true;
			UalItemInfo* ii = const_cast<UalItemInfo*>( &itemInfo );
			while ( ii )
			{
				if ( ii->assetInfo().equalTo( dropAssetInfo ) )
				{
					// the dragged items are being dropped onto one of it's items,
					// so avoid adding it
					doAdd = false;
					break;
				}
				ii = ii->getNext();
			}

			if ( doAdd ) 
			{
				// only add if dropping over an item not in the dragged set
				CWaitCursor wait;
				ii = const_cast<UalItemInfo*>( &itemInfo );
				while ( ii )
				{
					UalManager::instance().favourites().remove( ii->assetInfo() );
					UalManager::instance().favourites().addAt(
						ii->assetInfo_,
						dropAssetInfo );
					ii = ii->getNext();
				}
			}
		}
	}
	else
	{
		// don't accept dragging of folders to the smartList
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
		smartList_.clearDropTarget();
	}
}


/**
 *	This method updates a drag and drop operation when the mouse is hovering
 *	over the tree view.
 *
 *	@param itemInfo	Item being dragged.
 *	@param endDrag	True if this is the last update, false otherwise.
 */
void UalDialog::updateFolderTreeDrag( const UalItemInfo& itemInfo, bool endDrag )
{
	BW_GUARD;

	CPoint pt( itemInfo.x_, itemInfo.y_ );
	folderTree_.ScreenToClient( &pt );
	UINT flags;
	HTREEITEM dropItemT = folderTree_.HitTest( pt, &flags );
	VFolderItemDataPtr data = 0;
	if ( dropItemT )
		data = (VFolderItemData*)folderTree_.GetItemData( dropItemT );
	if ( itemInfo.isFolder_ )
	{
		// dragging a folder, so do folder-related stuff like Drag&Drop cloning or reordering
		if ( !endDrag )
		{
			// update
			folderTree_.SelectDropTarget( 0 );
			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
			if ( data && data->isVFolder() )
				folderTree_.SetInsertMark( dropItemT, FALSE );
			else
			{
				// dropping beyond the last item, so find the last item and
				// set the insert mark properly
				HTREEITEM item = folderTree_.GetChildItem( TVI_ROOT );
				while( item && folderTree_.GetNextItem( item, TVGN_NEXT ) )
					item = folderTree_.GetNextItem( item, TVGN_NEXT );

				if ( item )
					folderTree_.SetInsertMark( item, TRUE );
				else
				{
					// should never get here
					SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
					folderTree_.SetInsertMark( 0 );
				}
			}
			folderTree_.UpdateWindow();
		}
		else
		{
			// end drag
			VFolderPtr vfolder = folderTree_.getVFolder( itemInfo.assetInfo().text() );
			if ( !vfolder )
			{
				// add the dragged folder or vfolder
				UalManager::instance().copyVFolder( itemInfo.dialog_, this, itemInfo );
				vfolder = folderTree_.getVFolder( itemInfo.assetInfo().text() );
				VFolderPtr dropVFolder = folderTree_.getVFolder( data.getObject() );
				if ( vfolder && dropVFolder )
					folderTree_.moveVFolder( vfolder, dropVFolder );
			}
			else if ( itemInfo.dialog_ == this )
			{
				// folder already exists, reorder folders inside the same UAL
				if ( data )
				{
					VFolderPtr dropVFolder = folderTree_.getVFolder( data.getObject() );
					folderTree_.moveVFolder( vfolder, dropVFolder );
				}
				else
					folderTree_.moveVFolder( vfolder, 0 ); // put it last
			}
		}
	}
	else
	{
		// it's not a folder, so treat it like such
		folderTree_.SelectDropTarget( 0 );
		folderTree_.SetInsertMark( 0 );
		if ( data && data->getProvider().getObject() == favouritesFolderProvider_.getObject() )
		{
			// dropping inside the favourites folder, so it's valid
			if ( !endDrag )
			{
				//update
				SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
				if ( dropItemT )
				{
					if ( data->isVFolder() )
						folderTree_.SelectDropTarget( dropItemT ); // dropping on top of the favourites folder
					else
						folderTree_.SetInsertMark( dropItemT, FALSE );
				}
			}
			else
			{
				// end drag
				bool doAdd = true;
				UalItemInfo* ii = const_cast<UalItemInfo*>( &itemInfo );
				while ( ii )
				{
					if ( ii->assetInfo().equalTo( data->assetInfo() ) )
					{
						// the dragged items are being dropped onto one of it's items,
						// so avoid adding it
						doAdd = false;
						break;
					}
					ii = ii->getNext();
				}

				if ( doAdd )
				{
					CWaitCursor wait;
					ii = const_cast<UalItemInfo*>( &itemInfo );
					while ( ii )
					{
						if ( !data->isVFolder() )
						{
							// remove old item, if it exists, in order to add the new one in the proper location
							UalManager::instance().favourites().remove( ii->assetInfo_ );
						}
						// add to favourites
						if ( !UalManager::instance().favourites().getItem( ii->assetInfo_ ) )
							UalManager::instance().favourites().addAt(
								ii->assetInfo_,
								data->assetInfo() );
						else
							UalManager::instance().favourites().add( ii->assetInfo_ );
						ii = ii->getNext();
					}
				}
			}
		}
		else
			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
		folderTree_.UpdateWindow();
	}

	scrollWindow( &folderTree_, pt );
}


/**
 *	This method updates a drag and drop operation, finds out if the mouse is
 *	hovering on top of the list or the tree view, and if so, it lets them know.
 *
 *	@param itemInfo	Item being dragged.
 *	@param endDrag	True if this is the last update, false otherwise.
 *	return True if handled, false to continue looking for drop targets.
 */
bool UalDialog::updateDrag( const UalItemInfo& itemInfo, bool endDrag )
{
	BW_GUARD;

	CPoint pt( itemInfo.x_, itemInfo.y_ );
	HWND hwnd = ::WindowFromPoint( pt );
	if ( hwnd == smartList_.GetSafeHwnd() )
	{
		updateSmartListDrag( itemInfo, endDrag );
		return true;
	}
	smartList_.clearDropTarget();

	if ( hwnd == folderTree_.GetSafeHwnd() )
	{
		updateFolderTreeDrag( itemInfo, endDrag );
		return true;
	}
	folderTree_.SelectDropTarget( 0 );
	folderTree_.SetInsertMark( 0 );
	folderTree_.UpdateWindow();

	if ( ::IsChild( GetSafeHwnd(), hwnd ) )
	{
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
		return true;
	}

	return false;
}


/**
 *	This method is called when a filter is clicked, which requires an update
 *	of the list.
 *
 *	@param name		Name of the filter.
 *	@param pushed	State of the filter, true if checked, false if not.
 *	@param data		Filter specification object corresponding to the filter
 *					being clicked.
 */
void UalDialog::filterClicked( const wchar_t* name, bool pushed, void* data )
{
	BW_GUARD;

	FilterSpecPtr filter = (FilterSpec*)data;
	filter->setActive( pushed );
	HTREEITEM oldSel = folderTree_.GetSelectedItem();
	folderTree_.refreshVFolders();
	HTREEITEM sel = folderTree_.GetSelectedItem();
	if ( sel && sel != oldSel ) 
		folderTreeSelect( (VFolderItemData*)folderTree_.GetItemData( sel ) );
	smartList_.updateFilters();
	updateFiltersImage();
	refreshStatusBar();
}


/**
 *	This method is called when the search text changes, which requires an
 *	update of the list.
 *
 *	@param wParam	MFC param, ignored.
 *	@param lParam	MFC param, ignored.
 *	@result		MFC return 0.
 */
LRESULT UalDialog::OnSearchFieldChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	filterHolder_.setSearchText( search_.searchText() );
	smartList_.updateFilters();
	refreshStatusBar();
	return 0;
}


/**
 *	This method is called when the magnifying glass left to the search text box
 *	is clicked, which requires the filters visibility to be toggled.
 *
 *	@param wParam	MFC param, ignored.
 *	@param lParam	MFC param, ignored.
 *	@result		MFC return 0.
 */
LRESULT UalDialog::OnSearchFieldFilters( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if (!filtersCtrl_.empty())
	{
		showFilters_ = !showFilters_;
		CRect rect;
		GetClientRect( &rect );
		adjustFiltersSize( rect.Width(), rect.Height() );
		adjustSplitterSize( rect.Width(), rect.Height() );
	}
	return 0;
}


/**
 *	This method updates the dialog's status bar text and its tooltip.
 *
 *	@param text		New string to be the text for the status bar.
 */
void UalDialog::setStatusText( const BW::wstring& text )
{
	BW_GUARD;

	statusBar_.SetWindowText( text.c_str() );
	toolTip_.UpdateTipText( text.c_str(), &statusBar_ );
}


/**
 *	This method is called if an error occurs.  It simply relays error handling
 *	to the appropriate functor.
 *
 *	@param msg	Error string.
 */
void UalDialog::error( const BW::string& msg )
{
	BW_GUARD;

	if ( UalManager::instance().errorCallback() )
		(*UalManager::instance().errorCallback())( BW::string( "Asset Browser: " ) + msg );
}

BW_END_NAMESPACE

