#include "pch.hpp"

#include "binary_format_writer.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "math/boundbox.hpp"

namespace BW {
namespace CompiledSpace {



// ----------------------------------------------------------------------------
void BinaryFormatWriter::Stream::writeRaw( const void* pBuff, size_t size )
{
	data_.append( (char*)pBuff, size );
}


// ----------------------------------------------------------------------------
void BinaryFormatWriter::Stream::write( const uint32& value )
{
	this->writeRaw( (void*)&value, sizeof(uint32) );
}


// ----------------------------------------------------------------------------
void BinaryFormatWriter::Stream::write( const DynamicLooseOctree & tree )
{
	// Configuration
	write( tree.storage().config_ );

	// Streams
	write( tree.storage().bounds_ );
	write( tree.storage().centers_ );
	write( tree.storage().nodes_ );
	write( tree.storage().dataReferences_ );
	write( tree.storage().parents_ );
}


// ----------------------------------------------------------------------------
uint64 BinaryFormatWriter::Stream::size() const
{
	return (uint64)data_.size(); 
}


// ----------------------------------------------------------------------------
void* BinaryFormatWriter::Stream::data()
{
	return (void*)&data_[0]; 
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
BinaryFormatWriter::BinaryFormatWriter()
{

}


// ----------------------------------------------------------------------------
BinaryFormatWriter::~BinaryFormatWriter()
{
	this->clear();
}


// ----------------------------------------------------------------------------
size_t BinaryFormatWriter::numSections() const
{
	return sections_.size();
}


// ----------------------------------------------------------------------------
BinaryFormatWriter::Stream* BinaryFormatWriter::appendSection( 
	const FourCC& magic, uint32 version )
{
	MF_ASSERT( this->numSections() < BinaryFormatTypes::MAX_SECTIONS );

	for (size_t i = 0; i < sections_.size(); ++i)
	{
		if (sections_[i].magic_ == magic)
		{
			CRITICAL_MSG( "BinaryFormatWriter::appendSection:"
				"got duplicate section magic %x.\n", magic.value_ );
			return NULL;
		}
	}

	Section section;
	section.magic_ = magic;
	section.version_ = version;
	
	sections_.push_back( section );

	return &(sections_.back().stream_);
}


// ----------------------------------------------------------------------------
void BinaryFormatWriter::clear()
{
	sections_.clear();
}


// ----------------------------------------------------------------------------
bool BinaryFormatWriter::write( const char* filename )
{
	typedef BinaryFormatTypes::Header Header;
	typedef BinaryFormatTypes::SectionHeader SectionHeader;

	MultiFileSystem* rootFS = BWResource::instance().fileSystem();
	MF_ASSERT( rootFS != NULL );

	FILE* pFile = rootFS->posixFileOpen( filename, "wb" );
	if (!pFile)
	{
		return false;
	}


	// Container format header
	Header header;
	memset( &header, 0, sizeof(header) );

	header.magic_ = BinaryFormatTypes::FORMAT_MAGIC;
	header.version_ = BinaryFormatTypes::FORMAT_VERSION;
	header.numSections_ = (uint32)this->numSections();
	header.headerSize_ = (uint32)(sizeof(Header) + 
		sizeof(SectionHeader) * this->numSections());

	if (fwrite( &header, sizeof(header), 1, pFile ) != 1)
	{
		fclose( pFile );
		return false;
	}


	// Section headers
	uint64 sectionOffset = header.headerSize_;
	for (size_t i = 0; i < sections_.size(); ++i)
	{
		SectionHeader sectionHeader;
		memset( &sectionHeader, 0, sizeof(sectionHeader) );

		sectionHeader.magic_ = sections_[i].magic_;
		sectionHeader.version_ = sections_[i].version_;
		sectionHeader.offset_ = sectionOffset;
		sectionHeader.length_ = sections_[i].stream_.size();

		if (fwrite( &sectionHeader, sizeof(sectionHeader), 1, pFile ) != 1)
		{
			fclose( pFile );
			return false;
		}

		sectionOffset += sectionHeader.length_;
	}


	// Section payloads
	for (size_t i = 0; i < sections_.size(); ++i)
	{
		if (fwrite( sections_[i].stream_.data(),
			(size_t)sections_[i].stream_.size(), 1, pFile ) != 1)
		{
			fclose( pFile );
			return false;
		}
	}

	fclose( pFile );
	return true;
}

} // namespace CompiledSpace
} // namespace BW
