#ifndef CIRCULAR_ARRAY_HPP
#define CIRCULAR_ARRAY_HPP

#include "cstdmf/stdmf.hpp"

#include <string.h>

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	@internal
 *	A template class to wrap a circular array of various sizes,
 *	as long as the size is a power of two.
 */
template <class T> class CircularArray
{
public:
	typedef CircularArray<T> OurType;

	CircularArray( uint size ) : data_( new T[size] ), mask_( size-1 ) 
	{ 
		memset( data_, 0, sizeof(T) * this->size() );
	}
	~CircularArray()	{ bw_safe_delete_array(data_); }

	uint size() const	{ return mask_+1; }

	const T & operator[]( uint n ) const	{ return data_[n&mask_]; }
	T & operator[]( uint n )				{ return data_[n&mask_]; }
	void swap( OurType & other )
	{
		T * data = data_;
		uint mask = mask_;

		data_ = other.data_;
		mask_ = other.mask_;

		other.data_ = data;
		other.mask_ = mask;
	}

	void inflateToAtLeast( size_t newSize )
	{
		if (newSize > this->size())
		{
			uint size = this->size();
			while (newSize > size)
			{
				size *= 2;
			}

			OurType newWindow( size );
			this->swap( newWindow );
		}
	}

	void doubleSize( uint32 startIndex )
	{
		OurType newWindow( this->size() * 2 );

		for (uint i = 0; i < this->size(); ++i)
		{
			newWindow[ startIndex + i ] = (*this)[ startIndex + i ];
		}

		this->swap( newWindow );
	}
private:
	CircularArray( const OurType & other );
	OurType & operator=( const OurType & other );

	T * data_;
	uint mask_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // CIRCULAR_ARRAY_HPP
