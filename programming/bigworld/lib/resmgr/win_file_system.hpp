/**
 *	@file
 *
 *	This file defines the WinFileSystem class.
 */	
#ifndef _WIN_FILE_SYSTEM_HEADER
#define _WIN_FILE_SYSTEM_HEADER

#include "file_system.hpp"
#include "filename_case_checker.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides an implementation of IFileSystem that
 *	reads from a Windows filesystem.	
 */	
class WinFileSystem : public IFileSystem 
{
public:	
	WinFileSystem(const BW::StringRef& basePath);
	~WinFileSystem();

	virtual FileType	getFileType(const BW::StringRef& path, FileInfo * pFI);
	virtual FileType	getFileTypeEx(const BW::StringRef& path, FileInfo * pFI);
#ifdef EDITOR_ENABLED
	virtual FileType	getFileTypeCache(const BW::StringRef& path, FileInfo * pFI);
	static void useFileTypeCache( bool useCache ) { s_useFileTypeCache_ = useCache; }
#endif
	virtual BW::string	eventTimeToString( uint64 eventTime );

	virtual bool		readDirectory ( Directory& dir,
							const BW::StringRef& path);
	virtual BinaryPtr	readFile( const BW::StringRef& path );
	virtual BinaryPtr	readFile( const BW::StringRef & dirPath,
							uint index );

	virtual bool		makeDirectory(const BW::StringRef& path);
	virtual bool		writeFile(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		writeFileIfExists(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		moveFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath );
	virtual bool		copyFileOrDirectory( const BW::StringRef & oldPath,
							const BW::StringRef & newPath );
	virtual bool		eraseFileOrDirectory( const BW::StringRef & path );

	virtual FILE *		posixFileOpen( const BW::StringRef & path,
							const char * mode );

	virtual bool locateFileData( const BW::StringRef & path, 
		FileDataLocation * pLocationData );
	virtual FileStreamerPtr streamFile( const BW::StringRef& path );
	
	virtual BW::string	getAbsolutePath( const BW::StringRef& path ) const;

	virtual BW::string correctCaseOfPath(const BW::StringRef &path) const;
#if ENABLE_FILE_CASE_CHECKING
	virtual void		checkCaseOfPaths(bool enable);
#endif // ENABLE_FILE_CASE_CHECKING

	virtual FileSystemPtr	clone();

	virtual void enableModificationMonitor( bool enable );
	virtual void flushModificationMonitor(
		const ModificationListeners& listeners );
	virtual bool ignoreFileModification( const BW::string& fileName, 
					ResourceModificationListener::Action action,
					bool oneTimeOnly );

	virtual bool hasPendingModification( const BW::string& fileName );

	void addModifiedResource( const BW::StringRef& resourceID );

	static 	FileType getAbsoluteFileType( const BW::StringRef & path, 
							FileInfo * pFI = NULL );

private:
	WinFileSystem( const WinFileSystem & other );
	WinFileSystem & operator=( const WinFileSystem & other );

	static 	FileType getAbsoluteFileTypeInternal( const BW::StringRef& path, 
							FileInfo * pFI = NULL );

	BW::string					basePath_;
	mutable FilenameCaseChecker	caseChecker_;
#if ENABLE_FILE_CASE_CHECKING
	bool						checkCase_;
	SimpleMutex					pathCacheMutex_;
	BW::set<BW::string>		validPathCache_;
	BW::set<BW::string>		invalidPathCache_;
#endif // ENABLE_FILE_CASE_CHECKING

	static LONG callsTo_getFileType_;
	static LONG callsTo_readDirectory_;
	static LONG callsTo_readFile_;
	static LONG callsTo_makeDirectory_;
	static LONG callsTo_writeFile_;
	static LONG callsTo_moveFileOrDirectory_;
	static LONG callsTo_eraseFileOrDirectory_;
	static LONG callsTo_posixFileOpen_;

private:
	struct MemoryMappedFile
	{
		bool writable_;
		HANDLE hFile_;
		HANDLE hFileMapping_;
		size_t fileSize_;
		size_t mappedOffset_;
		size_t userOffset_;
		size_t mappedLength_;
		size_t userLength_;
		void* mappedPtr_;
		void* userPtr_;

		struct CompareFunc : public std::unary_function<MemoryMappedFile, void*>
		{
			CompareFunc( void* ptr ) : ptr_(ptr) { }
			bool operator()(const MemoryMappedFile& other)
			{
				return ptr_ == other.userPtr_;
			}

			void* ptr_;
		};
	};

	typedef BW::vector<MemoryMappedFile> MemoryMappedFiles;
	MemoryMappedFiles	memoryMappedFiles_;
	SimpleMutex			memoryMappedFilesLock_;
	size_t				systemAllocationGranularity_;

private:
#ifdef EDITOR_ENABLED
	static bool s_useFileTypeCache_;

	// cache results to file type queries in the tools
	typedef StringRefUnorderedMap< FileType > StringRefFileTypeHashMap;
	StringRefFileTypeHashMap fileTypeCache_;
	ReadWriteLock fileTypeCacheLock_;

	BW::vector<BW::string> modifiedResources_;
	ReadWriteLock modifiedResourcesLock_;
#endif

#if !CONSUMER_CLIENT_BUILD
	class MonitorThread : public SimpleThread
	{
	public:
		MonitorThread( WinFileSystem* fs );
		~MonitorThread();

		void flushModifications( const ModificationListeners& listeners );
		void addIgnoreFiles( const BW::string& fileName, 
						ResourceModificationListener::Action action,
						bool oneTimeOnly );
		bool hasPendingModification( const BW::string& fileName );

	private:
		void threadFunc();

	private:
		static void s_threadFunc( void* arg );

	private:
		WinFileSystem* fs_;
		BW::string fullPath_;
		HANDLE pathHandle_;
		HANDLE threadExitEvent_;
	
		SimpleMutex modifiedFilesMutex_;
		typedef BW::map<BW::string, ResourceModificationListener::Action> ModificationMap;
		ModificationMap modifiedFiles_;

		SimpleMutex ignoreFilesMutex_;
		struct  IgnoreFileContent
		{
			IgnoreFileContent( const BW::string& fileName, 
				ResourceModificationListener::Action action,
				bool oneTimeOnly
				)
			{
				fileName_ = fileName;
				action_ = action;
				oneTimeOnly_ = oneTimeOnly;
			}

			BW::string fileName_;
			ResourceModificationListener::Action action_;
			bool oneTimeOnly_;
		};
		BW::vector<IgnoreFileContent> ignoreFiles_;
	};
#else
	class MonitorThread;
#endif
	MonitorThread* monitorThread_;
};

BW_END_NAMESPACE

#endif

