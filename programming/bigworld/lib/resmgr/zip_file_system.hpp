#ifndef _ZIP_FILE_SYSTEM_HEADER
#define _ZIP_FILE_SYSTEM_HEADER

#include "file_system.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stringmap.hpp"
#include <stdio.h>
#include "cstdmf/concurrency.hpp"


#ifdef _WIN32
#define PACKED
#pragma pack(push, 1)
#else
#define PACKED __attribute__((packed))
#endif

BW_BEGIN_NAMESPACE

class ZipFileSystem;
typedef SmartPointer<ZipFileSystem> ZipFileSystemPtr;

/**
 *	This enumeration contains constants related to Zip files.
 */
enum
{
	DIR_FOOTER_SIGNATURE 	= 0x06054b50,
	DIR_ENTRY_SIGNATURE		= 0x02014b50,
	LOCAL_HEADER_SIGNATURE	= 0x04034b50,
	DIGITAL_SIG_SIGNATURE	= 0x05054b50,

	MAX_FIELD_LENGTH		= 1024,

	METHOD_STORE			= 0,
	METHOD_DEFLATE			= 8,

	ZIP_HEADER_MAGIC		= 0x04034b50
};

/**
 *	This class provides an implementation of IFileSystem
 *	that reads from a zip file.	
 */	
class ZipFileSystem : public IFileSystem
{
public:
	// Maximum supported Zip file size is 2GB
	static const uint64 MAX_ZIP_FILE_KBYTES = 2 * 1024 * 1024;

	ZipFileSystem();
	ZipFileSystem( const BW::StringRef& zipFile );
	ZipFileSystem( const BW::StringRef& zipFile, 
					FileSystemPtr parent,
					bool childrenAlreadyIntrospected = false );
	ZipFileSystem( ZipFileSystemPtr parentSystem,
					const BW::StringRef& zipSectionTag,
					bool childrenAlreadyIntrospected = false );
	~ZipFileSystem();

	// IFileSystem implementation
	virtual FileType	getFileType(const BW::StringRef& path,
							FileInfo * pFI = NULL );
	virtual FileType	getFileTypeEx( const BW::StringRef & path,
							FileInfo * pFI = NULL );
	virtual BW::string	eventTimeToString( uint64 eventTime );
	virtual bool		readDirectory( IFileSystem::Directory& dir,
							const BW::StringRef& path );
	virtual BinaryPtr	readFile( const BW::StringRef& path );
	virtual BinaryPtr	readFile( const BW::StringRef & dirPath, uint index );
	virtual bool		makeDirectory(const BW::StringRef& path);
	virtual bool		writeFile(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	virtual bool		writeFileIfExists(const BW::StringRef& path, 
							BinaryPtr pData, bool binary);
	bool				writeToStructure( const BW::StringRef& path, BinaryPtr pData );
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

	virtual FileSystemPtr	clone();

	BW::string			fileName(const BW::StringRef& dirPath, int index);
	BinaryPtr			createZip( );
	void				clear() { closeZip(); }
	void				clear(const BW::StringRef& dirPath);
	int					fileIndex( const BW::StringRef& name );

	int	fileCount( const BW::StringRef& dirPath ) const;
	bool childNode() const { return (parentZip_ != NULL); }

	const BW::string& tag() const { return tag_; }
	BW::string	absolutePath();

	int fileSize( const BW::StringRef & dirPath, uint index ) const;
	FileSystemPtr parentSystem() const { return parentSystem_; }
	static bool zipTest( BinaryPtr pBinary );

	bool dirTest(const BW::StringRef& name);

	void enableModificationMonitor( bool enable ) { }
	void flushModificationMonitor( const ModificationListeners& listeners ) {}
	bool hasPendingModification( const BW::string& fileName ) { return false; }

private:
	/**
	 *	This structure represents the header that appears directly before
	 *	every file within a zip file.
	 */
	struct LocalHeader
	{
		uint32	signature PACKED;
		uint16	extractorVersion PACKED;
		uint16	mask PACKED;
		uint16	compressionMethod PACKED;
		uint16	modifiedTime PACKED;
		uint16	modifiedDate PACKED;
		uint32	crc32 PACKED;
		uint32	compressedSize PACKED;
		uint32	uncompressedSize PACKED;
		uint16	filenameLength PACKED;
		uint16	extraFieldLength PACKED;
	};

	/**
	 *	This structure represents an entry in the directory table at the
	 *	end of a zip file.
	 */
	struct DirEntry
	{
		uint32	signature PACKED;
		uint16	creatorVersion PACKED;
		uint16	extractorVersion PACKED;
		uint16	mask PACKED;
		uint16	compressionMethod PACKED;
		uint16	modifiedTime PACKED;
		uint16	modifiedDate PACKED;
		uint32	crc32 PACKED;
		uint32	compressedSize PACKED;
		uint32	uncompressedSize PACKED;
		uint16	filenameLength PACKED;
		uint16	extraFieldLength PACKED;
		uint16	fileCommentLength PACKED;
		uint16	diskNumberStart PACKED;
		uint16	internalFileAttr PACKED;
		uint32	externalFileAttr PACKED;
		int32	localHeaderOffset PACKED;
	};

	/**
	 *	This class represents an individual file in the zip.
	 */
	class LocalFile
	{
	public:
		//TODO: move out inlines and comment
		LocalFile() : filename_(""), pData_(NULL), localOffset_(0),
					isFolder_(false), bCompressed_(false)
		{
			this->clear();
			entry_.localHeaderOffset = -1;
		}
		LocalFile(const BW::string& filename, BinaryPtr data) :
					localOffset_(0), isFolder_(false), bCompressed_(false)
		{
			this->clear();
			filename_ = filename;
			pData_ = data;
			entry_.localHeaderOffset = -1;

			updateHeader( false, true );
		}

		void update( const BW::string& filename, BinaryPtr data )
		{
			filename_ = filename;
			pData_ = data;
			bCompressed_ = false;
			updateHeader( true, true );
		}
		void update( const BW::string& filename )
		{
			filename_ = filename;
			updateHeader( false, false );
		}

		uint32 dataOffset() const
		{
			MF_ASSERT( entry_.localHeaderOffset >= 0 );
			return entry_.localHeaderOffset + sizeof(LocalHeader) +
					header_.filenameLength + header_.extraFieldLength;
		}

		uint32 sizeOnDisk();
		bool isValid() const { return (pData_ != NULL);	}
		uint32 compressedSize() const { return entry_.compressedSize; }
		uint32 uncompressedSize() const { return entry_.uncompressedSize; }

		void clear()
		{
			memset(&header_, 0, sizeof(LocalHeader));
			pData_ = NULL;
			filename_ = "";
			loadedHeader_ = false;
		}

		bool writeFile( FILE* pFile, uint32& offset );
		bool writeFile( BinaryPtr dst, uint32& offset );
		bool writeDirEntry( FILE* pFile, uint32& offset );
		bool writeDirEntry( BinaryPtr dst, uint32& offset );

		BW::string		filename_;
		BinaryPtr		pData_;
		LocalHeader		header_;
		DirEntry		entry_;
		bool			loadedHeader_;

		static BinaryPtr compressData( const BinaryPtr pData );
		
		BW::string	diskName();
		bool		isFolder(){ return isFolder_; }
		void		isFolder( bool isFolder ){ isFolder_ = isFolder; }
	private:
		uint32			localOffset_;
		bool			isFolder_;
		bool			bCompressed_;

		void updateHeader( bool clear = true, bool updateModifiedTime = true );

		friend class ZipFileSystem;
	};

	/**
	 *	Utility class that opens the zip file handle, and ensures it gets 
	 *	closed when this class goes out of scope.
	 */
	class FileHandleHolder
	{
	public:
		FileHandleHolder(ZipFileSystem* zfs) : valid_(false), zfs_(zfs)
		{
#ifdef EDITOR_ENABLED
			originalOpened_ = zfs_->pFile_ != NULL;
#endif
			MF_ASSERT(zfs_ != NULL);
			valid_ = zfs_->pFile_ != NULL || zfs->openZip();
		}

		~FileHandleHolder()
		{
#ifdef EDITOR_ENABLED
			//we close the file every time if the editor is enabled to make sure 
			//we don't hold the cdata file (preventing the work of bwlockd - committing to the source control)
			// whoever opened the file close the file.
			if (!originalOpened_ && zfs_->pFile_)
			{
				fclose(zfs_->pFile_);
				zfs_->pFile_ = NULL;
			}
#endif
		}

		bool isValid() const { return valid_; }

	private:
		bool valid_;
		ZipFileSystem* zfs_;
#ifdef EDITOR_ENABLED
		bool originalOpened_;
#endif
	};

	mutable SimpleMutex mutex_; // to make sure only one thread access the zlib stream

	// IndexPair: <central dir index, local directory index>
	typedef std::pair<uint32,uint32>			IndexPair; 
	typedef StringRefMap< IndexPair > FileMap;
	typedef StringRefMap< uint32 > FileDuplicatesMap;
	
	// DirPair: <internal name, external>
	typedef std::pair<Directory,Directory>		DirPair;
	typedef StringRefMap< DirPair > DirMap;
	typedef BW::vector<LocalFile>				CentralDir;

	FileDuplicatesMap	duplicates_;
	FileMap				fileMap_;
	DirMap				dirMap_;
	FILE*				pFile_;

	// path_: the path related to the .zip this node lives in,
	// ie. for a structure x/y.zip/z/a.zip/b/c,
	// the path of y.zip, a.zip, c would be: x/y.zip, z/a.zip, b/c
	// NOTE: do not set path_ manually, instead
	// call tag( tag ) to update the path_
	BW::string			path_;

	// tag_: the path related to the direct parent,
	// ie. for a structure x/y.zip/z/a.zip/b/c,
	// the tag of y.zip, a.zip, c would be: "", "", "c"
	BW::string			tag_;

	// only used by the top most node, 
	// pointer to the BWResource multiple file system
	FileSystemPtr		parentSystem_; 

	// parentZip_: pointer to the parent ZipFileSystem node,
	// used by fold node and nested zip node,
	// NULL for the topmost node
	ZipFileSystemPtr	parentZip_;	
	CentralDir			centralDirectory_;
	
	// offset_: offset related to the parent zip, 
	// non zero when it's a root of a nested zip,otherwise zero
	uint32				offset_;
	uint32				size_;

	ZipFileSystem( const ZipFileSystem & other );
	ZipFileSystem & operator=( const ZipFileSystem & other );

	bool				openZip();	
	void				closeZip();
	virtual BinaryPtr	readFileInternal(const BW::StringRef& path, bool uncompressData = true);
	void				updateFile(const BW::StringRef& name, BinaryPtr data);
	// return if it's a childNode who is not the root of a nested zip, so basically it's a folder node
	bool				internalChildNode() const 
						{ return (childNode() && offset_ == 0); }

	bool				checkDuplicate(const BW::StringRef& filename) const;
	BW::StringRef		decodeDuplicate(const BW::StringRef& filename) const;
	BW::string			encodeDuplicate(const BW::StringRef& filename, int count) const;

	void				getFileOffset(const BW::StringRef& path,
									uint32& offset, uint32& size);

	void resolvePath( BW::string& path, bool bExtraChecks=false ) const;
	bool resolveDuplicate( BW::string& path ) const;

	void tag( const BW::StringRef& zipSectionTag, bool amInternalChildNode );
	ZipFileSystem* residedZipRoot();
	FILE *	openFileFromCurrentZipRoot( const BW::StringRef & path, const char * mode );

	void beginReadLocalFile( LocalFile& localFile );
};

BW_END_NAMESPACE
#ifdef _WIN32
#pragma pack(pop)
#endif

#endif //_ZIP_FILE_SYSTEM_HEADER

