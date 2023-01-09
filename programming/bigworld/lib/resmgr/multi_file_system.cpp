#include "pch.hpp"
#include "bwresource.hpp"
#include "multi_file_system.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "file_streamer.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MultiFileSystem
// -----------------------------------------------------------------------------

BW::vector<BW::string> MultiFileSystem::modifiedDirectories_;
ReadWriteLock MultiFileSystem::modifiedDirectoriesLock_;


/**
 *	This is the constructor.
 */
MultiFileSystem::MultiFileSystem()
{
	BW_GUARD;
}

MultiFileSystem::MultiFileSystem( const MultiFileSystem & other ) :
	IFileSystem( other )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard otherGuard ( other.baseFileSystemsLock_ );
	baseFileSystems_.reserve( other.baseFileSystems_.size() );
	*this = other;
}

MultiFileSystem & MultiFileSystem::operator=( const MultiFileSystem & other )
{
	BW_GUARD;
	if (&other != this)
	{
		ReadWriteLock::WriteGuard bfsGuard ( baseFileSystemsLock_ );
		baseFileSystems_.clear();
		ReadWriteLock::ReadGuard otherGuard ( other.baseFileSystemsLock_ );
		for (FileSystemVector::const_iterator it = other.baseFileSystems_.begin();
			it != other.baseFileSystems_.end(); 
			++it)
		{
			baseFileSystems_.push_back( (*it)->clone() );
		}
	}
	return *this;
}


/**
 *	This is the destructor.
 */
MultiFileSystem::~MultiFileSystem()
{
	BW_GUARD;
	cleanUp();
}

void MultiFileSystem::cleanUp()
{
	BW_GUARD;
	ReadWriteLock::WriteGuard bfsGuard ( baseFileSystemsLock_ );
	baseFileSystems_.clear();
}


/**
 *	This method adds a base filesystem to the search path.
 *	FileSystems will be searched in the order they are added.
 *	Note that FileSystems are expected to be allocated on the
 *	heap, and are owned by this object. They will be freed by
 *	the MultiFileSystem when it is destroyed.
 *
 *	@param pFileSystem	Pointer to the FileSystem to add.
 *	@param index	Indicates where to insert the file system in to the current set.
 */
void MultiFileSystem::addBaseFileSystem( FileSystemPtr pFileSystem, int index )
{
	BW_GUARD;
	ReadWriteLock::WriteGuard bfsGuard ( baseFileSystemsLock_ );
	if (uint(index) >= baseFileSystems_.size())
		baseFileSystems_.push_back( pFileSystem );
	else
		baseFileSystems_.insert( baseFileSystems_.begin()+index, pFileSystem );
}

/**
 *	This method removes a base file system specified by its index
 */
void MultiFileSystem::delBaseFileSystem( int index )
{
	BW_GUARD;
	ReadWriteLock::WriteGuard bfsGuard ( baseFileSystemsLock_ );
	if (uint(index) >= baseFileSystems_.size())
	{
		return;
	}
	FileSystemVector::iterator it = baseFileSystems_.begin() + index;
	baseFileSystems_.erase( it );
}

	
/**
 *	This method reads the contents of a file.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *  @param skipCacheChecks	Don't check fileTypeCache_ for the existence of a file.
 *
 *	@return A BinaryBlock object containing the file data.
 */
IFileSystem::FileType MultiFileSystem::getFileType( const BW::StringRef& path,
	FileInfo * pFI )
{
	BW_GUARD;

	IFileSystem::FileType ft = FT_NOT_FOUND;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );

#ifdef EDITOR_ENABLED
	for (FileSystemVector::const_iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		ft = (*it)->getFileTypeCache( path, pFI );
		if (ft != FT_NOT_FOUND) return ft;
	}
#endif

	for (FileSystemVector::const_iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		ft = (*it)->getFileType( path, pFI );
		if (ft != FT_NOT_FOUND) break;
	}

	return ft;
}

IFileSystem::FileType MultiFileSystem::getFileTypeEx( const BW::StringRef& path,
	FileInfo * pFI )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end(); 
		++it)
	{
		FileType ft = (*it)->getFileTypeEx( path, pFI );
		if (ft != FT_NOT_FOUND) return ft;
	}
	return FT_NOT_FOUND;
}

/**
 *	This method converts a file event time to a string - only the first file system is
 *	asked.
 *
 *	@param eventTime The file event time to convert.
 */
BW::string	MultiFileSystem::eventTimeToString( uint64 eventTime )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	if (!baseFileSystems_.empty())
		return baseFileSystems_[0]->eventTimeToString( eventTime );

	return BW::string();
}

/**
 *	This method reads the contents of a directory. It uses a map to cache
 *  the directories as an optimisation. 
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A vector containing all the filenames in the directory.
 */
bool MultiFileSystem::readDirectory( Directory& dir, 
									 const BW::StringRef & path )
{
	BW_GUARD;
	PROFILER_SCOPED( MultiFileSystem_readDirectory );

	// check in the cache
	{
		ReadWriteLock::ReadGuard dcGuard ( dirCacheLock_ );
		StringRefDirHashMap::const_iterator it = dirCache_.find( path );
		if (it != dirCache_.end())
		{
			dir = it->second;
			return true;
		}
	}

	if (!readDirectoryUncached( dir, path ))
		return false;
	
	BW::StringRef dirPath = path;
	if (!dirPath.empty() && dirPath[dirPath.length() - 1] == '/')
	{
		// remove the trailing '/'
		dirPath = dirPath.substr( 0, dirPath.length() - 1 ); 
	}
	// add to the cache
	{
		ReadWriteLock::WriteGuard dcGuard ( dirCacheLock_ );
		dirCache_.insert( std::make_pair( dirPath, dir ) );
	}

	return true;
}

/**
 *	This method reads the contents of a file via directory index.
 *
 *	@param dirPath	Path relative to the base of the filesystem.
 *	@param index	Index into directory.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr MultiFileSystem::readFile(const BW::StringRef & dirPath, uint index)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		BinaryPtr pBinary = (*it)->readFile( dirPath, index );
		if (pBinary)
		{
			return pBinary;
		}
	}
	return NULL;
}


/**
 *	This method reads the contents of a file.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr MultiFileSystem::readFile(const BW::StringRef& path)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		BinaryPtr pBinary = (*it)->readFile( path );
		if  (pBinary)
		{
			return pBinary;
		}
	}
	return NULL;
}

/**
 *	This method reads the contents of a file.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param ret		Returned vector of binary blocks 
 */
void MultiFileSystem::collateFiles(const BW::StringRef& path,
								BW::vector<BinaryPtr>& ret)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end(); 
		++it)
	{
		BinaryPtr pBinary = (*it)->readFile( path );
		if (pBinary)
		{
			ret.push_back( pBinary );
		}
	}	
}


/**
 *	This method creates a new directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return True if successful
 */
bool MultiFileSystem::makeDirectory(const BW::StringRef& path)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->makeDirectory( path ))
		{
			return true;
		}
	}
	return false;
}

/**
 *	This method writes a file to disk.
 *	Iterate through the base file systems until the given path is found.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@param binary	Write as a binary file (Windows only)
 *	@return	True if successful
 */
bool MultiFileSystem::writeFile(const BW::StringRef& path,
		BinaryPtr pData, bool binary)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end(); 
		++it)
	{
		if ((*it)->writeFile( path, pData, binary ))
		{
			this->addModifiedDirectory( BWResource::getFilePath( path ) );
			return true;
		}
	}
	return false;
}

/**
 *	This method writes a file to disk.
 *	Iterate through the base file systems until the given path is found.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@param binary	Write as a binary file (Windows only)
 *	@return	True if successful
 */
bool MultiFileSystem::writeFileIfExists(const BW::StringRef& path,
		BinaryPtr pData, bool binary)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end(); 
		++it)
	{
		if ((*it)->writeFileIfExists( path, pData, binary ))
		{
			this->addModifiedDirectory( BWResource::getFilePath( path ) );
			return true;
		}
	}
	return false;
}

/**
 *	This method moves a file around.
 *	Iterate through the base file systems until the given path is found.
 *
 *	@param oldPath		The path to move from.
 *	@param newPath		The path to move to.
 *	@return	True if successful
 */
bool MultiFileSystem::moveFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->moveFileOrDirectory( oldPath, newPath ))
		{
			this->addModifiedDirectory( BWResource::getFilePath( oldPath ) );
			this->addModifiedDirectory( BWResource::getFilePath( newPath ) );
			return true;
		}
	}
	return false;
}

#if defined( _WIN32 )
/**
 *	This method copies a file around.
 *	Iterate through the base file systems until the given path is found.
 *
 *	@param oldPath		The path to copy from.
 *	@param newPath		The path to copy to.
 *	@return	True if successful
 */
bool MultiFileSystem::copyFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->copyFileOrDirectory( oldPath, newPath ))
		{
			this->addModifiedDirectory( BWResource::getFilePath( newPath ) );
			return true;
		}
	}
	return false;
}
#endif // defined( _WIN32 )


/**
 *	This method erases a given file or directory from the first
 *	base file system that it is found in.
 *	Iterate through the base file systems until the given path is found.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@return	True if successful, otherwise false.
 */
bool MultiFileSystem::eraseFileOrDirectory( const BW::StringRef & path )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->eraseFileOrDirectory( path ))
		{
			this->addModifiedDirectory( BWResource::getFilePath( path ) );
			return true;
		}
	}

	return false;
}


/**
 *	This method opens the given file using posix semantics.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param mode		The mode that the file should be opened in.
 *
 *	@return	True if successful
 */
FILE * MultiFileSystem::posixFileOpen( const BW::StringRef & path,
	const char * mode )
{
	BW_GUARD;
	FILE * pFile = NULL;

	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		pFile = (*it)->posixFileOpen( path, mode );
		if (pFile != NULL) 
		{
			if (strpbrk ( mode, "wa" ))
				this->addModifiedDirectory( BWResource::getFilePath( path ) );
			break;
		}
	}

	/*
	 //	This is useful to debug the order in which resources are 
	 //	searched for and loaded from the res path. I've used it 
	 //	more than once to trace how python modules are loaded.

	if (pFile == NULL)
	{
		DEBUG_MSG("Could not open: %s.\n", path.c_str());
	}
	else
	{
		DEBUG_MSG("Found file: %s.\n", path.c_str());
	}
	*/

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
bool MultiFileSystem::locateFileData( const BW::StringRef & inPath, 
	FileDataLocation * pLocationData )
{
	BW_GUARD;
	
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->locateFileData( inPath, pLocationData ))
		{
			return true;
		}
	}

	return false;
}


/**
 *	This method opens a file streamer for the given file
 *
 *	@param path		Path relative to the base of the filesystem
 *	@return	FileStreamerPtr that can read the file 
 *	@note not all files can be streamed, if this method returns NULL
 *	you may still be able to open the file
 */
FileStreamerPtr MultiFileSystem::streamFile( const BW::StringRef& path )
{
	BW_GUARD;
	FileStreamerPtr pFile;

	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		pFile = (*it)->streamFile( path );
		if (pFile.exists()) 
		{
			break;
		}
	}

	return pFile;
}


/**
 *	This method resolves the base dependent path to a fully qualified
 * 	path. Resolves it based on the first file system.
 *
 *	@param path			Path relative to the base of the filesystem.
 *
 *	@return	The fully qualified path.
 */
BW::string	MultiFileSystem::getAbsolutePath( const BW::StringRef& path ) const
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	return !baseFileSystems_.empty() ?
		baseFileSystems_[0]->getAbsolutePath( path ) : path.to_string();
}

/**
 *	This method resolves the base dependent path to a fully qualified
 * 	path. If returns the full path of the first file-system that has
 * 	contains the given file (or directory).
 *
 *	@param 	path	Path relative to the base of the filesystem.
 *
 *	@return	The type of the file (or directory) that matched the path.
 */
IFileSystem::FileType MultiFileSystem::resolveToAbsolutePath( 
		BW::string& path ) const
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::const_iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		IFileSystem::FileType type = (*it)->getFileType( path );
		if (type != FT_NOT_FOUND)
		{
			BW::string fullPath = (*it)->getAbsolutePath( path );
			// Check that it's not the zip filesystem.
			// __kyl__ (4/7/2008) A bit of a cheat here but I'm assuming
			// no one would want to resolve a full path that points to 
			// something inside a zip file.
			if (!fullPath.empty()) 
			{
				path = fullPath;

				return type;  
			}
		}
	}

	return FT_NOT_FOUND;
}


/**
 *	This corrects the case of the given path by getting the path that the
 *	operating system has on disk.
 *
 *	@param path		The path to get the correct case of.
 *	@return			The path corrected for case to match what is stored on
 *					disk.  If the path does not exist then it is returned.
 */
BW::string MultiFileSystem::correctCaseOfPath(const BW::StringRef &path) const
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::const_iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		BW::string corrected = (*it)->correctCaseOfPath( path );
		if (corrected != path)
			return corrected;
	}
	return path.to_string();
}

#if ENABLE_FILE_CASE_CHECKING
/**
 *	This enables or disables a filename case check.
 *
 *	@param enable	If true then files are checked for correct case in the
 *					filename, and if false then files are not checked.
 */
void MultiFileSystem::checkCaseOfPaths(bool enable)
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		(*it)->checkCaseOfPaths( enable );
	}
}


#endif // ENABLE_FILE_CASE_CHECKING


/**
 *	This method returns a IFileSystem pointer to a copy of this object.
 *
 *	@return	a copy of this object
 */
FileSystemPtr MultiFileSystem::clone()
{
	BW_GUARD;
	return new MultiFileSystem( *this );
}

/**
 *	This method enables/disables background monitoring of the file system
 *	for changes made at run time.
 *
 *	@param enable	If true then the file system will be monitored for changes.
 */
void MultiFileSystem::enableModificationMonitor( bool enable )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		(*it)->enableModificationMonitor( enable );
	}
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
bool MultiFileSystem::ignoreFileModification( const BW::string& fileName, 
					ResourceModificationListener::Action action,
					bool oneTimeOnly )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end();
		++it)
	{
		if ((*it)->ignoreFileModification( fileName, action, oneTimeOnly ))
		{
			return true;
		}
	}
	return false;
}


/**
 *	This method asks the monitor thread to check if a file is in the
 *	modification queue.
 *
 *	@param fileName			the file name to be checked.
 *	@return					if it's found.
 */
bool MultiFileSystem::hasPendingModification( const BW::string& fileName )
{
	BW_GUARD;
	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );

	for (FileSystemVector::iterator it = baseFileSystems_.begin();
		it != baseFileSystems_.end(); ++it)
	{
		if ((*it)->hasPendingModification( fileName ))
		{
			return true;
		}
	}

	return false;
}

/**
 *	This method flushes all recently detected file system changes to the given
 *	list of listeners. If the flushModificationMonitor has detected changes 
 *	to directories, then they will be re-added in the dirCache_.
 *
 *	@param listeners	list of listeners to post events to
 */
void MultiFileSystem::flushModificationMonitor(
	const ModificationListeners& listeners )
{
	BW_GUARD;

	{
		ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
		for (FileSystemVector::iterator it = baseFileSystems_.begin();
			it != baseFileSystems_.end();
			++it)
		{
			(*it)->flushModificationMonitor( listeners );
		}
	}

	// erase any modified directories from the cache
	ReadWriteLock::WriteGuard modirWriteGuard ( modifiedDirectoriesLock_ );
	ReadWriteLock::WriteGuard cacheWriteGuard ( dirCacheLock_ );
	for (size_t i = 0; i < modifiedDirectories_.size(); ++i)
	{
		StringRefDirHashMap::iterator it = 
			dirCache_.find( modifiedDirectories_[i] );
		if (it != dirCache_.end())
		{
			dirCache_.erase(it);
		}
	}
	modifiedDirectories_.clear();
}


void MultiFileSystem::addModifiedDirectory( const BW::StringRef& temp )
{
	BW::StringRef dirPath = temp;
	if (!dirPath.empty() && dirPath[dirPath.length() - 1] == '/')
	{
		// remove the trailing '/'
		dirPath = dirPath.substr( 0, dirPath.length() - 1 ); 
	}
	{
		ReadWriteLock::WriteGuard mdGuard ( modifiedDirectoriesLock_ );
		modifiedDirectories_.push_back( dirPath.to_string() );
	}
	
}


bool MultiFileSystem::readDirectoryUncached(IFileSystem::Directory& dir, 
											const BW::StringRef& path)
{
	BW_GUARD;

	bool res = false;

	ReadWriteLock::ReadGuard bfsGuard ( baseFileSystemsLock_ );
	for (FileSystemVector::iterator it = baseFileSystems_.begin(); 
		it != baseFileSystems_.end(); 
		++it)
	{
		res |= (*it)->readDirectory( dir, path );
	}

	return res;
}


// -----------------------------------------------------------------------------
// Section: NativeFileSystem
// -----------------------------------------------------------------------------

BW_END_NAMESPACE

#ifdef _WIN32
#include "win_file_system.hpp"
#else
#include "unix_file_system.hpp"
#endif

BW_BEGIN_NAMESPACE

/**
 *	Create a native file system
 */
FileSystemPtr NativeFileSystem::create( const BW::StringRef & path )
{
	BW_GUARD;
#ifdef _WIN32
	return new WinFileSystem( path );
#else
	return new UnixFileSystem( path );
#endif
}

/**
 * 	Same as IFileSystem::getFileType() except accepting an absolute path instead
 * 	of a res-relative path.
 */
IFileSystem::FileType NativeFileSystem::getAbsoluteFileType( 
		const BW::StringRef & path, IFileSystem::FileInfo * pFI )
{
	BW_GUARD;
#ifdef _WIN32
	return WinFileSystem::getAbsoluteFileType( path, pFI );
#else
	return UnixFileSystem::getAbsoluteFileType( path, pFI );
#endif
}

// don't ask...
const char * g_daysOfWeek[] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char * g_monthsOfYear[] =
	{ "Bad", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

BW_END_NAMESPACE

// multi_file_system.cpp
