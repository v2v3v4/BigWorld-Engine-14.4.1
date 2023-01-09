#ifndef NESTED_PROPERTY_CHANGE_HPP
#define NESTED_PROPERTY_CHANGE_HPP


#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bit_reader.hpp"
#include "cstdmf/value_or_null.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"
#include "sequence_value_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a more lightweight version of the PropertyChange class in the
 *	entitydef library. It reads the encoded change path and is responsible for
 *	streaming into the various property types.
 */
class NestedPropertyChange
{
public:

	// These have to match with entitydef/property_change.cpp.
	static const int MAX_SIMPLE_PROPERTY_CHANGE_ID = 60;
	static const int PROPERTY_CHANGE_ID_SINGLE = 61;
	static const int PROPERTY_CHANGE_ID_SLICE = 62;

	enum ChangeType
	{
		SINGLE,
		SLICE
	};

	typedef BW::vector< int32 > Path;
	typedef std::pair< int, int > Slice;

	NestedPropertyChange( ChangeType changeType, BinaryIStream & data );
	~NestedPropertyChange();

	int readNextIndex( uint propertyNumChildren );


	/**
	 *	This method returns whether there are more indices to be retrieved.
	 */
	bool isFinished() const
	{
		return isFinished_;
	}


	/**
	 *	This method returns the change path accumulated so far.
	 */
	const Path & path() const
	{
		return path_;
	}


	/**
	 *	This method returns the data stream associated with this change.
	 *
	 *	Should only be used for streaming leaf properties.
	 */
	BinaryIStream & data()
	{
		return data_;
	}


	/**
 	 *	This method applies this change to a value.
	 *
	 *	This templated function can be specialised for complex-typed values.
	 */
	template< typename TYPE >
	bool apply( TYPE & value )
	{
		data_ >> value;
		return !data_.error();
	}


	template< typename T >
	bool apply( BW::vector< T > & valueVector );


	/**
	 *	This method applies this change to a SequenceValueType value. 
	 */
	template< typename T >
	bool apply( SequenceValueType< T > & valueSequence )
	{
		return valueSequence.applyChange( *this );
	}


	/**
	 *	This method applies this change to a 'AllowNone' FIXED_DICT value.
	 */
	template< typename T >
	bool apply( ValueOrNull< T > & valueOrNull )
	{
		IF_NOT_MF_ASSERT_DEV( !valueOrNull.isNull() )
		{
			return false;
		}
		return this->apply( *(valueOrNull.get()) );
	}


	/**
	 *	This method returns the previous slice range that was modified in this
	 *	change, if any.
	 */
	const Slice & oldSlice() const
	{
		return oldSlice_;
	}


	/**
	 *	This method returns the new slice range that was added, if any.
	 */
	const Slice & newSlice() const
	{
		return newSlice_;
	}

private:
	int readNextIndexInternal( uint propertyNumChildren );
	void readOldSlice( uint propertyNumChildren );

	ChangeType changeType_;
	BitReader headerBits_;
	BinaryIStream & data_;
	bool isFinished_;
	Path path_;
	Slice oldSlice_;
	Slice newSlice_;
};


/**
 *	Template function specialisation for BW::vector.
 */
template< typename T >
bool NestedPropertyChange::apply( BW::vector< T > & valueVector )
{
	isFinished_ = (headerBits_.get( 1 ) == 0);
	uint numValues = (uint)valueVector.size();

	if (changeType_ == SINGLE)
	{
		int index = this->readNextIndexInternal( numValues );

		T & value = valueVector[index];
		if (!this->isFinished())
		{
			return this->apply( value );
		}
		else
		{
			data_ >> value;
			return !data_.error();
		}
	}
	else if (changeType_ == SLICE)
	{
		if (!isFinished_)
		{
			int index = this->readNextIndexInternal( numValues );
			return this->apply( valueVector[index] );
		}
		else
		{
			this->readOldSlice( numValues );

			typename BW::vector< T >::iterator oldBegin = 
				valueVector.begin() + oldSlice_.first;
			typename BW::vector< T >::iterator oldEnd = 
				valueVector.begin() + oldSlice_.second;

			valueVector.erase( oldBegin, oldEnd );

			newSlice_.first = newSlice_.second = oldSlice_.first;

			while (data_.remainingLength())
			{
				T value;
				data_ >> value;
				valueVector.insert( valueVector.begin() + newSlice_.second,
					value );

				++newSlice_.second;
			}

			return !data_.error();
		}
	}

	return false;
}

BW_END_NAMESPACE

#endif // NESTED_PROPERTY_CHANGE_HPP

