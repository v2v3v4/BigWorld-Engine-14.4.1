#ifndef SEQUENCE_VALUE_TYPE_HPP
#define SEQUENCE_VALUE_TYPE_HPP


BW_BEGIN_NAMESPACE

template< typename T, size_t SIZE >
class SequenceValueType;

template < typename T >
BinaryIStream & operator>> ( BinaryIStream & stream,
	SequenceValueType< T, 0 > & sequence );


/**
 *	This class represents a sequence_type property value type that has an
 *	optional fixed size and wraps the equivalent un-sized property value vector
 *	type. If the SIZE parameter is 0, then use the specialized stream operator
 *	methods.
 */
template< typename T, size_t SIZE = 0 >
class SequenceValueType
{
public:
	typedef BW::vector< T > sequence_type;
	typedef typename sequence_type::value_type value_type;
	typedef typename sequence_type::const_iterator const_iterator;
	typedef typename sequence_type::const_reference const_reference;
	typedef typename sequence_type::size_type size_type;

	SequenceValueType() : sequence_( SIZE )
	{}


	~SequenceValueType()
	{}

	SequenceValueType( const sequence_type & sequence ) :
			sequence_( sequence )
	{
		MF_ASSERT( SIZE == 0 || sequence_.size() == SIZE );
	}


	SequenceValueType & operator=( const sequence_type & sequence )
	{
		MF_ASSERT( SIZE == 0 || sequence.size() == SIZE );
		sequence_ = sequence;
		return * this;
	}

	void push_back( const value_type & value )
	{
		MF_ASSERT( (SIZE == 0) || (sequence_.size() < SIZE) );

		sequence_.push_back( value );
	}


	size_type size() const
	{
		return sequence_.size();
	}

	bool empty() const
	{
		return sequence_.empty();
	}


	const value_type & operator[]( size_type index ) const
	{
		return sequence_[index];
	}


	value_type & operator[]( size_type index )
	{
		return sequence_[index];
	}


	const_iterator begin() const
	{
		return sequence_.begin();
	}


	const_iterator end() const
	{
		return sequence_.end();
	}

	const_reference front() const
	{
		return sequence_.front();
	}

	const_reference back() const
	{
		return sequence_.back();
	}

	template< typename CHANGE >
	bool applyChange( CHANGE & change )
	{
			return change.apply( sequence_ );
	}

	bool operator==( const SequenceValueType & other ) const
	{
		return sequence_ == other.sequence_;
	}

	bool operator!=( const SequenceValueType & other ) const
	{
		return sequence_ != other.sequence_;
	}

	friend BinaryIStream & operator>> <> ( BinaryIStream & stream,
		SequenceValueType & sequence );

private:
	sequence_type sequence_;

};


template< typename T, size_t SIZE >
inline BinaryIStream & operator>>( BinaryIStream & stream,
	SequenceValueType< T, SIZE > & sequence )
{
	for (size_t i = 0; i < SIZE; ++i)
	{
		stream >> sequence[i];
	}

	return stream;
}


template < typename T, size_t SIZE >
inline BinaryOStream & operator<<( BinaryOStream & stream,
	const SequenceValueType< T, SIZE > & sequence )
{
	for (size_t i = 0; i < SIZE; ++i)
	{
		stream << sequence[i];
	}

	return stream;
}


/**
 *	These specialized methods are used for variable length sequence property
 *	value types. Unlike the default fixed size SequenceValueType, the sequence
 *	size is streamed with packed int.
 */

//	Note: the missing 'inline' is intended to avoid the VS C4396 warning:
//	"A specialization of a function template cannot specify any of the inline
//	specifiers. The compiler issues warning C4396 and ignores the inline
//	specifier." - http://msdn.microsoft.com/en-us/library/bb384968(v=vs.90).aspx
template< typename T >
BinaryIStream & operator>>( BinaryIStream & stream,
	SequenceValueType< T > & sequence )
{
	size_t size = stream.readPackedInt();
	sequence.sequence_.resize( size );

	for (size_t i = 0; i < size; ++i)
	{
		stream >> sequence.sequence_[i];
	}

	return stream;
}


template < typename T >
inline BinaryOStream & operator<<( BinaryOStream & stream,
	const SequenceValueType< T > & sequence )
{
	size_t size = sequence.size();
	stream.writePackedInt(( int )size);

	for (size_t i = 0; i < size; ++i)
	{
		stream << sequence[i];
	}

	return stream;
}


template< typename T >
inline bool isEqual( const SequenceValueType< T > & v1,
	const SequenceValueType< T > & v2 )
{
	typedef bool (*comparator)( const T &, const T & );

	return v1.size() == v2.size() &&
		std::equal( v1.begin(), v1.end(), v2.begin(),
			static_cast< comparator >( isEqual< T > ) );
}


BW_END_NAMESPACE

#endif // SEQUENCE_VALUE_TYPE_HPP

