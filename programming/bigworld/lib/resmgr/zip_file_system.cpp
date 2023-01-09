#include "pch.hpp"

#include "bwresource.hpp"
#include "zip_file_system.hpp"
#include "zip/zlib.h"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_util.hpp"
#include "file_streamer.hpp"
#include "cstdmf/profiler.hpp"

#include <time.h>

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )


#include <algorithm>

BW_BEGIN_NAMESPACE

// This is used to ensure that the path to the zip file fopened is correct
#define conformSlash(x) (x)

namespace
{

#if ENABLE_MSG_LOGGING
void checkIsUTF8( const BW::string & filename, uint16 mask )
{
	//if the file/folder name is not ascii, then check if it's in utf-8,
	//if it's not in utf-8, then it will not be displayed correctly.
	if (!filename.empty())
	{
		//only check the leaf files and the leaf folders,
		//so do it backward.
		bool nonAscii = false;

		BW::string::const_reverse_iterator rbegin = filename.rbegin();
		if (*rbegin == '/')
		{
			++rbegin;
		}

		//stop when first hit a '/' because we only want to check the leaves
		for (BW::string::const_reverse_iterator ritr = rbegin,
			rend = filename.rend();
			ritr != rend && *ritr != '/'; ++ritr)
		{
			if ((*ritr & 0x80) !=0)//ascii code range [0,127]
			{
				nonAscii = true;
				break;
			}
		}

		//If general purpose(entry.mask) bit 11 is set, the filename and comment must support The Unicode Standard
		if (nonAscii && ((mask & 0x800) == 0))//not ascii and not utf-8
		{				
			WARNING_MSG("ZipFileSystem::openZip the file name (%s) is neither ASCII nor UTF-8,"
				"it may not be handled correctly\n", filename.c_str() );

		}
	}
}
#endif // ENABLE_MSG_LOGGING

/**
 * This returns a string with all slashes normalised to single forward slashes
 */
void cleanSlashes( BW::string& filename )
{
	BW::string& result = filename;
	BW::string::size_type slashpos;
	// Clean up any backslashes
	std::replace( result.begin(), result.end(), '\\', '/' );
	if ( ( slashpos = result.find( "//", 0 ) ) != BW::string::npos )
	{
		// Clean up any duplicate slashes
		do
		{
			result.replace( slashpos, 2, "/" );
			slashpos = result.find( "//", slashpos );
		} while ( slashpos != BW::string::npos );
	}
	// Get rid of any leading slashes
	if ( !result.empty() && result[ 0 ] == '/' )
		result.erase( 0, 1 );

	// Get rid of any trailing slashes
	if ( !result.empty() && result[ result.length() - 1 ] == '/' )
	{
		result.erase( result.length() - 1, 1 );
	}
//	if ( result != filename )
//		TRACE_MSG( "cleanSlashes: %s => %s\n", filename.c_str(), result.c_str() );
}

/**
 * This template function performs a slash-safe lookup of a filename
 * in the provided map. It assumes the slashes loaded into the map
 * are correct!
 *
 * In a non-consumer build, it will check for case mismatch and log it
 * as a warning. This will slow down missed lookups considerably.
 */
template< class MapType > inline typename MapType::iterator fileMapLookup( MapType& fileMap, const BW::StringRef& filename )
{
	// Don't handle bad-case lookups for now
	BW::string mapKey = filename.to_string();
	cleanSlashes( mapKey );
#if ENABLE_FILE_CASE_CHECKING
	typename MapType::iterator fileIt = fileMap.find( mapKey );
	if ( fileIt != fileMap.end() )
		return fileIt;
	// Check for a case-mismatch. We can't use FilenameCaseChecker here
	// because it relies on the Windows Shell interface to find the right file
	for( fileIt = fileMap.begin(); fileIt != fileMap.end(); fileIt++ )
	{
		// Early out if we have a different filename length
		if ( mapKey.length() != fileIt->first.length() )
			continue;
		const BW::StringRef& candidateName = fileIt->first;
		if ( candidateName.equals_lower( mapKey ) )
		{
			// We found a match! Complain bitterly, but accept it.
			WARNING_MSG( "Case in filename %s does not match case in zip file %s\n", 
				mapKey.c_str(), fileIt->first.to_string().c_str() );
			return fileIt;
		}
	}
	return fileMap.end();
 #else // ENABLE_FILE_CASE_CHECKING
 	return fileMap.find( mapKey );
 #endif // ENABLE_FILE_CASE_CHECKING
}


/**
 * This template function performs a slash-safe lookup of a filename
 * in the provided map. It assumes the slashes loaded into the map
 * are correct!
 *
 * In a non-consumer build, it will check for case mismatch and log it
 * as a warning. This will slow down missed lookups considerably.
 */
template< class MapType > inline typename MapType::const_iterator fileMapLookupConst( const MapType& fileMap, const BW::StringRef& filename )
{
	// Don't handle bad-case lookups for now
	BW::string mapKey = filename.to_string();
	cleanSlashes( mapKey );
#if ENABLE_FILE_CASE_CHECKING
	typename MapType::const_iterator fileIt = fileMap.find( mapKey );
	if ( fileIt != fileMap.end() )
		return fileIt;
	for( fileIt = fileMap.begin(); fileIt != fileMap.end(); fileIt++ )
	{
		// Early out if we have a different filename length
		if ( mapKey.length() != fileIt->first.length() )
			continue;
		const BW::StringRef& candidateName = fileIt->first;
		if ( candidateName.equals_lower( mapKey ) )
		{
			// We found a match! Complain bitterly, but accept it.
			WARNING_MSG( "Case in filename %s does not match case in zip file %s\n", 
				mapKey.c_str(), fileIt->first.to_string().c_str() );
			return fileIt;
		}
	}
	return fileMap.end();
#else // ENABLE_FILE_CASE_CHECKING
	return fileMap.find( mapKey );
#endif // ENABLE_FILE_CASE_CHECKING
}


#ifdef _WIN32
#pragma pack(push, 1)
#endif

/**
 *	This structure represents the footer at the end of a Zip file.
 *	Actually, a zip file may contain a comment after the footer,
 *	but for our purposes we can assume it will not.
 */
struct DirFooter
{
	uint32	signature PACKED;
	uint16	diskNumber PACKED;
	uint16	dirStartDisk PACKED;
	uint16	dirEntries PACKED;
	uint16	totalDirEntries PACKED;
	uint32	dirSize PACKED;
	uint32	dirOffset PACKED;
	uint16	commentLength PACKED;
};

#ifdef _WIN32
#pragma pack(pop)
#endif

} // namespace anonymous

/**
*	get the root of the zip I lives in 
*	ie: for a structure x/y.zip/z/a.zip/b/c
*	the residedZipRoot of y.zip, z, a.zip, c would be: 
*	y.zip, y.zip, y.zip, a.zip
*
*/
ZipFileSystem* ZipFileSystem::residedZipRoot()
{
	ZipFileSystem* cur = this;
	ZipFileSystem* curParent = cur->parentZip_.getObject();
	while ( curParent )
	{
		if ( !curParent->childNode() // an outmost node
			|| !curParent->internalChildNode() )// a nested zip root
		{
			return curParent;
		}

		cur = curParent;
		curParent = curParent->parentZip_.getObject();
	}

	return cur;
}

/**
*	get the absolutePath of this node, 
*	ie: for a structure x/y.zip/z/a.zip/b/c
*	the absolutePath of y.zip, a.zip, c would be: 
*	x/y.zip, x/y.zip/z/a.zip, x/y.zip/z/a.zip/b/c
*
*/
BW::string ZipFileSystem::absolutePath()
{
	BW::string path = path_;
	ZipFileSystem* root = this;
	while(  root->childNode() )
	{
		root = root->residedZipRoot();
		MF_ASSERT( root != NULL );
		path = root->path_ + '/' + path;
	}
	return path;
}

/**
*	set the tag_ and update the path_, the full path
*	resides in the current level zip file,
*	NOTE: this has to be called after parentZip_ is set.
*
*	@param zipSectionTag		Section name of the zipSectio
*								can be different from the ZipFileSystem::tag_,
*								if it's a root of a zip 
*	@param amInternalChildNode	false if i am the root of a zip,
*								since this node hasn't been loaded yet,
*								so we can't rely on offset_ to judge if internalChildNode().
*							
*/
void ZipFileSystem::tag( const BW::StringRef& zipSectionTag, bool amInternalChildNode )
{

	// if we're an internal zip file, iterate up to find the root of current zip.
	path_.assign( zipSectionTag.data(), zipSectionTag.size() );
	ZipFileSystemPtr parent = parentZip_;
	while (  parent && parent->internalChildNode() )
	{
		path_ = parent->tag_ + "/" + path_;
		parent = parent->parentZip_;
	}

	// I am outmost node or the root of an internal zip)
	if ( !parentZip_ || !amInternalChildNode )
		tag_ = "";
	else
		tag_.assign( zipSectionTag.data(), zipSectionTag.size() );
}


/**
 *	Constructor for outmost node. 
 *	ie. *.zip specified in paths.xml,
 *	as one of BWResource's multiple file system
 *
 *	@param zipFile		Path of the zip file to open
 */
ZipFileSystem::ZipFileSystem( const BW::StringRef& zipFile ) :
	pFile_( NULL ),
	path_( zipFile.data(), zipFile.size() ),
	tag_( "" ),
	parentSystem_( NULL ),
	parentZip_( NULL ),
	offset_( 0 ),
	size_( 0 )
{

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );
	FileHandleHolder fhh(this);
}


/**
 *	Constructor for the outmost node. 
 *	ie. a regular *.zip file in a winFileSystem path.
 *
 *	@param zipFile						Path of the zip file to open
 *	@param parentSystem					Parent system used for loading
 *	@param childrenAlreadyIntrospected	if child zipSections are already created,
 *										we don't need load the file,like space data
 */
ZipFileSystem::ZipFileSystem( const BW::StringRef& zipFile,
								FileSystemPtr parentSystem,
								bool childrenAlreadyIntrospected ) :
	pFile_( NULL ),
	path_( zipFile.data(), zipFile.size() ),
	tag_( "" ),
	parentSystem_( parentSystem ),
	parentZip_( NULL ),
	offset_( 0 ),
	size_( 0 )
{
	if (!childrenAlreadyIntrospected)
	{
		// our own file mutex
		SimpleMutexHolder mtx( mutex_ );
		FileHandleHolder fhh(this);
	}
}


/**
 *	Constructor for child node
 *
 *	Used for .zip files or folder nested in zip files.
 *
 *	@param parentZip					Parent zip system
 *	@param zipSectionTag				Section name of the zipSection,
 *										can be different from the ZipFileSystem::tag_,
 *										if it's a root of a zip 
 *	@param childrenAlreadyIntrospected	if child zipSections are already created,
 *										we don't need load the file,like space data
 */
ZipFileSystem::ZipFileSystem( ZipFileSystemPtr parentZip,
							 const BW::StringRef& zipSectionTag,
							 bool childrenAlreadyIntrospected ) :
	pFile_( NULL ),
	parentSystem_( NULL ),
	parentZip_( parentZip ),
	offset_( 0 ),
	size_( 0 )
{
	// if parentZip is NULL, then you called the wrong construction.
	MF_ASSERT( parentZip != NULL );
	PROFILE_FILE_SCOPED( ZipFileSystem );
	bool amInternalChildNode = childrenAlreadyIntrospected ||
					parentZip->dirTest( zipSectionTag );

	this->tag( zipSectionTag, amInternalChildNode );

	if ( !amInternalChildNode )// a zip file and need load the file
	{
		// Get the offset for the internal file.
		parentZip->getFileOffset( zipSectionTag, offset_, size_ );

		// because we read from the parent zip, we need mutex from the parent zip 
		SimpleMutexHolder mtxParent( this->residedZipRoot()->mutex_ );

		// we also write to our own temp file, so we need our own file mutex as well
		SimpleMutexHolder mtxMe( mutex_ );

		FileHandleHolder fhh(this);
	}

}


/**
 *	This is the destructor.
 */
ZipFileSystem::~ZipFileSystem()
{
	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );
	this->closeZip();
}



ZipFileSystem::ZipFileSystem(  ) :
	pFile_( NULL ),
	path_( "" ),
	tag_( "" ),
	parentSystem_( NULL ),
	parentZip_( NULL ),
	offset_( 0 ),
	size_( 0 )	
{
}

/**
 *	Places the offset of the specified file into the passed references.
 *
 *	@param path	Name of the file
 *	@param fileOffset Output for the file offset.
 *	@param fileSize Output for the file size.
 */
void ZipFileSystem::getFileOffset(const BW::StringRef& path, uint32& fileOffset, uint32& fileSize)
{
	if (internalChildNode())
	{
		return parentZip_->getFileOffset( tag_ + "/" + path, fileOffset, fileSize );
	}

	FileMap::iterator it = fileMapLookup( fileMap_, path );
	if (it == fileMap_.end())
		return;

	LocalFile& file = centralDirectory_[ it->second.first ];

	this->beginReadLocalFile( file );

	fileOffset = file.dataOffset();
	fileSize = file.compressedSize();
}

/**
 *	Clear the specified directory.
 *
 *	@param dirPath	Name of the dir to clear
 */
void ZipFileSystem::clear(const BW::StringRef& dirPath)
{
	if (internalChildNode())
		return parentZip_->clear( parentZip_->tag() + "/" + dirPath );

	BW::string path2 = dirPath.to_string();
	resolvePath(path2);
	if(pFile_)
	{
		fclose(pFile_);
		pFile_ = NULL;
	}
	Directory& dir = dirMap_[ path2 ].first;
	Directory::iterator it = dir.begin();		
	for (; it != dir.end(); it++)
	{
		BW::string filename;
		if (path2 == "")
			filename = (*it);
		else
			filename = path2 + "/" + (*it);

		FileMap::iterator deleteMe = fileMapLookup( fileMap_, filename );
		MF_ASSERT( deleteMe != fileMap_.end() );
		if (deleteMe == fileMap_.end())
			continue;

		uint32 fileIndex = deleteMe->second.first;
		fileMap_.erase( deleteMe );
		centralDirectory_[ fileIndex ].clear();

		// Check if it's an encoded duplicate name..
		if (checkDuplicate(filename)) 
		{
			BW::StringRef decoded = decodeDuplicate(filename);

			FileDuplicatesMap::iterator it = fileMapLookup( duplicates_, decoded );
			if ( it != duplicates_.end() )
				duplicates_.erase( it );
		}
	}


	dirMap_[ path2 ].first.clear();
	dirMap_[ path2 ].second.clear();
}


/**
 *	This method load a local header of a file if it's not loaded yet.
 */
void ZipFileSystem::beginReadLocalFile( LocalFile& localFile )
{
	BW_GUARD;
	if (localFile.loadedHeader_)
	{
		return;
	}

	FileHandleHolder fhh(this);
	if (!fhh.isValid())
	{
		CRITICAL_MSG( "ZipFileSystem::beginReadLocalFile: "
			"Failed open zip file (%s)\n", localFile.filename_.c_str() );
		return;
	}

	// Read in the local header
	if (localFile.entry_.localHeaderOffset >= 0)
	{
		MF_VERIFY( fseek( pFile_, localFile.entry_.localHeaderOffset, SEEK_SET ) == 0 );
		fread( &localFile.header_, sizeof( LocalHeader ), 1, pFile_ );
	}
	localFile.loadedHeader_ = true;
}


/**
 *	This method opens a zipfile, and reads the directory.
 *	This will also populate the local directory for the zip.
 *
 *	@param path		Path of the zipfile.
 *
 *	@return True if successful.
 */
bool ZipFileSystem::openZip()
{
	//must be a zip root (either the outmost node or the root of a nested zip)
	MF_ASSERT( tag_ == "" );
	PROFILE_FILE_SCOPED( openZip );
	BW::StringRef dirComponent;
	BW::string fileComponent;
	DirFooter footer;
	DirPair blankDir;
	unsigned long i;
	int pos;

	if(pFile_)
	{
		fclose(pFile_);
		pFile_ = NULL;
	}

	if ( this->childNode() )// must be an internal zip root
	{
		ZipFileSystem* curZipRoot = this->residedZipRoot();
		MF_ASSERT( curZipRoot );
		// it's a root of current zip
		pFile_ = curZipRoot->openFileFromCurrentZipRoot( path_, "rb" );
		if (!pFile_)
			return false;
	}
	else if (parentSystem_)
	{
		if(!(pFile_ = parentSystem_->posixFileOpen(path_, "rb")))
		{
			return false;
		}
	}
	else
	{		
		if(!(pFile_ = bw_fopen(conformSlash(path_).c_str(), "rb")))
		{
			return false;
		}
	}

	if (centralDirectory_.size())
		return true;

	// Add a directory node for the root directory.
	dirMap_[""] = blankDir;

	MF_VERIFY( fseek(pFile_, 0, SEEK_END) == 0 );
	size_ = ftell( pFile_ );

	if(fseek(pFile_, size_-(int)sizeof(footer), SEEK_SET) != 0)
	{
		ERROR_MSG("ZipFileSystem::openZip Failed to seek to footer (opening %s)\n",
			path_.c_str());
		this->closeZip();
		return false;
	}

	if(fread(&footer, sizeof(footer), 1, pFile_) != 1)
	{
		ERROR_MSG("ZipFileSystem::openZip Failed to read footer (opening %s)\n",
			path_.c_str());
		this->closeZip();
		return false;
	}

	if(footer.signature != DIR_FOOTER_SIGNATURE)
	{
		ERROR_MSG("ZipFileSystem::openZip Invalid footer signature (opening %s)\n",
			path_.c_str());
		this->closeZip();
		return false;
	}

	if(fseek(pFile_, footer.dirOffset, SEEK_SET) != 0)
	{
		ERROR_MSG("ZipFileSystem::openZip Failed to seek to directory start (opening %s)\n",
			path_.c_str());
		this->closeZip();
		return false;
	}

	// Initialise the directory.
	uint32 dirOffset = 0;
	BinaryPtr dirBlock = NULL;
	if (footer.dirSize)
	{
		centralDirectory_.resize(footer.totalDirEntries);
		dirBlock = new BinaryBlock( NULL,
										footer.dirSize, 
										"BinaryBlock/ZipDirectory");

		if(fread(dirBlock->cdata(), dirBlock->len(), 1, pFile_) != 1)
		{
			ERROR_MSG("ZipFileSystem::openZip Failed to read directory block (opening %s)\n",
				path_.c_str());
			this->closeZip();
			return false;
		}
	}
	else
		return true; //just an empty zip

	for(i = 0; i < footer.totalDirEntries; i++)
	{
		LocalFile& localFile = centralDirectory_[i];
		DirEntry& entry = localFile.entry_;

		memcpy( &entry, dirBlock->cdata() + dirOffset, sizeof(entry) );
		dirOffset += sizeof(entry);

		if(entry.signature != DIR_ENTRY_SIGNATURE)
		{
			ERROR_MSG("ZipFileSystem::openZip Invalid directory signature (opening %s)\n",
				path_.c_str());
			this->closeZip();
			return false;
		}

		if(entry.filenameLength > MAX_FIELD_LENGTH)
		{
			ERROR_MSG("ZipFileSystem::openZip Filename is too long (opening %s)\n",
				path_.c_str());
			this->closeZip();
			return false;
		}

		if(entry.extraFieldLength > MAX_FIELD_LENGTH)
		{
			ERROR_MSG("ZipFileSystem::openZip Extra field is too long (opening %s)\n",
				path_.c_str());
			this->closeZip();
			return false;
		}

		if(entry.fileCommentLength > MAX_FIELD_LENGTH)
		{
			ERROR_MSG("ZipFileSystem::openZip File comment is too long (opening %s)\n",
				path_.c_str());
			this->closeZip();
			return false;
		}

		localFile.filename_.assign( dirBlock->cdata() + dirOffset,
			entry.filenameLength );
		dirOffset += entry.filenameLength;
		cleanSlashes( localFile.filename_ );

		const BW::StringRef filename = localFile.filename_;

		// can be an empty folder like "bla/bla/"
		if(filename[filename.length() - 1] == '/')
			localFile.isFolder( true );

		dirOffset += entry.extraFieldLength + entry.fileCommentLength;
		if( localFile.isFolder() )
		{
			dirMap_[ filename ] = blankDir;
		}

#if ENABLE_MSG_LOGGING
		checkIsUTF8( localFile.filename_, entry.mask );
#endif // ENABLE_MSG_LOGGING

		// Check if it's an encoded duplicate name..
		if (checkDuplicate(filename)) 
		{
			BW::StringRef decoded = decodeDuplicate(filename);

			// keep the name.. and add the decoded version to the duplicates list.
			if ( fileMapLookup( duplicates_, decoded ) == duplicates_.end() )
				duplicates_[decoded] = 1;
			else
				duplicates_[decoded]++;

		}
		// create the filename->directory index mapping
		fileMap_[ filename ].first = i;

		pos = static_cast<int>(filename.rfind('/'));
		if(pos == -1)
		{
			dirComponent = "";
			fileComponent = filename.to_string();
		}
		else
		{
			dirComponent = filename.substr(0, pos);
			fileComponent = filename.substr(pos + 1).to_string();
		}

		DirMap::iterator dirIter = fileMapLookup( dirMap_, dirComponent );
		if(dirIter == dirMap_.end())
		{
			dirMap_[ dirComponent ] = blankDir;
			DirPair& pair = dirMap_[ dirComponent ];

			//TODO: should this be stored on the full path not the file component?
			if (checkDuplicate(fileComponent))
			{
				pair.first.push_back( fileComponent );
				pair.second.push_back( decodeDuplicate( fileComponent ).to_string() );
			}
			else
			{
				pair.first.push_back( fileComponent );
				pair.second.push_back( fileComponent );
			}

			fileMap_[ filename ].second = 0;


			if (fileMapLookup( fileMap_, dirComponent ) == fileMap_.end())
			{
				updateFile( ( dirComponent ) , NULL );
			}
		}
		else
		{
			// add the directory index
			size_t size = dirIter->second.first.size();
			MF_ASSERT( size <= UINT_MAX );
			fileMap_[ filename ].second = static_cast<uint32>(size);

			if (checkDuplicate(fileComponent))
			{
				dirIter->second.first.push_back(fileComponent);
				dirIter->second.second.push_back(decodeDuplicate(fileComponent).to_string());
			}
			else
			{
				dirIter->second.first.push_back(fileComponent);
				dirIter->second.second.push_back(fileComponent);
			}
		}
	}


	return true;
}


/**
 *	This method determines if the input string is an encoded duplicate name.
 *	It is assumed that the file names (section names) will not contain spaces
 *	so that is used as the determining factor.
 *
 *	@param filename Name to test.
 *
 *	@return true if the name is a duplicate.
 */
bool ZipFileSystem::checkDuplicate(const BW::StringRef& filename) const
{
	return ( filename.find_first_of(' ') != filename.npos );
}


/**
 *	This method decodes a file name, returning the original duplicate name..
 *
 *	@param filename The name to decode.
 *
 *	@return The decoded string.
 */
BW::StringRef ZipFileSystem::decodeDuplicate(const BW::StringRef& filename) const
{
	BW::string::size_type pos = filename.find_first_of(' ');
	return filename.substr(0, pos);
}


/**
 *	This method encodes a file name.
 *
 *	@param filename The name to encode.
 *	@param count The duplicate count.
 *
 *	@return The encoded string.
 */
BW::string ZipFileSystem::encodeDuplicate(const BW::StringRef& filename, int count) const
{
	BW::string encoded = filename.to_string();
	const int B_LEN = 256;	
	char buff[B_LEN];
	int len = bw_snprintf( buff, B_LEN, " %d", count );
	encoded.append( &buff[0], len );
	return encoded;
}

/**
 *	Builds a zip file and returns it as a binary block.
 *
 *	[local file header 1]
 *	[file data 1]
 *	. 
 *	.
 *	.
 *	[local file header n]
 *	[file data n]    
 *	[central directory]
 *	[end of central directory record]
 *
 *	@return The file structure in a binary block.
 */
BinaryPtr ZipFileSystem::createZip()
{
	if (childNode())
		return NULL;

	CentralDir::iterator it;
	uint32 offset = 0;

	// Determine the file size.
	uint32 totalFileSize = sizeof(DirFooter);
	it=centralDirectory_.begin();
	while (it!=centralDirectory_.end())
	{
		if ((it)->isValid())
		{
			totalFileSize += (it)->sizeOnDisk();
			++it;
		}
		else //TODO: note that modifying the central dir means the indices in file map are wrong...
			it = centralDirectory_.erase(it); // remove invalid/unitialised files
	}	
	BinaryPtr pBinary = new BinaryBlock( NULL,
										totalFileSize, 
										"BinaryBlock/ZipCreate");

	// Write the files
	for (it=centralDirectory_.begin(); it!=centralDirectory_.end(); it++)
	{
		(it)->writeFile(pBinary,offset);
	}

	MF_ASSERT( offset < (uint32)pBinary->len() );

	// Build the directory footer.
	DirFooter footer;	memset(&footer, 0, sizeof(DirFooter));	
	footer.signature = DIR_FOOTER_SIGNATURE;
	footer.dirOffset = offset;
	MF_ASSERT( centralDirectory_.size() <= USHRT_MAX );
	footer.dirEntries = ( uint16 ) centralDirectory_.size();
	footer.totalDirEntries = static_cast<uint16>(centralDirectory_.size());
	footer.commentLength = 0;

	// Write the central directory
	for (it=centralDirectory_.begin(); it!=centralDirectory_.end(); it++)
	{
		(it)->writeDirEntry(pBinary,offset);
	}

	MF_ASSERT( offset < (uint32)pBinary->len() );

	// Write directory footer
	footer.dirSize = (offset - footer.dirOffset);
	memcpy(pBinary->cdata()+offset, &footer, sizeof(footer));
	offset += sizeof(footer);

	MF_ASSERT( offset == (uint32)pBinary->len() );

	return pBinary;
}


/**
 *	This method returns the file name associated with the given index.
 *
 *	@param dirPath Directory to index into.
 *	@param index File index.
 *
 *	@return The file name (decoded)
 */
BW::string ZipFileSystem::fileName( const BW::StringRef& dirPath, int index )
{
	if (internalChildNode())
		return parentZip_->fileName( parentZip_->tag() + "/" + dirPath, index );

	BW::string path2 = dirPath.to_string();
	resolvePath(path2);
	MF_ASSERT_DEBUG( fileMapLookup( dirMap_, path2 ) != dirMap_.end() );
	return dirMap_[ path2 ].second[ index ];
}


/**
 *	This method returns the index associated with a file name.
 *	If the file has duplicate names, it will get the first entry available.
 *
 *	@param name Name of the file.
 *
 *	@return The index associated with the name, -1 if it doesnt exist.
 */
int ZipFileSystem::fileIndex( const BW::StringRef& name )
{
	if (internalChildNode())
		return parentZip_->fileIndex(tag_ + "/" + name);

	BW::string path = name.to_string();
	resolveDuplicate(path);

	FileMap::iterator it = fileMapLookup( fileMap_, path );

	if (it != fileMap_.end())
		return it->second.second;
	else
		return -1;
}


bool ZipFileSystem::resolveDuplicate( BW::string& path ) const
{
	FileDuplicatesMap::const_iterator dupe = fileMapLookupConst( duplicates_, path );
	if (dupe != duplicates_.end())
	{
		path = encodeDuplicate(path, 1);
		return true;
	}

	return false;
}

void ZipFileSystem::resolvePath( BW::string& path, bool bExtraChecks ) const
{
	// ToDo: maybe this function should be removed since we made the tag_ and path_ consistant now.
	cleanSlashes( path );

	if (path_ == "")
		return;	


	//TODO: review this special case.
	if (path == tag_)
	{
		path = "";
		return;
	}

	if (bExtraChecks)
	{
		BW::string dirPath = path_;
		cleanSlashes( dirPath );
		size_t off = path.find( dirPath );
		if (off != BW::string::npos)
		{
			path = path.substr(MF_MIN(off + dirPath.size()+1, path.size()), path.size());
		}
		if (path.size()>1 && path[path.size()-1] == '.')
			path = path.erase(path.size()-2,path.size());
	}
}

/**
 *	This method reads the file via the file index.
 *
 *	@param dirPath	Path of the file to read.
 *  @param index	The array index of the file to read in the dirPath list.
 *
 *	@return The file data.
 */
BinaryPtr ZipFileSystem::readFile(const BW::StringRef & dirPath, uint index)
{
	if (internalChildNode())
		return parentZip_->readFile( parentZip_->tag() + "/" + dirPath, index );

	BWResource::checkAccessFromCallingThread( dirPath, "ZipFileSystem::readFile" );
	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	BW::string path2 = dirPath.to_string();
	resolvePath(path2);
	DirMap::iterator it = fileMapLookup( dirMap_, path2 );

	if(it != dirMap_.end() && index < it->second.first.size())
	{
		if (path2 == "")
			return readFileInternal( it->second.first[index] );
		else
			return readFileInternal( path2 + "/" + it->second.first[index] );
	}
	else
		return NULL;
}


/**
 *	This method reads the contents of a file through readFileInternal.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr ZipFileSystem::readFile(const BW::StringRef& path)
{
	if (internalChildNode())
		return parentZip_->readFile(tag_ + "/" + path);

	BWResource::checkAccessFromCallingThread( path, "ZipFileSystem::readFile" );

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	BW::string path2 = path.to_string();
	resolvePath(path2);
	return readFileInternal( path2 );
}


/**
 *	This method reads the contents of a file. It is not thread safe.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A BinaryBlock object containing the file data.
 */
BinaryPtr ZipFileSystem::readFileInternal(const BW::StringRef& inPath, bool uncompressData)
{
	char *pCompressed = NULL, *pUncompressed = NULL;
	BW::string path = inPath.to_string();
	resolveDuplicate(path);
	FileMap::iterator it = fileMapLookup( fileMap_, path );
	BinaryPtr pBinaryBlock;
	int r;

	if (it == fileMap_.end())
		return static_cast<BinaryBlock *>( NULL );

	LocalFile& file = centralDirectory_[it->second.first];
	//LocalHeader& hdr = file.header_;
	if (file.entry_.localHeaderOffset < 0) 
	{	//doesnt exist in the file (dummy entry for directory struct)
		return static_cast<BinaryBlock *>( NULL );
	}

	// We don't read the file twice, cause the file header has been 
	// changed, if we read the second time, it will be disaster.
	if (file.pData_)
		return file.pData_;

	FileHandleHolder fhh(this);
	if (!fhh.isValid())
	{
		ERROR_MSG("ZipFileSystem::readFile Failed open zip file (%s)\n",
			path.c_str());
		return static_cast<BinaryBlock *>( NULL );
	}

	this->beginReadLocalFile( file );

	if(fseek(pFile_, file.dataOffset(), SEEK_SET) != 0)
	{
		ERROR_MSG("ZipFileSystem::readFile Failed to seek past local header (%s in %s)\n",
			path.c_str(), path_.c_str());
		return static_cast<BinaryBlock *>( NULL );
	}
	if(file.entry_.compressionMethod != METHOD_STORE &&
		file.entry_.compressionMethod != METHOD_DEFLATE)
	{
		ERROR_MSG("ZipFileSystem::readFile Compression method %d not yet supported (%s in %s)\n",
			file.entry_.compressionMethod, path.c_str(), path_.c_str());
		return static_cast<BinaryBlock *>( NULL );
	}

	BinaryPtr pCompressedBin = new BinaryBlock( NULL, file.entry_.compressedSize, "BinaryBlock/ZipRead" );
	pCompressed = (char*)pCompressedBin->data();
	if (file.entry_.compressedSize)
	{
		if(!pCompressed)
		{
			ERROR_MSG("ZipFileSystem::readFile Failed to alloc data buffer (%s in %s)\n",
				path.c_str(), path_.c_str());
			return static_cast<BinaryBlock *>( NULL );
		}

		{
			PROFILE_FILE_SCOPED( zipRead );
			if(fread(pCompressed, 1, file.entry_.compressedSize, pFile_) != file.entry_.compressedSize)
			{
				ERROR_MSG("ZipFileSystem::readFile Data read error (%s in %s)\n",
					path.c_str(), path_.c_str());
				return static_cast<BinaryBlock *>( NULL );
			}
		}
	}
	
	if(uncompressData && file.entry_.compressedSize && file.entry_.compressionMethod == METHOD_DEFLATE)
	{
		PROFILE_FILE_SCOPED( zipDecompress );
		unsigned long uncompressedSize = file.entry_.uncompressedSize;
		z_stream zs;
		memset( &zs, 0, sizeof(zs) );

		BinaryPtr pUncompressedBin = new BinaryBlock( NULL, file.entry_.uncompressedSize, "BinaryBlock/ZipRead" );
		pUncompressed = (char*)pUncompressedBin->data();
		if(!pUncompressed)
		{
			ERROR_MSG("ZipFileSystem::readFile Failed to alloc data buffer (%s in %s)\n",
				path.c_str(), path_.c_str());
			return static_cast<BinaryBlock *>( NULL );
		}

		zs.zalloc = NULL;
		zs.zfree = NULL;

		// Note that we dont use the uncompress wrapper function in zlib,
		// because we need to pass in -MAX_WBITS to inflateInit2 as the
		// window size. This disables the zlib header, see zlib.h for
		// details. This is what we want, since zip files don't contain
		// zlib headers.

		if(inflateInit2(&zs, -MAX_WBITS) != Z_OK)
		{
			ERROR_MSG("ZipFileSystem::readFile inflateInit2 failed (%s in %s)\n",
				path.c_str(), path_.c_str());
			return static_cast<BinaryBlock *>( NULL );
		}

		zs.next_in = (unsigned char *)pCompressed;
		zs.avail_in = file.entry_.compressedSize;
		zs.next_out = (unsigned char *)pUncompressed;
		zs.avail_out = uncompressedSize;

		if((r = inflate(&zs, Z_FINISH)) != Z_STREAM_END)
		{
			ERROR_MSG("ZipFileSystem::readFile Decompression error %d (%s in %s)\n", r,
				path.c_str(), path_.c_str());
			inflateEnd(&zs);
			return static_cast<BinaryBlock *>( NULL );
		}

		inflateEnd(&zs);
		pBinaryBlock = pUncompressedBin;
	}
	else
	{
		pBinaryBlock = pCompressedBin;
	}

	if(pFile_)
	{
#ifdef EDITOR_ENABLED
		fclose(pFile_);
		pFile_ = NULL;
#endif
	}

	return pBinaryBlock;
}


/**
 *	This method closes the zip file and frees all resources associated
 *	with it.
 */
void ZipFileSystem::closeZip()
{
	if(pFile_)
	{
		fclose(pFile_);
		pFile_ = NULL;
	}

	duplicates_.clear();

	fileMap_.clear();
	dirMap_.clear();

	centralDirectory_.clear();
}


/**
 *	This method reads the contents of a directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return A vector containing all the filenames in the directory.
 */
bool ZipFileSystem::readDirectory( IFileSystem::Directory& dir, 
								   const BW::StringRef& path )
{
	PROFILE_FILE_SCOPED( zip_read_dir );
	if (internalChildNode())
		return parentZip_->readDirectory(dir, tag_ + "/" + path);

	BWResource::checkAccessFromCallingThread( path, "ZipFileSystem::readDirectory" );

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	BW::string path2 = path.to_string();
	resolvePath(path2);
	DirMap::iterator it = fileMapLookup( dirMap_, path2 );

	if (it != dirMap_.end())
	{
		dir = it->second.second;
		return true;
	}

	return false;
}


/**
 *	This method gets the file type, and optionally additional info.
 *
 *	@param path		Absolute path.
 *	@param pFI		If not NULL, this is set to the type of this file.
 *
 *	@return File type enum.
 */
IFileSystem::FileType ZipFileSystem::getFileType(const BW::StringRef& path,
	FileInfo * pFI )
{
	PROFILE_FILE_SCOPED( ZipFileSystem_Constructor );
	if (internalChildNode())
		return parentZip_->getFileType(tag_ + "/" + path, pFI);

	BWResource::checkAccessFromCallingThread( path, "ZipFileSystem::getFileType" );

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	BW::string path2 = path.to_string();
	resolvePath(path2,true);
	FileMap::const_iterator ffound = fileMapLookupConst( fileMap_, path2 );
	if (ffound == fileMap_.end())
		return FT_NOT_FOUND;

	const LocalFile& file = centralDirectory_[ffound->second.first];
	const DirEntry& hdr = file.entry_;

	FileType ft = (fileMapLookupConst( dirMap_, path2 ) != dirMap_.end()) ?
		FT_DIRECTORY : FT_FILE;

	if (pFI != NULL)
	{
		pFI->size = hdr.uncompressedSize;
#ifdef _WIN32
		FILETIME _ft;
		DosDateTimeToFileTime(hdr.modifiedDate,hdr.modifiedTime,&_ft);
		uint64 alltime = uint64(_ft.dwHighDateTime) << 32 | uint64(_ft.dwLowDateTime);
#else
		uint32 alltime =
			uint32(hdr.modifiedDate) << 16 | uint32(hdr.modifiedTime);
#endif //_WIN32

		pFI->created = alltime;
		pFI->modified = alltime;
		pFI->accessed = alltime;
		// TODO: Look for NTFS/Unix extra fields
	}
	return ft;

}

IFileSystem::FileType ZipFileSystem::getFileTypeEx(const BW::StringRef& path,
	FileInfo * pFI )
{
	PROFILE_FILE_SCOPED( zipGetFileTypeEx );

	if (internalChildNode())
		return parentZip_->getFileTypeEx(tag_ + "/" + path, pFI);

	BWResource::checkAccessFromCallingThread( path,
			"ZipFileSystem::getFileTypeEx" );

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );
	BW::string path2 = path.to_string();
	resolvePath(path2,true);

	FileMap::iterator ffound;

	resolveDuplicate(path2);
	ffound = fileMapLookup( fileMap_, path2 );

	if (ffound == fileMap_.end())
		return FT_NOT_FOUND;

	LocalFile& file = centralDirectory_[ffound->second.first];
	DirEntry& hdr = file.entry_;

	FileType ft = FT_FILE;
	if (fileMapLookup( dirMap_, path2 ) != dirMap_.end())
		return FT_DIRECTORY;	

	if (pFI != NULL)
	{
		pFI->size = hdr.uncompressedSize;
		uint32 alltime =
			uint32(hdr.modifiedDate) << 16 | uint32(hdr.modifiedTime);
		pFI->created = alltime;
		pFI->modified = alltime;
		pFI->accessed = alltime;
		// TODO: Look for NTFS/Unix extra fields
	}


	// already loaded.
	if (file.pData_)
	{
		if ( hdr.compressionMethod == METHOD_DEFLATE )
		{
			PROFILE_FILE_SCOPED( loadedDeflate );
			uint32 magic=0;
			int result;
			z_stream zs;
			memset( &zs, 0, sizeof( zs ) );
			zs.zalloc = NULL;
			zs.zfree = NULL;
			if ( inflateInit2( &zs, -MAX_WBITS ) != Z_OK )
			{
				ERROR_MSG( "ZipFileSystem::getFileTypeEx: inflateInit2 failed (%s in %s)\n",
					path.to_string().c_str(), path_.c_str() );
				return FT_FILE; //return no file instead?
			}
			zs.next_in = reinterpret_cast< Bytef* >( file.pData_->cdata() );
			zs.avail_in = file.pData_->len();
			zs.next_out = reinterpret_cast< Bytef* >( &magic );
			zs.avail_out = sizeof( magic );
			while ( ( result = inflate( &zs, Z_SYNC_FLUSH ) ) == Z_OK && zs.avail_out != 0 )
				;// Empty while
			inflateEnd( &zs );
			if ( result != Z_OK )
			{
				ERROR_MSG( "ZipFileSystem::getFileTypeEx: Decompression error %d (%s in %s)\n",
					result, path.to_string().c_str(), path_.c_str() );
				return FT_FILE; //return no file instead?
			}
			if (magic == ZIP_HEADER_MAGIC)
				return FT_ARCHIVE;
		} else {
			PROFILE_FILE_SCOPED( zipTest );
			// Uncompressed
			if (zipTest(file.pData_))
				return FT_ARCHIVE;
		}
	}
	else
	{
		PROFILE_FILE_SCOPED( zipNotLoadedDeflate );
		uint32 magic=0;
		FileHandleHolder fhh(this);
		if (!fhh.isValid())
		{
			ERROR_MSG("ZipFileSystem::getFileType Failed open zip file (%s)\n",
				path.to_string().c_str());
			return FT_NOT_FOUND;
		}

		// Check if it's a zip file.
		this->beginReadLocalFile( file );
		MF_VERIFY( fseek(pFile_, file.dataOffset() , SEEK_SET) == 0 );

		if (hdr.uncompressedSize == 0)
		{
			return FT_FILE;
		}
		else if ( hdr.compressionMethod == METHOD_DEFLATE )
		{
			PROFILE_FILE_SCOPED( zipReadDecompress );
			int result;
			z_stream zs;
			memset( &zs, 0, sizeof( zs ) );
			zs.zalloc = NULL;
			zs.zfree = NULL;
			if ( inflateInit2( &zs, -MAX_WBITS ) != Z_OK )
			{
				ERROR_MSG( "ZipFileSystem::getFileTypeEx: inflateInit2 failed (%s in %s)\n",
					path.to_string().c_str(), path_.c_str() );
				return FT_FILE; //return no file instead?
			}
			uint16 input;
			zs.next_in = reinterpret_cast< Bytef* >( &input );
			zs.avail_in = sizeof( input );
			zs.next_out = reinterpret_cast< Bytef* >( &magic );
			zs.avail_out = sizeof( magic );
			size_t readRecords;
			//read elements until we get the first deflated uint32 (the header)
			while ( ( readRecords = fread( &input, sizeof(input), 1, pFile_ ) ) == 1
				&& ( result = inflate( &zs, Z_SYNC_FLUSH ) ) == Z_OK
				&& zs.avail_out != 0 )
			{
				if ( readRecords != 1 )
				{
					inflateEnd( &zs );
					return FT_FILE; //return no file instead?
				}
				if ( result != Z_OK )
				{
					inflateEnd( &zs );
					ERROR_MSG( "ZipFileSystem::getFileTypeEx: Decompression error %d (%s in %s)\n",
						result, path.to_string().c_str(), path_.c_str() );
					return FT_FILE; //return no file instead?

				}
				// We decompressed successfully, but still have not gotten enough data
				MF_ASSERT( zs.total_out < sizeof( magic ) );
				// Inflate is happily advancing and reducing the pointers and length.
				// That's good for the output, but we're reading into the same input again.
				zs.next_in = reinterpret_cast< Bytef* >( &input );
				zs.avail_in = sizeof( input );
			}
			inflateEnd( &zs );
		} else {
			PROFILE_FILE_SCOPED( zipJustLoad );
			size_t frr = 1;
			frr = fread( &magic, sizeof(magic), 1, pFile_ );
			if (!frr)
				return FT_FILE; //return no file instead?
		}


		if (magic == ZIP_HEADER_MAGIC)
		    {
				return FT_ARCHIVE;
		    }
	}
	return ft;
}

extern const char * g_daysOfWeek[];
extern const char * g_monthsOfYear[];


/**
 *	Translate an event time from a zip file to a string
 */
BW::string	ZipFileSystem::eventTimeToString( uint64 eventTime )
{
	uint16 zDate = uint16(eventTime>>16);
	uint16 zTime = uint16(eventTime);

	char buf[32];
	bw_snprintf( buf, sizeof(buf), "%s %s %02d %02d:%02d:%02d %d",
		"Unk", g_monthsOfYear[(zDate>>5)&15], zDate&31,
		zTime>>11, (zTime>>5)&15, (zTime&31)*2, 1980 + (zDate>>9) );
	return buf;
}


/**
 *	This method creates a new directory.
 *
 *	@param path		Path relative to the base of the filesystem.
 *
 *	@return True if successful
 */
bool ZipFileSystem::makeDirectory(const BW::StringRef& /*path*/)
{
	return false;
}


/**
 *	This method returns the size of this file on disk. Includes the directory
 *	entry size
 *
 *
 *	@return The file size
 */
uint32 ZipFileSystem::LocalFile::sizeOnDisk()
{
	return	static_cast<uint32>(sizeof(LocalHeader) 
			+ diskName().length()*2
			+ pData_->len() 
			+ sizeof(DirEntry));
}


/**
 *	This method writes the file data to the a file stream.
 *
 *	@param pFile File stream to write to.
 *	@param offset Output for the accumulation of the file offset.
 *
 *	@return true if sucessful
 */
bool ZipFileSystem::LocalFile::writeFile( FILE* pFile, uint32& offset )
{
	BW_GUARD;
	//in case the file name and length not consistent.
	this->updateHeader( false, false );

	localOffset_ = offset;
	offset += (uint32)fwrite(&header_, 1, sizeof(header_), pFile);
	offset += (uint32)fwrite(diskName().c_str(), 1, diskName().length(), pFile);
	offset += (uint32)fwrite(pData_->data(), 1, pData_->len(), pFile);
	return true;
}


/**
 *	This method writes the file data to the a binary block.
 *
 *	@param dst Destination binary block.
 *	@param offset Output for the accumulation of the buffer offset.
 *
 *	@return true if sucessful
 */
bool ZipFileSystem::LocalFile::writeFile( BinaryPtr dst, uint32& offset )
{
	//in case the file name and length not consistent.
	this->updateHeader( false, false );

	localOffset_ = offset;

	memcpy((dst->cdata() + offset), &header_, sizeof(header_));
	offset += sizeof(header_);

	memcpy((dst->cdata() + offset), diskName().c_str(), diskName().length());
	offset += static_cast<uint32>(diskName().length());

	memcpy((dst->cdata() + offset), pData_->data(), pData_->len());
	offset += pData_->len();

	return true;
}

/**
*	This method return the name of the file that should be written on the disk
*
*	@return the name of the file
*/
BW::string	ZipFileSystem::LocalFile::diskName()
{
	return isFolder()? filename_ + "/": filename_;
}

/**
 *	This method writes the file directory entry to the a binary block.
 *
 *	@param dst Destination binary block.
 *	@param offset Output for the accumulation of the buffer offset.
 *
 *	@return true if sucessful
 */
bool ZipFileSystem::LocalFile::writeDirEntry( BinaryPtr dst, uint32& offset )
{
	memset(&entry_, 0, sizeof(DirEntry));
	entry_.signature = DIR_ENTRY_SIGNATURE;
	entry_.filenameLength = (uint16)diskName().length(); //max = MAX_FIELD_LENGTH
	entry_.extraFieldLength = 0;
	entry_.fileCommentLength = 0;
	entry_.localHeaderOffset = localOffset_;
	entry_.extractorVersion = header_.extractorVersion;
	entry_.creatorVersion = header_.extractorVersion;
	entry_.modifiedDate = header_.modifiedDate;
	entry_.modifiedTime  = header_.modifiedTime;
	entry_.uncompressedSize = header_.uncompressedSize;
	entry_.compressedSize = header_.compressedSize;
	entry_.compressionMethod = header_.compressionMethod;
	entry_.crc32 = header_.crc32;

	memcpy((dst->cdata() + offset), &entry_, sizeof(entry_));
	offset += sizeof(entry_);
	memcpy((dst->cdata() + offset), diskName().c_str(), entry_.filenameLength);
	offset += entry_.filenameLength;

	return true;
}


/**
 *	This method writes the file directory entry to the a file stream.
 *
 *	@param pFile File stream to write to.
 *	@param offset Output for the accumulation of the file offset.
 *
 *	@return true if sucessful
 */
bool ZipFileSystem::LocalFile::writeDirEntry( FILE* pFile, uint32& offset )
{
	memset(&entry_, 0, sizeof(DirEntry));
	entry_.signature = DIR_ENTRY_SIGNATURE;
	entry_.filenameLength = (uint16)diskName().length(); //max = MAX_FIELD_LENGTH
	entry_.extraFieldLength = 0;
	entry_.fileCommentLength = 0;
	entry_.localHeaderOffset = localOffset_;
	entry_.extractorVersion = header_.extractorVersion;
	entry_.creatorVersion = header_.extractorVersion;
	entry_.modifiedDate = header_.modifiedDate;
	entry_.modifiedTime  = header_.modifiedTime;
//	entry_.externalFileAttr = 0x0;
	entry_.uncompressedSize = header_.uncompressedSize;
	entry_.compressedSize = header_.compressedSize;
	entry_.compressionMethod = header_.compressionMethod;
	entry_.crc32 = header_.crc32;

	offset += (uint32)fwrite(&entry_, 1, sizeof(entry_), pFile);	
	offset += (uint32)fwrite( diskName().c_str(), 1, entry_.filenameLength, pFile );
	return true;
}


/**
 *	This static method will compress the passed data block
 *
 *	TODO: best to move this into BinaryBlock similar to "compress"
 *			it just doesn't contain the zlib headers and is under finer control
 *
 */
/*static*/ BinaryPtr ZipFileSystem::LocalFile::compressData( const BinaryPtr pData )
{
	int r;
	z_stream zs;
	memset( &zs, 0, sizeof(zs) );

	BinaryPtr pCompressed = new BinaryBlock(NULL, pData->len()+1,
								"BinaryBlock/ZipUncompressedData");

	// Note that we dont use the compress wrapper function in zlib,
	// because we need to pass in -MAX_WBITS to deflateInit2 as the
	// window size. This disables the zlib header, see zlib.h for
	// details. This is what we want, since zip files don't contain
	// zlib headers.

	zs.next_in = (unsigned char *)pData->cdata();
	zs.avail_in = pData->len();
	zs.next_out = (unsigned char *)pCompressed->cdata();
	zs.avail_out = pCompressed->len();
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;

	int res = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, METHOD_DEFLATE,
								-MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
	if (res != Z_OK)
	{
		ERROR_MSG("ZipFileSystem::LocalFile::compressData"
					"deflateInit2 failed %s\n",
					zs.msg ? zs.msg : "" );
		return static_cast<BinaryBlock *>( NULL );
	}	
	//	If the parameter flush is set to Z_FINISH, pending input is pro-
	//	cessed, pending output is flushed and deflate() returns with
	//	Z_STREAM_END if there was enough output space; if deflate() re-
	//	turns with Z_OK, this function must be called again with Z_FINISH
	//	and more output space (updated avail_out but no more input data,
	//	until it returns with Z_STREAM_END or an error. After deflate()
	//	has returned Z_STREAM_END, the only possible operations on the
	//	stream are deflateReset() or deflateEnd().
	r = deflate(&zs, Z_FINISH);
	if (r == Z_OK)
	{
		// compressed data is bigger than the uncompressed.... don't compress.
		deflateEnd(&zs);
		return static_cast<BinaryBlock *>( NULL );
	}
	else if(r != Z_STREAM_END)
	{
		ERROR_MSG("ZipFileSystem::writeFile Compression error %d \n", r );
		deflateEnd(&zs);
		return static_cast<BinaryBlock *>( NULL );
	}
	MF_ASSERT( zs.total_out <= (uint)pCompressed->len() );
	BinaryPtr pBlock = new BinaryBlock(pCompressed->cdata(), zs.total_out,
								"BinaryBlock/ZipCompressedData");
	(void)deflateEnd(&zs);
	return pBlock;
}


/**
 *	This method updates the local header for this file.
 *
 */
void ZipFileSystem::LocalFile::updateHeader( bool clear/*= true*/ , 
											bool updateModifiedTime/*= false*/)
{
	if (clear)
	{
		uint16 modifiedDate = 0, modifiedTime = 0;
		if (!updateModifiedTime)
		{
			// don't change time stamp
			modifiedDate = header_.modifiedDate;
			modifiedTime = header_.modifiedTime;
		}
		memset( &header_, 0, sizeof(LocalHeader) );
		if (!updateModifiedTime)
		{
			header_.modifiedDate = modifiedDate;
			header_.modifiedTime = modifiedTime;
		}
	}

	header_.signature = LOCAL_HEADER_SIGNATURE;
	header_.filenameLength = static_cast<uint16>(diskName().length());
	header_.extraFieldLength = 0;
	header_.extractorVersion = 10;

	// Compress data
	if (!bCompressed_ && pData_)
	{
		header_.crc32 = crc32( header_.crc32, (unsigned char*)pData_->data(), pData_->len() );
		bCompressed_ = true;

		header_.uncompressedSize = pData_->len();
		BinaryPtr pCompressed = NULL;

		if (pData_->canZip() && !ZipFileSystem::zipTest( pData_ ))
		{
			pCompressed = LocalFile::compressData( pData_ );			
		}

		if (pCompressed)
		{
			header_.compressionMethod = METHOD_DEFLATE;
			pData_ = pCompressed;
		}
		else
			header_.compressionMethod = METHOD_STORE;

		header_.compressedSize = pData_->len();
	}

	if (updateModifiedTime)
	{
		// Fill the time stamps
		time_t tVal = time(NULL);
		tm* sys_T = localtime(&tVal);
		int day = sys_T->tm_mday;
		int month = sys_T->tm_mon+1;
		int year = 1900 + sys_T->tm_year;
		int sec = sys_T->tm_sec;
		int min = sys_T->tm_min;
		int hour = sys_T->tm_hour;
		if (year <= 1980) //lol
			year = 0;
		else
			year -= 1980;
		header_.modifiedDate = (uint16) (day + (month << 5) + (year << 9));
		header_.modifiedTime = (uint16) ((sec >> 1) + (min << 5) + (hour << 11));
	}
}


/**
 *	This method updates the zip directory with the input file.
 *	Files with a duplicate name are encoded to avoid multiple file names 
 *	in the zip.
 *
 *	@param name File name.
 *	@param data Binary data for the file.
 *
 */
void ZipFileSystem::updateFile(const BW::StringRef& name, BinaryPtr data)
{
	BW::string dirComponent, fileComponent;

	BW::string realName = name.to_string();
	cleanSlashes( realName );

	// Search the existing files.
	FileMap::iterator it = fileMapLookup( fileMap_, name );
	if (it == fileMap_.end()) // not found (add new)
	{
		// Check the duplicates map
		FileDuplicatesMap::iterator it2 = fileMapLookup( duplicates_, name );
		if (it2 != duplicates_.end())
		{
			// It's a duplicate, add it to the directory with an encoded name.
			duplicates_[name]++;
			realName = encodeDuplicate(realName, duplicates_[name]);
		}
		LocalFile file(realName, data);
		centralDirectory_.push_back(file);

		// Update the directory index mapping.
		fileMap_[ realName ].first = static_cast<uint32>(centralDirectory_.size()-1);
	}
	else // Found in the mapping already....register as a duplicate.
	{
		// copy the non-encoded name entry into a newly encoded entry..
		realName = encodeDuplicate(name, 1);
		// add an entry in the duplicates map for the non-encoded name.
		duplicates_[name] = 1;
		uint32 centralIndex = (it)->second.first;
		uint32 dirIndex = (it)->second.second;
		centralDirectory_[centralIndex].update(realName);

		BW::string dir, fileName;
		int pos = static_cast<int>(realName.rfind('/'));
		if(pos == -1)
		{
			dir = "";
			fileName = realName;
		}
		else
		{
			dir = realName.substr(0, pos);
			fileName = realName.substr(pos + 1);
		}
		dirMap_[ dir ].first[ dirIndex ] = fileName;

		// erase the map entry for the non-encoded name
		fileMap_.erase(it);
		fileMap_[ realName ].first = centralIndex;

		// Now for the actual file..
		realName = encodeDuplicate(name, 2);
		LocalFile file(realName, data);
		centralDirectory_.push_back(file);
		fileMap_[ realName ].first = static_cast<uint32>(centralDirectory_.size()-1);
		duplicates_[name]++;
	}
	DirPair blankDir;


	int pos = static_cast<int>(realName.rfind('/'));
	if(pos == -1)
	{
		dirComponent = "";
		fileComponent = realName;
	}
	else
	{
		dirComponent = realName.substr(0, pos);
		fileComponent = realName.substr(pos + 1);
	}

	//update the directory listings
	DirMap::iterator dirIter = fileMapLookup( dirMap_, dirComponent );
	if (dirIter == dirMap_.end()) // no directory listing found
	{
		dirMap_[ dirComponent ] = blankDir;
		DirPair& pair = dirMap_[ dirComponent ];

		if (checkDuplicate(fileComponent))
		{
			pair.first.push_back(fileComponent);
			pair.second.push_back(decodeDuplicate(fileComponent).to_string());
		}
		else
		{
			pair.first.push_back(fileComponent);
			pair.second.push_back(fileComponent);
		}

		fileMap_[ realName ].second = 0;

		if (fileMapLookup( fileMap_, dirComponent ) == fileMap_.end())
		{
			if (dirComponent != "")
			{
				updateFile( ( dirComponent ) , NULL );
			}
		}
	}
	else
	{
		// add the directory index
		fileMap_[ realName ].second = static_cast<uint32>(dirIter->second.first.size());

		if (checkDuplicate(fileComponent))
		{
			dirIter->second.first.push_back(fileComponent);
			dirIter->second.second.push_back(decodeDuplicate(fileComponent).to_string());
		}
		else
		{
			dirIter->second.first.push_back(fileComponent);
			dirIter->second.second.push_back(fileComponent);
		}	
	}
}

/**
*	This method tries to write a file into the disk
*
*	@param path		Path relative to the base of the filesystem.
*	@param pData	Data to write to the file
*	@param binary	Write as a binary file (Windows only)
*	@return	True if successful
*/
bool ZipFileSystem::writeFile( const BW::StringRef& path,
			BinaryPtr pData, bool binary )
{
	if (!pData)
		return false;

	// ToDo: currently we don't support nest zip,
	if (childNode())
		return false;

	BW::string filename = path.to_string();
	resolvePath(filename);	
	cleanSlashes( filename );

	// check if the directory exist, if not return false
	BW::string fileComponent, dirComponent;
	int pos = static_cast<int>(filename.rfind('/'));
	if(pos == -1)
	{
		dirComponent = "";
		fileComponent = filename;
	}
	else
	{
		dirComponent = filename.substr( 0, pos );
		fileComponent = filename.substr( pos + 1 );
	}

	if ( fileMapLookup( dirMap_, dirComponent ) == dirMap_.end() )
		return false;// directory not exist 


	// update the file entry of filename
	FileMap::iterator myIndex = fileMapLookup( fileMap_, filename );
	if ( myIndex == fileMap_.end() )//file not exist
	{
		// add the file entry to the structure
		updateFile( filename, pData );

		myIndex = fileMapLookup( fileMap_, filename );
		MF_ASSERT( myIndex != fileMap_.end() );
	}
	else
	{
		// update the file entry associated with filename
		uint32 centralIndex = (myIndex)->second.first;
		centralDirectory_[ centralIndex ].update( filename, pData );
	}


	BWResource::checkAccessFromCallingThread( path, "ZipFileSystem::writeFile" );
	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	// open a temp file for write
	// we could have collected all data (via createZip()) then write in one go into pFile_,
	// but in case the file size is too huge and createZip() consumes too much memory,
	// so better use a temp file, and write unit by unit to the temp file,
	// then rename it to path_
	BW::string tempPath = path_ +"_tmp";
	FILE* pTempFile = NULL;
	if(!(pTempFile = bw_fopen(conformSlash(tempPath).c_str(), "w+b")))
		return false;

	uint32 offset = 0;
	int fileEntries = 0;

	// writeFile
	for (CentralDir::iterator fileIter = centralDirectory_.begin(); fileIter != centralDirectory_.end(); fileIter++)
	{
		DirMap::iterator dirIter = fileMapLookup( dirMap_, fileIter->filename_ );
		if( dirIter != dirMap_.end() && // is a folder
			dirIter->second.first.size() != 0 )// has files under it
			continue;

		if ( fileIter->filename_ != myIndex->first && //not myself
			fileIter->pData_ == NULL  ) // not read from disk yet
		{
			fileIter->pData_ = this->readFileInternal( fileIter->filename_, false );
			fileIter->bCompressed_ = true;
		}

		fileIter->writeFile( pTempFile, offset );

		fileIter->pData_ = NULL;//release the memory

		fileEntries ++;
	}

	// Build the directory footer.
	DirFooter footer;	memset(&footer, 0, sizeof(DirFooter));	
	footer.signature = DIR_FOOTER_SIGNATURE;
	footer.dirOffset = offset;
	footer.dirEntries = fileEntries;
	footer.totalDirEntries = fileEntries;
	footer.commentLength = 0;

	// writeDirEntry
	for (CentralDir::iterator fileIter = centralDirectory_.begin(); fileIter != centralDirectory_.end(); fileIter++)
	{
		DirMap::iterator dirIter = fileMapLookup( dirMap_, fileIter->filename_ );
		if( dirIter != dirMap_.end() && // is a folder
			dirIter->second.first.size() != 0 )// has files under it
			continue;

		fileIter->writeDirEntry( pTempFile, offset );
	}

	// Write directory footer
	footer.dirSize = (offset - footer.dirOffset);
	offset += static_cast<uint32>(fwrite( &footer, 1, sizeof(footer), pTempFile));

	size_ = ftell( pTempFile );
	MF_ASSERT( offset == size_ );
	
	fclose( pTempFile );
	// we leave pFile_ as NULL so FileHandleHolder
	// will open it again when it's used.
	// if it's tool,pFile is closed right after opened.
	if (pFile_)
	{
		fclose( pFile_ );
		pFile_ = NULL;
	}

	// delete the old one, rename the temp to path_
	if ( bw_unlink( path_.c_str() ) == -1 ||
		bw_rename( tempPath.c_str(), path_.c_str() ) == -1 )
	{
		ERROR_MSG("ZipFileSystem::writeFile: Unable to rename %s to %s \n",
			tempPath.c_str(), path.to_string().c_str());
		return false;
	}

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
bool ZipFileSystem::writeFileIfExists( const BW::StringRef& path,
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
 *	This method writes a file into the zip structure. (not saved to disk)
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param pData	Data to write to the file
 *	@return	True if successful
 */
bool ZipFileSystem::writeToStructure(const BW::StringRef& path,
		BinaryPtr pData )
{
	if (!pData)
		return true;

	BW::string filename = path.to_string();
	resolvePath(filename);	

	// If this is a nested zip, pass along the write to the current root
	if (internalChildNode())
		return parentZip_->writeToStructure( tag_ + "/" + path , pData);

	// Add the file information to the directory
	updateFile(filename, pData);
	return true;
}


/**
 *	This method moves a file around.
 *
 *	@param oldPath		The path to move from.
 *	@param newPath		The path to move to.
 *
 *	@return	True if successful
 */
bool ZipFileSystem::moveFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	return false;
}


/**
 *	This method copies a file around.
 *
 *	@param oldPath		The path to move from.
 *	@param newPath		The path to move to.
 *
 *	@return	True if successful
 */
bool ZipFileSystem::copyFileOrDirectory( const BW::StringRef & oldPath,
	const BW::StringRef & newPath )
{
	return false;
}


/**
 *	This method erases a file or directory.
 *
 *	@param filePath		Path relative to the base of the filesystem.
 *	@return	True if successful
 */
bool ZipFileSystem::eraseFileOrDirectory( const BW::StringRef & filePath )
{
	BW::string filename = filePath.to_string();
	resolvePath(filename);

	// If this is a nested zip, pass along the write to the current root
	if (internalChildNode())
		return parentZip_->eraseFileOrDirectory( tag_ + "/" + filename );

	FileDuplicatesMap::iterator dupe = fileMapLookup( duplicates_, filename );
	if (dupe != duplicates_.end())
	{
		filename = encodeDuplicate(filename, dupe->second);
		//remove from the duplicates
		if (dupe->second > 1)
			dupe->second = dupe->second - 1;
		else
			duplicates_.erase( dupe );
	}
	BW::string fileComponent, dirComponent;
	int pos = static_cast<int>(filename.rfind('/'));
	if(pos == -1)
	{
		dirComponent = "";
		fileComponent = filename;
	}
	else
	{
		dirComponent = filename.substr(0, pos);
		fileComponent = filename.substr(pos + 1);
	}

	// Remove this file from all the directory / file mappings
	FileMap::iterator deleteMe = fileMapLookup( fileMap_, filename );
	if (deleteMe == fileMap_.end())
		return false;
	uint32 fileIndex = deleteMe->second.first;
	uint32 dirIndex = deleteMe->second.second;
	DirPair& dirPair = dirMap_[ dirComponent ];

	dirPair.first.erase(dirPair.first.begin() + dirIndex);
	dirPair.second.erase(dirPair.second.begin() + dirIndex);

	fileMap_.erase( deleteMe );
	centralDirectory_[ fileIndex ].clear();

	if (dirPair.first.size() != dirIndex ) //not the last place in the list.. rebuild
	{
		// Run through this directory and update the indices
		Directory::iterator it = dirPair.first.begin();
		for (;it != dirPair.first.end(); it++)
		{
			BW::string fullName;
			if (dirComponent != "")
				fullName = dirComponent + "/" + (*it);
			else
				fullName = (*it);
			FileMap::iterator file = fileMapLookup( fileMap_, fullName );
			// should always exist.. or there's a problem elsewhere
			MF_ASSERT(file != fileMap_.end());
			file->second.second = (uint32) (it - dirPair.first.begin());
		}
	}			


	return true;
}


/**
*	This method must be only called by nested zip root. It first extracts the file
*	from the Zip file, and then saves it to a temporary file
*
*	@param path		Path relative to the base of the filesystem.
*	@param mode		The mode that the file should be opened in.
*	@return	the file pointer if successful, otherwise NULL
*/
FILE *	ZipFileSystem::openFileFromCurrentZipRoot( const BW::StringRef & path, const char * mode )
{
	PROFILE_FILE_SCOPED( openFileFromCurrentZipRoot );
#ifdef _WIN32

	wchar_t wbuf[MAX_PATH+1];
	wchar_t wfname[MAX_PATH+1];

	BinaryPtr bin = readFileInternal( path );
	if ( !bin )
	{
		return NULL;
	}

	GetTempPath( MAX_PATH, wbuf );
	GetTempFileName( wbuf, L"bwt", 0, wfname );

	FILE * pFile = _wfopen( wfname, L"w+bDT" );

	if (!pFile)
	{
		ERROR_MSG("ZipFileSystem::openFileFromCurrentZipRoot: Unable to create temp file (%s in %s)\n",
			path.to_string().c_str(), path_.c_str());
		return NULL;
	}

	if (fwrite( bin->cdata(), bin->len(), 1, pFile ) != 1)
	{
		ERROR_MSG( "ZipFileSystem::openFileFromCurrentZipRoot: temp file write failed\n" );
		fclose( pFile );
		return NULL;
	}

	MF_VERIFY( fseek( pFile, 0, SEEK_SET ) == 0 );
	return pFile;

#else // _WIN32

	BinaryPtr bin = readFileInternal( path );
	if ( !bin )
	{
		return NULL;
	}

	// TODO: Unix guys should use fmemopen instead here.
	FILE * pFile = tmpfile();

	if (!pFile)
	{
		ERROR_MSG("ZipFileSystem::openFileFromCurrentZipRoot: Unable to create temp file (%s in %s)\n",
			path.to_string().c_str(), path_.c_str());
		return NULL;
	}

	if (fwrite( bin->cdata(), 1, bin->len(), pFile ) != 1)
	{
		ERROR_MSG( "ZipFileSystem::openFileFromCurrentZipRoot: temp file write failed\n" );
		fclose( pFile );
		return NULL;
	}

	MF_VERIFY( fseek( pFile, 0, SEEK_SET ) == 0 );
	return pFile;

#endif // _WIN32

}


/**
 *	This method opens a file using POSIX semantics. It first extracts the file
 *  from the Zip file, and then saves it to a temporary file
 *
 *	@param path		Path relative to the base of the filesystem.
 *	@param mode		The mode that the file should be opened in.
 *	@return	file pointer if successful
 */
FILE * ZipFileSystem::posixFileOpen( const BW::StringRef& path,
		const char * mode )
{
	if (parentSystem_)
		return parentSystem_->posixFileOpen(path,mode);

	// our own file mutex
	SimpleMutexHolder mtx( mutex_ );

	return openFileFromCurrentZipRoot( path, mode );
}


/**
 *	This class implements the IFileStreamer interface for streaming from a file in a zip archive
 */
class ZipFileStreamer : public IFileStreamer
{
public:
	ZipFileStreamer( FILE* f, size_t baseOffset, size_t size, ZipFileSystem* pFileSystem, SimpleMutex& mutex ) :
	  file_( f ),
	  baseOffset_( baseOffset ),
	  size_( size ),
	  currentOffset_( 0 ),
	  pFileSystem_( pFileSystem ),
	  mutex_( mutex )
	{
		MF_ASSERT( file_ );
		MF_VERIFY( fseek( file_, static_cast<long>(baseOffset), SEEK_SET ) == 0 );
	}
	size_t read( size_t nBytes, void* buffer )
	{
		MF_ASSERT( file_ );

		if ((currentOffset_ + nBytes) <= size_)
		{
			currentOffset_ += nBytes;
		}
		else
		{
			ERROR_MSG( "ZipFileStreamer::read - Trying to read outside the file\n" );
			nBytes = size_ - currentOffset_;
			currentOffset_ = size_;
		}

		return fread( buffer, nBytes, 1, file_ ) * nBytes;
	}

	bool skip( int nBytes )
	{
		MF_ASSERT( file_ );

		int32 newOffset = int32(currentOffset_) + nBytes;
		if (newOffset < 0 ||
			newOffset > int32( size_ ) )
		{
			ERROR_MSG( "ZipFileStreamer::skip: "
					"Trying to seek outside the file, seek pos: %d, "
					"valid range: 0 - %" PRIzu "\n",
				newOffset, size_ );
			return false;
		}

		currentOffset_ = size_t(newOffset);

		return fseek( file_, nBytes, SEEK_CUR ) == 0;
	}

	bool setOffset( size_t offset )
	{
		MF_ASSERT( file_ );
		if (offset > size_)
		{
			ERROR_MSG( "ZipFileStreamer::setOffset - Trying to set the offset beyond the end of the file\n" );
			return false;
		}

		currentOffset_ = offset;
		return fseek( file_, (long)(baseOffset_ + offset), SEEK_SET ) == 0;
	}

	size_t getOffset() const
	{
		return currentOffset_;
	}

	~ZipFileStreamer()
	{
		mutex_.give();
#ifdef EDITOR_ENABLED
		if (file_)
		{
			fclose( file_ );
		}
#endif
	}

	void* memoryMap( size_t offset, size_t length, bool writable )
	{
		CRITICAL_MSG( "Unimplemented - Use native file system for mapping" );
		return NULL;
	}

	void memoryUnmap( void * p )
	{
		CRITICAL_MSG( "Unimplemented - Use native file system for mapping" );
	}

private:
	FILE* file_;
	size_t baseOffset_;
	size_t size_;
	size_t currentOffset_;
	ZipFileSystemPtr pFileSystem_;
	SimpleMutex& mutex_;
};


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
bool ZipFileSystem::locateFileData( const BW::StringRef & inPath, 
	FileDataLocation * pLocationData )
{
	MF_ASSERT( pLocationData );

	SimpleMutexHolder smh( mutex_ );

	BW::string path = inPath.to_string();
	resolveDuplicate( path );
	FileMap::iterator it = fileMapLookup( fileMap_, path );

	// Check if the file is in this zip archive
	if (it != fileMap_.end())
	{
		LocalFile& file = centralDirectory_[it->second.first];

		// Is this a directory entry rather than a file entry?
		if (fileMapLookup( dirMap_, path ) == dirMap_.end()) // not a folder
		{
			// Is the file uncompressed?
			if (file.entry_.compressionMethod == METHOD_STORE)
			{
				if (pFile_ || openZip())
				{
					this->beginReadLocalFile( file );
				
					pLocationData->hostResourceID = path_;
					pLocationData->offset = file.dataOffset();
					return true;
				}
			}
		}
	}

	return false;
}


/**
 *	This method tries to create a filestreamer for a file in a zip archive
 *	We only allow streaming of uncompressed (stored) files in zip archives
 *	It is up to the calling code to handle the case where a streamer
 *	can not be created and load the object directly in stead.
 *
 *	@param inPath the path of the zip file
 */
FileStreamerPtr ZipFileSystem::streamFile( const BW::StringRef& inPath )
{
	if (parentSystem_)
		return parentSystem_->streamFile( inPath );

	// Don't use SimpleMutexHolder here as we want to manually release the mutex
	// in the ZipFileStreamer
	mutex_.grab();

	BW::string path = inPath.to_string();
	resolveDuplicate(path);
	FileMap::iterator it = fileMapLookup( fileMap_, path );

	FileStreamerPtr pStreamer;

	// Check if the file is in this zip archive
	if (it != fileMap_.end())
	{
		LocalFile& file = centralDirectory_[it->second.first];

		// Is this a directory entry rather than a file entry?
		if (fileMapLookup( dirMap_, path ) == dirMap_.end()) // not a folder
		{
			// Is the file uncompressed?
			if (file.entry_.compressionMethod == METHOD_STORE)
			{
				if (pFile_ || openZip())
				{
					this->beginReadLocalFile( file );
					pStreamer = new ZipFileStreamer( pFile_, file.dataOffset(), file.entry_.uncompressedSize, this, mutex_ );
				}
			}
			else
			{
				ERROR_MSG( "ZipFileSystem::streamFile - trying to stream compressed file %s from zip archive %s, only "
					"uncompressed files in zip archives can be streamed\n",
					inPath.to_string().c_str(), path_.c_str() );
			}
		}
		else
		{
			ERROR_MSG( "ZipFileSystem::streamFile - trying to stream a directory %s from zip archive %s\n",
				inPath.to_string().c_str(), path_.c_str() );
		}
	}

	// If we didn't create the streamer, release the mutex
	if (!pStreamer.exists())
	{
		mutex_.give();
	}
#ifdef EDITOR_ENABLED
	else
	{
		pFile_ = NULL;
	}
#endif

	return pStreamer;
}


/**
 *	This method resolves the base dependent path to a fully qualified
 * 	path.
 *
 *	@param path			Path relative to the base of the filesystem.
 *
 *	@return	The fully qualified path.
 */
BW::string	ZipFileSystem::getAbsolutePath( const BW::StringRef& path ) const
{
	// Absolute paths to a file inside a zip file is meaningless to almost any
	// application. Returning an empty string means that 
	// MultiFileSystem::getAbsolutePath() will skip over us and give an 
	// absolute path to a real file system.
	return BW::string();
}


/**
 *	This method returns a IFileSystem pointer to a copy of this object.
 *
 *	@return	a copy of this object
 */
FileSystemPtr ZipFileSystem::clone()
{
	return new ZipFileSystem( path_ );
}


/**
 *	This determines if the given tag is a directory in the
 *	zip file system.
 *
 *	@param name Directory name
 *
 *	@return	Whether or not the given name is a directory
 */
bool ZipFileSystem::dirTest( const BW::StringRef& name )
{ 
	// If this isn't the root of the current zip, then
	// we need to iterate up until we find it since the root
	// owns the directory map.
	if ( internalChildNode() )
	{
		return parentZip_->dirTest( tag_ + "/" + name );
	}
	else
	{
		return dirMap_.find( name) != dirMap_.end();
	}
}

/**
 *	This method returns the size of the archive.
 *
 *	@return	the number of bytes this archive takes up.
 */
int ZipFileSystem::fileSize( const BW::StringRef & dirPath, uint index ) const
{
	if (internalChildNode())
		return parentZip_->fileSize( parentZip_->tag() + "/" + dirPath, index);

	BW::string path2 = dirPath.to_string();
	resolvePath(path2);
	DirMap::const_iterator it = fileMapLookupConst( dirMap_, path2 );

	if (it != dirMap_.end() && index < it->second.first.size())
	{
		BW::string fullName;
		if (path2 == "")
			fullName = it->second.first[index];
		else
			fullName = path2 + "/" + it->second.first[index];
		FileMap::const_iterator fileIt = fileMapLookupConst( fileMap_, fullName );
		if (fileIt != fileMap_.end())
		{
			const LocalFile& file = centralDirectory_[fileIt->second.first];
			BinaryPtr pDat = file.pData_;
			if (pDat) // modified file
			{
				return pDat->len();
			}
			else
			{
				return file.uncompressedSize();
			}
		}
	}
	return 0;
}

/**
 *	This method returns the size directory specified
 *
 *	@param dirPath Directory name
 *
 *	@return	the number of bytes this archive takes up.
 */
int	ZipFileSystem::fileCount( const BW::StringRef& dirPath ) const
{ 
	if (internalChildNode())
		return parentZip_->fileCount( parentZip_->tag() + "/" + dirPath );
	BW::string path2 = dirPath.to_string();		
	resolvePath(path2);
	DirMap::const_iterator dir = fileMapLookupConst( dirMap_, path2 );
	if (dir != dirMap_.end())
		return (int)dir->second.first.size();
	else		
		return 0;
}


/**
 *	This method tests a binary block to see if it is a zip file.
 *
 *	@param pBinary	Binary block to test.
 *
 *	@return	true if pBinary is a zip file.
 */
/*static*/ bool ZipFileSystem::zipTest( BinaryPtr pBinary )
{	
	uint32 magic;
	if (pBinary->len() > (int)sizeof( magic ))
	{
		magic = ((uint32*)pBinary->data())[0];
		if (magic == ZIP_HEADER_MAGIC)
		    return true;
	}
	return false;
}


BW_END_NAMESPACE

// zip_file_system.cpp
