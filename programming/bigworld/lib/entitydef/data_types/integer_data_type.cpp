#include "pch.hpp"

#include "integer_data_type.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: IntegerDataType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
template <class INT_TYPE>
IntegerDataType< INT_TYPE >::IntegerDataType( MetaDataType * pMeta ) :
	DataType( pMeta )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::isSameType( ScriptObject pValue )
{
	NativeLong intValue;

	if (!pValue.convertTo( intValue, ScriptErrorClear() ))
	{
		return false;
	}

	INT_TYPE value = INT_TYPE( intValue );
	if (intValue != NativeLong( value ))
	{
		ERROR_MSG( "IntegerDataType::isSameType: "
				"%" PRIzd " is out of range (truncated = %" PRIzd ").\n",
			(ssize_t)intValue, ssize_t(value) );

		return false;
	}

	return true;
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
template <class INT_TYPE>
void IntegerDataType< INT_TYPE >::setDefaultValue( DataSectionPtr pSection )
{
	defaultValue_ = (pSection) ? pSection->as< INT_TYPE >() : 0;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
template <class INT_TYPE>
int IntegerDataType< INT_TYPE >::streamSize() const
{
	return sizeof( INT_TYPE );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::addToSection( DataSource & source,
	DataSectionPtr pSection ) const
{
	INT_TYPE value;

	if (!source.read( value ))
	{
		return false;
	}

	pSection->setLong( value );
	return true;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::createFromSection( DataSectionPtr pSection,
	DataSink & sink ) const
{
	long longValue = pSection->asLong();
	INT_TYPE value = (INT_TYPE) longValue;
	MF_ASSERT_DEV( long(value) == longValue );

	return sink.write( value );
}

template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	INT_TYPE value = 0;
	stream >> value;
	if (stream.error()) return false;

	pSection->setLong( value );
	return true;
}

template <class INT_TYPE>
void IntegerDataType< INT_TYPE >::addToMD5( MD5 & md5 ) const
{
	if (std::numeric_limits<INT_TYPE>::is_signed)
		md5.append( "Int", sizeof( "Int" ) );
	else
		md5.append( "Uint", sizeof( "Uint" ) );
	int size = sizeof(INT_TYPE);
	md5.append( &size, sizeof(int) );
}


template <class INT_TYPE>
DataType::StreamElementPtr IntegerDataType< INT_TYPE >::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		typedef SimpleStreamElement< INT_TYPE > IntegerStreamElementT;
		return StreamElementPtr( new IntegerStreamElementT( *this ) );
	}
	return StreamElementPtr();
}


template <class INT_TYPE>
bool IntegerDataType< INT_TYPE >::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const IntegerDataType& otherInt =
		static_cast< const IntegerDataType& >( other );
	return defaultValue_ < otherInt.defaultValue_;
}


SIMPLE_DATA_TYPE( IntegerDataType< uint8 >,  UINT8 )
SIMPLE_DATA_TYPE( IntegerDataType< uint16 >, UINT16 )

SIMPLE_DATA_TYPE( IntegerDataType< int8 >,  INT8 )
SIMPLE_DATA_TYPE( IntegerDataType< int16 >, INT16 )
SIMPLE_DATA_TYPE( IntegerDataType< int32 >, INT32 )

#ifdef _LP64
SIMPLE_DATA_TYPE( IntegerDataType< uint32 >, UINT32 )
SIMPLE_DATA_TYPE( IntegerDataType< int64 >, INT64 )
#else
// Types in long_integer_data_type.cpp
#endif

BW_END_NAMESPACE

// integer_data_type.cpp
