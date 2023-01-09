#include "pch.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>

#include "cstdmf/debug.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/profiler.hpp"

#include "bwresource.hpp"
#include "unix_file_system.hpp"
#include "file_handle_streamer.hpp"

#include "bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

/**
 *	This is the constructor.
 *
 *	@param basePath		Base path of the filesystem, including trailing '/'
 */
UnixFileSystem::UnixFileSystem(const BW::StringRef& basePath) :
	basePath_( basePath.data(), basePath.size() )
{
	if (!basePath_.empty() && *basePath_.rbegin() != '/' && *basePath_.rbegin() != '\\')
	{
		basePath_ += '/';
	}
}

/**
 *	This is the destructor.
 */
UnixFileSystem::~UnixFileSystem()
{
}


/**
 *	This method returns the file type of a given path within the file system.
 *
 *	@param path		Path relative to the base of the filesystem.
 *  @param pFI		Pointer to a FileInfo structure to fill with the file's
 *                  details such as size, modification time..etc.
 *
 *	@return The type of file as a FileType enum value.
 */
IFileSystem::FileType UnixFileSystem::getFileType( const BW::StringRef& path,
	FileInfo * pFI )
{
	BWResource::checkAccessFromCallingThread( path,
			"UnixFileSystem::getFileType" );

	return UnixFileSystem::getAbsoluteFileTypeInternal( basePath_ + path, pFI );
}


/**
 *	This method returns the file type of a absolute path.
 *
 *	@param	path	Absolute path.
 * 	@param	pFI		Output parameter. If non-NULL, will contain additional 
 * 					information about the file.	
 *
 *	@return The type of file
 */
IFileSystem::FileType UnixFileSystem::getAbsoluteFileType( 
		const BW::StringRef& path, FileInfo * pFI )
{
	BWResource::checkAccessFromCallingThread( path,
			"UnixFileSystem::getAbsoluteFileType" );

	return UnixFileSystem::getAbsoluteFileTypeInternal( path, pFI );	
}

/**
 *	This method returns the file type of a absolute path. Does not check
 * 	access from calling thread.
 *
 *	@param	path	Absolute path.
 * 	@param	pFI		Output parameter. If non-NULL, will contain additional 
 * 					information about the file.	
 *
 *	@return The type of file
 */
inline IFileSystem::FileType UnixFileSystem::getAbsoluteFileTypeInternal( 
		const BW::StringRef& path, FileInfo * pFI )
{
	PROFILER_SCOPED( getAbsoluteFileTypeInternal );
	struct stat s;

	FileType ft = FT_NOT_FOUND;

	BWConcurrency::startMainThreadIdling();
	bool good = stat(path.to_string().c_str(), &s) == 0;
	BWConcurrency::endMainThreadIdling();

	if (good)
	{
		if(S_ISREG(s.st_mode))
		{
			ft = FT_FILE;
		}
		if(S_ISDIR(s.st_mode))
		{
			ft = FT_DIRECTORY;
		}

		if (ft != FT_NOT_FOUND && pFI != NULL)
		{
			pFI->size = s.st_size;
			pFI->created = std::min( s.st_ctime, s.st_mtime );
			pFI->modified = s.st_mtime;
			pFI->accessed = s.st_atime;
		}
	}

	return ft;
}


/*
 *	Overvide from IFileSystem. Also returns whether the file is an archive.
 */
IFileSystem::FileType UnixFileSystem::getFileTypeEx( const BW::StringRef& path,
	FileInfo * pFI )
{
	FileType ft = this->UnixFileSystem::getFileType( path, pFI );

	if (ft == FT_FILE)
	{
		PROFILER_SCOPED( getFileTypeEx_verifyZip );
		FILE* pFile = this->posixFileOpen( path, "rb" );
		if( pFile )
		{
			uint16 magic=0;
			int frr = fread( &magic, sizeof( magic ), 1, pFile );
			fclose(pFile);

			if ((frr == 1) && (magic == ZIP_MAGIC))
			{
				ft = FT_ARCHIVE;
			}
		}
	}

	return ft;
}


/**
 *	Convert the event time into a CVS/Entries style string
 */
BW::string	UnixFileSystem::eventTimeToString( uint64 eventTime )
{
	struct tm etm;
	memset( &etm, 0, sizeof(etm) );

	time_t eventUnixTime = time_t(eventTime);
	gmtime_r( &eventUnixTime, &etm );

	char buf[32];
/*	sprintf( buf, "%s %s %02d %02d:%02d:%02d %d",
		g_daysOfWeek[etm.tm_wday], g_monthsOfYear[etm.tm_mon+1], etm.tm_mday,
		etm.tm_hour, etm.tm_min, etm.tm_sec, 1900 + etm.tm_year );			*/
	asctime_r( &etm, buf );
	if (buf[24] == '\n') buf[24] = 0;
	return buf;
}


/**
 *	This method reads the contents of a directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A vector containing all the filenames in the directory.
 */
bool UnixFileSystem::readDirectory( IFileSystem::Directory& dir,
								    const BW::StringRef& path )
{
	PROFILER_SCOPED( readDirectory );
	BWResource::checkAccessFromCallingThread( path,
			"UnixFileSystem::readDirectory" );

	bool br = false;

	DIR* pDir;
	dirent* pEntry;

	BWConcurrency::startMainThreadIdling();

	pDir = opendir((basePath_ + path).c_str());
	if (pDir != NULL)
	{
		pEntry = readdir(pDir);

		while (pEntry)
		{
			const char * name = pEntry->d_name;

			if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
			{
				dir.push_back(name);
			}

			pEntry = readdir(pDir);
		}

		closedir(pDir);

		br = true;
	}

	BWConcurrency::endMainThreadIdling();

	return br;
}


BinaryPtr UnixFileSystem::readFile(const BW::StringRef & dirPath, uint index)
{
	PROFILER_SCOPED( readFile_with_idx );
	IFileSystem::Directory dir;
	readDirectory( dir, dirPath );
	if (dir.size() && index < dir.size())
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
BinaryPtr UnixFileSystem::readFile(const BW::StringRef& path)
{
	PROFILER_SCOPED( readFile );
	BWResource::checkAccessFromCallingThread( path,
			"UnixFileSystem::readFile" );

	FILE* pFile;
	char* buf;
	int len;

	BWConcurrency::startMainThreadIdling();
	pFile = bw_fopen((basePath_ + path).c_str(), "rb");
	BWConcurrency::endMainThreadIdling();

	if(!pFile)
		return static_cast<BinaryBlock *>( NULL );

	MF_VERIFY( fseek(pFile, 0, SEEK_END) == 0);
	len = ftell(pFile);
	MF_VERIFY( fseek(pFile, 0, SEEK_SET) == 0);

	if(len <= 0)
	{
		ERROR_MSG( "UnixFileSystem: No data in %s%s\n",
			basePath_.c_str(), path.to_string().c_str() );
		fclose( pFile );
		return static_cast<BinaryBlock *>( NULL );
	}

	BinaryPtr bp = new BinaryBlock( NULL, len, "BinaryBlock/UnixFileSystem" );
	buf = (char *) bp->data();

	BWConcurrency::startMainThreadIdling();
	bool bad = !fread( buf, len, 1, pFile );

	BWConcurrency::endMainThreadIdling();

	if (bad)
	{
		ERROR_MSG( "UnixFileSystem: Error reading from %s\n",
			path.to_string().c_str() );
		fclose( pFile );
		return static_cast<BinaryBlock *>( NULL );
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
bool UnixFileSystem::makeDirectory(const BW::StringRef& path)
{
	PROFILER_SCOPED( makeDirectory );
	return mkdir((basePath_ + path).c_str(), 0770) == 0;
}

/**
 *	This method writes a file to disk.
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@param binary	Write as a binary file (Windows only)
 *	@return	True if successful
 */
bool UnixFileSystem::writeFile(const BW::StringRef& path,
		BinaryPtr pData, bool /*binary*/)
{
	PROFILER_SCOPED( writeFile );
	BWConcurrency::startMainThreadIdling();
	FILE* pFile = bw_fopen((basePath_ + path).c_str(), "w");
	BWConcurrency::endMainThreadIdling();

	if(!pFile)
	{
		return false;
	}

	if(pData->len())
	{
		BWConcurrency::startMainThreadIdling();
		bool bad = fwrite(pData->data(), pData->len(), 1, pFile) != 1;
		BWConcurrency::endMainThreadIdling();

		if (bad)
		{
			ERROR_MSG("UnixFileSystem: Error writing to %s\n",
				path.to_string().c_str());
			return false;
		}
	}

	fclose(pFile);
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
bool UnixFileSystem::writeFileIfExists( const BW::StringRef& path,
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
 *  @param mode		Mode to use to open the file with, eg: "r", "w", "a"
 *                  (see system documentation for the fopen function
 *                  for details).
 *
 *	@return	 A FILE pointer on success, NULL on error.
 */
FILE * UnixFileSystem::posixFileOpen( const BW::StringRef& path,
	const char * mode )
{
	PROFILER_SCOPED( posixFileOpen );
	BWConcurrency::startMainThreadIdling();
	//char mode[3] = { writeable?'w':'r', binary?'b':0, 0 };
	FILE * pFile = bw_fopen( (basePath_ + path).c_str(), mode );
	BWConcurrency::endMainThreadIdling();

	if (!pFile)
	{
		// This case will happen often with multi res paths, don't flag it as
		// an error
		//ERROR_MSG("WinFileSystem: Failed to open %s\n", path.c_str());
		return NULL;
	}

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
bool UnixFileSystem::locateFileData( const BW::StringRef & path, 
	FileDataLocation * pLocationData )
{
	MF_ASSERT( pLocationData );

	BW::string filename = basePath_ + path;

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
 *	@param path the path of the file to stream
 *
 *	@return the file streamer
 */
FileStreamerPtr UnixFileSystem::streamFile( const BW::StringRef& path )
{
	PROFILER_SCOPED( streamFile );
	FileStreamerPtr pFileStreamer;

	FILE* pFile = posixFileOpen( path, "rb" );

	if (pFile)
	{
		pFileStreamer = new FileHandleStreamer( pFile );
	}

	return pFileStreamer;
}


/**
 *	This method resolves the base dependent path to a fully qualified
 * 	path.
 *
 *	@param path			Path relative to the base of the filesystem.
 *
 *	@return	The fully qualified path.
 */
BW::string	UnixFileSystem::getAbsolutePath( const BW::StringRef& path ) const
{
	return basePath_ + path;
}


/**
 *	This method renames the file or directory specified by oldPath
 *	to be specified by newPath. oldPath and newPath need not be in common.
 *
 *	Returns true on success
 */
bool UnixFileSystem::moveFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	PROFILER_SCOPED( moveFileOrDirectory );
	return rename(
		(basePath_+oldPath).c_str(), (basePath_+newPath).c_str() ) == 0;
}


/**
 *	This method erases the given file or directory.
 *	Directories need not be empty.
 *
 *	Returns true on success
 */
bool UnixFileSystem::eraseFileOrDirectory( const BW::StringRef & path )
{
	PROFILER_SCOPED( eraseFileOrDirectory );
	FileType ft = this->getFileType( path, NULL );
	if (ft == FT_FILE)
	{
		return unlink( (basePath_+path).c_str() ) == 0;
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
		return rmdir( (basePath_+path).c_str() ) == 0;
	}
	else
	{
		return false;
	}
}

/**
 *	This is a virtual copy constructor.
 *
 *	Returns a copy of this object.
 */
FileSystemPtr UnixFileSystem::clone()
{
	return new UnixFileSystem( basePath_ );
}

BW_END_NAMESPACE

// unix_file_system.cpp
