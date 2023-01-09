#ifndef QUICK_FILE_WRITER_HPP
#define QUICK_FILE_WRITER_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_vector.hpp"
#include "resmgr/binary_block.hpp"

BW_BEGIN_NAMESPACE

class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryPtr;


/**
 *	This class is used to used to write from a buffer to a
 *	binary block.
 */
class QuickFileWriter
{
public:
	QuickFileWriter() { }

	template <class T> QuickFileWriter & operator <<( const T & t )
	{
		int len = sizeof( T );
		data_.append( (char*)&t, len );
		return *this;
	}

	template <class E> QuickFileWriter & operator <<( BW::vector<E> & v )
	{
		this->writeV( v );
		return *this;
	}

	template <class E> QuickFileWriter & operator <<( const BW::vector<E> & v )
	{
		this->writeV( v );
		return *this;
	}

	template <class T> QuickFileWriter & write( const T & t )
	{
		this->write( (void*)&t, sizeof(T) );
		return *this;
	}

	template <class E> QuickFileWriter & write( const BW::vector<E> & v )
	{
		this->writeV( v );
		return *this;
	}

	QuickFileWriter& write( const void* buffer, size_t size )
	{
		data_.append( (char*) buffer, size );
		return *this;
	}

	BinaryPtr output()
	{
		return new BinaryBlock( data_.data(), data_.size(), "BinaryBlock/QuickFileWriter" );
	}

private:
	BW::string data_;

	template <class E> void writeV( const BW::vector<E> & v )
	{
		if (v.empty())
		{
			return;
		}
		int elen = sizeof( E ); // Quenya: star :)
		int vlen = static_cast<int>(elen * v.size());
		data_.append( (char*) &v.front(), vlen );
	}
};

BW_END_NAMESPACE

#endif // QUICK_FILE_WRITER_HPP
