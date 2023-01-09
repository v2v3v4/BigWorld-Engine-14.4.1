#ifndef BWRESOURCE_HPP
#define BWRESOURCE_HPP

#include "datasection.hpp"
#include "file_system.hpp"

#include "cstdmf/singleton.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/bw_util.hpp"

BW_BEGIN_NAMESPACE

class MultiFileSystem;
class DataSectionCache;
class BWResourceImpl;
class BWResolver;
class ResourceModificationListener;

typedef SmartPointer<MultiFileSystem> MultiFileSystemPtr;

// Separator character for BW_RES_PATH
#ifdef __unix__
#define BW_RES_PATH_SEPARATOR ":"
#else
#define BW_RES_PATH_SEPARATOR ";"
#endif

/**
 *	This class is intended to be used as a singleton. It supports functionality
 *	for working with files in BigWorld's resource path.
 *
 *	It is also useful for operating multiple resource system contexts,
 *	where it may be desirable for extra resources to be visible to some
 *	threads but not others. In that case multiple instances of BWResource
 *	are permitted.
 *
 *  This class now contains all functionality previously owned by the now
 *  deprecated FilenameResolver, so use this class instead.
 */
class BWResource : public Singleton< BWResource >
{
public:
	BWResource();
	~BWResource();

	// Init methods taken from the now deprecated FilenameResolver
	static bool					init( int & argc, const char * argv[],
									bool removeArgs = false );
	static bool					init( const BW::string& fullPath, bool addAsPath = true );
	static bool					init( const BW::vector< BW::string > & paths );

	static void					fini();

	void						refreshPaths();

	/// Returns the root section
	DataSectionPtr 				rootSection();
	MultiFileSystem*			fileSystem();
	IFileSystem*				nativeFileSystem();

	/// Remove a resource from the cache.
	void						purge( const BW::StringRef& resourceID,
									bool recurse = false,
		   							DataSectionPtr spSection = NULL );
	void						purgeAll();

	/// Saves a section.
	bool						save( const BW::string & resourceID );

	/// Shortcut method for opening a section
	static DataSectionPtr		openSection( const BW::StringRef & resourceID,
									bool makeNewSection = false,
									DataSectionCreator* creator = NULL );

	/// File system monitoring methods
	void addModificationListener( ResourceModificationListener* listener );
	void removeModificationListener( ResourceModificationListener* listener );
	
	void enableModificationMonitor( bool enable );
	void flushModificationMonitor();
	bool ignoreFileModification( const BW::string& fileName, 
				ResourceModificationListener::Action action,
				bool oneTimeOnly );
	bool hasPendingModification( const BW::string& fileName );


	bool path( size_t idx, char* pathRet, size_t maxPath );
	bool path( size_t idx, wchar_t* pathRet, size_t maxPath );

	// thread watching methods
	static void watchAccessFromCallingThread(bool watch);
	static bool watchAccessFromCallingThread();
	static void checkAccessFromCallingThread(
		const BW::StringRef& path, const char* msg );

	// Static methods taken from the now deprecated FilenameResolver
	static IFileSystem::FileType getFileType( const BW::StringRef& file, IFileSystem::FileInfo * pFI );
	static bool					isDir( const BW::StringRef& file );
	static bool					isDirOrBasePath( const BW::StringRef& pathname );
	static bool					isFile( const BW::StringRef& file );
	static bool					fileExists( const BW::StringRef& file );
	static bool					fileAbsolutelyExists( const BW::StringRef& file );
	static bool					pathIsRelative( const BW::StringRef& path );
	static bool					searchPathsForFile( BW::string& file );
	static bool					searchForFile( const BW::StringRef& directory,
		BW::string& file );
	static BW::StringRef		getFilename( const BW::StringRef& file );
	static BW::string			getFilePath( const BW::StringRef& file );

	template< typename T >
	static bool				getExtensionT( const T * file, T * retExtension, size_t retSize );
	static BW::StringRef	getExtension( const BW::StringRef& file );

	template< typename T >
	static bool		removeExtensionT( T* file );
	static BW::StringRef	removeExtension( const BW::StringRef& file );

	template< typename T >
	static bool		changeExtensionT( T * file, const T* newExtension, bool includingDot = true );
	static BW::string		changeExtension( const BW::StringRef& file,
		const BW::StringRef& newExtension );
	static       BW::string 	formatPath( const BW::StringRef& path );
	static const BW::string	getDefaultPath( void );
	static const BW::string	getPath( int index );
	static int					getPathNum();
	static BW::string			getPathAsCommandLine();
	static void					addPath( const BW::StringRef& path, int atIndex=-1 );
	static void					addPaths( const BW::StringRef& path, const char* desc, const BW::StringRef& pathXml = "" );
	static void					addSubPaths( const BW::StringRef& subPath );
	void						delPath( int index );
	static BW::string			getCurrentDirectory();
	static bool					setCurrentDirectory( const BW::StringRef &currentDirectory );
	static bool					ensurePathExists( const BW::StringRef& path );
	static bool					ensureAbsolutePathExists( const BW::StringRef& path );
	static bool					isValid( );
	static bool					isFileOlder( const BW::StringRef & oldFile,
		const BW::StringRef & newFile, int mustBeOlderThanSecs = 0 );
	static uint64				modifiedTime( const BW::StringRef & fileOrResource );
	static IFileSystem::FileType resolveToAbsolutePath( BW::string& path );
	template< typename T >
	static bool					resolveToRelativePathT( T * path, size_t pathSize );
	static BW::string			fixCommonRootPath( const BW::StringRef& rootPath );
	static IFileSystem::FileType resolveToAbsolutePath( BW::wstring& path );
	static DataSectionPtr		openSectionInTree( const BW::StringRef& filePath, const BW::StringRef& sectionToOpen);

	static const BW::string	dissolveFilename( const BW::StringRef& file );

#ifdef _WIN32
	static const BW::string	appDirectory();
	static const BW::string	appPath();
	static const BW::string	appDataDirectory( bool roaming = true );
	static const BW::string	userDocumentsDirectory();
	static const BW::string	userPicturesDirectory();
	static const BW::string	appCompanyName();
	static const BW::string	appProductName();
	static const BW::string	appProductVersion();
#endif

#ifdef EDITOR_ENABLED
	// file system specific methods, hidden from the BWClient because they can
	// break working with Zip file-systems for instance.
	static const BW::string	findFile( const BW::StringRef& file );
	static const BW::string	removeDrive( const BW::StringRef& file );
    static void                 defaultDrive( const BW::StringRef& drive );
	static const BW::string	resolveFilename( const BW::StringRef& file );
	static bool			        validPath( const BW::StringRef& file );

//-----------------------------------------------------------------------------
	// wide versions, editor-only
	static const BW::wstring	getFilenameW( const BW::string& file );
	static const BW::wstring	getFilenameW( const BW::wstring& file );

	static const BW::wstring	getExtensionW( const BW::string& file );
	static const BW::wstring	getExtensionW( const BW::wstring& file );

	static const BW::wstring	removeExtensionW( const BW::string& file );
	static const BW::wstring	removeExtensionW( const BW::wstring& file );

	static bool			        validPathW( const BW::wstring& file );
	static void                 defaultDriveW( const BW::wstring& drive );

	static const BW::wstring	findFileW( const BW::string& file );
	static const BW::wstring	removeDriveW( const BW::string& file );
	static const BW::wstring	dissolveFilenameW( const BW::string& file );
	static const BW::wstring	resolveFilenameW( const BW::string& file );

	static const BW::wstring	findFileW( const BW::wstring& file );
	static const BW::wstring	removeDriveW( const BW::wstring& file );
	static const BW::wstring	dissolveFilenameW( const BW::wstring& file );
	static const BW::wstring	resolveFilenameW( const BW::wstring& file );

#endif // EDITOR_ENABLED

#if ENABLE_FILE_CASE_CHECKING
	// Methods to enable case sensitive checking.
	static void					checkCaseOfPaths(bool enable);
	static bool					checkCaseOfPaths();
#endif // ENABLE_FILE_CASE_CHECKING
	static BW::string			correctCaseOfPath(BW::StringRef const &path);

	static void overrideAppDirectory(const BW::StringRef &dir);

public:
	/**
	 *	A utility class for temporarily changing the file access
	 *	watcher state for this thread, whilst automatically restoring
	 *	the old state when the class instance goes out of scope.
	 */
	class WatchAccessFromCallingThreadHolder
	{
	public:
		WatchAccessFromCallingThreadHolder( bool newState )
		{
			oldState_ = BWResource::watchAccessFromCallingThread();
			BWResource::watchAccessFromCallingThread( newState );
		}

		~WatchAccessFromCallingThreadHolder()
		{
			BWResource::watchAccessFromCallingThread( oldState_ );
		}

	private:
		bool oldState_;
	};


private:
	static BWResource & makeInstance();

	friend std::ostream& operator<<(std::ostream&, const BWResource&);
	BWResourceImpl* pimpl_;
// to let BWResourceImpl use private methods
	friend class BWResourceImpl;
	friend class BWResolver;

	template< typename T >
	static int	findExtensionPos( const T * path );
	template< typename T >
	static bool			hasDriveInPathT( const T* path );
	template< typename T >
	static bool			pathIsRelativeT( const T* path );

};



/**
 *  -- BWResolver - AVOID USING THIS CLASS DIRECTLY --
 *  Helper class, implements the resolve/dissolve functionality, previously
 *  provided by the FilenameResolver.
 *  -- IMPORTANT NOTES --
 *  - New code should stay away from this class unless absolutely
 *  necesary, since it breaks relative path names and file system
 *  abstraction.
 *  - If needed, as for example in the tools, the preferred way to use this
 *  functionality is through the BWResource class, so it makes sure it is only
 *  in the tools with the EDITOR_ENABLED flag.
 */
class BWResolver
{
public:
	static BW::string	findFile( const BW::StringRef& file );
	static BW::string	removeDrive( const BW::StringRef& file );
    static void                 defaultDrive( const BW::StringRef& drive );
	static BW::string	dissolveFilename( const BW::StringRef& file );
	static BW::string	resolveFilename( const BW::StringRef& file );
};


/**
 * Checks for the existants of a drive letter
 *
 * @returns true if a drive is supplied in the path
 */
template< typename T >
bool BWResource::hasDriveInPathT( const T* path )
{
	BW_GUARD;
	size_t pos;
	return 	bw_str_find_first_of( path, T( ':' ), pos ) ||					// has a drive colon
    	( path[0] == T('\\') || path[0] == T('/') );	// starts with a root char
}


/**
 * Check is the path is relative and not absolute.
 *
 * @returns true if the path is relative
 */
template< typename T >
bool BWResource::pathIsRelativeT( const T* path )
{
	BW_GUARD;
	// has no drive letter, and no root directory
	return !hasDriveInPathT( path ) && ( path[0] != T('/') && path[0] != T('\\') );
}


/**
 *	This method tris to find the extension's "."
 *
 *	@param file		filename string
 *	@return 		the position of ".", or -1 if not found.
 */
template< typename T >
/*static*/ int BWResource::findExtensionPos( const T * file )
{
	BW_GUARD;
	size_t dotPos;	
	if (bw_str_find_last_of( file, T( '.' ) ,dotPos ))
	{
		size_t pos, fileNamePos = bw_str_len( file );
		if (bw_str_find_last_of( file, T( '/' ), pos ))
		{
			fileNamePos = pos;
		}
		if (bw_str_find_last_of( file, T( '\\' ), pos ) && 
			pos < fileNamePos)
		{
			fileNamePos = pos;
		}
		if (dotPos > fileNamePos)
		{
			return static_cast<int>(dotPos);
		}
	}
	return -1;
}


/**
 *	Change the extension of the filename to be newExtension
 *
 *	@param file				filename string
 *	@param newExtension		new extension.
 */
template< typename T >
/*static*/ bool BWResource::changeExtensionT( T * file, 
						const T* newExtension, bool includingDot /*= true*/ )
{
	BW_GUARD;
	size_t extPos = BWResource::findExtensionPos( file );
	if (extPos == -1)
	{
		// no extension
		extPos = bw_str_len( file );
	}
	if (!includingDot)
	{
		file[extPos++] = T('.');
	}
	size_t lenExt = bw_str_len( newExtension );
	memcpy( &file[ extPos ], newExtension, sizeof(T) * lenExt );
	file[ extPos + lenExt ] = 0;
	return true;
}


/**
*	Retrieves the extension of a filename.
*
*	@param file				filename string
*	@param retExtention		return extension string
*/
template< typename T >
/*static*/ bool BWResource::getExtensionT( const T * file, T * retExtension,
	size_t retSize )
{
	BW_GUARD;
	int dotPos = BWResource::findExtensionPos( file );
	if (dotPos != -1)
	{
		bw_str_substr( retExtension, retSize, file, dotPos + 1, bw_str_len( file ) - dotPos - 1 );
		return true;
	}
	// no extension
	return false;
}


/**
 *	Resolves the given path to an relative path.
 *	Note: the returned path will be sanitised ('\\' -> '/')
 * 
 * 	@param	path	Input: The path to resolve, null-terminated.
 * 					Output: The resolved path.
 *					it's assumed that path has been allocated
 *					with enough memory.
 * 
 * 	@return	success or not
 */
template< typename T >
bool BWResource::resolveToRelativePathT( T * path, size_t pathSize )
{
	BW_GUARD;
	if (BWResource::pathIsRelativeT( path ))
	{
		return false;
	}

	T bwPath[256];
	T formatedPath[256];
	bw_str_copy( formatedPath, 256, path );
	BWUtil::sanitisePathT( formatedPath );
	for (size_t i = 0; i < static_cast<size_t>(BWResource::getPathNum()); ++i)
	{
		BWResource::instance().path( i, bwPath, 256 );
		size_t pos;
		if (bw_str_find_first_of( formatedPath, bwPath, pos ))
		{
			BW::string::size_type index = bw_str_len( bwPath );
			if (formatedPath[index] == T('/'))
			{
				bw_str_substr( path, pathSize, formatedPath,
					index + 1, bw_str_len( formatedPath ) - index - 1 );
				return true;
			}
		}
	}
	return false;
}

/**
*	Removes the extension from the filename,and
*	modify directly the passed char* file.
*	@param file		NULL-terminated filename string
*/
template< typename T >
/*static*/ bool BWResource::removeExtensionT( T* file )
{
	BW_GUARD;
	int dotPos = BWResource::findExtensionPos( file );
	if (dotPos != -1)
	{
		file[ dotPos ] = 0;
		return true;
	}
	return false;
}

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "bwresource.ipp"
#endif

#endif // BWRESOURCE_HPP
