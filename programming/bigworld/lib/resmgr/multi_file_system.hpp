#ifndef _MULTI_FILE_SYSTEM_HEADER
#define _MULTI_FILE_SYSTEM_HEADER

#include "file_system.hpp"
#include "cstdmf/stringmap.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides an implementation of IFileSystem that
 *	reads from several other IFileSystems.	
 */	
class MultiFileSystem : public IFileSystem 
{
public:
	MultiFileSystem();
	MultiFileSystem( const MultiFileSystem & other );
	MultiFileSystem & operator=( const MultiFileSystem & other );
	~MultiFileSystem();

	void				addBaseFileSystem( FileSystemPtr pFileSystem,
							int index = -1 );
	void				delBaseFileSystem( int index );

	// IFileSystem
	virtual FileType	getFileType( const BW::StringRef& path,
							FileInfo * pFI = NULL );
	virtual FileType	getFileTypeEx( const BW::StringRef& path,
							FileInfo * pFI = NULL );
	virtual BW::string	eventTimeToString( uint64 eventTime );

	virtual bool		readDirectory( Directory& dir, 
							const BW::StringRef & path );
	virtual BinaryPtr	readFile(const BW::StringRef& path);
	virtual BinaryPtr	readFile( const BW::StringRef & dirPath,
							uint index );

	virtual void		collateFiles(const BW::StringRef& path,
								BW::vector<BinaryPtr>& ret);

	virtual bool		makeDirectory(const BW::StringRef& path);
	virtual bool		writeFile(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		writeFileIfExists(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		moveFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath );
#if defined( _WIN32 )
	virtual bool		copyFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath );
#endif // defined( _WIN32 )
	virtual bool		eraseFileOrDirectory( const BW::StringRef & path );

	virtual FILE *		posixFileOpen( const BW::StringRef & path,
							const char * mode );

	virtual bool locateFileData( const BW::StringRef & path, 
		FileDataLocation * pLocationData );
	virtual FileStreamerPtr streamFile( const BW::StringRef& path );
	
	virtual BW::string	getAbsolutePath( const BW::StringRef& path ) const;
	
	FileType 			resolveToAbsolutePath( BW::string& path ) const;

	virtual BW::string correctCaseOfPath(const BW::StringRef &path) const;
#if ENABLE_FILE_CASE_CHECKING
	virtual void		checkCaseOfPaths(bool enable);
#endif

	virtual FileSystemPtr	clone();

	virtual void		enableModificationMonitor( bool enable );
	virtual void		flushModificationMonitor(
		const ModificationListeners& listeners );
	virtual bool		ignoreFileModification( const BW::string& fileName, 
							ResourceModificationListener::Action action,
							bool oneTimeOnly );
	virtual bool		hasPendingModification( const BW::string& fileName );

	static void			addModifiedDirectory( const BW::StringRef& dirPath );

private:
	typedef BW::vector<FileSystemPtr>	FileSystemVector;

	FileSystemVector	baseFileSystems_;

	void cleanUp();
	bool readDirectoryUncached(Directory& dir, const BW::StringRef& path);

	typedef StringRefUnorderedMap< Directory > StringRefDirHashMap;
	StringRefDirHashMap dirCache_;

	static BW::vector<BW::string> modifiedDirectories_;
	
	// locks to control container access
	mutable ReadWriteLock dirCacheLock_;
	mutable ReadWriteLock baseFileSystemsLock_;
	static ReadWriteLock modifiedDirectoriesLock_;
	
};
typedef SmartPointer<MultiFileSystem> MultiFileSystemPtr;

BW_END_NAMESPACE

#endif // _MULTI_FILE_SYSTEM_HEADER
