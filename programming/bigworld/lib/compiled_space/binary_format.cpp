#include "pch.hpp"

#include "binary_format.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/file_streamer.hpp"

#include "math/boundbox.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
BinaryFormat::Stream::Stream() :
	pBuffer_( NULL ),
	length_( 0 ),
	cursor_( 0 )
{

}

// ----------------------------------------------------------------------------
BinaryFormat::Stream::Stream( void* pBuffer, size_t length ) :
	pBuffer_( pBuffer ),
	length_( length ),
	cursor_( 0 )
{
	MF_ASSERT( pBuffer != NULL );
	MF_ASSERT( length > 0 );
}


// ----------------------------------------------------------------------------
BinaryFormat::Stream::Stream( Stream& other )
{
	pBuffer_ = other.pBuffer_;
	length_ = other.length_;
	cursor_ = other.cursor_;
}


// ----------------------------------------------------------------------------
bool BinaryFormat::Stream::isValid() const
{
	return pBuffer_ != NULL && length_ > 0;
}


// ----------------------------------------------------------------------------
size_t BinaryFormat::Stream::length() const
{
	return length_;
}


// ----------------------------------------------------------------------------
bool BinaryFormat::Stream::eof() const
{
	return cursor_ >= length_;
}


// ----------------------------------------------------------------------------
size_t BinaryFormat::Stream::size() const
{
	return length_;
}

// ----------------------------------------------------------------------------
void BinaryFormat::Stream::reset()
{
	pBuffer_ = NULL;
	cursor_ = 0;
	length_ = 0;
}


// ----------------------------------------------------------------------------
size_t BinaryFormat::Stream::tell() const
{
	return cursor_;
}


// ----------------------------------------------------------------------------
void BinaryFormat::Stream::seek( size_t offset )
{
	cursor_ = std::min( offset, length_ );
}


// ----------------------------------------------------------------------------
void* BinaryFormat::Stream::readRaw( size_t len )
{
	if (cursor_ + len > length_)
	{
		return NULL;
	}

	void * pRet = (void*)(((unsigned char*)pBuffer_) + cursor_);
	cursor_ += len;
	return pRet;
}


// ----------------------------------------------------------------------------
bool BinaryFormat::Stream::read( uint32& dest )
{
	void* pSrc = this->readRaw( sizeof(uint32) );
	if (!pSrc)
	{
		return false;
	}

	memcpy( &dest, pSrc, sizeof(uint32) );
	return true;
}


// ----------------------------------------------------------------------------
bool BinaryFormat::Stream::read( StaticLooseOctree& dest )
{
	bool result = true;

	// Read configuration information
	result &= read( dest.storage().config_ );
	
	// Read in data arrays
	result &= read( dest.storage().bounds_ );
	result &= read( dest.storage().centers_ );
	result &= read( dest.storage().nodes_ );
	result &= read( dest.storage().dataReferences_ );
	result &= read( dest.storage().parents_ );

	return result;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
BinaryFormat::BinaryFormat() : 
	pHeader_(NULL),
	pSectionHeaders_(NULL)
{
	memset( &mappedSections_, 0, sizeof(mappedSections_) );
}


// ----------------------------------------------------------------------------
BinaryFormat::~BinaryFormat()
{
	this->close();
}


// ----------------------------------------------------------------------------
bool BinaryFormat::open( const char* resourceID )
{
	typedef BinaryFormatTypes::Header Header;
	typedef BinaryFormatTypes::SectionHeader SectionHeader;

	if (this->isValid())
	{
		return false;
	}

	MultiFileSystem* rootFS = BWResource::instance().fileSystem();
	IFileSystem* nativeFS = BWResource::instance().nativeFileSystem();
	MF_ASSERT( rootFS != NULL );

	IFileSystem::FileDataLocation fileLocation;
	if (!rootFS->locateFileData( resourceID, &fileLocation ))
	{
		return false;
	}

	pFileStream_ = nativeFS->streamFile( fileLocation.hostResourceID );
	if (!pFileStream_)
	{
		return false;
	}

	baseFileOffset_ = fileLocation.offset;
	pHeader_ = (Header*)pFileStream_->memoryMap( (size_t)baseFileOffset_, 
		sizeof(Header), false );
	if (!pHeader_)
	{
		pFileStream_ = NULL;
		return false;
	}

	if (pHeader_->magic_ != BinaryFormatTypes::FORMAT_MAGIC ||
		pHeader_->version_ != BinaryFormatTypes::FORMAT_VERSION)
	{
		pFileStream_->memoryUnmap( pHeader_ );
		pFileStream_ = NULL;
		pHeader_ = NULL;
		return false;
	}

	size_t finalHeaderSize = sizeof(Header) +
			pHeader_->numSections_ * sizeof(SectionHeader);

	pFileStream_->memoryUnmap( pHeader_ );

	pHeader_ = (Header*)pFileStream_->memoryMap(
		(size_t) baseFileOffset_, finalHeaderSize, false );
	if (!pHeader_)
	{
		this->close();
		return false;
	}

	pSectionHeaders_ = (SectionHeader*)(pHeader_+1);

	MF_ASSERT( this->numSections() <= BinaryFormatTypes::MAX_SECTIONS );

	resourceID_ = resourceID;
	return true;
}


// ----------------------------------------------------------------------------
void BinaryFormat::close()
{
	MultiFileSystem* rootFS = BWResource::instance().fileSystem();
	MF_ASSERT( rootFS != NULL );

	if (pHeader_)
	{
		MF_ASSERT( pFileStream_ != NULL );
		pFileStream_->memoryUnmap( pHeader_ );
	}

	pHeader_ = NULL;
	pSectionHeaders_ = NULL;
	pFileStream_ = NULL;
	memset( &mappedSections_, 0, sizeof(mappedSections_) );
	resourceID_ = "";
}


// ----------------------------------------------------------------------------
const char* BinaryFormat::resourceID() const
{
	return resourceID_.c_str();
}


// ----------------------------------------------------------------------------
bool BinaryFormat::isValid() const
{
	return pFileStream_ && pHeader_ != NULL && pSectionHeaders_ != NULL;
}


// ----------------------------------------------------------------------------
size_t BinaryFormat::numSections() const
{
	MF_ASSERT( this->isValid() );
	return pHeader_->numSections_;
}


// ----------------------------------------------------------------------------
FourCC BinaryFormat::sectionMagic( size_t sectionIdx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( sectionIdx < this->numSections() );
	return pSectionHeaders_[sectionIdx].magic_;
}


// ----------------------------------------------------------------------------
uint32 BinaryFormat::sectionVersion( size_t sectionIdx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( sectionIdx < this->numSections() );
	return pSectionHeaders_[sectionIdx].version_;
}


// ----------------------------------------------------------------------------
uint64 BinaryFormat::sectionLength( size_t sectionIdx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( sectionIdx < this->numSections() );
	return pSectionHeaders_[sectionIdx].length_;
}


// ----------------------------------------------------------------------------
size_t BinaryFormat::findSection( FourCC magic )
{
	for (size_t i = 0; i < this->numSections(); ++i)
	{
		if (this->sectionMagic( i ) == magic)
		{
			return i;
		}
	}

	return NOT_FOUND;
}


// ----------------------------------------------------------------------------
BinaryFormat::Stream* BinaryFormat::openSection( size_t sectionIdx, 
	bool writable )
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( sectionIdx < this->numSections() );
	MF_ASSERT( mappedSections_[sectionIdx].ptr_ == NULL ); // TODO: ref count?

	mappedSections_[sectionIdx].ptr_ =
		pFileStream_->memoryMap(
					(size_t)(baseFileOffset_ + pSectionHeaders_[sectionIdx].offset_),
					(size_t)pSectionHeaders_[sectionIdx].length_,
					writable );

	mappedSections_[sectionIdx].stream_ =
		Stream( (void*)mappedSections_[sectionIdx].ptr_,
				(size_t)pSectionHeaders_[sectionIdx].length_ );

	return &mappedSections_[sectionIdx].stream_;
}


// ----------------------------------------------------------------------------
BinaryFormat::Stream* BinaryFormat::findAndOpenSection( FourCC magic,
						   uint32 requiredVersion, const char* nameForErrors,
						   bool writable )
{
	size_t sectionIdx = this->findSection( magic );
	if (sectionIdx == BinaryFormat::NOT_FOUND)
	{
		ASSET_MSG( "'%s' does not contain %s.\n",
			resourceID_.c_str(), nameForErrors );
		return NULL;
	}

	uint32 foundVersion = this->sectionVersion( sectionIdx );
	if (foundVersion != requiredVersion)
	{
		ASSET_MSG( "'%s' has the wrong version of %s (expected %d, got %d).\n",
			resourceID_.c_str(), nameForErrors, foundVersion, requiredVersion );
		return NULL;
	}

	Stream* pStream = this->openSection( sectionIdx, writable );
	if (!pStream)
	{
		ASSET_MSG( "'%s' failed to map %s into memory.\n",
			resourceID_.c_str(), nameForErrors );
		return NULL;
	}

	return pStream;
}


// ----------------------------------------------------------------------------
void BinaryFormat::closeSection( Stream* pStream )
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( pStream != NULL );
	
	void* ptr = NULL;
	for (size_t i = 0; i < BinaryFormatTypes::MAX_SECTIONS; ++i)
	{
		if (pStream == &mappedSections_[i].stream_ &&
			mappedSections_[i].ptr_ != NULL)
		{
			ptr = mappedSections_[i].ptr_;
			mappedSections_[i].ptr_ = NULL;
			mappedSections_[i].stream_.reset();
			continue;
		}
	}
	
	if (!ptr)
	{
		return;
	}

	pFileStream_->memoryUnmap( ptr );
}

} // namespace CompiledSpace
} // namespace BW
