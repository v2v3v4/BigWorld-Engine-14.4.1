#ifndef VALUE_OR_NULL_HPP
#define VALUE_OR_NULL_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent values that might also be NULL.
 */
template <class T>
class ValueOrNull
{
public:
	/**
	 *	Constructor.
	 */
	ValueOrNull() : 
			value_(), 
			isNull_( true ) 
	{}


	/**
	 *	Constructor.
	 */
	explicit ValueOrNull( const T & x ) : 
			value_( x ),
			isNull_( false )
	{}
	

	/**
	 *	Copy constructor.
	 */
	template <class OTHER>
	ValueOrNull( const ValueOrNull< OTHER > & other ):
			value_(),
			isNull_( other.isNull_ )
	{
		if (!isNull_)
		{
			value_ = other.value_;
		}
	}


	/**
	 *	Assignment operator.
	 */
	template <class OTHER>
	ValueOrNull & operator=( const ValueOrNull< OTHER > & other )
	{
		if (other.isNull_)
		{
			this->setNull();
		}
		else
		{
			this->setValue( other.value_ );
		}

		return *this;
	}


	/**
	 *	This method initialises this object from an input stream.
	 *
	 *	@param data 	The input data stream.
	 *
	 *	@return 		true on success, false on failure.
	 */
	bool initFromStream( BinaryIStream & data )
	{
		uint8 hasValue;
		data >> hasValue;

		isNull_ = !hasValue;

		if (hasValue)
		{
			return value_.initFromStream( data );
		}
		else
		{
			this->setNull();
			return true;
		}
	}


	/**
	 *	This method serialises this object onto the given output stream.
	 *
	 *	@param data 	The output data stream.
	 *
	 *	@return 		true on success, false on failure.
	 */
	bool addToStream( BinaryOStream & data )
	{
		if (isNull_)
		{
			data << uint8( 0 );
			return true;
		}
		else
		{
			data << uint8( 1 );
			return value_.addToStream( data );
		}
	}


	/**
	 *	This method sets this value to a null state.
	 */
	void setNull()				
	{ 
		isNull_ = true; 
		value_ = T();
	}

	/**
	 *	This method sets a value.
	 */
	void setValue( const T & x ) { value_ = x; isNull_ = false; }

	/**
	 *	This method returns whether this value is null.
	 */
	bool isNull() const			{ return isNull_; }

	/**
	 *	This method gets the value.
	 */
	const T * get() const		{ return isNull_ ? NULL : &value_; }
	T * get() 					{ return isNull_ ? NULL : &value_; }

protected:
	T 		value_;
	bool 	isNull_;
};


/**
 *	Streaming operator for input streams.
 *
 *	@param stream 	The input stream.
 *
 *	@return a reference to the input stream.
 */
template< typename T >
inline BinaryIStream & operator>>( BinaryIStream & stream,
		ValueOrNull< T > & value )
{
	value.initFromStream( stream );
	return stream;
}


/**
 *	Streaming operator for output streams.
 *
 *	@param stream 	The output stream.
 *
 *	@return a reference to the output stream.
 */
template< typename T >
inline BinaryOStream & operator<<( BinaryOStream & stream,
		const ValueOrNull< T > & value )
{
	value.addToStream( stream );
	return stream;
}


/**
 *	Equality operator
 */
template< typename T, typename OTHER >
bool operator==( const ValueOrNull< T > & left,
		const ValueOrNull< OTHER > & right )
{
	if (left.isNull() && right.isNull())
	{
		return true;
	}
	if (left.isNull() || right.isNull())
	{
		return false;
	}
	return (*left.get() == *right.get()); 
}


/**
 *	Inequality operator
 */
template< typename T, typename OTHER >
bool operator!=( const ValueOrNull< T > & left,
		const ValueOrNull< OTHER > & right )
{
	return !(left == right);
}

BW_END_NAMESPACE

#endif // VALUE_OR_NULL_HPP
