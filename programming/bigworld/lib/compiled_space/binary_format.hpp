#ifndef BINARY_CONTAINER_FORMAT_HPP
#define BINARY_CONTAINER_FORMAT_HPP

#include "compiled_space/forward_declarations.hpp"

#include "binary_format_types.hpp"

#include "cstdmf/static_array.hpp"
#include "math/loose_octree.hpp"

#include "resmgr/file_system.hpp"


namespace BW {
namespace CompiledSpace {


class COMPILED_SPACE_API BinaryFormat
{
public:
	// Returned by findSection if magic not found.
	static const size_t NOT_FOUND = size_t(-1);

public:
	class Stream
	{
	public:
		Stream();
		Stream( void* pBuffer, size_t length );
		Stream( Stream& other );

		bool isValid() const;

		size_t length() const;

		bool eof() const;
		size_t tell() const;
		void seek( size_t offset );

		size_t size() const;
		void reset();

		bool read( uint32& dest );

		template<class T>
		bool read( T& dest );

		template<class T>
		bool read( T*& dest );
		
		template<class T>
		bool read( ExternalArray<T>& dest );

		bool read( StaticLooseOctree& dest );

		void* readRaw( size_t len );

	private:
		void* pBuffer_;
		size_t length_, cursor_;
	};

public:
	BinaryFormat();
	~BinaryFormat();

	bool open( const char* resourceID );
	void close();

	size_t numSections() const;

	FourCC sectionMagic( size_t sectionIdx ) const;
	uint32 sectionVersion( size_t sectionIdx ) const;
	uint64 sectionLength( size_t sectionIdx ) const;

	size_t findSection( FourCC magic );

	Stream* openSection( size_t sectionIdx, bool writable = false );

	Stream* findAndOpenSection( FourCC magic,
		uint32 requiredVersion, const char* nameForErrors, 
		bool writable = false );

	void closeSection( Stream* stream );

	bool isValid() const;

	const char* resourceID() const;

private:
	BW::string resourceID_;
	uint64 baseFileOffset_;
	FileStreamerPtr pFileStream_;
	BinaryFormatTypes::Header* pHeader_;
	BinaryFormatTypes::SectionHeader* pSectionHeaders_;

	struct MappedSection
	{
		void* ptr_;
		Stream stream_;
	};

	MappedSection mappedSections_[BinaryFormatTypes::MAX_SECTIONS];
};


////////////////
template<class T>
bool BinaryFormat::Stream::read( T& dest )
{
	T* pSrc = NULL;
	this->read( pSrc );
	if (!pSrc)
	{
		return false;
	}

	memcpy( &dest, pSrc, sizeof(T) );
	return true;
}

template<class T>
bool BinaryFormat::Stream::read( T*& dest )
{
	BW_STATIC_ASSERT( std::is_standard_layout<T>::value, 
		Must_Be_Standard_Layout );

	uint32 typeSize;
	if (!this->read( typeSize ))
	{
		return false;
	}

	MF_ASSERT( typeSize == sizeof(T));
	if (typeSize != sizeof(T))
	{
		return false;
	}

	void* pSrc = this->readRaw( sizeof(T) );
	if (!pSrc)
	{
		return false;
	}

	dest = (T*)pSrc;
	return true;
}

template<class T>
bool BinaryFormat::Stream::read( ExternalArray<T>& dest )
{
	BW_STATIC_ASSERT( std::is_standard_layout<T>::value, 
		Must_Be_Standard_Layout );

	uint32 typeSize;
	if (!this->read( typeSize ))
	{
		return false;
	}

	MF_ASSERT( typeSize == sizeof(T));
	if (typeSize != sizeof(T))
	{
		return false;
	}

	uint32 num;
	if (!this->read( num ))
	{
		return false;
	}
	
	void* pData = this->readRaw( sizeof(T) * num );
	if (!pData)
	{
		return false;
	}

	dest.assignExternalStorage( (T*)pData, (size_t)num, (size_t)num );
	return true;
}

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_BINARY_HPP
