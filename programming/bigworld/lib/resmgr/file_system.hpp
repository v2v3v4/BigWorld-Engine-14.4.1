#ifndef _FILE_SYSTEM_HEADER
#define _FILE_SYSTEM_HEADER

#include "binary_block.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stdmf.hpp"
#include "resource_modification_listener.hpp"

// FileSystem related constants
const int ZIP_MAGIC = 0x4b50;
#if defined _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

BW_BEGIN_NAMESPACE

class IFileSystem;
typedef SmartPointer<IFileSystem> FileSystemPtr;

class IFileStreamer;
typedef SmartPointer<IFileStreamer> FileStreamerPtr;

/**
 *	This class provides an abstract interface to a filesystem.
 */	
class IFileSystem : public SafeReferenceCount
{
public:
	/**
	 *	This typedef represents a directory of filenames.
	 */	
	typedef BW::vector<BW::string> Directory;

	/**
	 *	This enumeration describes a file type.
	 */	 
	enum FileType
	{
		FT_NOT_FOUND,
		FT_DIRECTORY,
		FT_FILE,
		FT_ARCHIVE
	};

	/**
	 *	This structure provides extra info about a file
	 */
	struct FileInfo
	{
		uint64 size;
		uint64 created;
		uint64 modified;
		uint64 accessed;
	};

	struct FileDataLocation
	{
		BW::string hostResourceID;
		uint64 offset;
	};

	virtual ~IFileSystem() {}

	virtual FileType	getFileType( const BW::StringRef & path,
							FileInfo * pFI = NULL ) = 0;
	virtual FileType	getFileTypeEx( const BW::StringRef & path,
							FileInfo * pFI = NULL )
							{return getFileType(path,pFI);}
#ifdef EDITOR_ENABLED
	virtual FileType	getFileTypeCache( const BW::StringRef & path,
							FileInfo * pFI = NULL )
							{return FT_NOT_FOUND;}
#endif
	virtual BW::string	eventTimeToString( uint64 eventTime ) = 0;

	virtual bool		readDirectory( Directory& dir, 
							const BW::StringRef & path ) = 0;
	virtual BinaryPtr	readFile( const BW::StringRef & path ) = 0;
	virtual BinaryPtr	readFile( const BW::StringRef & dirPath,
							uint index ) = 0;

	virtual bool		makeDirectory( const BW::StringRef & path ) = 0;
	virtual bool		writeFile( const BW::StringRef & path, 
							BinaryPtr pData, bool binary ) = 0;
	virtual bool		writeFileIfExists(const BW::StringRef& path, 
							BinaryPtr pData, bool binary) = 0;
	virtual bool		moveFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath ) = 0;
#if defined( _WIN32 )
	virtual bool		copyFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath ) = 0;
#endif // defined( _WIN32 )
	virtual bool		eraseFileOrDirectory( const BW::StringRef & path ) = 0;

	virtual FILE *		posixFileOpen( const BW::StringRef & path,
							const char * mode ) { return NULL; }

	virtual bool locateFileData( const BW::StringRef & path, 
		FileDataLocation * pLocationData ) = 0;
	virtual FileStreamerPtr streamFile( const BW::StringRef& path ) = 0;
	
	virtual BW::string	getAbsolutePath( const BW::StringRef& path ) const = 0;

	virtual BW::string correctCaseOfPath(const BW::StringRef &path) const
							{ return path.to_string(); }

	virtual void		enableModificationMonitor( bool enable ) = 0;
	virtual void		flushModificationMonitor(
			const ModificationListeners& listeners ) = 0;
	virtual bool ignoreFileModification( const BW::string& fileName, 
					ResourceModificationListener::Action action,
					bool oneTimeOnly )	{ return false;}

	virtual bool hasPendingModification( const BW::string& fileName ) = 0;

#if ENABLE_FILE_CASE_CHECKING
	virtual void		checkCaseOfPaths(bool enable) {}
#endif // ENABLE_FILE_CASE_CHECKING

	virtual FileSystemPtr	clone() = 0;

};

/**
 *	The file system that is native to the platform being compiled for
 */
namespace NativeFileSystem
{
	FileSystemPtr create( const BW::StringRef & path );
	
	IFileSystem::FileType getAbsoluteFileType( const BW::StringRef & path, 
							IFileSystem::FileInfo * pFI = NULL );
};

BW_END_NAMESPACE

#endif // _FILE_SSTEM_HEADER

