#include "pch.hpp"

#include "long_integer_data_type.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: LongIntegerDataType
// -----------------------------------------------------------------------------

/**
*	This template class is used to represent the different types of integer data
*	type.
*
*	@ingroup entity
*/
template <class INT_TYPE>
LongIntegerDataType< INT_TYPE >::LongIntegerDataType( MetaDataType * pMeta ) :
	DataType( pMeta )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::isSameType( ScriptObject pValue )
{
	INT_TYPE value;
	return pValue.convertTo( value, ScriptErrorClear() );
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
template <class INT_TYPE>
void LongIntegerDataType< INT_TYPE >::setDefaultValue( DataSectionPtr pSection )
{
	defaultValue_ = (pSection) ? pSection->as< INT_TYPE >() : 0L;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
template <class INT_TYPE>
int LongIntegerDataType< INT_TYPE >::streamSize() const
{
	return sizeof( INT_TYPE );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::addToSection( DataSource & source,
	DataSectionPtr pSection ) const
{
	INT_TYPE value;

	if (!source.read( value ))
	{
		return false;
	}

	pSection->be( value );
	return true;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::createFromSection( DataSectionPtr pSection,
	DataSink & sink ) const
{
	INT_TYPE value = pSection->as<INT_TYPE>();

	return sink.write( value );
}

template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	INT_TYPE value = 0;
	stream >> value;
	if (stream.error()) return false;

	pSection->be( value );
	return true;
}

template <class INT_TYPE>
void LongIntegerDataType< INT_TYPE >::addToMD5( MD5 & md5 ) const
{
	if (std::numeric_limits<INT_TYPE>::is_signed)
		md5.append( "Int", sizeof( "Int" ) );
	else
		md5.append( "Uint", sizeof( "Uint" ) );
	int size = sizeof(INT_TYPE);
	md5.append( &size, sizeof(int) );
}


template <class INT_TYPE>
DataType::StreamElementPtr LongIntegerDataType< INT_TYPE >::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		typedef SimpleStreamElement<INT_TYPE> LongIntegerStreamElementT;
		return StreamElementPtr( new LongIntegerStreamElementT( *this ) );
	}
	return StreamElementPtr();
}


template <class INT_TYPE>
bool LongIntegerDataType< INT_TYPE >::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const LongIntegerDataType& otherInt =
		static_cast< const LongIntegerDataType& >( other );
	return defaultValue_ < otherInt.defaultValue_;
}


#ifdef _LP64
// Types in integer_data_type.cpp
#else
SIMPLE_DATA_TYPE( LongIntegerDataType<uint32>, UINT32 )
SIMPLE_DATA_TYPE( LongIntegerDataType<int64>, INT64 )
#endif

SIMPLE_DATA_TYPE( LongIntegerDataType<uint64>, UINT64 )

BW_END_NAMESPACE

// long_integer_data_type.cpp
