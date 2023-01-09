
#include "pch.hpp"
#include "ual_dialog.hpp"
#include "vfolder_multi_provider.hpp"
#include "list_multi_provider.hpp"
#include "ual_vfolder_loader.hpp"
#include "resmgr/bwresource.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/string_utils.hpp"


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	LoaderRegistry: VFolder loaders vector singleton class
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method returns the appropriate VFolder loader for the give section.
 *
 *	@param sectionName	VFolder data section being loaded.
 *	@return		VFolder loader for the data section, or NULL if not found. 
 */
UalVFolderLoaderPtr LoaderRegistry::loader( const BW::string& sectionName )
{
	BW_GUARD;

	VFolderLoaders::iterator loader = LoaderRegistry::loaders().end();

	for( VFolderLoaders::iterator i = LoaderRegistry::loaders().begin();
		i != LoaderRegistry::loaders().end(); ++i )
	{
		if ( (*i)->test( sectionName ) )
		{
			loader = i;
			break;
		}
	}

	if ( loader == LoaderRegistry::loaders().end() )
	{
		return NULL; 
	}

	return *loader;
}


///////////////////////////////////////////////////////////////////////////////
//	UalVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method forwards errors to the Asset Browser dialog.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param msg	Error message string.
 */
void UalVFolderLoader::error( UalDialog* dlg, const BW::string& msg )
{
	BW_GUARD;

	if ( dlg )
		dlg->error( msg );
}


/**
 *	This method loads all the information common to all VFolder loaders.
 *	Usually called from the "load" method of derived classes before performing
 *	its own loading.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section containing the loader information.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param defaultThumbSize	Default list item thumbnail size.
 */
void UalVFolderLoader::beginLoad( UalDialog* dlg, DataSectionPtr section, DataSectionPtr customData, int defaultThumbSize )
{
	BW_GUARD;

	if( icon_ )	
	{
		DestroyIcon( icon_ );
		icon_ = NULL;
	}
	if( iconSel_ )	
	{
		DestroyIcon( iconSel_ );
		iconSel_ = NULL;
	}

	displayName_ = !customData?section->asWideString():customData->asWideString();
	icon_ = dlg->iconFromXml( section, "icon" );
	iconSel_ = dlg->iconFromXml( section, "iconSel" );
	show_ = section->readBool( "show", true );
	folderData_ = new UalFolderData();
	folderData_->internalTag_ = section->readWideString( "internalTag" );
	folderData_->thumbSize_ = section->readInt( "thumbnailSize", defaultThumbSize );
	folderData_->originalThumbSize_ = folderData_->thumbSize_;
	if ( folderData_->thumbSize_ < 0 || folderData_->thumbSize_ > 2 )
	{
		folderData_->thumbSize_ = defaultThumbSize;
		dlg->error( "Wrong thumbnailSize. Valid values are 0, 1 or 2" );
	}
	folderData_->showInList_ = section->readBool( "showInList", true );
	folderData_->multiItemDrag_ = section->readBool( "multiItemDrag", false );

	BW::vector<DataSectionPtr> sections;
	section->openSections( "disableFilter", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		bw_tokenise( (*s)->asWideString(), L",;", folderData_->disabledFilters_ );
	}
	sections.clear();

	XmlItemList customItemList;
	customItemList.setDataSection( section->openSection( "customItems" ) );
	customItemList.getItems( folderData_->customItems_ );

	folderData_->idleText_ = section->readWideString( "searchIdleText", dlg->search_.idleText() );

	dlg->folderData_.push_back( folderData_ );
}


/**
 *	This method creates and returns the VFolder once it's been loaded from XML.
 *	Usually called from the "load" method of derived classes after performing
 *	its own loading.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param provider	Virtual folder item provider.
 *	@param parent	Parent virtual folder, if any.
 *	@param expandable	True if the folder should be expandable, false if not.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@param subVFolders		True to indicate that there are nested VFolders.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalVFolderLoader::endLoad( UalDialog* dlg, VFolderProviderPtr provider,
							VFolderPtr parent, bool expandable,
							bool addToFolderTree /*=true*/, bool subVFolders /*=false*/ )
{
	BW_GUARD;

	provider->setFolderTree( &dlg->folderTree_ );
	provider->setFilterHolder( &dlg->filterHolder_ );

	VFolderPtr ret;
	if ( addToFolderTree )
	{
		ret = dlg->folderTree_.addVFolder(
			displayName_, provider, parent,
			icon_, iconSel_, show_,
			expandable,
			&folderData_->customItems_,
			(void*)folderData_.getObject(),
			subVFolders );
	}
	else
	{
		// don't add it to the folderTree_ tree control, just return it
		ret = new VFolder( NULL, 
			displayName_, NULL, provider, expandable,
			true, &folderData_->customItems_,
			(void*)folderData_.getObject(),
			subVFolders );
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//	UalVFolderLoaderFactory
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.  Simply registers the specified loader on creation.
 *
 *	@param loader	Virtual folder to register.
 */
UalVFolderLoaderFactory::UalVFolderLoaderFactory( UalVFolderLoaderPtr loader )
{
	BW_GUARD;

	UalDialog::registerVFolderLoader( loader );
}


///////////////////////////////////////////////////////////////////////////////
//	UalFilesVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method validates a path specified in the XML config.
 *
 *	@param path		File path to validate.
 *	@return		True if the path is a valid one, false otherwise.
 */
bool UalFilesVFolderLoader::pathIsGood( const BW::wstring& path )
{
	BW_GUARD;

	return !path.empty() && path != L"."
		&& path != L"./" && path != L"/"
		&& path != L".\\" && path != L"\\";
}


/**
 *	This method creates and returns a file-scanning VFolder.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalFilesVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	beginLoad( dlg, section, customData, 2 );

	BW::wstring type = section->readWideString( "type", L"FILE" );

	int flags = FILETREE_SHOWSUBFOLDERS;
	if ( section->readBool( "showSubfolders", true ) )
		flags |= FILETREE_SHOWSUBFOLDERS;
	else
		flags &= ~FILETREE_SHOWSUBFOLDERS;
	if ( section->readBool( "showFiles", false ) )
		flags |= FILETREE_SHOWFILES;
	else
		flags &= ~FILETREE_SHOWFILES;
	if ( !section->readBool( "recurseFiles", true ) )
		flags |= FILETREE_DONTRECURSE;
	else
		flags &= ~FILETREE_DONTRECURSE;

	bool expandable = (flags & FILETREE_SHOWFILES) || (flags & FILETREE_SHOWSUBFOLDERS);

	BW::wstring paths;
	BW::vector<DataSectionPtr> sections;
	if ( !customData )
		section->openSections( "path", sections );
	else
		customData->openSections( "path", sections );

	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		BW::wstring xmlPath = (*s)->asWideString();
		BW::vector<BW::wstring> pathVec;
		bw_tokenise( xmlPath, L",;", pathVec );

		for ( int i = 0; i < UalManager::instance().getNumPaths(); i++ )
		{
			for( BW::vector<BW::wstring>::iterator p = pathVec.begin();
				p != pathVec.end(); ++p )
			{
				BW::wstring path = UalManager::instance().getPath( i );
				bool xmlPathGood = pathIsGood( *p );

				if ( xmlPathGood )
				{
					path += L"/";
					path += *p;
				}
				if ( PathIsDirectory( path.c_str() ) )
				{
					if ( !paths.empty() )
						paths += L";";
					paths += path;
				}
			}
		}
		for( BW::vector<BW::wstring>::iterator p = pathVec.begin();
			p != pathVec.end(); ++p )
		{
			if ( pathIsGood( *p ) && PathIsDirectory( (*p).c_str() ) )
			{
				if ( !paths.empty() )
					paths += L";";
				paths += *p;
			}
		}
	}
	sections.clear();

	BW::wstring extensions;
	section->openSections( "extension", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		if ( extensions.length() > 0 )
			extensions += L";";
		extensions += (*s)->asWideString();
	}
	sections.clear();

	BW::wstring excludeFolders;
	section->openSections( "folderExclude", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		if ( excludeFolders.length() > 0 )
			excludeFolders += L";";
		excludeFolders += (*s)->asWideString();
	}
	sections.clear();

	BW::wstring includeFolders;
	section->openSections( "folderInclude", sections );
	for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
	{
		if ( includeFolders.length() > 0 )
			includeFolders += L";";
		includeFolders += (*s)->asWideString();
	}
	sections.clear();

	VFolderFileProvider* prov = new VFolderFileProvider(
		UalManager::instance().thumbnailManager().postfix(),
		type, paths, extensions, includeFolders, excludeFolders, flags );
	prov->setListProvider( dlg->fileListProvider() );

	return endLoad( dlg,
		prov,
		parent, expandable, addToFolderTree );
}
static UalVFolderLoaderFactory filesFactory( new UalFilesVFolderLoader() );


///////////////////////////////////////////////////////////////////////////////
//	UalXmlVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method creates and returns an XML list based VFolder.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalXmlVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	beginLoad( dlg, section, customData, 2 );

	bool showItems = section->readBool( "showItems", false );
	BW::wstring path = section->readWideString( "path" );

	BW::string npath;
	bw_wtoutf8( path, npath );
	if ( !BWResource::fileExists( npath ) )
		error( dlg, BW::string( "XML file not found: " ) + npath + "." );

	VFolderXmlProvider* prov = new VFolderXmlProvider( path );
	prov->setListProvider( dlg->xmlListProvider() );
	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );
	ret->setSortSubFolders( prov->getSort() );
	return ret;
}
static UalVFolderLoaderFactory xmlFactory( new UalXmlVFolderLoader() );


///////////////////////////////////////////////////////////////////////////////
//	UalHistoryVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method creates and returns a history tracking VFolder.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalHistoryVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	beginLoad( dlg, section, customData, 2 );

	bool showItems = section->readBool( "showItems", false );
	BW::wstring path = section->readWideString( "path" );
	UalManager::instance().history().setPath( path );
	int maxItems = section->readInt( "maxItems", UalManager::instance().history().getMaxItems() );
	if ( maxItems < 0 )
		error( dlg, "Wrong History/maxItems. Must be greater or equal to zero." );
	else
		UalManager::instance().history().setMaxItems( maxItems );

	if ( section->readBool( "clearOnLoad", false ) )
		UalManager::instance().history().clear();

	BW::string npath;
	bw_wtoutf8( path, npath );
	if ( !BWResource::fileExists( npath ) )
		error( dlg, BW::string( "History file not found: " ) + npath + "." );

	VFolderXmlProvider* prov = new VFolderXmlProvider( path );
	prov->setListProvider( dlg->historyListProvider() );
	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );
	ret->setSortSubFolders( prov->getSort() );
	dlg->historyFolderProvider( prov );
	return ret;
}
static UalVFolderLoaderFactory historyFactory( new UalHistoryVFolderLoader() );


///////////////////////////////////////////////////////////////////////////////
//	UalFavouritesVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method creates and returns a favourites list VFolder.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalFavouritesVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	beginLoad( dlg, section, customData, 2 );

	bool showItems = section->readBool( "showItems", false );
	BW::wstring path = section->readWideString( "path" );
	UalManager::instance().favourites().setPath( path );

	BW::string npath;
	bw_wtoutf8( path, npath );
	if ( !BWResource::fileExists( npath ) )
		error( dlg, BW::string( "Favourites file not found: " ) + npath + "." );

	VFolderXmlProvider* prov = new VFolderXmlProvider( path );
	prov->setListProvider( dlg->favouritesListProvider() );
	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );
	ret->setSortSubFolders( prov->getSort() );
	dlg->favouritesFolderProvider( prov );
	return ret;
}
static UalVFolderLoaderFactory favouritesFactory( new UalFavouritesVFolderLoader() );


///////////////////////////////////////////////////////////////////////////////
//	UalMultiVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method creates and returns a multi-VFolder, which allows for merging
 *	results from two or more VFolders into one.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalMultiVFolderLoader::load( 
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	DataSectionPtr vfolders = section->openSection( "Providers" );
	if ( vfolders == NULL )
		return NULL;

	beginLoad( dlg, section, customData, 2 );

	bool showItems = section->readBool( "showItems", true );

	// create the MultiVFolder providers and link them
	VFolderMultiProvider* prov = new VFolderMultiProvider();
	ListMultiProvider* listProv = new ListMultiProvider();
	prov->setListProvider( listProv );
	
	VFolderPtr ret = endLoad( dlg,
		prov,
		parent, showItems, addToFolderTree );

	for ( int i = 0; i < vfolders->countChildren(); ++i )
	{
		DataSectionPtr cs = vfolders->openChild( i );
		if ( !cs )
		{
			char str[256];
			_snprintf( str, 255,
				"Failed opening section %d in the 'Providers' section of MultiVFolder '%s'.",
				i, section->asString().c_str() );
			str[255] = '\0';
			error( dlg, str );
			continue;
		}

		UalVFolderLoaderPtr loader = LoaderRegistry::loader( cs->sectionName() );
		if ( loader == NULL )
		{
			char str[256];
			_snprintf( str, 255,
				"Cannot load unknown sub-folder type '%s' in MultiVFolder '%s'.",
				cs->sectionName().c_str(), section->asString().c_str() );
			str[255] = '\0';
			error( dlg, str );
			continue;
		}

		// load the sub-provider through the LoaderRegistry into a 'vfolder'
		// VFolder object. Note the use of false for the addToFolderTree param
		// to avoid getting the sub-vfolder added to the tree control.
		// **NOTE**: At the moment, only one 'Files' provider is supported in a
		// MultiVFolder folder.
		VFolderPtr vfolder = loader->load( dlg, cs, parent, customData, false/*addToFolderTree*/ );
		
		if ( vfolder == NULL )
		{
			char str[256];
			_snprintf( str, 255,
				"Failed loading sub-folder of type '%s' of MultiVFolder '%s'.",
				cs->sectionName().c_str(), section->asString().c_str() );
			str[255] = '\0';
			error( dlg, str );
			continue;
		}

		// add the sub-providers to the respective Multi providers
		// (VFolder/List)
		prov->addProvider( vfolder->getProvider() );
		listProv->addProvider( vfolder->getProvider()->getListProvider() );
	}

	return ret;
}
static UalVFolderLoaderFactory multiFactory( new UalMultiVFolderLoader() );


///////////////////////////////////////////////////////////////////////////////
//	UalPlainVFolderLoader
///////////////////////////////////////////////////////////////////////////////

/**
 *	This method creates and returns an empty, plain VFolder, useful for nesting
 *	VFolders.
 *
 *	@param dlg	Asset Browser dialog using this loader.
 *	@param section	Data section with the folder's config and parameters.
 *	@param parent	Parent virtual folder, if any.
 *	@param customData	Overrides for tags in "section", useful for when
 *						loading a subfolder of a VFolder.
 *	@param addToFolderTree	True to add the new VFolder to the tree view.
 *	@return		Newly created and loaded VFolder.
 */
VFolderPtr UalPlainVFolderLoader::load(
	UalDialog* dlg, DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
	bool addToFolderTree )
{
	BW_GUARD;

	if ( !dlg || !section || !test( section->sectionName() ) )
		return 0;

	beginLoad( dlg, section, customData, 2 );

	return endLoad( dlg,
		0, // NULL provider
		parent,
		true/*expandable*/, addToFolderTree, true/*subVFolders*/ );
}
static UalVFolderLoaderFactory plainFactory( new UalPlainVFolderLoader() );

BW_END_NAMESPACE

