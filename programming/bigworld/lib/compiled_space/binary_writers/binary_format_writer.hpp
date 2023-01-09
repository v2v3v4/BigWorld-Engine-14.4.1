#ifndef BINARY_FORMAT_WRITER_HPP
#define BINARY_FORMAT_WRITER_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format_types.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_vector.hpp"
#include "math/loose_octree.hpp"

BW_BEGIN_NAMESPACE

	class BinaryBlock;
	typedef SmartPointer<BinaryBlock> BinaryPtr;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class BinaryFormatWriter
{
public:
	class Stream
	{
	public:

		void writeRaw( const void* pBuff, size_t size );

		void write( const uint32& value );
		void write( const DynamicLooseOctree & tree );


		template <class T>
		void write( const T & t );

		template <class E> 
		void write( const BW::vector<E> & v );

		uint64 size() const;
		void* data();

	private:
		BW::string data_;
	};

public:
	BinaryFormatWriter();
	~BinaryFormatWriter();

	size_t numSections() const;
	Stream* appendSection( const FourCC& magic, uint32 version );

	void clear();

	bool write( const char* filename );

private:
	struct Section
	{
		FourCC magic_;
		uint32 version_;
		Stream stream_;
	};

private:
	BW::vector<Section> sections_;
};


/////////
template <class T>
void BinaryFormatWriter::Stream::write( const T & t )
{
	BW_STATIC_ASSERT( std::is_standard_layout<T>::value, Must_Be_Standard_Layout );
	this->write( (uint32)sizeof(T) );
	this->writeRaw( (void*)&t, sizeof(T) );
}

template <class T> 
void BinaryFormatWriter::Stream::write( const BW::vector<T> & v )
{
	BW_STATIC_ASSERT( std::is_standard_layout<T>::value, Must_Be_Standard_Layout );
	MF_ASSERT( data_.size() <= std::numeric_limits<uint32>::max() );

	this->write( (uint32)sizeof(T) );

	uint32 count = (uint32)v.size();
	this->write( count );

	if (count > 0)
	{
		size_t dataLen = count * sizeof(T);
		data_.append( (char*) &v.front(), dataLen );
	}
}

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_BINARY_HPP
