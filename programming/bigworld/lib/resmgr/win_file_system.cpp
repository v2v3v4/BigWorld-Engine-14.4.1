#include "pch.hpp"
#include "bwresource.hpp"
#include "win_file_system.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_stack_container.hpp"

#include "win_file_streamer.hpp"
#include "resource_modification_listener.hpp"
#include "multi_file_system.hpp"

#include "cstdmf/cstdmf_windows.hpp"

#include <direct.h>
#include <algorithm>


#define conformSlash(x) (x)

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )


BW_BEGIN_NAMESPACE

long WinFileSystem::callsTo_getFileType_ = 0;
long WinFileSystem::callsTo_readDirectory_ = 0;
long WinFileSystem::callsTo_readFile_ = 0;
long WinFileSystem::callsTo_makeDirectory_ = 0;
long WinFileSystem::callsTo_writeFile_ = 0;
long WinFileSystem::callsTo_moveFileOrDirectory_ = 0;
long WinFileSystem::callsTo_eraseFileOrDirectory_ = 0;
long WinFileSystem::callsTo_posixFileOpen_ = 0;

// ----------------------------------------------------------------------------
// Section: WinFileSystem
// ----------------------------------------------------------------------------

/**
 *	This is the constructor.
 *
 *	@param basePath		Base path of the filesystem, including trailing '/'
 */
WinFileSystem::WinFileSystem(const BW::StringRef& basePath) :
	basePath_( basePath.data(), basePath.size() )
#if ENABLE_FILE_CASE_CHECKING
	,checkCase_(false)
#endif // ENABLE_FILE_CASE_CHECKING
	,monitorThread_(NULL)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );
	systemAllocationGranularity_ = (size_t)systemInfo.dwAllocationGranularity;

	if (!basePath_.empty() && *basePath_.rbegin() != '/' && *basePath_.rbegin() != '\\')
	{
		basePath_ += '\\';
	}

	// TODO: This should be moved to another place, such as a global library
	// intialisation.
	static bool s_watchersInited = false;

	if (!s_watchersInited)
	{
		s_watchersInited = true;
		MF_WATCH(	"WinFileSystem/getFileType()",
					WinFileSystem::callsTo_getFileType_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::getFileType() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/readDirectory()",
					WinFileSystem::callsTo_readDirectory_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::readDirectory() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/readFile()",
					WinFileSystem::callsTo_readFile_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::readFile() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/makeDirectory()",
					WinFileSystem::callsTo_makeDirectory_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::makeDirectory() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/writeFile()",
					WinFileSystem::callsTo_writeFile_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::writeFile() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/moveFileOrDirectory()",
					WinFileSystem::callsTo_moveFileOrDirectory_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::moveFileOrDirectory() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/eraseFileOrDirectory()",
					WinFileSystem::callsTo_eraseFileOrDirectory_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::eraseFileOrDirectory() since "
					"program start.");

		MF_WATCH(	"WinFileSystem/posixFileOpen()",
					WinFileSystem::callsTo_posixFileOpen_,
					Watcher::WT_READ_ONLY,
					"The number of calls to WinFileSystem::posixFileOpen() since "
					"program start.");
	}
}

/**
 *	This is the destructor.
 */
WinFileSystem::~WinFileSystem()
{
	this->enableModificationMonitor( false );
}


#ifdef EDITOR_ENABLED
bool WinFileSystem::s_useFileTypeCache_ = false;

IFileSystem::FileType WinFileSystem::getFileTypeCache( const BW::StringRef & path,
	FileInfo * pFI = NULL )
{
	if (!s_useFileTypeCache_)
	{
		return FT_NOT_FOUND;
	}

	ReadWriteLock::ReadGuard ftcGuard ( fileTypeCacheLock_ );
	StringRefFileTypeHashMap::const_iterator it = fileTypeCache_.find( path );
	if (it != fileTypeCache_.end())
	{
		return it->second;
	}

	return FT_NOT_FOUND;
}
#endif


/**
 *	This method returns the file type of a given path within the file system.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *
 *	@return The type of file
 */
IFileSystem::FileType WinFileSystem::getFileType( const BW::StringRef & path,
	FileInfo * pFI )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( getFileType );

	InterlockedIncrement( &callsTo_getFileType_ );

	BWResource::checkAccessFromCallingThread( path, "WinFileSystem::getFileType" );

	if ( path.empty() )
		return FT_NOT_FOUND;

	char buffer[MAX_PATH + 1];
	BW::StringBuilder builder( buffer, ARRAY_SIZE(buffer) );
	if (basePath_.length() + path.length() >= builder.numFree())
		return FT_NOT_FOUND;
	builder.append( basePath_ );
	builder.append( path );
	const char* filename = conformSlash( builder.string() );

	IFileSystem::FileType result = 
		WinFileSystem::getAbsoluteFileTypeInternal( filename, pFI );

#if ENABLE_FILE_CASE_CHECKING
	if (result != FT_NOT_FOUND && checkCase_)
	{
		caseChecker_.check( BW::StringRef( filename ) );
	}
#endif // ENABLE_FILE_CASE_CHECKING

#ifdef EDITOR_ENABLED
	if (result != FT_NOT_FOUND && s_useFileTypeCache_)
	{
		ReadWriteLock::WriteGuard ftcGuard ( fileTypeCacheLock_ );
		fileTypeCache_.insert( std::make_pair( path, result ) );
	}
#endif

	return result;
}


/**
 *	This method returns the file type of a given path within the file system.
 *
 *	@param path		Absolute path to the file.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *
 *	@return The type of file
 */
IFileSystem::FileType WinFileSystem::getAbsoluteFileType( 
		const BW::StringRef & path, FileInfo * pFI )
{
	BWResource::checkAccessFromCallingThread( path, 
			"WinFileSystem::getAbsoluteFileType" );

	if ( path.empty() )
		return FT_NOT_FOUND;

	return WinFileSystem::getAbsoluteFileTypeInternal( conformSlash( path ), 
			pFI );
}


/**
 *	This method returns the file type of a given path within the file system.
 * 	Does not do slash normalisation, diary entries or check access from 
 * 	main thread.
 *
 *	@param path		Absolute path to the file.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *
 *	@return The type of file
 */
IFileSystem::FileType WinFileSystem::getAbsoluteFileTypeInternal( 
		const BW::StringRef& path, FileInfo * pFI )
{
	WIN32_FILE_ATTRIBUTE_DATA attrData;
	wchar_t wpath[MAX_PATH + 1] = { 0 };
	bw_utf8tow( path.data(), path.size(), wpath, MAX_PATH );
	MF_ASSERT_DEBUG( path.size() == 0 || wcslen( wpath ));
	BOOL ok = GetFileAttributesEx(
		wpath,
		GetFileExInfoStandard,
		&attrData );
	//OutputDebugString( (path + "\n").c_str() );

	if (!ok || attrData.dwFileAttributes == DWORD(-1))
		return FT_NOT_FOUND;

	if (pFI != NULL)
	{
		pFI->size = uint64(attrData.nFileSizeHigh)<<32 | attrData.nFileSizeLow;
		pFI->created = *(uint64*)&attrData.ftCreationTime;
		pFI->modified = *(uint64*)&attrData.ftLastWriteTime;
		pFI->accessed = *(uint64*)&attrData.ftLastAccessTime;
	}

	if (attrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return FT_DIRECTORY;

	return FT_FILE;
}

//TODO: add this as a configurable list.
// The code linked to this #define is in fact working fine. It is currently
// disabled until it is determined whether it is beneficial to use or not.
//
//#define ZIP_TEST_EARLY_OUT

/**
 *	This method returns the file type of a given path within the file system.
 *	This extended version checks if the file is an archive.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *
 *	@return The type of file
 */
IFileSystem::FileType WinFileSystem::getFileTypeEx( const BW::StringRef & path,
	FileInfo * pFI )
{
	PROFILE_FILE_SCOPED( getFileTypeEx );

	IFileSystem::FileType ft = getFileType(path, pFI);
	if (ft != FT_FILE)
		return ft;

	FILE* pFile;

#ifdef ZIP_TEST_EARLY_OUT
	size_t pos = path.rfind( "." );
	if (pos != BW::string::npos)
	{
		BW::string ext = path.substr( pos + 1 );
		if ( !(ext == "cdata" || ext == "zip"))
			return ft;
	}
	else
		return ft;
#endif

	pFile = this->posixFileOpen( path, "rb" );
	if(!pFile)
		return FT_NOT_FOUND;

	MF_VERIFY( fseek(pFile, 0, SEEK_SET) == 0 ); //needed?
	uint16 magic=0;
	size_t frr = 1;
	frr = fread( &magic, 2, 1, pFile );
	fclose(pFile);
	if (!frr)
		return FT_FILE; //return no file instead?

	if (magic == ZIP_MAGIC)
		return FT_ARCHIVE;

	return ft;
}

extern const char * g_daysOfWeek[];
extern const char * g_monthsOfYear[];

/**
 *	This method converts the given file event time to a string.
 *	The string is in the same format as would be stored in a CVS/Entries file.
 */
BW::string	WinFileSystem::eventTimeToString( uint64 eventTime )
{
	SYSTEMTIME st;
	memset( &st, 0, sizeof(st) );
	FileTimeToSystemTime( (const FILETIME*)&eventTime, &st );
	char buf[32];
	bw_snprintf( buf, sizeof(buf), "%s %s %02d %02d:%02d:%02d %d",
		g_daysOfWeek[st.wDayOfWeek], g_monthsOfYear[st.wMonth], st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wYear );
	return buf;
}


/**
 *	This method reads the contents of a directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A vector containing all the filenames in the directory.
 */
bool WinFileSystem::readDirectory ( IFileSystem::Directory &dir,
								    const BW::StringRef& path )
{
	PROFILE_FILE_SCOPED( win_read_dir );

	InterlockedIncrement( &callsTo_readDirectory_ );

	BWResource::checkAccessFromCallingThread( path, "WinFileSystem::readDirectory" );

	bool done = false;
	WIN32_FIND_DATA findData;

	// Set the max size to be twice that of max path as both
	// basePath_ and path need to be valid file paths but the combined
	// dirstr will not necessarily be a valid file path.
	DECLARE_STACKSTR( dirstr, MAX_PATH * 2 );
	dirstr.append( basePath_ );
	dirstr.append( path.begin(), path.end() );
	dirstr = conformSlash( dirstr );

	if (!dirstr.empty() && *dirstr.rbegin() != '\\' && *dirstr.rbegin() != '/')
		dirstr += "\\*";
	else
		dirstr += "*";

	DECLARE_WSTACKSTR( wdirstr, MAX_PATH * 2 );
	bw_utf8tow( dirstr, wdirstr );
	HANDLE findHandle = FindFirstFile( wdirstr.c_str(), &findData );
	if (findHandle == INVALID_HANDLE_VALUE)
		return false;
		
#if ENABLE_FILE_CASE_CHECKING
	if (checkCase_)
		caseChecker_.check(dirstr);
#endif // ENABLE_FILE_CASE_CHECKING

	DECLARE_STACKSTR( name, MAX_PATH );
	while(!done)
	{
		bw_wtoutf8( findData.cFileName, name );

		if(name != "." && name != "..")
		{
			dir.push_back(name);
#ifdef EDITOR_ENABLED
			if (s_useFileTypeCache_)
			{
				ReadWriteLock::WriteGuard ftcGuard ( fileTypeCacheLock_ );
				fileTypeCache_.insert( std::make_pair( 
					path.empty() ? name : path + "/" + name, 
					findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ?
					FT_DIRECTORY : FT_FILE ) );
			}
#endif
		}

		if(!FindNextFile(findHandle, &findData))
			done = true;
	}

	FindClose(findHandle);
	return true;
}

/**
 *	This method reads the contents of a file via index in a directory.
 *
 *	@param dirPath		Path relative to the base of the filesystem.
 *	@param index		Index into the directory.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr WinFileSystem::readFile(const BW::StringRef & dirPath, uint index)
{
	IFileSystem::Directory dir;
	readDirectory( dir, dirPath);
	if ( index < dir.size() )
	{
		BW::string fileName = dir[index];

		return readFile(dirPath + "/" + fileName);
	}
	else
		return NULL;
}

/**
 *	This method reads the contents of a file
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr WinFileSystem::readFile(const BW::StringRef& path)
{
	InterlockedIncrement( &callsTo_readFile_ );

	BWResource::checkAccessFromCallingThread( path, "WinFileSystem::readFile" );

	FILE* pFile;
	char* buf;
	int len;

	pFile = this->posixFileOpen( path, "rb" );

	if(!pFile)
		return NULL;

	int res = fseek(pFile, 0, SEEK_END);
	if (res)
	{
		ERROR_MSG("WinFileSystem: fseek failed %s\n", path.to_string().c_str());
		fclose( pFile );
		return NULL;
	}

	len = ftell(pFile);
	res = fseek(pFile, 0, SEEK_SET);
	if (res)
	{
		ERROR_MSG("WinFileSystem: fseek failed %s\n", path.to_string().c_str());
		fclose( pFile );
		return NULL;
	}


	if(len <= 0)
	{
		ERROR_MSG("WinFileSystem: No data in %s\n", path.to_string().c_str());
		fclose( pFile );
		return NULL;
	}

	BinaryPtr bp = new BinaryBlock(NULL, len, "BinaryBlock/WinFileSystem");
	buf = (char*)bp->data();

	if(!buf)
	{
		ERROR_MSG("WinFileSystem: Failed to alloc %d bytes for '%s'\n", len, path.to_string().c_str());
		fclose( pFile );
		return NULL;
	}

	size_t frr = 1;
	int extra = 0;
	for (int sofa = 0; sofa < len; sofa += extra)
	{
		extra = std::min( len - sofa, 128*1024 );	// read in 128KB chunks
		frr = fread( buf+sofa, extra, 1, pFile );
		if (!frr) break;
	}
	if (!frr)
	{
		ERROR_MSG("WinFileSystem: Error reading from %s\n", path.to_string().c_str());
		fclose( pFile );
		return NULL;
	}

	fclose( pFile );

	return bp;
}

/**
 *	This method creates a new directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return True if successful
 */
bool WinFileSystem::makeDirectory( const BW::StringRef& path )
{
	InterlockedIncrement( &callsTo_makeDirectory_ );

	BW::string conformedPath = conformSlash(basePath_ + path);
	BW::wstring wpath;
	bw_utf8tow( conformedPath, wpath );
	return CreateDirectory(
		wpath.c_str(), NULL ) == TRUE;
}

/**
 *	This method writes a file to disk.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@param binary	Write as a binary file (Windows only)
 *	@return	True if successful
 */
bool WinFileSystem::writeFile( const BW::StringRef& path,
		BinaryPtr pData, bool binary )
{
	InterlockedIncrement( &callsTo_writeFile_ );

	FILE* pFile = this->posixFileOpen( path, binary?"wb":"w" );
	if (!pFile) return false;

	if (pData->len())
	{
		if (fwrite( pData->data(), pData->len(), 1, pFile ) != 1)
		{
			fclose( pFile );
			CRITICAL_MSG( "WinFileSystem: Error writing to %s. Disk full?\n",
				path.to_string().c_str() );
			// this->eraseFileOrDirectory( path );
			return false;
		}
	}

	fclose( pFile );
	return true;
}

/**
 *	This method writes a file to disk only if it already exists.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@param binary	Write as a binary file (Windows only)
 *	@return	True if successful
 */
bool WinFileSystem::writeFileIfExists( const BW::StringRef& path,
		BinaryPtr pData, bool binary )
{
	FileDataLocation locDat;
	bool doesexist = locateFileData(path, &locDat);
	if(doesexist)
	{
		return writeFile(path, pData, binary);
	}
	return false;
}

/**
 *	This method opens a file using POSIX semantics.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param mode		The mode that the file should be opened in.
 *	@return	True if successful
 */
FILE * WinFileSystem::posixFileOpen( const BW::StringRef& path,
		const char * mode )
{
	PROFILE_FILE_SCOPED( posixFileOpen );

	InterlockedIncrement( &callsTo_posixFileOpen_ );

	//char mode[3] = { writeable?'w':'r', binary?'b':0, 0 };

	BW::string fullPath = conformSlash(basePath_ + path);	

#if ENABLE_FILE_CASE_CHECKING
	if (checkCase_)
		caseChecker_.check(fullPath);
#endif // ENABLE_FILE_CASE_CHECKING

	FILE * pFile = bw_fopen( fullPath.c_str(), mode );

	if (!pFile || ferror(pFile))
	{
		// This case will happen often with multi 
		// res paths, don't flag it as an error
		// ERROR_MSG("WinFileSystem: Failed to open %s\n", fullPath.c_str());
		return NULL;
	}

#if ENABLE_FILE_CASE_CHECKING
	if ( checkCase_ )
	{
		SimpleMutexHolder pathCacheMutexHolder( pathCacheMutex_ );

		if ( invalidPathCache_.count( fullPath ) )
		{
			// We already know this is an invalid path
			return pFile;
		}

		WIN32_FIND_DATA fileInfo;
		BW::vector< BW::StringRef > paths;
		paths.reserve( 16 );
		bw_tokenise( BW::StringRef( fullPath ), "/\\", paths );
		BW::string testPath;
		testPath.reserve( MAX_PATH + 1 );
		testPath.append( paths.front().data(), paths.front().size() );
		testPath.append( "/" );
		BW::wstring wtestPath;
		BW::string ncFileName;
		for ( BW::vector< BW::StringRef >::iterator it = paths.begin() + 1;
			it != paths.end() ; ++it )
		{
			const BW::StringRef & pathPart = *it;
			testPath.append( pathPart.data(), pathPart.length() );

			if ( !validPathCache_.count( testPath ) )
			{
				bw_utf8tow( testPath, wtestPath );
				HANDLE hFile = FindFirstFile( wtestPath.c_str(), &fileInfo );

				if ( hFile == INVALID_HANDLE_VALUE )
					break;

				FindClose( hFile );

				bw_wtoutf8( fileInfo.cFileName, ncFileName );
				if ( pathPart != ncFileName )
				{
					WARNING_MSG( "WinFileSystem: Case mismatch opening %s (%s)\n",
						fullPath.c_str(), ncFileName.c_str() );

					invalidPathCache_.insert( fullPath );

					break;
				}
				else
				{
					validPathCache_.insert( testPath );
				}
			}

			testPath += "/";
		}
	} // if ( checkCase_ )
#endif // ENABLE_FILE_CASE_CHECKING

	return pFile;
}


/**
 *	This method attempts to locate a resource on disk. 
 *	On success the host container resource and the offset 
 *	within it are resolved.
 *
 *	@param path the path of the file to locate
 *	@param pLocationData the data to be retreived on success
 *
 *	@return true if the resource was found.
 */
bool WinFileSystem::locateFileData( const BW::StringRef & path, 
	FileDataLocation * pLocationData )
{
	MF_ASSERT( pLocationData );

	if ( path.empty() )
	{
		return FT_NOT_FOUND;
	}

	char buffer[MAX_PATH + 1];
	BW::StringBuilder builder( buffer, ARRAY_SIZE(buffer) );
	if (basePath_.length() + path.length() >= builder.numFree())
	{
		return FT_NOT_FOUND;
	}
	builder.append( basePath_ );
	builder.append( path );
	const char* filename = builder.string();

	FileType type = getAbsoluteFileType( filename, NULL );
	if (type == FT_NOT_FOUND)
	{
		return false;
	}

	// Since this file system is not a container file system,
	// Simply return the requested resource and 0 offset
	// since the resource exists directly on disk.
	pLocationData->hostResourceID = filename;
	pLocationData->offset = 0;

	return true;
}


/**
 *	This method tries to create a IFileStreamer for the supplied file path
 *
 *	@param inPath the path of the file to stream
 *	@return the file streamer
 */
FileStreamerPtr WinFileSystem::streamFile( const BW::StringRef& path )
{
	FileStreamerPtr pFileStreamer;

	char buffer[MAX_PATH + 1];
	BW::StringBuilder builder( buffer, ARRAY_SIZE(buffer) );
	if (basePath_.length() + path.length() >= builder.numFree())
	{
		return NULL;
	}

	builder.append( basePath_ );
	builder.append( path );
	
	wchar_t filename[MAX_PATH + 1] = { 0 };
	bw_utf8tow( conformSlash( builder.string() ), 
		builder.length(), filename, MAX_PATH );

	HANDLE hFile = ::CreateFileW( filename, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if (hFile != INVALID_HANDLE_VALUE)
	{
		pFileStreamer = new WinFileStreamer( hFile );
	}

	return pFileStreamer;
}


/**
 *	This method resolves the base dependent path to a fully qualified
 * 	path.
 *
 *	@param path			Path relative to the base of the filesystem.
 *	@param checkExists	When true, returns the path if the file or directory
 * 						exists and returns an empty string if it doesn't. 
 * 						When false, returns the fully qualified path even
 * 						if the file or directory doesn't exist.
 *
 *	@return	The fully qualified path or empty string. See checkExists.
 */
BW::string	WinFileSystem::getAbsolutePath( const BW::StringRef& path ) const
{
	return conformSlash(basePath_ + path);
}


/**
 *	This corrects the case of the given path by getting the path that the
 *	operating system has on disk.
 *
 *	@param path		The path to get the correct case of.
 *	@return			The path corrected for case to match what is stored on
 *					disk.  If the path does not exist then it is returned.
 */
BW::string WinFileSystem::correctCaseOfPath(const BW::StringRef &path) const
{
	if (path.empty())
	{
		return path.to_string();
	}

	BW::string fullPath = conformSlash(basePath_ + path);
	BW::string corrected;
	caseChecker_.filenameOnDisk(fullPath, corrected);
	return corrected.substr(basePath_.length(), BW::string::npos);
}


#if ENABLE_FILE_CASE_CHECKING


/**
 *	This enables or disables a filename case check.
 *
 *	@param enable	If true then files are checked for correct case in the
 *					filename, and if false then files are not checked.
 */
void WinFileSystem::checkCaseOfPaths(bool enable)
{
	checkCase_ = enable;
}


#endif // ENABLE_FILE_CASE_CHECKING


/**
 *	This method renames the file or directory specified by oldPath
 *	to be specified by newPath. oldPath and newPath need not be in common.
 *
 *	Returns true on success
 */
bool WinFileSystem::moveFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	BW::string oldFullPath = conformSlash(basePath_ + oldPath);
	BW::string newFullPath = conformSlash(basePath_ + newPath);
	
#if ENABLE_FILE_CASE_CHECKING
	if (checkCase_)
		caseChecker_.check(oldFullPath);
#endif // ENABLE_FILE_CASE_CHECKING

	BW::wstring woldFullPath;
	BW::wstring wnewFullPath;
	bw_utf8tow( oldFullPath, woldFullPath );
	bw_utf8tow( newFullPath, wnewFullPath );

	bool res = !!MoveFileEx(
		woldFullPath.c_str(),
		wnewFullPath.c_str(),
		MOVEFILE_REPLACE_EXISTING );

#ifdef EDITOR_ENABLED
	if (res)
	{
		addModifiedResource( oldPath );
		addModifiedResource( newPath );
	}
#endif
	return res;
}


/**
 *	This method copies the file or directory specified by oldPath
 *	to be specified by newPath. oldPath and newPath need not be in common.
 *
 *	Returns true on success
 */
bool WinFileSystem::copyFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	BW::string oldFullPath = conformSlash(basePath_ + oldPath);
	BW::string newFullPath = conformSlash(basePath_ + newPath);
	
#if ENABLE_FILE_CASE_CHECKING
	if (checkCase_)
		caseChecker_.check(oldFullPath);
#endif // ENABLE_FILE_CASE_CHECKING

	BW::wstring woldFullPath;
	BW::wstring wnewFullPath;
	bw_utf8tow( oldFullPath, woldFullPath );
	bw_utf8tow( newFullPath, wnewFullPath );

	bool res = !!CopyFileEx(
		woldFullPath.c_str(),
		wnewFullPath.c_str(),
		NULL,
		NULL,
		NULL,
		0 );

#ifdef EDITOR_ENABLED
	if (res)
	{
		addModifiedResource( newPath );
	}
#endif
	return res;
}



/**
 *	This method erases the file or directory specified by path.
 *	Directories need not be empty.
 *
 *	Returns true on success
 */
bool WinFileSystem::eraseFileOrDirectory( const BW::StringRef & path )
{
	bool res = false;

	FileType ft = this->getFileType( path, NULL );
	if (ft == FT_FILE)
	{
		BW::string fullPath = conformSlash(basePath_+path);
#if ENABLE_FILE_CASE_CHECKING
		if (checkCase_)
		{
			caseChecker_.check(fullPath);
		}
#endif // ENABLE_FILE_CASE_CHECKING

		BW::wstring wfullPath;
		bw_utf8tow( fullPath, wfullPath );
		res = !!DeleteFile( wfullPath.c_str() );
	}
	else if (ft == FT_DIRECTORY)
	{
		Directory d;
		this->readDirectory( d, path );
		for (uint i = 0; i < d.size(); i++)
		{
			if (!this->eraseFileOrDirectory( path + "/" + d[i] ))
				return false;
		}
		BW::string conformedPath = conformSlash(basePath_+path);
		BW::wstring wconformedPath;
		bw_utf8tow( conformedPath, wconformedPath );
		res = !!RemoveDirectory( wconformedPath.c_str() );
	}
	
#ifdef EDITOR_ENABLED
	if (res)
	{
		addModifiedResource( path );
	}
#endif
	return res;
}


/**
 *	This method returns a IFileSystem pointer to a copy of this object.
 *
 *	@return	a copy of this object
 */
FileSystemPtr WinFileSystem::clone()
{
	return new WinFileSystem( basePath_ );
}

/**
 *	This method enables/disables background monitoring of the file system
 *	for changes made at run time.
 *
 *	@param enable	If true then the file system will be monitored for changes.
 */
void WinFileSystem::enableModificationMonitor( bool enable )
{
#if !CONSUMER_CLIENT_BUILD
	if (enable && !monitorThread_)
	{
		monitorThread_ = new MonitorThread( this );
	}
	else if (!enable && monitorThread_)
	{
		bw_safe_delete( monitorThread_ );
	}
#endif // CONSUMER_CLIENT_BUILD
}

/**
 *	This method flushes all recently detected file system changes to the given
 *	list of listeners.
 *
 *	@param listeners	list of listeners to post events to
 */
void WinFileSystem::flushModificationMonitor(
	const ModificationListeners& listeners )
{
#if !CONSUMER_CLIENT_BUILD
	if (monitorThread_)
	{
		monitorThread_->flushModifications( listeners );
	}
#endif // CONSUMER_RELEASE

#ifdef EDITOR_ENABLED
	// erase any modified directories from the cache
	ReadWriteLock::WriteGuard drWriteGuard ( modifiedResourcesLock_ );
	ReadWriteLock::WriteGuard ftcGuard (fileTypeCacheLock_ );
	for (size_t i = 0; i < modifiedResources_.size(); ++i)
	{
		fileTypeCache_.erase( modifiedResources_[i] );
	}
	modifiedResources_.clear();
#endif
}


/**
 *	This method tell the monitor thread to ignore the 
 *	file with the specified action
 *
 *	@param fileName			the file name to be ignored.
 *	@param action			the match action of the file.
 *	@param oneTimeOnly		if false, then this file will be always ignored,
 *	@return					if it's handled.
 */
bool WinFileSystem::ignoreFileModification( const BW::string& fileName, 
								ResourceModificationListener::Action action,
								bool oneTimeOnly )
{
#if !CONSUMER_CLIENT_BUILD
	if (monitorThread_)
	{
		if (this->getFileType( fileName, NULL ) == FT_FILE)
		{
			monitorThread_->addIgnoreFiles( fileName, action, oneTimeOnly );
			return true;
		}
	}
#endif // CONSUMER_RELEASE
	return false;
}


/**
 *	This method asks the monitor thread to check if a file is in the
 *	 modification queue.
 *
 *	@param fileName			the file name to be checked.
 *	@return					if it's found.
 */
bool WinFileSystem::hasPendingModification( const BW::string& fileName )
{
	BW_GUARD;
#if !CONSUMER_CLIENT_BUILD
	return monitorThread_ && monitorThread_->hasPendingModification( fileName );
#else
	return false;
#endif
}


#ifdef EDITOR_ENABLED
void WinFileSystem::addModifiedResource( const BW::StringRef& resourceID )
{
	ReadWriteLock::WriteGuard drGuard ( modifiedResourcesLock_ );
	modifiedResources_.push_back( resourceID.to_string() );
}
#endif


// ----------------------------------------------------------------------------
// Section: WinFileSystem::MonitorThread
// ----------------------------------------------------------------------------
#if !CONSUMER_CLIENT_BUILD
WinFileSystem::MonitorThread::MonitorThread( WinFileSystem* fs ) :
	fs_(fs), 
	pathHandle_(INVALID_HANDLE_VALUE)
{
	MF_ASSERT( fs != NULL );

	if (BWUtil::isAbsolutePath( fs_->basePath_ ))
	{
		fullPath_ = fs_->basePath_;
	}
	else
	{
		char cwd[MAX_PATH+1];
		_getcwd( cwd, MAX_PATH );
		fullPath_ = BWUtil::normalisePath( cwd ) + "/" + fs_->basePath_;
	}

	threadExitEvent_ = ::CreateEvent( NULL, TRUE, FALSE, NULL );
	pathHandle_ = CreateFileW( bw_utf8tow( fullPath_ ).c_str(),
		FILE_LIST_DIRECTORY | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING,  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );
	if (pathHandle_ != INVALID_HANDLE_VALUE)
	{
		this->SimpleThread::init( s_threadFunc, this, "File System Monitor" );
	}
	else
	{
		ERROR_MSG( "WinFileSystem::MonitorThread: "
			"could not open path %s.\n", fullPath_.c_str() );
	}
}

WinFileSystem::MonitorThread::~MonitorThread()
{
	if (pathHandle_ != INVALID_HANDLE_VALUE)
	{
		::SetEvent( threadExitEvent_ );
		::WaitForSingleObject( this->SimpleThread::handle(), INFINITE );
		::CloseHandle( pathHandle_ );
		::CloseHandle( threadExitEvent_ );
		pathHandle_ = INVALID_HANDLE_VALUE;
		threadExitEvent_ = INVALID_HANDLE_VALUE;
	}
}


void WinFileSystem::MonitorThread::flushModifications(
	const ModificationListeners& listeners )
{
	SimpleMutexHolder smh( modifiedFilesMutex_ );
	for (ModificationMap::iterator modIt = modifiedFiles_.begin();
		modIt != modifiedFiles_.end(); )
	{
		ModificationMap::iterator current = modIt;
		++modIt;

		bool ignoreFile = false;
		// Check if this file is in ignore list.
		{
			SimpleMutexHolder smh( ignoreFilesMutex_ );
			BW::vector<IgnoreFileContent>::iterator igEntryIt = ignoreFiles_.begin();
			for (;igEntryIt != ignoreFiles_.end(); ++igEntryIt)
			{
				if ((*igEntryIt).fileName_ == (*current).first && 
					(*igEntryIt).action_ == (*current).second)
				{
					if ((*igEntryIt).oneTimeOnly_)
					{
						ignoreFiles_.erase( igEntryIt );
					}
					ignoreFile = true;
					break;
				}
			}
		}

		if (!ignoreFile)
		{
			for (ModificationListeners::const_iterator listenerIt = listeners.begin();
				listenerIt != listeners.end(); ++listenerIt)
			{
				(*listenerIt)->onResourceModified( fs_->basePath_, (*current).first, 
					(*current).second == ResourceModificationListener::ACTION_MODIFIED_DELETED ?
					ResourceModificationListener::ACTION_DELETED : (*current).second );
			}

#ifdef EDITOR_ENABLED
			fs_->addModifiedResource( (*current).first );
#endif

			// This directory has changed so we store it in the MFS to
			// be removed and readded with its new contents.
			BW::string dirPath = BWResource::getFilePath( (*current).first );
			MultiFileSystem::addModifiedDirectory( dirPath );
#ifdef EDITOR_ENABLED
			MultiFileSystem::addModifiedDirectory( fs_->basePath_ + dirPath );
#endif
		}

		modifiedFiles_.erase( current );
	}
}


/**
 *	This method ignore the file with the specified action
 *
 *	@param fileName			the file name to be ignored.
 *	@param action			the match action of the file.
 *	@param oneTimeOnly		if false, then this file will be always ignored,
 */
void WinFileSystem::MonitorThread::addIgnoreFiles( const BW::string& fileName, 
							ResourceModificationListener::Action action,
							bool oneTimeOnly )
{
	BW_GUARD;
	SimpleMutexHolder smh( ignoreFilesMutex_ );
	BW::vector<IgnoreFileContent>::iterator igEntryIt = ignoreFiles_.begin();
	for (;igEntryIt != ignoreFiles_.end(); ++igEntryIt)
	{
		IgnoreFileContent& fileContent = *igEntryIt;
		if (fileContent.fileName_ == fileName && 
			fileContent.action_ == action)
		{
			if (!oneTimeOnly && fileContent.oneTimeOnly_)
			{
				//overwrite temp one.
				fileContent.oneTimeOnly_ = false;
			}
			return;
		}
	}

	ignoreFiles_.push_back( IgnoreFileContent( fileName, action, oneTimeOnly) );
}


bool WinFileSystem::MonitorThread::hasPendingModification( const BW::string& fileName )
{
	BW_GUARD;
	SimpleMutexHolder smh( modifiedFilesMutex_ );

	for (ModificationMap::iterator itr = modifiedFiles_.begin();
		itr != modifiedFiles_.end(); ++itr)
	{
		if (itr->first == fileName)
		{
			return true;
		}
	}

	return false;
}


void WinFileSystem::MonitorThread::threadFunc()
{
	DEBUG_MSG( "WinFileSystem::MonitorThread: begin (path='%s')\n",
		fullPath_.c_str() );

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	while (pathHandle_ != INVALID_HANDLE_VALUE)
	{
		PROFILE_FILE_SCOPED( readDirChanges );
		FILE_NOTIFY_INFORMATION buffer[1024];
		DWORD bytesRead = 0;
		BOOL res = ::ReadDirectoryChangesW( pathHandle_, 
			buffer,
			sizeof(buffer),
			TRUE,
			FILE_NOTIFY_CHANGE_DIR_NAME    |
			FILE_NOTIFY_CHANGE_FILE_NAME   |
			FILE_NOTIFY_CHANGE_LAST_WRITE	|
			FILE_NOTIFY_CHANGE_CREATION,
			&bytesRead,
			&overlapped,
			NULL );

		if (!res)
		{
			return;
		}

		HANDLE handles[2] = { overlapped.hEvent, threadExitEvent_ };
		// wait for either threadexitevent, or ovelrapped.event to be signalled.
		DWORD waitResult = ::WaitForMultipleObjects( 2, &handles[0], FALSE, INFINITE ); 
		// if the event that was signalled was the exit thread event
		if (waitResult == WAIT_OBJECT_0 + 1) 
		{
			// thread exit signaled
			::CloseHandle( overlapped.hEvent );
			return;
		}
		MF_ASSERT( waitResult == WAIT_OBJECT_0 );

		if( !::GetOverlappedResult( pathHandle_, &overlapped, &bytesRead, FALSE ))
		{
			// failed, go round again.
			continue;
		}

		if (bytesRead)
		{
			const char *cur = reinterpret_cast<const char *>(buffer);
			PROFILE_FILE_SCOPED( Process_dir_changes );
			do
			{
				const FILE_NOTIFY_INFORMATION &info = 
					*reinterpret_cast<const FILE_NOTIFY_INFORMATION*>( cur );

				size_t filenameLen = info.FileNameLength / sizeof(wchar_t);
				wchar_t filename[1024];
				wcsncpy( filename, info.FileName, filenameLen );
				filename[filenameLen] = NULL;

				ResourceModificationListener::Action action;
				if (info.Action == FILE_ACTION_RENAMED_NEW_NAME ||
					info.Action == FILE_ACTION_ADDED )
				{
					action = ResourceModificationListener::ACTION_ADDED;
				}
				else if (info.Action == FILE_ACTION_RENAMED_OLD_NAME ||
					info.Action == FILE_ACTION_REMOVED )
				{
					action = ResourceModificationListener::ACTION_DELETED;
				}
				else
				{
					action = ResourceModificationListener::ACTION_MODIFIED;
				}

				BW::string narrow = bw_wtoutf8( filename );
				BWUtil::sanitisePath( narrow );
						
				SimpleMutexHolder smh( modifiedFilesMutex_ );
				ModificationMap::iterator modEntryIt = modifiedFiles_.find(narrow);
				if (modEntryIt == modifiedFiles_.end() )
				{
					modifiedFiles_.insert( std::make_pair(narrow, action) );
				}
				else if ((*modEntryIt).second == ResourceModificationListener::ACTION_ADDED)
				{
					if (action == ResourceModificationListener::ACTION_DELETED)
					{
						(*modEntryIt).second = ResourceModificationListener::ACTION_MODIFIED_DELETED;
					}
				}
				else if ((*modEntryIt).second == ResourceModificationListener::ACTION_DELETED)
				{
					if (action == ResourceModificationListener::ACTION_ADDED)
					{
						(*modEntryIt).second = ResourceModificationListener::ACTION_MODIFIED;
					}
				}
				else if ((*modEntryIt).second == ResourceModificationListener::ACTION_MODIFIED)
				{
					(*modEntryIt).second = action;
				}
				else if ((*modEntryIt).second == ResourceModificationListener::ACTION_MODIFIED_DELETED)
				{
					if (action == ResourceModificationListener::ACTION_ADDED)
					{
						(*modEntryIt).second = ResourceModificationListener::ACTION_ADDED;
					}
				}

				if (info.NextEntryOffset == 0)
				{
					break;
				}

				cur = cur + info.NextEntryOffset;
			}
			while (true);
		}
	}
}

//static 
void WinFileSystem::MonitorThread::s_threadFunc( void* arg )
{
	MonitorThread* monitorThread = (MonitorThread*)arg;
	monitorThread->threadFunc();
}

#endif // CONSUMER_CLIENT_BUILD

BW_END_NAMESPACE

// win_file_system.cpp
