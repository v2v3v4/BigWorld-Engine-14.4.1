
#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "thumbnail_manager.hpp"
#include "vfolder_file_provider.hpp"
#include "list_file_provider.hpp"
#include "common/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
//	Section: VFolderFileItemData
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 *
 *	@param provider	VFolder provider this item comes from.
 *	@param type		Whether it's a file or a folder.
 *	@param assetInfo	Item's asset browser info struct.
 *	@param group	Group ID, different for files than folders.
 *	@param expandable	True if this item is an expandable subfolder.
 */
VFolderFileItemData::VFolderFileItemData(
		VFolderProviderPtr provider,
		ItemDataType type,
		const AssetInfo& assetInfo,
		int group,
		bool expandable ) :
	VFolderItemData( provider, assetInfo, group, expandable ),
	type_( type )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
VFolderFileItemData::~VFolderFileItemData()
{
	BW_GUARD;
}


/**
 *	This method is called to handle the case when "this" is equal to the other
 *	specified item.  For files, if the item is duplicated, we just make sure
 *	only the item from the first specified search path in paths.xml is taken.
 *
 *	@param data		Other item's data.
 *	@return		True to ignore the "data" item, false to keep "this" and data.
 */
bool VFolderFileItemData::handleDuplicate( VFolderItemDataPtr data )
{
	BW_GUARD;

	if ( !data || data->isVFolder() )
		return false;

	VFolderFileItemData* fileData = (VFolderFileItemData*)data.getObject();
	if ( type_ == ITEMDATA_FOLDER && fileData->getItemType() == ITEMDATA_FOLDER )
	{
		BW::wstring newPath = assetInfo().longText();
		newPath += L";";
		newPath += fileData->assetInfo().longText();
		assetInfo().longText( newPath );
		return true;
	}
	else
	{		
		// check if both are files, and if both have the same relative paths
		BW::string nlongText1, nlongText2;
		bw_wtoutf8( assetInfo().longText(), nlongText1 );
		bw_wtoutf8( fileData->assetInfo().longText(), nlongText2 );

		if ( type_ == fileData->type_ &&
			BWResource::dissolveFilename( nlongText1 ) ==
			BWResource::dissolveFilename( nlongText2 ) )
			return true; // ignore new data, assume the first one is valid
		else
			return false; // keep it, it is a folder or a file in another path
	}
}


///////////////////////////////////////////////////////////////////////////////
//	Section: VFolderFileItemData
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 */
VFolderFileProvider::VFolderFileProvider()
{
	BW_GUARD;

	init( L"", L"", L"", L"", L"", 0 );
}


/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Postfix used to name thumbnails (.thumbnail.bmp).
 *	@param type		This provider's type name.
 *	@param paths	Semicolon separated list of paths to scan.
 *	@param extensions	List of filename extensions of files to scan for.
 *	@param includeFolders	Semicolon separated list of paths to include.
 *	@param excludeFolders	Semicolon separated list of paths to exclude.
 *	@param flags	Initialisation flags. See FILETREE_FLAGS in the hpp.
 */
VFolderFileProvider::VFolderFileProvider(
	const BW::wstring& thumbnailPostfix,
	const BW::wstring& type,
	const BW::wstring& paths,
	const BW::wstring& extensions,
	const BW::wstring& includeFolders,
	const BW::wstring& excludeFolders,
	int flags ) :
	thumbnailPostfix_( thumbnailPostfix )
{
	BW_GUARD;

	init( type, paths , extensions, includeFolders, excludeFolders, flags );
}


/**
 *	Destructor.
 */
VFolderFileProvider::~VFolderFileProvider()
{
	BW_GUARD;

	clearFinderStack();
}


/**
 *	This method initialises the provider with all the needed info for scanning
 *	for the desired files and folders.
 *
 *	@param type		This provider's type name.
 *	@param paths	Semicolon separated list of paths to scan.
 *	@param extensions	List of filename extensions of files to scan for.
 *	@param includeFolders	Semicolon separated list of paths to include.
 *	@param excludeFolders	Semicolon separated list of paths to exclude.
 *	@param flags	Initialisation flags. See FILETREE_FLAGS in the hpp.
 */
void VFolderFileProvider::init(
	const BW::wstring& type,
	const BW::wstring& paths,
	const BW::wstring& extensions,
	const BW::wstring& includeFolders,
	const BW::wstring& excludeFolders,
	int flags )
{
	BW_GUARD;

	type_ = type;

	flags_ = flags;
	
	paths_.clear();
	extensions_.clear();
	includeFolders_.clear();
	excludeFolders_.clear();

	BW::wstring pathsL = paths;
	std::replace( pathsL.begin(), pathsL.end(), L'/', L'\\' );
	bw_tokenise( pathsL, L";,", paths_ );

	BW::wstring extL = extensions;
	// TODO:UNICODE: is lowercase really needed?
	// StringUtils::toLowerCase( extL );
	bw_tokenise( extL, L";,", extensions_ );

	BW::wstring includeFoldersL = includeFolders;
	std::replace( includeFoldersL.begin(), includeFoldersL.end(), L'/', L'\\' );
	bw_tokenise( includeFoldersL, L";,", includeFolders_ );

	BW::wstring excludeFoldersL = excludeFolders;
	std::replace( excludeFoldersL.begin(), excludeFoldersL.end(), L'/', L'\\' );
	bw_tokenise( excludeFoldersL, L";,", excludeFolders_ );

	StringUtils::filterSpecVectorT( paths_, excludeFolders_ );
}


/**
 *	This method removes the top element from the file scanner stack for when
 *	recursively scanning for files.
 */
void VFolderFileProvider::popFinderStack()
{
	BW_GUARD;

	finderStack_.pop();
}


/**
 *	This method clears the file scanner stack.
 */
void VFolderFileProvider::clearFinderStack()
{
	BW_GUARD;

	while( !finderStack_.empty() )
		popFinderStack();
}


/**
 *	This method is called to prepare the enumerating of items in a VFolder
 *	or subfolder.
 *
 *	@param parent	Parent VFolder, if any.
 *	@return		True of there are items in it, false if empty.
 */
bool VFolderFileProvider::startEnumChildren( const VFolderItemDataPtr parent )
{
	BW_GUARD;

	clearFinderStack();
	BW::vector<BW::wstring>* paths;
	BW::vector<BW::wstring> subPaths;

	if ( !parent || parent->isVFolder() )
	{
		paths = &paths_;
	}
	else
	{
		if ( ((VFolderFileItemData*)parent.getObject())->getItemType() == VFolderFileItemData::ITEMDATA_FILE )
			return false;

		BW::wstring fullPath = parent->assetInfo().longText().c_str();
		bw_tokenise( fullPath, L",;", subPaths );
		paths = &subPaths;
	}

	if ( paths->empty() )
		return false;

	BW::wstring path = *(paths->begin());
	path += L"\\*.*";
	FileFinderPtr finder = new FileFinder();
	if ( !finder->files.FindFile( path.c_str() ) )
		return false;

	finder->paths = *paths;
	finder->path = finder->paths.begin();
	finder->pathEnd = finder->paths.end();
	finder->eof = false;

	finderStack_.push( finder );
	return true;
}


/**
 *	This method returns the top of the file scanning stack.
 *
 *	@return		Top of the file scanning stack.
 */
VFolderFileProvider::FileFinderPtr VFolderFileProvider::topFinderStack()
{
	BW_GUARD;

	FileFinderPtr finder = finderStack_.top();

	if ( finder->eof )
	{
		// unwind stack to get next path
		if ( ++(finder->path) == finder->pathEnd )
		{
			while ( finder->eof )
			{
				popFinderStack();
				if ( finderStack_.empty() )
					return 0;
				finder = finderStack_.top();
			}
		}
		else
		{
			BW::wstring path = *(finder->path);
			path += L"\\*.*";
			if ( finder->files.FindFile( path.c_str() ) )
				finder->eof = false;
		}
	}

	return finder;
}


/**
 *	This method is called to iterate to and get the next item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param img		Returns the thumbnail for the item, if available.
 *	@return		Next item in the provider.
 */
VFolderItemDataPtr VFolderFileProvider::getNextChild( ThumbnailManager& thumbnailManager, CImage& img )
{
	BW_GUARD;

	if ( finderStack_.empty() )
		return 0;

	FileFinderPtr finder = topFinderStack();

	if ( !finder )
		return 0;

	bool found = false;
	int group;
	BW::wstring name;
	VFolderFileItemData::ItemDataType type;
	while( !finder->eof )
	{
		finder->eof = finder->files.FindNextFile()?false:true;

		if ( !finder->files.IsDirectory() )
		{
			// check filters
			filterHolder_->enableSearchText( false );
			BW::string nroot;
			BW::string nfilename;
			BW::string nfilepath;
			bw_wtoutf8( (LPCTSTR)finder->files.GetFileName(), nfilename );
			bw_wtoutf8( (LPCTSTR)finder->files.GetFilePath(), nfilepath );
			bw_wtoutf8( (LPCTSTR)finder->files.GetRoot(), nroot );
			if ( ( !filterHolder_ || filterHolder_->filter( (LPCTSTR)finder->files.GetFileName(), (LPCTSTR)finder->files.GetFilePath() ) ) &&
				( includeFolders_.empty() || StringUtils::matchSpecT( BW::wstring( (LPCTSTR)finder->files.GetRoot()), includeFolders_ ) ) )
			{
				// file
				if ( ( flags_ & FILETREE_SHOWFILES )
					&& StringUtils::matchExtensionT( BW::wstring( (LPCTSTR)finder->files.GetFileName() ), extensions_ )
					&& finder->files.GetFileName().Find( thumbnailPostfix_.c_str() ) == -1 
					&& finder->files.GetFileName().Find( L".thumbnail.bmp" ) == -1 )
				{
					BW::string filePath;
					bw_wtoutf8( (LPCTSTR)finder->files.GetFilePath(), filePath );
					// if it's not a DDS, or if its a DDS and a corresponding
					// source image file doesn't exists, return the file.
					BW::wstring wbmp; bw_utf8tow( BWResource::changeExtension( filePath, ".bmp" ), wbmp );
					BW::wstring wpng; bw_utf8tow( BWResource::changeExtension( filePath, ".png" ), wpng );
					BW::wstring wtga; bw_utf8tow( BWResource::changeExtension( filePath, ".tga" ), wtga );
					if ( BWResource::getExtension( filePath ) != "dds" ||
						(!PathFileExists( wbmp.c_str() ) &&
						!PathFileExists( wpng.c_str() ) &&
						!PathFileExists( wtga.c_str() ) ) )
					{
						name = (LPCTSTR)finder->files.GetFileName();
						group = FILEGROUP_FILE;
						type = VFolderFileItemData::ITEMDATA_FILE;
						thumbnailManager.create( (LPCTSTR)finder->files.GetFilePath(), img, 16, 16, folderTree_ );
						found = true;
					}
				}
			}
			filterHolder_->enableSearchText( true );
			if ( found )
				break;
		}
		else if ( !finder->files.IsDots() )
		{
			// dir
			if ( excludeFolders_.empty() ||
					!StringUtils::matchSpecT( BW::wstring( (LPCTSTR)finder->files.GetFilePath() ), excludeFolders_ ) )
			{
				if ( flags_ & FILETREE_SHOWSUBFOLDERS )
				{
					// return the folder's name
					name = (LPCTSTR)finder->files.GetFileName();
					group = FILEGROUP_FOLDER;
					type = VFolderFileItemData::ITEMDATA_FOLDER;
					found = true;
					break;
				}
				else if ( ( flags_ & FILETREE_SHOWFILES )
						&& !( flags_ & FILETREE_DONTRECURSE ) )
				{
					// push the folder in the finder stack to find all files in it
					BW::vector<BW::wstring> subPaths;
					BW::wstring path = (LPCTSTR)finder->files.GetFilePath();
					subPaths.push_back( path );
					path += L"\\*.*";
					FileFinderPtr newFinder = new FileFinder();
					if ( newFinder->files.FindFile( path.c_str() ) )
					{
						newFinder->paths = subPaths;
						newFinder->path = newFinder->paths.begin();
						newFinder->pathEnd = newFinder->paths.end();
						newFinder->eof = false;

						finderStack_.push( newFinder );
						finder = newFinder;
					}
				}
			}
		}

		if ( finder->eof )
		{
			finder = topFinderStack();

			if ( !finder )
				return 0;
		}
	}

	if ( found )
	{
		VFolderFileItemData* newItem =
			new VFolderFileItemData( this,
				type,
				AssetInfo(
					type==VFolderFileItemData::ITEMDATA_FOLDER?L"FOLDER":L"FILE",
					name.c_str(),
					(LPCTSTR)finder->files.GetFilePath() ),
				group, (group == FILEGROUP_FILE)?false:true );
		return newItem;
	}
	else
	{
		return 0;
	}
}


/**
 *	This method creates the thumbnail for an item.
 *
 *	@param thumbnailManager		Object providing the thumbnails for items.
 *	@param data		Item data.
 *	@param img		Returns the thumbnail for the item, if available.
 */
void VFolderFileProvider::getThumbnail( ThumbnailManager& thumbnailManager, VFolderItemDataPtr data, CImage& img )
{
	BW_GUARD;

	if ( !data )
		return;

	thumbnailManager.create( data->assetInfo().longText(), img, 16, 16, folderTree_ );
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
const BW::wstring VFolderFileProvider::getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished )
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
		if (finished)
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_FILE_PROVIDER/DESCRIPTION", 
					getPathsString(),
					numItems
				);
		}
		else
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_FILE_PROVIDER/DESCRIPTION_LOADING", 
					getPathsString(),
					numItems
				);
		}
	}
	else if ( !data->isCustomItem() &&
		((VFolderFileItemData*)data.getObject())->getItemType() == VFolderFileItemData::ITEMDATA_FOLDER )
	{
		if (finished)
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_FILE_PROVIDER/DESCRIPTION", 
					data->assetInfo().longText(),
					numItems
				);
		}
		else
		{
			desc = 
				Localise
				(
					L"UAL/VFOLDER_FILE_PROVIDER/DESCRIPTION_LOADING", 
					data->assetInfo().longText(),
					numItems
				);
		}
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
bool VFolderFileProvider::getListProviderInfo(
	VFolderItemDataPtr data,
	BW::wstring& retInitIdString,
	ListProviderPtr& retListProvider,
	bool& retItemClicked )
{
	BW_GUARD;

	if ( !data || !listProvider_ )
		return false;

	int flags = ListFileProvider::LISTFILEPROV_DEFAULT;
	BW::wstring fullPath;
	if ( getFlags() & FILETREE_DONTRECURSE ) 
		flags |= ListFileProvider::LISTFILEPROV_DONTRECURSE;
	if ( data->isVFolder() )
		fullPath = getPathsString();
	else if ( !data->isCustomItem() && ((VFolderFileItemData*)data.getObject())->getItemType() == VFolderFileItemData::ITEMDATA_FOLDER )
		fullPath = data->assetInfo().longText();
	else
	{
		// item is a file, so select the item's parent folder to fill up the list
		retItemClicked = true;
		HTREEITEM item = data->getTreeItem();
		VFolderFileItemData* parentData = 0;
		while( item )
		{
			item = folderTree_->GetParentItem( item );
			if ( item )
			{
				parentData = (VFolderFileItemData*)folderTree_->GetItemData( item );
				if ( parentData &&
						( parentData->isVFolder() ||
						parentData->getItemType() == VFolderFileItemData::ITEMDATA_FOLDER ) )
					break;
				parentData = 0;
			}
		}
		if ( parentData )
		{
			if ( parentData->isVFolder() )
				fullPath = getPathsString();
			else if ( parentData->getItemType() == VFolderFileItemData::ITEMDATA_FOLDER )
				fullPath = parentData->assetInfo().longText();
		}
	}

	// construct a list string to detect when the init params are redundant and avoid flicker
	wchar_t flagsStr[80];
	bw_snwprintf( flagsStr, ARRAY_SIZE(flagsStr), L"%d", flags );
	BW::wstring listInit =
		getType() +
		fullPath +
		getExtensionsString() +
		getIncludeFoldersString() +
		getExcludeFoldersString() +
		flagsStr;

	if ( retInitIdString == listInit && retListProvider == listProvider_ )
		return false;

	if ( !fullPath.empty() )
	{
		retListProvider = listProvider_;
		((ListFileProvider*)listProvider_.getObject())->init(
			getType(),
			fullPath,
			getExtensionsString(),
			getIncludeFoldersString(),
			getExcludeFoldersString(),
			flags );
		retInitIdString = listInit;
		return true;
	}

	return false;
}


/**
 *	This method returns the provider's type name.
 *
 *	@return		The provider's type name.
 */
const BW::wstring VFolderFileProvider::getType()
{
	BW_GUARD;

	return type_;
}


/**
 *	This method returns the provider's initialisation flags.
 *
 *	@return		The provider's initialisation flags.
 */
int VFolderFileProvider::getFlags()
{
	BW_GUARD;

	return flags_;
}


/**
 *	This method returns the provider's paths.
 *
 *	@return		The provider's paths.
 */
const BW::vector<BW::wstring>& VFolderFileProvider::getPaths()
{
	BW_GUARD;

	return paths_;
}


/**
 *	This method returns the provider's filename extensions.
 *
 *	@return		The provider's filename extensions.
 */
const BW::vector<BW::wstring>& VFolderFileProvider::getExtensions()
{
	BW_GUARD;

	return extensions_;
}


/**
 *	This method returns the provider's include folders.
 *
 *	@return		The provider's include folders.
 */
const BW::vector<BW::wstring>& VFolderFileProvider::getIncludeFolders()
{
	BW_GUARD;

	return includeFolders_;
}


/**
 *	This method returns the provider's exclude folders.
 *
 *	@return		The provider's exclude folders.
 */
const BW::vector<BW::wstring>& VFolderFileProvider::getExcludeFolders()
{
	BW_GUARD;

	return excludeFolders_;
}


/**
 *	This method returns the provider's paths in a semicolon-separated string.
 *
 *	@return		The provider's paths in a semicolon-separated string.
 */
const BW::wstring VFolderFileProvider::getPathsString()
{
	BW_GUARD;

	return StringUtils::vectorToStringT( paths_ );
}


/**
 *	This method returns the provider's filename extensions in a
 *	semicolon-separated string.
 *
 *	@return		The filename extensions in a semicolon-separated string.
 */
const BW::wstring VFolderFileProvider::getExtensionsString()
{
	BW_GUARD;

	return StringUtils::vectorToStringT( extensions_ );
}


/**
 *	This method returns the provider's include folders in a semicolon-separated
 *	string.
 *
 *	@return		The provider's include folders in a semicolon-separated string.
 */
const BW::wstring VFolderFileProvider::getIncludeFoldersString()
{
	BW_GUARD;

	return StringUtils::vectorToStringT( includeFolders_ );
}


/**
 *	This method returns the provider's exclude folders in a semicolon-separated
 *	string.
 *
 *	@return		The provider's exclude folders in a semicolon-separated string.
 */
const BW::wstring VFolderFileProvider::getExcludeFoldersString()
{
	BW_GUARD;

	return StringUtils::vectorToStringT( excludeFolders_ );
}

BW_END_NAMESPACE

