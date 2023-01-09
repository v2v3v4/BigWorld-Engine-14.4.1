#include "pch.hpp"

#include "cstdmf/bwversion.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"

#include "bwresource.hpp"
#include "xml_section.hpp"
#include "data_section_cache.hpp"
#include "data_section_census.hpp"
#include "dir_section.hpp"
#include "zip_file_system.hpp"
#include "multi_file_system.hpp"

#ifndef CODE_INLINE
#include "bwresource.ipp"
#endif

#ifdef _WIN32
#include <direct.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <iomanip>
#include <strstream>
#else
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif // _WIN32

#include <errno.h>
#include <memory>

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

#ifdef USE_MEMORY_TRACER
#define DEFAULT_CACHE_SIZE (0)
#else
#define DEFAULT_CACHE_SIZE (100 * 1024)
#endif


#define PATHS_FILE "paths.xml"
#define SOURCE_FILE "source.xml"
#define PACKED_EXTENSION "zip"
#define FOLDER_SEPERATOR '/'


#define RES_PATH_HELP												\
	"For more information on how to correctly setup and run "		\
	"BigWorld client, please refer to the Client Installation "		\
	"Guide, in bigworld/doc directory."


/// resmgr library Singleton

BW_SINGLETON_STORAGE( BWResource )

#ifdef BW_EXPORTER
char* g_exporterErrorMsg;
#endif

// File static values
namespace
{
// Stores whether BWResource has been initialised.
bool g_inited = false;

// Ideally, this pointer should not be used. It is a fallback if the singleton
// instance of BWResource was not created at the application level.
std::auto_ptr< BWResource > s_pBWResourceInstance;
}

typedef BW::vector<BW::string> STRINGVECTOR;


/**
 *  This local class contains the private implementation details for the 
 *  BWResource class.
 */
class BWResourceImpl
{
public:
	BWResourceImpl() :
		rootSection_( NULL ),
		fileSystem_( NULL ),
		nativeFileSystem_( NULL ),
		paths_(),
		defaultDrive_()
#if ENABLE_FILE_CASE_CHECKING		
		,checkCase_( false )
#endif // ENABLE_FILE_CASE_CHECKING
#ifdef _WIN32
		,appVersionData_( NULL )
#endif // _WIN32
	{
		BW_GUARD;
		DataSectionCensus::init();
		nativeFileSystem_ = NativeFileSystem::create( "" );
	};


	~BWResourceImpl()
	{
		BW_GUARD;
		cleanUp( true );
		nativeFileSystem_ = NULL;
#ifdef _WIN32
		if ( appVersionData_ )
		{
			bw_free( appVersionData_ );
			appVersionData_ = NULL;
		}
#endif // _WIN32
	};

	void cleanUp( bool final );

	// private interface taken from the now deprecated FilenameResolver
	bool loadPathsXML( const BW::StringRef& customAppFolder = "" );

	static bool isValidZipSize( uint64 size, const BW::StringRef & path );
	static bool isValidZipFile( const BW::StringRef & path );

	void						postAddPaths();

	static bool					hasDriveInPath( const BW::StringRef& path );
	static bool					pathIsRelative( const BW::StringRef& path );

	static BW::string			formatSearchPath( const BW::StringRef& path, 
		const BW::StringRef& pathXml = "" );

#ifdef _WIN32
	bool loadPathsXMLInternal( const BW::StringRef& directory );

	static BW::string	getAppDirectory();

	// Windows Known Folders lookup methods
	static const BW::string	getAppDataDirectory( bool roaming );
	static const BW::string	getUserDocumentsDirectory();
	static const BW::string	getUserPicturesDirectory();

	// Version info (VS_VERSION_INFO) access methods
	const BW::string			getAppVersionString( 
		const BW::string & stringName );
	void						getAppVersionData();
	const BW::string			getAppVersionLanguage();
#endif // _WIN32

	void enableModificationMonitor( bool enable );
	void addModificationListener( ResourceModificationListener* listener );
	void removeModificationListener( ResourceModificationListener* listener );
	void flushModificationMonitor();
	bool ignoreFileModification( const BW::string& fileName, 
			ResourceModificationListener::Action action,
			bool oneTimeOnly );
	bool hasPendingModification( const BW::string& fileName );

	// Member data
	DataSectionPtr				rootSection_;
	MultiFileSystemPtr			fileSystem_;
	FileSystemPtr				nativeFileSystem_;

	STRINGVECTOR				paths_;

    BW::string                 defaultDrive_;
#if ENABLE_FILE_CASE_CHECKING
	bool						checkCase_;
#endif // ENABLE_FILE_CASE_CHECKING

#ifdef _WIN32
	static BW::string          appDirectoryOverride_;
	void*						appVersionData_;
#endif // _WIN32

	SimpleMutex modificationListenersMutex_;
	ModificationListeners modificationListeners_;
};


// function to compare case-insensitive in windows, case sensitive in unix
namespace
{
	int mf_pathcmp( const char* a, const char* b )
	{
#if defined( __unix__ ) || defined( __APPLE__ )
		return strcmp( a, b );
#else
		return bw_stricmp( a, b );
#endif // defined( __unix__ ) || defined( __APPLE__ )
	}
};


void BWResourceImpl::cleanUp( bool final )
{
	BW_GUARD;
	rootSection_ = NULL;

	// Be careful here. If users hang on to DataSectionPtrs after
	// destruction of the cache, this could cause a problem at
	// shutdown time. Could make the cache a reference counted
	// object and make the DataSections use smart pointers to it,
	// if this is actually an issue.

	fileSystem_ = NULL;

	if (final)
	{
		DataSectionCache::fini();
		DataSectionCensus::fini();
	}
}


/**
 *	This static method finds out if a size value is bigger than the maximum Zip
 *	file size supported.
 *
 *	@param size		Size value to compare with the maximum supported size.
 *	@param path		Path of the file, used only for error reporting.
 *	@return	True if size is less or equal to the maximum supported size, false
 *			otherwise.
 */
/*static*/
bool BWResourceImpl::isValidZipSize( uint64 size, const BW::StringRef & path )
{
	BW_GUARD;
	if (size > ZipFileSystem::MAX_ZIP_FILE_KBYTES * 1024)
	{
#ifdef MF_SERVER
		ERROR_MSG( "BWResource::addPaths: "
				"Resource path is not a valid '%s' file: %s"
				"\n\nOnly Zip files of %" PRIu64 "GB or less are supported.",
				PACKED_EXTENSION,
				path.to_string().c_str(),
				ZipFileSystem::MAX_ZIP_FILE_KBYTES/(1024 * 1024) );
#else
		CRITICAL_MSG( "Search path is not a valid '%s' file: %s. "
				"Only files of %" PRIu64 "GB or less are supported. "
				RES_PATH_HELP,
				PACKED_EXTENSION,
				path.to_string().c_str(),
				ZipFileSystem::MAX_ZIP_FILE_KBYTES/(1024 * 1024) );
#endif

		return false;
	}
	return true;
}


/**
 *	This static method checks that if a specified file is a zip file.
 *
 *	@param path		Path of the file.
 *	@return	True if the file is a zip file, false
 *			otherwise.
 */
/*static*/
bool BWResourceImpl::isValidZipFile( const BW::StringRef & path )
{
	FileSystemPtr tempFS = NativeFileSystem::create( "" );

	IFileSystem::FileInfo fileInfo;
	tempFS->getFileType( path, &fileInfo );

	if (!isValidZipSize( fileInfo.size, path ))
	{
		return false;
	}

	FILE* f = tempFS->posixFileOpen( path, "rb" );
	if (f)
	{
		uint32 magic;
		size_t frr = 1;
		frr = fread( &magic, sizeof( uint32 ), 1, f );
		fclose( f );
		if (!frr)
			return false; //return no file instead?

		if (magic == ZIP_HEADER_MAGIC)
			return true;

	}

	return false;
}


void BWResourceImpl::postAddPaths()
{
	BW_GUARD;
	if (!g_inited)
	{
		CRITICAL_MSG( "BWResourceImpl::postAddPaths: "
				"BWResource::init should have been called already.\n" );
		return;
	}

	if ( fileSystem_ )
	{
		// already called, so clean up first.
		cleanUp( false );
	}

	fileSystem_ = new MultiFileSystem();

	int pathIndex = 0;
	BW::string path;

	path = BWResource::getPath( pathIndex++ );

	FileSystemPtr tempFS = NativeFileSystem::create( "" );

	// add an appropriate file system for each path
	while (!path.empty())
	{
		// check to see if the path is actually a Zip file
		BW::string pakPath = path;
		FileSystemPtr baseFileSystem;

		// also check to see if the path contains a "res.zip" Zip file
		BW::string pakPath2 = path;
		if (*pakPath2.rbegin() != '/') pakPath2.append( "/" );
		pakPath2 += "res.";
		pakPath2 += PACKED_EXTENSION;

		IFileSystem::FileInfo fileInfo;
		IFileSystem::FileType paType = tempFS->getFileType( pakPath, &fileInfo );

		if ( BWResource::getExtension( pakPath ) == PACKED_EXTENSION &&
			(paType == IFileSystem::FT_ARCHIVE || paType == IFileSystem::FT_FILE) )
		{
			// it's a Zip file system, so add it
			INFO_MSG( "BWResource::BWResource: Path is package %s\n",
					pakPath.c_str() );
			if (isValidZipFile( pakPath ))
			{
				baseFileSystem = new ZipFileSystem( pakPath );
			}
			else
			{
				CRITICAL_MSG( "BWResourceImpl::postAddPaths: "
					"'%s' is not a valid zip file, please check if it is corrupted.\n",
					path.c_str() );
			}
		}
		else if ( tempFS->getFileType( pakPath2, &fileInfo ) == IFileSystem::FT_FILE )
		{
			// it's a Zip file system, so add it
			INFO_MSG( "BWResource::BWResource: Found package %s\n",
					pakPath2.c_str() );
			if (isValidZipFile( pakPath2 ))
			{
				baseFileSystem = new ZipFileSystem( pakPath2 );
			}
			else
			{
				CRITICAL_MSG( "BWResourceImpl::postAddPaths: "
					"'%s' is not a valid zip file, please check if it is corrupted.\n",
					pakPath2.c_str() );
			}
		}
		else if ( tempFS->getFileType( path, NULL ) == IFileSystem::FT_DIRECTORY )
		{
			if (*path.rbegin() != '/') path.append( "/" );

			// it's a standard path, so add a native file system for it
			baseFileSystem = NativeFileSystem::create( path );
		}
		else
		{
			CRITICAL_MSG( "BWResourceImpl::postAddPaths: "
				"Invalid resource path '%s'\n", path.c_str() );
		}

		MF_ASSERT( baseFileSystem );
		fileSystem_->addBaseFileSystem( baseFileSystem );

		path = BWResource::getPath( pathIndex++ );
	}

	tempFS = NULL;

#ifdef EDITOR_ENABLED
	// add a default native file system, to handle absolute paths in the tools
	fileSystem_->addBaseFileSystem( NativeFileSystem::create( "" ) );
#endif

	char * cacheSizeStr = ::getenv( "BW_CACHE_SIZE" );
	int cacheSize = cacheSizeStr ? atoi(cacheSizeStr) : DEFAULT_CACHE_SIZE;

	DataSectionCache::instance()->setSize( cacheSize );
	DataSectionCache::instance()->clear();

	//new DataSectionCache( cacheSize );
	rootSection_ = new DirSection( "", fileSystem_.getObject() );

#if ENABLE_FILE_CASE_CHECKING
	if (checkCase_)
		fileSystem_->checkCaseOfPaths(checkCase_);
#endif // ENABLE_FILE_CASE_CHECKING
}


void BWResourceImpl::enableModificationMonitor( bool enable )
{
	BW_GUARD;
	fileSystem_->enableModificationMonitor( enable );
}


void BWResourceImpl::addModificationListener(
	ResourceModificationListener* listener )
{
	BW_GUARD;
	SimpleMutexHolder smh( modificationListenersMutex_ );
	if (std::find( modificationListeners_.begin(), 
			modificationListeners_.end(), 
			listener ) == modificationListeners_.end())
	{
		modificationListeners_.push_back( listener );
	}
}


void BWResourceImpl::removeModificationListener(
	ResourceModificationListener* listener )
{
	BW_GUARD;
	SimpleMutexHolder smh( modificationListenersMutex_ );
	ModificationListeners::iterator it =
		std::find( modificationListeners_.begin(),
				modificationListeners_.end(),
				listener );

	while (it != modificationListeners_.end())
	{
		modificationListeners_.erase( it );
		it = std::find( modificationListeners_.begin(),
				modificationListeners_.end(),
				listener );
	}

	// The resource modification implementation currently relies upon some 
	// win32 code. Since server does not support any reloading anyway, 
	// disable the calls into the implementation.
#if defined( _WIN32 )
	if (listener)
	{
		listener->cancelReloadTasks();
	}
#endif // defined( _WIN32 )
}


void BWResourceImpl::flushModificationMonitor()
{
	BW_GUARD;
	SimpleMutexHolder smh( modificationListenersMutex_ );
	fileSystem_->flushModificationMonitor(
		modificationListeners_ );
}


bool BWResourceImpl::ignoreFileModification( const BW::string& fileName, 
							ResourceModificationListener::Action action,
							bool oneTimeOnly )
{
	BW_GUARD;
	return fileSystem_->ignoreFileModification( fileName, action, oneTimeOnly );
}


bool BWResourceImpl::hasPendingModification( const BW::string& fileName )
{
	BW_GUARD;
	return fileSystem_->hasPendingModification( fileName );
}


// -----------------------------------------------------------------------------
// Section: BWResource
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
BWResource::BWResource(): 
		Singleton< BWResource >()
{
	BW_GUARD;
	pimpl_ = new BWResourceImpl();
}

/**
 *	Destructor
 */
BWResource::~BWResource()
{
	BW_GUARD;
	bw_safe_delete(pimpl_);
}

void BWResource::refreshPaths()
{
	BW_GUARD;
	pimpl_->postAddPaths();
}

DataSectionPtr BWResource::rootSection()
{
	BW_GUARD;
	return pimpl_->rootSection_;
}

MultiFileSystem* BWResource::fileSystem()
{
	BW_GUARD;
	return pimpl_->fileSystem_.getObject();
}

IFileSystem* BWResource::nativeFileSystem()
{
	BW_GUARD;
	return pimpl_->nativeFileSystem_.getObject();
}

/**
 *	This method purges an item from the cache, given its relative path from
 *	the root section.
 *
 *	@param path			The path to the item to purge.
 *	@param recursive	Set this to true to perform a full recursive purge.
 *	@param spSection	If not NULL, this should be a pointer to the data
 *						section at 'path'. This is used to improve efficiency
 *						and avoid a problem with duplicate child names.
 */
void BWResource::purge( const BW::StringRef & path, bool recursive,
		DataSectionPtr spSection )
{
	BW_GUARD;
	if (recursive)
	{
		if (!spSection)
			spSection = pimpl_->rootSection_->openSection( path );

		if (spSection)
		{
			DataSectionIterator it = spSection->begin();
			DataSectionIterator end = spSection->end();

			while (it != end)
			{
				this->purge( path + "/" + (*it)->sectionName(), true, *it );
				++it;
			}
		}
	}

	// remove it from the cache
	DataSectionCache::instance()->remove( path.to_string() );

	// remove it from the census too
	DataSectionPtr pSection = DataSectionCensus::find( path );
	if (pSection)
	{
		 DataSectionCensus::del( pSection.get() );
	}
}

/**
 *	This method purges everything from the cache and census.
 */
void BWResource::purgeAll()
{
	BW_GUARD;
	DataSectionCache::instance()->clear();
	DataSectionCensus::clear();
}


/**
 *  This method saves the section with the given pathname.
 *
 *  @param resourceID     The relative path of the section.
 */
bool BWResource::save( const BW::string & resourceID )
{
	BW_GUARD;
	DataSectionPtr ptr = DataSectionCensus::find( resourceID );

	if (!ptr)
	{
		WARNING_MSG( "BWResource::saveSection: %s not in census\n",
			resourceID.c_str() );
		return false;
	}

	return ptr->save();
}


/**
 *	Standard open section method.
 *	Always looks in the census first.
 */
DataSectionPtr BWResource::openSection( const BW::StringRef & resourceID,
										bool makeNewSection,
										DataSectionCreator* creator )
{
	BW_GUARD;
	PROFILER_SCOPED( BWResource_openSection );

	DataSectionPtr pExisting = DataSectionCensus::find( resourceID );

	if (pExisting) return pExisting;

	return instance().rootSection()->openSection( resourceID, 
										makeNewSection, creator );
}


// Needs to be constucted like this for broken Linux thread local storage.
THREADLOCAL( bool ) g_doneLoading( false );

/**
 *	Causes the file system to issue warnings if
 *	files are loaded from the calling thread.
 *
 *	@param	watch	true if calling thread should be watched. False otherwise.
 */
void BWResource::watchAccessFromCallingThread(bool watch)
{
	BW_GUARD;
	g_doneLoading = watch;
}

/**
 *	Determines whether or not we are issuing warnings about loading
 *	from the calling thread.
 *
 *	@return True if we are watching access from the calling thread.
 */
bool BWResource::watchAccessFromCallingThread()
{
	BW_GUARD;
	return g_doneLoading;
}

/**
 *	Displays the proper warnings if
 *	files are loaded from the calling thread.
 */
void BWResource::checkAccessFromCallingThread(
	const BW::StringRef& path, const char* msg )
{
	BW_GUARD;

	if ( g_doneLoading )
	{
		WARNING_MSG( "%s: Accessing %s from non-loading thread.\n",
			msg, path.to_string().c_str() );
	}
}

//
//
//	Static methods (taken from the now deprecated BWResource)
//
//

static BW::string appDrive_;

#ifdef __unix__
// difference in .1 microsecond steps between FILETIME and time_t
const uint64 unixTimeBaseOffset = 116444736000000000ull;
const uint64 unixTimeBaseMultiplier = 10000000;

inline uint64 unixTimeToFileTime( time_t t )
{
	return uint64(t) * unixTimeBaseMultiplier + unixTimeBaseOffset;
}
#endif

#if !defined( _WIN32 )
// these are already defined in Win32
int __argc = 0;
char ** __argv = NULL;
#endif

#ifdef _WIN32

void BWResource::overrideAppDirectory(const BW::StringRef &dir)
{
	BW_GUARD;
	BWResourceImpl::appDirectoryOverride_.assign( dir.data(), dir.size() );
}


/*static*/ BW::string BWResourceImpl::getAppDirectory()
{
	BW_GUARD;
	BW::string appDirectory( "." );

	if (!BWResourceImpl::appDirectoryOverride_.empty())
	{
		appDirectory = BWResourceImpl::appDirectoryOverride_;
		BWUtil::sanitisePath( appDirectory );
	}
	else
	{
		#if BWCLIENT_AS_PYTHON_MODULE
			appDirectory = BWResource::getCurrentDirectory();
		#else
			appDirectory = BWUtil::executableDirectory();
		#endif

		#ifdef EDITOR_ENABLED
			appDrive_ = appDirectory[0];
			appDrive_ += ":";
		#endif
	}

	MF_ASSERT( !appDirectory.empty() );

	if (appDirectory[appDirectory.length() - 1] != '/')
	{
		appDirectory += "/";
	}

	return appDirectory;
}

namespace {

HRESULT getWindowsPath( int cisdl, BW::string& out )
{
	BW_GUARD;
	wchar_t wappDataPath[MAX_PATH];
	HRESULT result= SHGetFolderPath( NULL, cisdl, NULL,
								SHGFP_TYPE_CURRENT, wappDataPath );
	if( SUCCEEDED( result ) )
	{	
		bw_wtoutf8( wappDataPath, out );
		std::replace( out.begin(), out.end(), '\\', '/' );
		out += "/";
		return result;
	}
	return result;
}

} // anonymous namespace

/*static*/ const BW::string BWResourceImpl::getAppDataDirectory( bool roaming )
{
	BW_GUARD;
	int flags = CSIDL_FLAG_CREATE;
	if (roaming)
	{
		flags |= CSIDL_APPDATA;
	}
	else
	{
		flags |= CSIDL_LOCAL_APPDATA;
	}

	BW::string ret;
	HRESULT result = getWindowsPath( flags, ret );
	if (FAILED(result))
	{	
		WARNING_MSG( "BWResourceImpl::getAppDataDirectory: Couldn't locate "
			"application data directory: 0x%08lx\n", result );
	}
	return ret;
}


/*static*/ const BW::string BWResourceImpl::getUserDocumentsDirectory()
{
	BW_GUARD;
	BW::string ret;
	HRESULT result = getWindowsPath( CSIDL_MYDOCUMENTS|CSIDL_FLAG_CREATE, ret );
	if (FAILED(result))
	{	
		WARNING_MSG( "BWResourceImpl::userDocumentsDirectory: Couldn't locate "
			"My Documents: 0x%08lx\n", result );
	}
	return ret;
}

/*static*/ const BW::string BWResourceImpl::getUserPicturesDirectory()
{
	BW_GUARD;
	BW::string ret;
	HRESULT result = getWindowsPath( CSIDL_MYPICTURES|CSIDL_FLAG_CREATE, ret );
	if (FAILED(result))
	{	
		WARNING_MSG( "BWResourceImpl::getUserPicturesDirectory: Couldn't locate "
			"My Pictures: 0x%08lx\n", result );
	}
	return ret;
}


const BW::string BWResourceImpl::getAppVersionString(
	const BW::string & stringName )
{
	BW_GUARD;
	getAppVersionData();
	if ( !appVersionData_ )
	{
		return "";
	}
	const BW::string language = getAppVersionLanguage();
	if ( language == "" )
	{
		return "";
	}
	BW::stringstream lookupPath;
	lookupPath << "\\StringFileInfo\\" << language << "\\" << stringName;

	BW::wstring wlookupPath;
	bw_utf8tow( lookupPath.str(), wlookupPath );

	unsigned int resultLen;
	char* resultPtr;
	if ( VerQueryValue( appVersionData_, const_cast< wchar_t * >( wlookupPath.c_str() ),
		reinterpret_cast< void ** >( &resultPtr ), &resultLen ) == 0 )
	{
		// Bad resource, above name is bad, or no info for this entry.
		WARNING_MSG( "BWResourceImpl::getAppVersionString: "
			"VerQueryValue failed looking up '%s'\n", lookupPath.str().c_str() );
		return "";
	}
	BW::string result( resultPtr, resultLen );
	return result;
}


const BW::string BWResourceImpl::getAppVersionLanguage()
{
	BW_GUARD;
	getAppVersionData();
	if ( ! appVersionData_ )
	{
		return "";
	}

	// This comes from the MSDN page on VerQueryValue's sample code
	struct {
		uint16 lang;
		uint16 codepage;
	} *pTranslations;

	unsigned int translationsLen;

	if ( VerQueryValue( appVersionData_, L"\\VarFileInfo\\Translation",
		reinterpret_cast< void ** >( &pTranslations ), &translationsLen ) == 0 )
	{
		// Bad resource, above name is bad, or no info for this entry.
		WARNING_MSG( "BWResourceImpl::getAppVersionLanguage: "
			"VerQueryValue failed\n" );
		return "";
	}

	if ( translationsLen == 0 )
	{
		return "";
	}

	unsigned int overrun = translationsLen % sizeof( *pTranslations );

	IF_NOT_MF_ASSERT_DEV( overrun == 0 )
	{
		translationsLen -= overrun;
	}

	BW::stringstream result;
	result.setf( std::ios::hex, std::ios::basefield );
	result.setf( std::ios::right, std::ios::adjustfield );
	result.fill( '0' );

	uint16 currLang = LANGIDFROMLCID( GetThreadLocale() );
	for ( unsigned int i=0; i < ( translationsLen/sizeof( *pTranslations ) );
		i++ )
	{
		if ( pTranslations[ i ].lang == currLang )
		{
			result << std::setw( 4 ) << pTranslations[ i ].lang;
			result << std::setw( 4 ) << pTranslations[ i ].codepage;
			break;
		}
	}
	if ( result.str().empty() )
	{
		// Fallback to first language listed.
		result << std::setw( 4 ) << pTranslations[ 0 ].lang;
		result << std::setw( 4 ) << pTranslations[ 0 ].codepage;
	}

	// TODO: Does this need to be cached somewhere?
	return result.str();

}

void BWResourceImpl::getAppVersionData()
{
	BW_GUARD;
	if ( appVersionData_ )
	{
		return;
	}

	wchar_t wfileNameBuffer[MAX_PATH];
	int result =
		GetModuleFileName( NULL, wfileNameBuffer, ARRAY_SIZE( wfileNameBuffer ) );
	if ( result == 0 )
	{
		WARNING_MSG( "BWResourceImpl::getAppVersionData: GetModuleFileName "
			"failed: 0x%08lx\n", GetLastError() );
		return;
	}
	if ( result == MAX_PATH )
	{
		// MAX_PATH means truncation which should never happen...
		WARNING_MSG( "BWResourceImpl::getAppVersionData: GetModuleFileName "
			"unexpectedly wanted to return more than %u bytes.\n", MAX_PATH );
		return;
	}

	int versionSize;
	DWORD scratchValue;
	versionSize = GetFileVersionInfoSize( wfileNameBuffer, &scratchValue );
	if ( versionSize == 0 )
	{
		WARNING_MSG( "BWResourceImpl::getAppVersionData: "
			"GetFileVersionInfoSize failed: 0x%08lx\n", GetLastError() );
		return;
	}

	// Using malloc/free because this is an opaque buffer, not a character array
	// or other usefully newable data.
	appVersionData_ = bw_malloc( versionSize );
	if ( ! appVersionData_ )
	{
		ERROR_MSG( "BWResourceImpl::getAppVersionData: malloc failed!\n" );
		return;
	}
	// From here on, any failure path must free and clear appVersionData_
	if ( ! GetFileVersionInfo( wfileNameBuffer, NULL, versionSize,
		appVersionData_ ) )
	{
		WARNING_MSG( "BWResourceImpl::getAppVersionData: "
			"GetFileVersionInfo failed: 0x%08lx\n", GetLastError() );
		bw_free( appVersionData_ );
		appVersionData_ = NULL;
		return;
	}
}
#endif // _WIN32


/**
 *	Load the paths.xml file, or avoid loading it if possible.
 */
bool BWResourceImpl::loadPathsXML( const BW::StringRef& customAppFolder )
{
	BW_GUARD;
	if (paths_.empty())
	{
		char * basePath;

		// The use of the BW_RES_PATH environment variable is deprecated
		if ((basePath = ::getenv( "BW_RES_PATH" )) != NULL)
		{
			WARNING_MSG( "The use of the BW_RES_PATH environment variable is " \
					"deprecated post 1.8.1\n" );

			BWResource::addPaths( basePath, "from BW_RES_PATH env variable" );
		}
	}

	if (paths_.empty())
	{
#if defined( _WIN32 )
		bool loadedXML = false;

		// Try explicitly given app folder...
		if (!customAppFolder.empty())
		{
			loadedXML = loadPathsXMLInternal( customAppFolder );
		}

		// ...otherwise try CWD...
		if (!loadedXML)
		{
			loadedXML = loadPathsXMLInternal( BWResource::getCurrentDirectory() );
		}

		// ...otherwise try app directory.
		if (!loadedXML)
		{
			loadedXML = loadPathsXMLInternal( BWResource::appDirectory() );
		}
#elif !defined( __APPLE__ )
		// Try reading ~/.bwmachined.conf
		char * pHomeDir = NULL;

		struct passwd * pPWEnt = ::getpwuid( getUserId() );
		if (pPWEnt)
		{
			pHomeDir = pPWEnt->pw_dir;
		}

		if (pHomeDir)
		{
			char buffer[1024];
			snprintf( buffer, sizeof(buffer), "%s/.bwmachined.conf", pHomeDir );
			FILE * pFile = bw_fopen( buffer, "r" );

			if (pFile)
			{
				while (paths_.empty() &&
						fgets( buffer, sizeof( buffer ), pFile ) != NULL)
				{
					// Find first non-whitespace character in buffer
					char *start = buffer;
					while (*start && isspace( *start ))
						start++;

					if (*start != '#' && *start != '\0')
					{
						if (sscanf( start, "%*[^;];%s", start ) == 1)
						{
							BWResource::addPaths( start,
									"from ~/.bwmachined.conf" );
						}
						else
						{
							WARNING_MSG( "BWResource::BWResource: "
								"Invalid line in ~/.bwmachined.conf - \n"
								"\t'%s'\n",
								buffer );
						}
					}
				}
				fclose( pFile );
			}
			else
			{
				WARNING_MSG( "BWResource::BWResource: Unable to open %s\n",
						buffer );
			}
		}

		if (paths_.empty())
		{
			ERROR_MSG( "BWResource::BWResource: No res path set.\n"
				"\tIs ~/.bwmachined.conf correct or 'BW_RES_PATH' environment "
					"variable set?\n" );
		}
#endif
	}

#if _WIN32
#ifndef _RELEASE
	if (paths_.empty())
	{
#if defined( BW_EXPORTER )
	g_exporterErrorMsg = new char[1024];
	bw_snprintf( g_exporterErrorMsg, 1024,
		"BWResource::BWResource: "
		"No paths found in BW_RES_PATH environment variable "
		"and no paths.xml file found in the working directory. " );
#else
		CRITICAL_MSG(
			"BWResource::BWResource: "
			"No paths found in BW_RES_PATH environment variable "
			"and no paths.xml file found in the working directory. "
			RES_PATH_HELP );
#endif
	}
#endif // not _RELEASE
#endif // _WIN32

#ifdef MF_SERVER
#ifdef _WIN32
	COORD c = { 80, 2000 };
	SetConsoleScreenBufferSize( GetStdHandle( STD_OUTPUT_HANDLE ), c );
#endif
#endif

	this->postAddPaths();

	return !paths_.empty();
}

#ifdef _WIN32
bool BWResourceImpl::loadPathsXMLInternal( const BW::StringRef& directory )
{
	BW_GUARD;
	#ifdef LOGGING
	dprintf( "BWResource: app directory is %s\n",
		directory.c_str() );
	dprintf( "BWResource: using %spaths.xml\n",
		directory.c_str() );
	std::ofstream os( "filename.log", std::ios::trunc );
	os << "app dir: " << directory << "\n";
	#endif

	// load the search paths
	BW::string fullPathsFileName = directory + PATHS_FILE;
	if ( nativeFileSystem_->getFileType( fullPathsFileName ) != IFileSystem::FT_NOT_FOUND )
	{
		DEBUG_MSG( "Loading.. %s\n", fullPathsFileName.c_str() );
		BinaryPtr binBlock = nativeFileSystem_->readFile( fullPathsFileName );
		XMLSectionPtr spRoot =
			XMLSection::createFromBinary( "root", binBlock );
		if ( spRoot )
		{
			// load the paths
			BW::string path = "Paths";
			DataSectionPtr spSection = spRoot->openSection( path );

			STRINGVECTOR paths;
			if ( spSection )
				spSection->readStrings( "Path", paths );

			// Format the paths
			for (STRINGVECTOR::iterator i = paths.begin(); i != paths.end(); ++i)
			{
				BWResource::addPaths( *i, "from paths.xml", directory );
			}

			return true;
		}
		else
		{
			DEBUG_MSG( "Failed to open paths.xml" );
			return false;
		}
	}
	else
	{
		return false;
	}
}
#endif


/**
 *	This method initialises the BWResource from command line arguments.
 */
bool BWResource::init( int & argc, const char * argv[], bool removeArgs )
{
	BW_GUARD;
	// This should be called before the BWResource is constructed.
	MF_ASSERT( !g_inited );
	g_inited = true;

	BWResource & instance = BWResource::makeInstance();

	for (int i = 0; i < argc - 1; i++)
	{
		if (strcmp( argv[i], "--res" ) == 0 ||
			strcmp( argv[i], "-r" ) == 0)
		{
			addPaths( argv[i + 1], "from command line" );

			if (removeArgs)
			{
				argv[i] = NULL;
				argv[i + 1] = NULL;
			}
			++i;
			continue;
		}

		else if (strcmp( argv[i], "-UID" ) == 0 ||
			strcmp( argv[i], "-uid" ) == 0)
		{
			INFO_MSG( "UID set to %s (from command line).\n", argv[ i+1 ] );
#ifndef _WIN32
			::setenv( "UID", argv[ i+1 ], true );
#else
			BW::string addStr = "UID=";
			addStr += argv[ i+1 ];
			::_putenv( addStr.c_str() );
#endif
			if (removeArgs)
			{
				argv[i] = NULL;
				argv[i + 1] = NULL;
			}
			++i;
			continue;
		}
	}

	if (removeArgs)
	{
		BWUtil::compressArgs( argc, argv );
	}

	return instance.pimpl_->loadPathsXML();
}


/**
 * Initialise BWResource to use the given path directly
 */
bool BWResource::init( const BW::string& fullPath, bool addAsPath )
{
	BW_GUARD;
	// This should be called before the BWResource is constructed.
	MF_ASSERT( !g_inited );
	g_inited = true;

	BWResource & instance = BWResource::makeInstance();

	bool res = instance.pimpl_->loadPathsXML( fullPath );

	if (addAsPath)
	{
		BWResource::addPath( fullPath );
	}

	return res;
}


/**
 * Initialise BWResource with a given set of paths
 */
bool BWResource::init( const BW::vector< BW::string > & paths )
{
	BW_GUARD;
	// This should be called before the BWResource is constructed.
	MF_ASSERT( !g_inited );
	g_inited = true;

	BWResource & instance = BWResource::makeInstance();

	typedef BW::vector< BW::string >::const_iterator Iterator;
	for (Iterator iter = paths.begin(), end = paths.end(); iter != end; ++iter)
	{
		BWResource::addPath( *iter );
	}

	instance.pimpl_->postAddPaths();

	return true;
}


/**
 *	This method finalises the singleton instance.
 */
void BWResource::fini()
{
	BW_GUARD;
	s_pBWResourceInstance.reset();
	g_inited = false;
}


/**
 *	This method creates the singleton instance of BWResource.
 */
// TODO: This method should not be needed once all BWResource instances are made
// at the application level.
BWResource & BWResource::makeInstance()
{
	BW_GUARD;
	MF_ASSERT( s_pBWResourceInstance.get() == NULL );

	if (BWResource::pInstance())
	{
		return BWResource::instance();
	}
	else
	{
		DEBUG_MSG( "BWResource::makeInstance: "
			"BWResource was not created at the app level.\n"
			"Create a BWResource as a member of the application or main.\n" );
		s_pBWResourceInstance.reset( new BWResource );

		return *s_pBWResourceInstance;
	}
}


/**
 *	This method searches for a file in the current search path, recursing into
 *	sub-directories.
 *
 *	@param file the file to find
 *
 *	@return true if the file is found
 */
bool BWResource::searchPathsForFile( BW::string& file )
{
	BW_GUARD;
	BW::string filename = getFilename( file ).to_string();
	STRINGVECTOR::iterator it  = instance().pimpl_->paths_.begin( );
	STRINGVECTOR::iterator end = instance().pimpl_->paths_.end( );
	bool found = false;

	// search for the file in the supplied paths
	while( !found && it != end )
	{
		found = searchForFile( formatPath((*it)), filename );
		it++;
	}

	if ( found )
		file = filename;

	return found;
}

/**
 *	Searches for a file given a start directory, recursing into sub-directories
 *
 *	@param directory start directory of search
 *	@param file the file to find
 *
 *	@return true if the file is found
 *
 */
bool BWResource::searchForFile( const BW::StringRef& directory,
		BW::string& file )
{
	BW_GUARD;
	bool result = false;

	BW::string searchFor = formatPath( directory ) + file;

	// search for file in the supplied directory
	if ( instance().fileSystem()->getFileType( searchFor, NULL ) ==
		IFileSystem::FT_FILE )
    {
		result = true;
		file = searchFor;
	}
	else
	{
		// if not found, get all directories and start to search them
		searchFor  = formatPath( directory );
		IFileSystem::Directory dirList;
		instance().fileSystem()->readDirectory( dirList, searchFor );

		for( IFileSystem::Directory::iterator d = dirList.begin();
			d != dirList.end() && !result; ++d )
		{
			searchFor  = formatPath( directory ) + (*d);

       		// check if it is a directory
			if ( instance().fileSystem()->getFileType( searchFor, NULL ) ==
				IFileSystem::FT_DIRECTORY )
			{
            	// recurse into the directory, and start it all again
				printf( "%s\n", searchFor.c_str( ) );

				result = searchForFile( searchFor, file );
			}
		}
	}

	return result;
}

/**
 *	Dissolves a full filename (with drive and path information) to relative
 *	filename.
 *
 *	@param file the files name
 *
 *	@return the dissolved filename, "" if filename cannot be dissolved
 *
 */
const BW::string BWResource::dissolveFilename( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResolver::dissolveFilename( file );
}

#ifdef EDITOR_ENABLED

/**
 *	Finds a file by looking in the search paths
 *
 *	Path Search rules.
 *		If the files path has a drive letter in it, then only the filename is
 *		used. The files path must be relative and NOT start with a / or \ to use
 *		the search path.
 *
 *		If either of the top 2 conditions are true or the file is not found with
 *		its own path appended to the search paths then the file is searched for
 *		using just the filename.
 *
 *	@param file the files name
 *
 *	@return files full path and filename
 *
 */
const BW::string BWResource::findFile( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResolver::findFile( file );
}

/**
 *	Resolves a file into an absolute filename based on the search paths
 *	@see findFile()
 *
 *	@param file the files name
 *
 *	@return the resolved filename
 *
 */
const BW::string BWResource::resolveFilename( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResolver::resolveFilename( file );
}


/**
 *  Determines whether a dissolved filename is a valid BigWorld path or not.
 *
 *	@param file the files name
 *
 *	@return a boolean for whether the file name is valid
 */
bool BWResource::validPath( const BW::StringRef& file )
{
	BW_GUARD;

	if (file.empty())
		return false;

	if (file.size() == 1)
		return file[0] != ':' && file[0] != '/' && file[0] != '\\';

	bool hasDriveName = (file[1] == ':');
	bool hasNetworkName = (file[0] == '/' && file[1] == '/') || 
						  (file[0] == '\\' && file[1] == '\\');

	return !(hasDriveName || hasNetworkName);
}

/**
 *	Removes the drive specifier from the filename
 *
 *	@param file filename string
 *
 *	@return filename minus any drive specifier
 *
 */
const BW::string BWResource::removeDrive( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResolver::removeDrive( file );
}

void BWResource::defaultDrive( const BW::StringRef& drive )
{
	BW_GUARD;
	BWResolver::defaultDrive( drive );
}

#endif // EDITOR_ENABLED


#if ENABLE_FILE_CASE_CHECKING

/**
 *	This enables/disables checking of filename case.
 *
 *	@param enable	If true then the case of filenames are checked as used.  
 *					This is useful to check for problems with files used on
 *					both Windows and Linux, but comes at a cost in execution
 *					speed.
 */
void BWResource::checkCaseOfPaths(bool enable)
{
	BW_GUARD;
	BWResource::instance().pimpl_->checkCase_ = enable;
	BWResource::instance().fileSystem()->checkCaseOfPaths(enable);
}


/**
 *	This gets whether checking of filename's case is disabled or enabled.
 *
 *	@return			True if filename case checking is enabled, false if it's 
 *					disabled.
 */
bool BWResource::checkCaseOfPaths()
{
	BW_GUARD;
	return BWResource::instance().pimpl_->checkCase_;
}

#endif // ENABLE_FILE_CASE_CHECKING


/**
 *	This gets the name of a path on disk, correcting the case if necessary.
 *
 *	@param path		The path to get on disk.
 *	@return			The case-sensitive name used by the file system.  If the
 *					path does not exist then it is returned.
 */
BW::string BWResource::correctCaseOfPath(BW::StringRef const &path)
{
	BW_GUARD;
	return BWResource::instance().fileSystem()->correctCaseOfPath(path);
}


/**
 *	Get the type and information of a file
 *
 *	@param pathname	The relative path name - relative to the rest paths.
 *	@param pFI		The file info structure used to retrive the information of
 *					the file. Set to NULL will 
 *
 *	@return			The type of input file
 */
IFileSystem::FileType BWResource::getFileType( const BW::StringRef& pathname, IFileSystem::FileInfo* pFI )
{
	BW_GUARD;
	return BWResource::instance().fileSystem()->getFileType(
		pathname, pFI );
}


/**
 *	Tests if a path name is a directory
 *
 *	@param pathname The relative path name - relative to the rest paths.
 *
 *	@return true if it is a directory
 *
 */
bool BWResource::isDir( const BW::StringRef& pathname )
{
	BW_GUARD;
	IFileSystem::FileType ft = BWResource::instance().fileSystem()->getFileType(pathname, NULL );

	return(ft == IFileSystem::FT_DIRECTORY);
}

bool BWResource::isDirOrBasePath( const BW::StringRef& pathname )
{
	BW_GUARD;
	if( !isDir( pathname ) )
	{
		int numPaths = BWResource::getPathNum();

		for(int i=0;i<numPaths;i++)
		{
			BW::string currPath = BWResource::getPath(i);
			BW::string currPathWithTrailingSlash = BWResource::formatPath(currPath);
			if((pathname == currPath) || (pathname == currPathWithTrailingSlash))
			{
				return true;
				break;
			}
		}
	}

	return false;
}

/**
 *	Tests if a path name is a file
 *
 *	@param pathname The relative path name - relative to the rest paths.
 *
 *	@return true if it is a file
 *
 */
bool BWResource::isFile( const BW::StringRef& pathname )
{
	BW_GUARD;
	return BWResource::instance().fileSystem()->getFileType(
		pathname, NULL ) == IFileSystem::FT_FILE;
}


/**
 *	Tests if a file exists.
 *
 *	@param file The relative path to the file - relative to the rest paths.
 *
 *	@return true if the file exists
 *
 */
bool BWResource::fileExists( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResource::instance().fileSystem()->getFileType(
		file, NULL ) != IFileSystem::FT_NOT_FOUND;
}

/**
 *	Tests if a file exists.
 *
 *	@param file The absolute path to the file (not including drive letter).
 *
 *	@return true if the file exists
 *
 */
bool BWResource::fileAbsolutelyExists( const BW::StringRef& file )
{
	BW_GUARD;
	return BWResource::instance().pimpl_->nativeFileSystem_->getFileType(
		file, NULL ) != IFileSystem::FT_NOT_FOUND;
}

/**
 *	Tests if the given path is absolute or relative
 *
 *	@param path The path to test.
 *
 *	@return true if the path is relative.
 *
 */
bool BWResource::pathIsRelative( const BW::StringRef& path )
{
	BW_GUARD;
	return BWResourceImpl::pathIsRelative( path );
}

/**
 *	Retrieves the filename of a file.
 *
 *	@param file file to get filename from.
 *
 *	@return filename string.
 *
 */
BW::StringRef BWResource::getFilename( const BW::StringRef& file )
{
	BW_GUARD;
	BW::StringRef::size_type pos = file.find_last_of( "\\/" );

	if (pos != BW::StringRef::npos)
		return file.substr( pos + 1, file.length() );
	else
		return file;
}

/**
 *	Retrieves the path from a file.
 *
 *	@param file file to get path of.
 *
 *	@return path string
 *
 */
BW::string BWResource::getFilePath( const BW::StringRef & file )
{
	BW_GUARD;
	return BWUtil::getFilePath( file );
}

/**
 *	Retrieves the extension of a filename.
 *
 *	@param file filename string
 *
 *	@return extension string
 *
 */
BW::StringRef BWResource::getExtension( const BW::StringRef& file )
{
	BW_GUARD;
	BW::StringRef::size_type pos = file.find_last_of( "." );
	BW::StringRef::size_type fileNamePos = 
		std::min( file.find_last_of( '/' ), file.find_last_of( '\\' ) );
	if( pos != BW::StringRef::npos &&
		(fileNamePos == BW::StringRef::npos || pos > fileNamePos) )
	{
		return file.substr( pos + 1 );
	}

	// no extension
	return BW::StringRef();
}

/**
 *	Removes the extension from the filename
 *
 *	@param file filename string
 *
 *	@return filename minus any extension
 *
 */
BW::StringRef BWResource::removeExtension( const BW::StringRef& file )
{
	BW_GUARD;
	BW::StringRef::size_type pos = file.find_last_of( "." );
	BW::StringRef::size_type fileNamePos = 
		std::min( file.find_last_of( '/' ), file.find_last_of( '\\' ) );
	if( pos != BW::StringRef::npos &&
		(fileNamePos == BW::StringRef::npos || pos > fileNamePos) )
	{
		return file.substr( 0, pos );
	}

	// no extension
	return file;
}

/**
 *	Removes the extension from the filename and replaces it with the given
 *	extension
 *
 *	@param file filename string
 *	@param newExtension the new extension to give the file, including the
 *	initial .
 *
 *	@return file with its extension changed to newExtension
 *
 */
BW::string BWResource::changeExtension( const BW::StringRef& file,
										const BW::StringRef& newExtension )
{
	BW_GUARD;
	BW::StringRef baseFile = removeExtension( file );
	return baseFile + newExtension;
}

/**
 *	This method makes certain that the path contains a trailing folder separator.
 *
 *	@param path 	The path string to be formatted.
 *
 *	@return The path with a trailing folder separator.
 */
BW::string BWResource::formatPath( const BW::StringRef & path )
{
	BW_GUARD;
	return BWUtil::formatPath( path );
}

/**
 * Gets the current working directory
 *
 * @returns string containing the current working directory
 */
BW::string BWResource::getCurrentDirectory()
{
	BW_GUARD;
#ifdef _WIN32
	// get the working directory
	wchar_t wbuffer[MAX_PATH];
	GetCurrentDirectory( ARRAY_SIZE( wbuffer ), wbuffer ); 	 

	BW::string dir = bw_wtoutf8( wbuffer );
	dir.append( "/" );

	BWUtil::sanitisePath( dir );
	return dir;
#else
	// TODO:PM May want to implement non-Windows versions of these.
	return "";
#endif
}

/**
 * Sets the current working directory
 *
 * @param currentDirectory path to make the current directory
 *
 * @returns true if successful
 */
bool BWResource::setCurrentDirectory(
		const BW::StringRef &currentDirectory )
{
	BW_GUARD;
#if defined(_WIN32)
	BW::string cwd( currentDirectory.data(), currentDirectory.size() );
	BW::wstring wcwd;
	bw_utf8tow( cwd, wcwd );
	return SetCurrentDirectory( wcwd.c_str( ) ) ? true : false;
#else
	// TODO:PM May want to implement non-Windows versions of these.
	(void) currentDirectory;

	return false;
#endif
}


/**
 * Checks for the existants of a drive letter
 *
 * @returns true if a drive is supplied in the path
 */
bool BWResourceImpl::hasDriveInPath( const BW::StringRef& path )
{
	BW_GUARD;
	return !path.empty() && (path.find( ':' ) != BW::StringRef::npos ||					// has a drive colon
    	( path[0] == '\\' || path[0] == '/' ));	// starts with a root char
}


/**
 * Check is the path is relative and not absolute.
 *
 * @returns true if the path is relative
 */
bool BWResourceImpl::pathIsRelative( const BW::StringRef& path )
{
	BW_GUARD;
	// has no drive letter, and no root directory
	return !path.empty() && !hasDriveInPath( path ) && ( path[0] != '/' && path[0] != '\\');
}

/**
 * Returns the first path in the paths.xml file
 *
 * @returns first path in paths.xml
 */
const BW::string BWResource::getDefaultPath( void )
{
	BW_GUARD;
	return instance().pimpl_->paths_.empty() ? BW::string( "" ) : instance().pimpl_->paths_[0];
}

/**
 * Returns the path in the paths.xml file at position index
 *
 * @returns path at position index in paths.xml
 */
const BW::string BWResource::getPath( int index )
{
	BW_GUARD;
	if (!instance().pimpl_->paths_.empty() &&
			0 <= index && index < (int)instance().pimpl_->paths_.size() )
	{
		return instance().pimpl_->paths_[index];
	}
	else
		return BW::string( "" );
}

int BWResource::getPathNum()
{
	BW_GUARD;
	return int( instance().pimpl_->paths_.size() );
}

BW::string BWResource::getPathAsCommandLine()
{
	BW_GUARD;
	BW::string commandLine;
	int totalPathNum = getPathNum();

	if ( totalPathNum > 0 )
		commandLine = "--res \"";

	for( int i = 0; i < totalPathNum; ++i )
	{
		commandLine += getPath( i );
		if( i != totalPathNum - 1 )
			commandLine += ';';
	}

	if ( !commandLine.empty() )
		commandLine += "\"";

	return commandLine;
}

/**
 * Ensures that the path exists
 *
 */
bool BWResource::ensurePathExists( const BW::StringRef& path )
{
	BW_GUARD;
	BW::string nPath = path.to_string();
    std::replace( nPath.begin(), nPath.end(), '/', '\\' );

    if ( nPath.empty() )
		return false; // just to be safe

    if ( nPath[nPath.size()-1] != '\\' )
		nPath = getFilePath( nPath );

    std::replace( nPath.begin(), nPath.end(), '/', '\\' );

	// skip drive letter, if using absolute paths
	BW::string::size_type poff = 0;
	if ( nPath.size() > 1 && nPath[1] == ':' )
		poff = 2;
	// skip network drive, if using absolute paths
	else if ( nPath.size() > 1 && nPath[0] == '\\' && nPath[1] == '\\' )
		poff = nPath.find_first_of( '\\', 2 );

	while (1)
	{
		poff = nPath.find_first_of( '\\', poff + 1 );
		if (poff == BW::string::npos)
			break;

		BW::string subpath = nPath.substr( 0, poff );
		instance().fileSystem()->makeDirectory( subpath.c_str() );
	}
	return true;
}

/**
 * Ensures that the absolute path exists on the system
 */
bool BWResource::ensureAbsolutePathExists( const BW::StringRef& path )
{
	BW_GUARD;
	BW::string nPath = path.to_string();
    std::replace( nPath.begin(), nPath.end(), '/', '\\' );

    if ( nPath.empty() )
	{
		return false; // just to be safe
	}

    if ( nPath[nPath.size()-1] != '\\' )
		nPath = getFilePath( nPath );

    std::replace( nPath.begin(), nPath.end(), '/', '\\' );

	// skip drive letter, if using absolute paths
	BW::string::size_type poff = 0;
	if ( nPath.size() > 1 && nPath[1] == ':' )
		poff = 2;
	// skip network drive, if using absolute paths
	else if ( nPath.size() > 1 && nPath[0] == '\\' && nPath[1] == '\\' )
		poff = nPath.find_first_of( '\\', 2 );

	while (1)
	{
		poff = nPath.find_first_of( '\\', poff + 1 );
		if (poff == BW::string::npos)
			break;

		BW::string subpath = nPath.substr( 0, poff );
#ifdef _WIN32

		BW::wstring wsubpath = bw_utf8tow( subpath ); 
		if (!CreateDirectory( wsubpath.c_str(), NULL ) && 
			GetLastError() != ERROR_ALREADY_EXISTS) 
#else
		if( mkdir( subpath.c_str(), 0 ) == -1 && errno == ENOENT)
#endif // _WIN32
		{
			return false;
		}

	}
	return true;
}

bool BWResource::isValid()
{
	BW_GUARD;
	return instance().pimpl_->paths_.size( ) != 0;
}

BW::string BWResourceImpl::formatSearchPath( const BW::StringRef& path, const BW::StringRef& pathXml )
{
	BW_GUARD;
	BW::string tmpPath( path.data(), path.size() );

#ifdef _WIN32
	// get the applications path and load the config file from it
	BW::string appDirectory;

	if (pathXml != "")
	{
		appDirectory = pathXml.to_string();
	}
	else
	{
		appDirectory = BWResourceImpl::getAppDirectory();
	}

	// Does it contain a drive letter?
	if (tmpPath.size() > 1 && tmpPath[1] == ':')
	{
		MF_ASSERT(isalpha(tmpPath[0]));
	}
	// Is it a relative tmpPath?
	else if (tmpPath.size() > 0 && tmpPath[0] != '/')
	{
		tmpPath = appDirectory + tmpPath;
		// make sure slashes are windows-style
		std::replace( tmpPath.begin(), tmpPath.end(), '/', '\\' );

		// canonicalise (that is, remove redundant relative path info)
		BW::wstring wtmpPath;
		bw_utf8tow( tmpPath, wtmpPath );
		wchar_t wfullPath[MAX_PATH];
		if ( PathCanonicalize( wfullPath, wtmpPath.c_str() ) )
		{
			bw_wtoutf8( wfullPath, tmpPath );
		}
	}
	else
	{
		// make sure slashes are windows-style
		std::replace( tmpPath.begin(), tmpPath.end(), '/', '\\' );

		// get absolute path
		BW::wstring wtmpPath;
		bw_utf8tow( tmpPath, wtmpPath );
		wchar_t wfullPath[MAX_PATH];
		wchar_t* wfilePart;
		if ( GetFullPathName( wtmpPath.c_str(), ARRAY_SIZE( wfullPath ), wfullPath, &wfilePart ) )
		{
			bw_wtoutf8( wfullPath, tmpPath );
		}
		#ifndef EDITOR_ENABLED
			// not editor enabled, but win32, so remove drive introduced by
			// GetFullPathName to be consistent.
			tmpPath = BWResolver::removeDrive( tmpPath );
		#endif //  EDITOR_ENABLED
	}
	#if ENABLE_FILE_CASE_CHECKING
		// Since the case of the path can sometimes be wrong, we make sure the case is correct
		// here, this is so that the case checking doesn't get the actual res path wrong, just
		// the resource string
		tmpPath = BWResource::instance().pimpl_->nativeFileSystem_->correctCaseOfPath( tmpPath );
	#endif
#endif // _WIN32

	// Forward slashes only
	std::replace( tmpPath.begin(), tmpPath.end(), '\\', '/' );

	// Trailing slashes aren't allowed
	if (tmpPath[tmpPath.size() - 1] == '/')
		tmpPath = tmpPath.substr( 0, tmpPath.size() - 1 );

	return tmpPath;
}

void BWResource::addPath( const BW::StringRef& path, int atIndex )
{
	BW_GUARD;
	BW::string fpath = BWResourceImpl::formatSearchPath( path );

	BWResourceImpl* pimpl = instance().pimpl_;
	if (uint(atIndex) >= pimpl->paths_.size())
		atIndex = static_cast<int>(pimpl->paths_.size());
	pimpl->paths_.insert( pimpl->paths_.begin()+atIndex, fpath );

	INFO_MSG( "Added res path: \"%s\"\n", fpath.c_str() );

	if ( instance().fileSystem() )
		instance().refreshPaths(); // already created fileSystem_, so recreate them
}

/**
 *	Add all the PATH_SEPERATOR separated paths in path to the search path
 */
void BWResource::addPaths( const BW::StringRef& path, const char* desc, const BW::StringRef& pathXml )
{
	BW_GUARD;
	BW::vector< BW::StringRef > pathParts;
	bw_tokenise< BW::StringRef >( path, BW_RES_PATH_SEPARATOR, pathParts );

	for ( BW::vector< BW::StringRef >::iterator it = pathParts.begin() ;
			it != pathParts.end() ; ++it )
	{
		const BW::StringRef & pathPart = *it;

		BW::string formattedPath = BWResourceImpl::formatSearchPath( pathPart, pathXml );

#ifdef MF_SERVER
		// Convert the res path to an absolute string if it isn't already
		if (formattedPath.size() > 0 && formattedPath[0] != '/')
		{
			// get the executable path and load the config file from it
			BW::string tmpPath = BWUtil::executableDirectory() + '/' + formattedPath + '\0';

			char buf[PATH_MAX + 1];
			memset( buf, 0, sizeof( buf ) );
			strncpy( buf, tmpPath.c_str(), tmpPath.size() );

			// canonicalise (that is, remove redundant relative path info)
			char *absPath = canonicalize_file_name( buf );
			if (absPath)
			{
				formattedPath = absPath;

				// malloc'ed in canonicalize_file_name() above.
				raw_free( absPath );
			}
		}
#endif

		if (instance().pimpl_->nativeFileSystem_->getFileType( formattedPath, 0 ) ==
				IFileSystem::FT_NOT_FOUND)
		{
#ifdef MF_SERVER
			ERROR_MSG( "BWResource::addPaths: "
						"Resource path does not exist: %s\n",
						formattedPath.c_str() );

			if (formattedPath.find( ';' ) != BW::string::npos)
			{
				ERROR_MSG( "BWResource::addPaths: "
						"Note that the separator character is ':', not ';'\n" );
			}
#elif defined( BW_EXPORTER )
			g_exporterErrorMsg = new char[1024];
			bw_snprintf( g_exporterErrorMsg, 1024,
				"Search path does not exist in paths.xml: %s. The exporter will not function."
				"Fix paths.xml so it only contains valid paths and try again.",
				formattedPath.c_str() );
#else
			CRITICAL_MSG( "Search path does not exist: %s "
						RES_PATH_HELP, formattedPath.c_str() );
#endif
		}
		else
		{
			instance().pimpl_->paths_.push_back( formattedPath );
			INFO_MSG( "Added res path: \"%s\" (%s).\n", formattedPath.c_str(), desc );
		}
	}

	#if defined( _EVALUATION )
    STRINGVECTOR::iterator it  = instance().pimpl_->paths_.begin( );
    STRINGVECTOR::iterator end = instance().pimpl_->paths_.end( );
    while ( it != end ) {
		if ( it->find( "bigworld\\res" ) != BW::string::npos &&
			std::distance( it, end ) > 1 )
		{
			CRITICAL_MSG(
				"<bigworld/res> is not the last entry in the search path (%s)."
				"This is usually an error. Press 'No' to ignore and proceed "
				"(you may experience errors or other unexpected behaviour). %d"
				RES_PATH_HELP, path.c_str(), std::distance( it, end ) );
		}
		++it;
	}
	#endif

	if ( instance().fileSystem() )
		instance().refreshPaths(); // already created fileSystem_, so recreate them
}


/**
 * Add sub paths (BW_RES_PATH_SEPARATOR deliminated). Will append the subPath to all
 * paths that currently exist. Places them *before* the appended path to ensure
 * that the subpath is searched first.
 * @param subPaths Delimited list of relative sub paths (eg. "tools;tools/whatever")
 */
void BWResource::addSubPaths( const BW::StringRef& subPaths )
{
	BW::vector< BW::StringRef > subPathParts;
	bw_tokenise< BW::StringRef >(	subPaths,
									BW_RES_PATH_SEPARATOR,
									subPathParts );

	STRINGVECTOR& paths = instance().pimpl_->paths_;

	// Build a new list of paths.
	STRINGVECTOR newPaths( paths.size() );

	for (STRINGVECTOR::iterator
			pathIt( paths.begin() );
			pathIt != paths.end();
			++pathIt)
	{
		const BW::string& path = *pathIt;

		// Add sub paths if they exist.
		for (BW::vector< BW::StringRef >::iterator
				subPathIt( subPathParts.begin() );
				subPathIt != subPathParts.end();
				++subPathIt)
		{
			const BW::StringRef& subPath = *subPathIt;
			const BW::string fullSubPath = path + "/" + subPath;

			if (fileAbsolutelyExists( fullSubPath ))
			{
				newPaths.push_back( fullSubPath );
			}
		}

		// Add existing path to the list.
		newPaths.push_back( path );
	}

	// Update paths with new paths.
	paths = newPaths;
}


/**
 * Remove the path in the paths.xml file at position index
 * used mainly for testing.
 */
void BWResource::delPath( int index )
{
	BW_GUARD;
	if (!instance().pimpl_->paths_.empty() &&
			0 <= index && index < (int)instance().pimpl_->paths_.size() )
	{
		instance().pimpl_->paths_.erase(instance().pimpl_->paths_.begin() + index);
	}
}


/**
 *	Compares the modification dates of two files, and returns
 *	true if the first file 'oldFile' is indeed older (by at least
 *	mustBeOlderThanSecs seconds) than the second file 'newFile'.
 *	Returns false on error.
 */
bool BWResource::isFileOlder( const BW::StringRef & oldFile,
	const BW::StringRef & newFile, int mustBeOlderThanSecs )
{
	BW_GUARD;
	bool result = false;

	uint64 ofChanged = modifiedTime( oldFile );
	uint64 nfChanged = modifiedTime( newFile );

	if (ofChanged != 0 && nfChanged != 0)
	{
		uint64	mbotFileTime = uint64(mustBeOlderThanSecs) * uint64(10000000);
		result = (ofChanged + mbotFileTime < nfChanged);
	}

	return result;
}


/**
 *	Returns the time the given file (or resource) was modified.
 *	Returns 0 on error (e.g. the file does not exist)
 *  Only files are handled for backwards compatibility.
 */
uint64 BWResource::modifiedTime( const BW::StringRef & fileOrResource )
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	BW::string filename = BWResourceImpl::hasDriveInPath( fileOrResource ) ?
		fileOrResource.to_string() : resolveFilename( fileOrResource );
#else
	BW::string filename( fileOrResource.data(), fileOrResource.size() );
#endif

	IFileSystem::FileInfo fi;
	if ( instance().fileSystem()->getFileType( filename, &fi ) == IFileSystem::FT_FILE )
		return fi.modified;

	return 0;
}


/**
 *	Fixes up the passed path so that the appropriate common root is used.
 *
 *	@param rootPath	The path that needs to be fixed up.
 *	@return	The fixed up path.
 */
/*static*/ BW::string BWResource::fixCommonRootPath( const BW::StringRef& rootPathRef )
{
	BW_GUARD;

	// There are three special scenarios that need to be handled.
	//
	// 1)	rootPath is a root disk drive, e.g. c:
	//		In this case we need to append a forward slash at the end.
	// 2)	rootPath may only have a partial folder name, which may match one of
	//		the res paths, e.g.
	//		If there are two res paths defined in paths.xml:
	//			c:/mf/fantasydemo/res
	//			c:/mf_1_8/fantasydemo/res
	//		The common root would be c:/mf.  This may mistakenly resolve to
	//		using c:/mf/ as the common root which would be incorrect.
	//		Special care must be taken to remove the partial folder name
	//		so that the true root path is used, i.e c:/ from the example above.
	// 3)	rootPath has many nested folders, e.g. c:/dev/mf
	//		In this case the standard path stripping command can be used
	// ---------------------------------------------------------------------
	//
	// First check for scenario 1)
	BW::string rootPath( rootPathRef.data(), rootPathRef.size() );
	if (rootPath.size() > 0 &&
		rootPath.at( rootPath.size() - 1 ) == ':')
	{
		rootPath += "/";
	}
	else
	{
		// Check if the last forward slash is proceeded by a colon.  If it
		// is, truncate everything after the last forward slash.
		BW::string::size_type iLastForwardSlash = rootPath.rfind( "/" );
		if (iLastForwardSlash != BW::string::npos &&
			iLastForwardSlash > 0 &&
			rootPath.at( iLastForwardSlash - 1 ) == ':')
		{
			rootPath = rootPath.substr( 0, iLastForwardSlash + 1 );
		}
		else
		{
			//Remove anything after the last directory slash
			rootPath = rootPath.substr( 0, iLastForwardSlash );
		}
	}

	return rootPath;
}

/**
 *	Walks up through the resource tree specified by path, looking for the 
 *	given desiredSection, starting at the leaf. Returns the open DataSection
 *	if it was found, otherwise returns NULL.
 *
 *	@param path The resource path to start searching from
 *	@param desiredSection The name of the section to find
 *	@return	DataSectionPtr The open DataSection, if the desiredSection was found
 */
/*static*/ DataSectionPtr BWResource::openSectionInTree( const BW::StringRef& path, 
														const BW::StringRef& desiredSection )
{
	BW_GUARD;
	DataSectionPtr ds = NULL;
	BW::string settingsName;
	BW::string tempPath( path.data(), path.size() );

	if (*tempPath.rbegin() != '/')
	{
		tempPath += '/';
	}

	while (!tempPath.empty())
	{
		settingsName = BWResolver::dissolveFilename( tempPath + desiredSection );
		ds = BWResource::openSection( settingsName );
		if (ds)
		{
			break;
		}
		else if (tempPath.length() > 1)
		{
			size_t slashPos = tempPath.rfind( '/', tempPath.size()-2 );
			if (slashPos == BW::string::npos)
			{
				break;
			}

			tempPath = tempPath.substr( 0, slashPos+1 );
		}
		else
		{
			break;
		}
	}

	return ds;
}


/**
 *	Resolves the given path to an absolute path. Loops through each filesystem
 * 	and resolves the path to the first one that contains the file.
 * 
 * 	If the path is already an absolute path (i.e. it starts with a slash or 
 * 	drive letter), it simply returns the type of the file.
 * 
 * 	To resolve the path to a full path without checking for the existence of 
 * 	the file, see MultiFileSystem::getAbsolutePath().
 * 
 * 	@param	path	Input: The path to resolve.
 * 					Output: The resolved path. Does not
 * 			resolve the path if path does not exist in any file systems.
 * 
 * 	@return	The type of the file. 
 */
IFileSystem::FileType BWResource::resolveToAbsolutePath( 
		BW::string& path )
{
	BW_GUARD;
	IFileSystem::FileType type;
	if (path.empty() || BWResourceImpl::pathIsRelative( path ))
	{
		type = instance().fileSystem()->resolveToAbsolutePath( path );
	}
	else
	{
		type = NativeFileSystem::getAbsoluteFileType( path );
	}

	// Ensure that it ends with a path separator if it is a directory
	if (type == IFileSystem::FT_DIRECTORY && (path.rbegin() != path.rend()) && 
			(*path.rbegin() != PATH_SEPARATOR))
	{
		path += PATH_SEPARATOR;
	}

	return type;
}

/**
 *	Resolves the given path to an absolute path. Loops through each filesystem
 * 	and resolves the path to the first one that contains the file.
 * 
 * 	If the path is already an absolute path (i.e. it starts with a slash or 
 * 	drive letter), it simply returns the type of the file.
 * 
 * 	To resolve the path to a full path without checking for the existence of 
 * 	the file, see MultiFileSystem::getAbsolutePath().
 * 
 * 	@param	path	Input: The path to resolve.
 * 					Output: The resolved path. Does not
 * 			resolve the path if path does not exist in any file systems.
 * 
 * 	@return	The type of the file. 
 */
IFileSystem::FileType BWResource::resolveToAbsolutePath( 
		BW::wstring& path )
{
	BW::string utf8;
	bw_wtoutf8WS( path.c_str(), utf8 );
	IFileSystem::FileType type = resolveToAbsolutePath( utf8 );
	bw_utf8towSW( utf8, path );
	return type;
}

#ifdef _WIN32
/*static*/ const BW::string BWResource::appDirectory()
{
	BW_GUARD;
	return BWResourceImpl::getAppDirectory();
}


/*static*/ const BW::string BWResource::appPath()
{
	BW_GUARD;
#ifdef  BWCLIENT_AS_PYTHON_MODULE
	MF_ASSERT( false && "Should not get called in PyModuleHybrid." );
#endif
	return BWUtil::executablePath();
}


/**
 *	This method returns the path to an appropriate subdirectory of the current user's CSIDL_APPDATA,
 *  and ensures the method exists.
 *
 *
 *	@return	The path to the existant directory, terminated with a slash.
 *
 */
/*static*/ const BW::string BWResource::appDataDirectory( bool roaming )
{
	BW_GUARD;
	return BWResourceImpl::getAppDataDirectory( roaming );
}

/**
 *	This method returns the path to an appropriate subdirectory of the current user's CSIDL_MYPICTURES,
 *  and ensures the method exists.
 *
 *	@return	The path to the existant directory, terminated with a slash, or "" if the directory
 *			could not be found or created.
 *
 */
/*static*/ const BW::string BWResource::userPicturesDirectory()
{
	BW_GUARD;
	return BWResourceImpl::getUserPicturesDirectory();
}

/**
 *	This method returns the path to an appropriate subdirectory of the current user's CSIDL_APPDATA,
 *  and ensures the method exists.
 *
 *	@return	The path to the existant directory, terminated with a slash, or "" if the directory
 *			could not be found or created.
 *
 */
/*static*/ const BW::string BWResource::userDocumentsDirectory()
{
	BW_GUARD;
	return BWResourceImpl::getUserDocumentsDirectory();
}

/**
 *	This method returns the company name defined in this application's VS_VERSION_INFO structure
 *
 *	This method takes the string matching the current locale if possible, or the first found
 *  string otherwise.
 *
 *	@return	The company name from this application's VS_VERSION_INFO structure or "" if it could
 *			not be retrieved.
 *
 */
/*static*/ const BW::string BWResource::appCompanyName()
{
	BW_GUARD;
	return instance().pimpl_->getAppVersionString( "CompanyName" );
}

/**
 *	This method returns the product name defined in this application's VS_VERSION_INFO structure
 *
 *	This method takes the string matching the current locale if possible, or the first found
 *  string otherwise.
 *
 *	@return	The product name from this application's VS_VERSION_INFO structure or "" if it could
 *			not be retrieved.
 *
 */
/*static*/ const BW::string BWResource::appProductName()
{
	BW_GUARD;
	return instance().pimpl_->getAppVersionString( "ProductName" );
}


/**
 *	This method returns the Product Version from the application's VS_VERSION_INFO structure
 *
 *	@return	The product version from this application's VS_VERSION_INFO structure or "" if it could
 *			not be retrieved.
 *
 */
/*static*/const BW::string BWResource::appProductVersion()
{
	BW_GUARD;
	return instance().pimpl_->getAppVersionString( "ProductVersion" );
}
#endif // _WIN32


std::ostream& operator<<(std::ostream& o, const BWResource& /*t*/)
{
	BW_GUARD;
	o << "BWResource\n";
	return o;
}


void BWResource::enableModificationMonitor( bool enable )
{
	BW_GUARD;
	instance().pimpl_->enableModificationMonitor( enable );
}

void BWResource::addModificationListener( 
	ResourceModificationListener* listener )
{
	BW_GUARD;
	return instance().pimpl_->addModificationListener( listener );
}

void BWResource::removeModificationListener(
	ResourceModificationListener* listener )
{
	BW_GUARD;
	instance().pimpl_->removeModificationListener( listener );
}

void BWResource::flushModificationMonitor()
{
	BW_GUARD;
	instance().pimpl_->flushModificationMonitor();
}


bool BWResource::ignoreFileModification( const BW::string& fileName, 
						ResourceModificationListener::Action action,
						bool oneTimeOnly )
{
	BW_GUARD;
	return  instance().pimpl_->ignoreFileModification( fileName, action, oneTimeOnly );
}


bool BWResource::hasPendingModification( const BW::string& fileName )
{
	BW_GUARD;
	return instance().pimpl_->fileSystem_->hasPendingModification( fileName );
}


/**
 *	return the path by index,
 * 	@param	path	Output: The path to return.
 * 
 * 	@return	success or not
 */
bool BWResource::path( size_t idx, char* pathRet, size_t maxPath )
{
	BW_GUARD;
	pathRet[0] = 0;
	if (idx < instance().pimpl_->paths_.size())
	{
		bw_str_append( pathRet, maxPath, instance().pimpl_->paths_[idx].c_str() );
		return true;
	}
	return false;
}


/**
 *	return the path by index,
 * 	@param	path	Output: The path to return.
 * 
 * 	@return	success or not
 */
bool BWResource::path( size_t idx, wchar_t* pathRet, size_t maxPath )
{
	BW_GUARD;
	pathRet[0] = 0;
	if (idx < instance().pimpl_->paths_.size())
	{
		bw_utf8tow( instance().pimpl_->paths_[idx], pathRet, 256 );
		return true;
	}
	return false;
}
// -----------------------------------------------------------------------------
// Section: BWResolver
// -----------------------------------------------------------------------------

/**
 *	Finds a file by looking in the search paths
 *
 *	Path Search rules.
 *		If the files path has a drive letter in it, then only the filename is
 *		used. The files path must be relative and NOT start with a / or \ to use
 *		the search path.
 *
 *		If either of the top 2 conditions are true or the file is not found with
 *		its own path appended to the search paths then the file is searched for
 *		using just the filename.
 *
 *	@param file the files name
 *
 *	@return files full path and filename
 *
 */
BW::string BWResolver::findFile( const BW::StringRef& file )
{
	BW_GUARD;
	BW::string tFile;
	bool found = false;

	// is the file relative and have no drive letter
	if ( !BWResourceImpl::hasDriveInPath( file ) && BWResourceImpl::pathIsRelative( file ) )
	{
		STRINGVECTOR::iterator it  = BWResource::instance().pimpl_->paths_.begin( );
		STRINGVECTOR::iterator end = BWResource::instance().pimpl_->paths_.end( );

		// search for the file in the supplied paths
		while( !found && it != end )
		{
			tFile  = BWResource::formatPath((*it)) + file;
			found = BWResource::fileAbsolutelyExists( tFile );
			it++;
		}

		// the file doesn't exist, let's see about its directory
		if (!found)
		{
			BW::string dir = BWResource::getFilePath( file );

			STRINGVECTOR::iterator it = BWResource::instance().pimpl_->paths_.begin( );
            for (; it != end; ++it)
			{
				BW::string path = BW::string( BWResource::formatPath((*it)) );
				found = BWResource::fileAbsolutelyExists( path + dir + '.' );
				if (found)
				{
					tFile = path + file;
					break;
				}
			}
		}
	}
	else
	{

		// the file is already resolved!
		tFile.assign( file.data(), file.size() );
		found = true;
	}

	if ( !found )
	{
    	tFile  = BWResource::formatPath( BWResource::getDefaultPath() ) + file;
    }

	return tFile;
}

/**
 *	Resolves a file into an absolute filename based on the search paths
 *	@see findFile()
 *
 *	@param file the files name
 *
 *	@return the resolved filename
 *
 */
BW::string BWResolver::resolveFilename( const BW::StringRef& file )
{
	BW_GUARD;
	BW::string tmpFile( file.data(), file.size() );
    std::replace( tmpFile.begin(), tmpFile.end(), '\\', '/' );
    BW::string foundFile = findFile( tmpFile );

    if ( strchr( foundFile.c_str(), ':' ) == NULL && ( foundFile[0] == '/' || foundFile[0] == '\\' ) )
    	foundFile = appDrive_ + foundFile;

#ifdef LOGGING
    if( file != foundFile && !fileExists( foundFile ) )
    {
        dprintf( "BWResource - Invalid File: %s \n\tResolved to %s\n", file.c_str(), foundFile.c_str() );
	    std::ofstream os( "filename.log", std::ios::app | std::ios::out );
    	os << "Error resolving file: " << file << "\n";
    }
#endif

	return foundFile;
}

/**
 *	Dissolves a full filename (with drive and path information) to relative
 *	filename.
 *
 *	@param file the files name
 *
 *	@return the dissolved filename, "" if filename cannot be dissolved
 *
 */
BW::string BWResolver::dissolveFilename( const BW::StringRef& file )
{
	BW_GUARD;
	BW::string newFile( file.data(), file.size() );
    std::replace( newFile.begin(), newFile.end(), '\\', '/' );

	BW::string path;
	bool found = false;

	// is the filename valid
	// we can only dissolve an absolute path (has a root), and a drive specified
	// is optional.

    if ( !BWResourceImpl::hasDriveInPath( newFile ) && BWResourceImpl::pathIsRelative( newFile ) )
        return newFile;

    STRINGVECTOR::iterator it  = BWResource::instance().pimpl_->paths_.begin( );
    STRINGVECTOR::iterator end = BWResource::instance().pimpl_->paths_.end( );

    // search for the file in the supplied paths
    while( !found && it != end )
    {
        // get the search path
        path   = BW::string( BWResource::formatPath((*it)) );

        // easiest test: is the search path a substring of the file
        if ( mf_pathcmp( path.c_str(), newFile.substr( 0, path.length( ) ).c_str() ) == 0 )
        {
            newFile = newFile.substr( path.length( ), file.length( ) );
            found = true;
        }

        if ( !found )
        {
            // is the only difference the drive specifiers?

#ifdef _WIN32
			// remove drive letters if in Win32
            BW::string sPath = removeDrive( path );
			BW::string fDriveLess = removeDrive( newFile );
#else
            BW::string sPath = path;
			BW::string fDriveLess = newFile;
#endif
            // remove the filenames
            sPath = BWResource::formatPath( sPath );
			BW::string fPath = BWResource::getFilePath( fDriveLess );

            if ( ( fPath.length( ) > sPath.length() ) &&
				( mf_pathcmp( sPath.c_str(), fPath.substr( 0, sPath.length() ).c_str() ) == 0 ) )
            {
                newFile = fDriveLess.substr( sPath.length(), file.length() );
                found = true;
            }
        }

        // point to next search path
        it++;
    }

    if ( !found )
    {
        // there is still more we can do!
        // we can look for similar paths in the search path and the
        // file's path
        // eg. \demo\zerodata\scenes & m:\bigworld\zerodata\scenes
        // TODO: implement this feature if required

        // the file is invalid
#ifdef LOGGING
        dprintf( "BWResource - Invalid: %s\n", file.c_str() );
	    std::ofstream os( "filename.log", std::ios::app | std::ios::out );
    	os << "Error dissolving file: " << file << "\n";
        newFile = "";
#endif
    }


	return newFile;
}

/**
 *	Removes the drive specifier from the filename
 *
 *	@param file filename string
 *
 *	@return filename minus any drive specifier
 *
 */
BW::string BWResolver::removeDrive( const BW::StringRef& file )
{
	BW_GUARD;
	BW::StringRef tmpFile( file );

    BW::string::size_type firstOf = tmpFile.find_first_of( ":" );
	if ( firstOf != BW::string::npos )
		tmpFile = tmpFile.substr( firstOf + 1 );

	return tmpFile.to_string();
}

void BWResolver::defaultDrive( const BW::StringRef& drive )
{
	BW_GUARD;
    STRINGVECTOR::iterator it  = BWResource::instance().pimpl_->paths_.begin( );
    STRINGVECTOR::iterator end = BWResource::instance().pimpl_->paths_.end( );

	BWResource::instance().pimpl_->defaultDrive_.assign( drive.data(), drive.size() );

    // search for the file in the supplied paths
    while( it != end )
    {
        // get the search path
        BW::string path = *it;

        if ( path.c_str()[1] != ':' )
        {
            path = BWResource::instance().pimpl_->defaultDrive_ + path;
            (*it) = path;
        }

        ++it;
    }
}

#ifdef _WIN32
BW::string BWResourceImpl::appDirectoryOverride_ = "";
#endif

BW_END_NAMESPACE

// bwresource.cpp
