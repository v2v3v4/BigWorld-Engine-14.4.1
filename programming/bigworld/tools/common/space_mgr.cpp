#include "pch.hpp"
#include "space_mgr.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk_space.hpp"
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <sstream>
#include <algorithm>
#include <tchar.h>


BW_BEGIN_NAMESPACE

// helper function to ensure that paths are understood by windows even if
// EDITOR_ENABLED is not defined (i.e. NavGen)
static void pathToWindows( BW::wstring& path )
{
	BW_GUARD;

	// make sure it has a drive letter.
	std::replace( path.begin(), path.end(), '/', '\\' );
	wchar_t fullPath[MAX_PATH];
	wchar_t* filePart = NULL;
	if ( GetFullPathName( path.c_str(), ARRAY_SIZE( fullPath ), fullPath, &filePart ) )
	{
		path = fullPath;
	}
}

class FolderFilter : public IFolderFilter
{
public:
	FolderFilter()
	{}

public: // IUnknown implementation
	STDMETHOD( QueryInterface )( IN REFIID riid, IN OUT LPVOID* ppvObj )
	{
		BW_GUARD;

		*ppvObj = NULL;

		if( IsEqualIID( riid, IID_IUnknown ) )
			*ppvObj =  static_cast< IUnknown* >( this );
		else if( IsEqualIID( riid, IID_IFolderFilter ) )
			*ppvObj =  static_cast< IFolderFilter* >( this );

		return *ppvObj ? S_OK : E_NOINTERFACE;
	}
	STDMETHOD_( ULONG, AddRef )( VOID )
	{
		return 1;
	}
	STDMETHOD_( ULONG, Release )( VOID )
	{
		return 1;
	}

public: // IFolderFilter implementation
	STDMETHOD( ShouldShow )( IN IShellFolder* pIShellFolder, IN LPCITEMIDLIST pidlFolder, IN LPCITEMIDLIST pidlItem )
	{
		BW_GUARD;

		MF_ASSERT( pIShellFolder != NULL );	
		MF_ASSERT( pidlItem	  != NULL );
	
		// If an item is a folder, then accept it
		LPCITEMIDLIST pidl[ 1 ] = { pidlItem };
		SFGAOF ulAttr = SFGAO_FOLDER;
		pIShellFolder->GetAttributesOf( 1, pidl, &ulAttr );
	
		//Ignore all non-folders
		if ( ( ulAttr & SFGAO_FOLDER ) !=  SFGAO_FOLDER )
			return ( S_FALSE );
		
		STRRET name;
		ZeroMemory( &name, sizeof( name ) );

		pIShellFolder->GetDisplayNameOf( pidlItem, SHGDN_FORPARSING, &name );

		wchar_t * pdir = NULL;
		StrRetToStr( &name, pidlItem, &pdir );

		BW::wstring dir = pdir;

		// INFO_MSG( "Callback for [%S]\n", dir.c_str() );

		CoTaskMemFree( pdir );

		//Add some cosmetics for string matching
		if( *dir.rbegin() != L'\\' )
			dir += L'\\';

		for( int i = 0; i < BWResource::getPathNum(); ++i )
		{
			BW::wstring wpath;
			bw_utf8tow( BWResource::getPath( i ), wpath );
			pathToWindows( wpath );

			if( _wcsnicmp( dir.c_str(), wpath.c_str(), dir.size() ) == 0 || 
				_wcsnicmp( dir.c_str(), wpath.c_str(), wpath.size() ) == 0 )
				return S_OK;
		}

		//The folder is neither in the paths nor a parent, don't allow it
		return ( S_FALSE );
	}
	STDMETHOD( GetEnumFlags )( IN IShellFolder* /*pIShellFolder*/, IN LPCITEMIDLIST /*pidlFolder*/, IN HWND* /*phWnd*/, OUT LPDWORD pdwFlags )
	{
		BW_GUARD;

		*pdwFlags = SHCONTF_FOLDERS;
		return S_OK;
	}
};

static FolderFilter folderFilter;

SpaceNameManager::SpaceNameManager( MRUProvider& mruProvider, BW::vector<BW::string>::size_type maxMRUEntries /*= 10*/ ):
	mruProvider_( mruProvider ), maxMRUEntries_( maxMRUEntries )
{
	BW_GUARD;

	for( BW::vector<BW::string>::size_type i = 0; i < maxMRUEntries_; ++i )
	{
		BW::stringstream ss;
		ss << "space/mru" << i;
		BW::string spacePath = mruProvider_.get( ss.str() );
		if( spacePath.size() )
			recentSpaces_.push_back( spacePath );
	}
}

void SpaceNameManager::addSpaceIntoRecent( const BW::string& space )
{
	BW_GUARD;

	for( BW::vector<BW::string>::iterator iter = recentSpaces_.begin();
		iter != recentSpaces_.end(); ++iter )
		if( *iter == space )
		{
			recentSpaces_.erase( iter );
			break;
		}
	if( recentSpaces_.size() >= maxMRUEntries_ )
		recentSpaces_.pop_back();
	recentSpaces_.insert( recentSpaces_.begin(), space );

	for( BW::vector<BW::string>::size_type i = 0; i < maxMRUEntries_; ++i )
	{
		BW::stringstream ss;
		ss << "space/mru" << i;
		mruProvider_.set( ss.str(), "" );
	}

	for( BW::vector<BW::string>::size_type i = 0; i < num(); ++i )
	{
		BW::stringstream ss;
		ss << "space/mru" << i;
		mruProvider_.set( ss.str(), entry( i ) );
	}
}

void SpaceNameManager::removeSpaceFromRecent( const BW::string& space )
{
	BW_GUARD;

	for( BW::vector<BW::string>::iterator iter = recentSpaces_.begin();
		iter != recentSpaces_.end(); ++iter )
		if( *iter == space )
		{
			recentSpaces_.erase( iter );
			break;
		}

	for( BW::vector<BW::string>::size_type i = 0; i < maxMRUEntries_; ++i )
	{
		BW::stringstream ss;
		ss << "space/mru" << i;
		mruProvider_.set( ss.str(), "" );
	}

	for( BW::vector<BW::string>::size_type i = 0; i < num(); ++i )
	{
		BW::stringstream ss;
		ss << "space/mru" << i;
		mruProvider_.set( ss.str(), entry( i ) );
	}
}

BW::vector<BW::string>::size_type SpaceNameManager::num() const
{
	return recentSpaces_.size();
}

BW::string SpaceNameManager::entry( BW::vector<BW::string>::size_type index ) const
{
	BW_GUARD;

	return recentSpaces_.at( index );
}

BW::string SpaceNameManager::browseForSpaces( HWND parent )
{
	BW::wstring wpath;

	if (num())
	{
		wpath = bw_utf8tow( BWResolver::resolveFilename( entry( 0 ) ) );

		pathToWindows( wpath );
	}

	return browseForSpaces( parent, wpath );
}

BW::string SpaceNameManager::browseForSpaces( HWND parent, const BW::wstring& defaultPath )
{
	BW_GUARD;

	CoInitialize( NULL );
	wchar_t path[ MAX_PATH ];
	ZeroMemory( path, sizeof( path ) );
	BW::string result;
	BROWSEINFO browseInfo = { 0 };
    browseInfo.hwndOwner = parent;
    browseInfo.pidlRoot = commonRoot();
    browseInfo.pszDisplayName = path;
    browseInfo.lpszTitle = Localise(L"COMMON/SPACE_MGR/SELECT_FOLDER");
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
    browseInfo.lpfn = BrowseCallbackProc;
	browseInfo.lParam = (LPARAM)defaultPath.c_str();
	LPITEMIDLIST pidl = SHBrowseForFolder( &browseInfo );
	if( pidl || browseInfo.pidlRoot )
	{
		if( pidl )
			bw_wtoutf8( getFolderByPidl( pidl ), result );
		LPMALLOC malloc;
		if( SUCCEEDED( SHGetMalloc( &malloc ) ) )
		{
			if( pidl )
				malloc->Free( pidl );
			if( browseInfo.pidlRoot )
				malloc->Free( (void*)browseInfo.pidlRoot );
		}
	}
	CoUninitialize();
	return result;
}

int CALLBACK SpaceNameManager::BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	BW_GUARD;

	WCHAR* path = (WCHAR*)lpData;
	BW::wstring wpath;

	switch( uMsg )
	{
	case BFFM_INITIALIZED:
		SendMessage( hwnd, BFFM_SETOKTEXT, 0, (LPARAM) L"&Open Space");
		if (_tcslen( path ))
		{
			SendMessage( hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)path );
		}
		break;
	case BFFM_SELCHANGED:
		wpath = getFolderByPidl( (LPITEMIDLIST)lParam );
		if( !wpath.empty() )
		{
			if( *wpath.rbegin() != L'\\' )
				wpath += L'\\';
			pathToWindows( wpath );
			SendMessage( hwnd, BFFM_ENABLEOK, 0,
				GetFileAttributes( ( wpath + SPACE_SETTING_FILE_NAME_W ).c_str() )
				!= INVALID_FILE_ATTRIBUTES );
		}
		break;
	case BFFM_IUNKNOWN:
		if( lParam )
		{
			IUnknown* unk = (IUnknown*)lParam;
			IFolderFilterSite* site;
			if( SUCCEEDED( unk->QueryInterface( IID_IFolderFilterSite, (LPVOID*)&site ) ) )
			{
				site->SetFilter( &folderFilter );
				site->Release();
			}
		}
		break;
	}
	return 0;
}

BW::wstring SpaceNameManager::getFolderByPidl( LPITEMIDLIST pidl )
{
	BW_GUARD;

	wchar_t path[ MAX_PATH ];
	if( SHGetPathFromIDList( pidl, path ) )
		return path;
	return L"";
}

LPITEMIDLIST SpaceNameManager::commonRoot()
{
	BW_GUARD;

	if( BWResource::getPathNum() == 0 )
		return NULL;
	BW::string root = BWResource::getPath( 0 );
	_strlwr( &root[0] );
	for( int i = 1; i < BWResource::getPathNum(); ++i )
	{
		BW::string s = BWResource::getPath( i );
		_strlwr( &s[0] );
		if( s.size() > root.size() )
			s.resize( root.size() );
		else
			root.resize( s.size() );
		while( s != root )
			s.resize( s.size() - 1 ), root.resize( root.size() - 1 );
	}

	if( !root.empty() )
	{
		BW::wstring wroot;
		root = BWResource::fixCommonRootPath( root );
		bw_utf8tow( root, wroot );
		pathToWindows( wroot );
		
		SFGAOF f;
		LPITEMIDLIST pidl;
		if( SUCCEEDED( SHParseDisplayName( wroot.c_str(), NULL, &pidl, 0, &f ) ) )
			return pidl;
	}

	//If the base root was not on the same drive then use the "Drives" virtual folder.
	LPITEMIDLIST pidlDrives = NULL;
	SHGetFolderLocation( NULL, CSIDL_DRIVES, 0, 0, &pidlDrives );
    return pidlDrives;


}

BW_END_NAMESPACE
