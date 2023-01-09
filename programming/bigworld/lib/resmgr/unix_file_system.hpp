#ifndef _UNIX_FILE_SYSTEM_HEADER
#define _UNIX_FILE_SYSTEM_HEADER

#include "file_system.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides an implementation of IFileSystem that
 *	reads from a Unix filesystem.	
 */	
class UnixFileSystem : public IFileSystem
{
public:	
	UnixFileSystem(const BW::StringRef& basePath);
	~UnixFileSystem();

	virtual FileType getFileType(   const BW::StringRef& path, FileInfo * pFI );
	virtual FileType getFileTypeEx( const BW::StringRef& path, FileInfo * pFI );

	virtual BW::string	eventTimeToString( uint64 eventTime );

	virtual bool		readDirectory ( Directory& dir,
							const BW::StringRef& path);
	virtual BinaryPtr	readFile(const BW::StringRef& path);
	virtual BinaryPtr	readFile(const BW::StringRef& dirPath, uint index);

	virtual bool		makeDirectory(const BW::StringRef& path);
	virtual bool		writeFile(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		writeFileIfExists(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		moveFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath );
	virtual bool		eraseFileOrDirectory( const BW::StringRef & path );

	virtual FILE *		posixFileOpen( const BW::StringRef & path,
							const char * mode );

	virtual bool locateFileData( const BW::StringRef & path, 
		FileDataLocation * pLocationData );
	virtual FileStreamerPtr streamFile( const BW::StringRef& path );

	
	virtual BW::string	getAbsolutePath( const BW::StringRef& path ) const;

	virtual FileSystemPtr clone();
	
	virtual void enableModificationMonitor( bool enable ) {}
	virtual void flushModificationMonitor(
		const ModificationListeners& listeners ) {}
	virtual bool hasPendingModification(
		const BW::string& fileName ) { return false; }

	static 	FileType getAbsoluteFileType( const BW::StringRef & path, 
							FileInfo * pFI = NULL );
	
private:
	BW::string			basePath_;

	static 	FileType getAbsoluteFileTypeInternal( const BW::StringRef & path, 
							FileInfo * pFI = NULL );
};

BW_END_NAMESPACE

#endif

