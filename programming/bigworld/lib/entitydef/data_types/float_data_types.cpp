#include "pch.hpp"

#include "float_data_types.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: FloatDataType
// -----------------------------------------------------------------------------

// TODO: Need to think about how to do float types properly. There is probably
// a few parameter to these types. E.g. How many bits are used, are they fixed
// point floats, and what are the range of values.

/**
 *	Constructor.
 */
template <class FLOAT_TYPE>
FloatDataType< FLOAT_TYPE >::FloatDataType( MetaDataType * pMeta ) :
	DataType( pMeta )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::isSameType( ScriptObject pValue )
{
	FLOAT_TYPE floatValue;
	return pValue.convertTo( floatValue, ScriptErrorClear() );
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
template <class FLOAT_TYPE>
void FloatDataType< FLOAT_TYPE >::setDefaultValue( DataSectionPtr pSection )
{
	defaultValue_ = (pSection) ? pSection->as< FLOAT_TYPE >() : 0.f;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
template <class FLOAT_TYPE>
int FloatDataType< FLOAT_TYPE >::streamSize() const
{
	return sizeof( FLOAT_TYPE );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	FLOAT_TYPE value;

	if (!source.read( value ))
	{
		Script::clearError();
		return false;
	}

	pSection->be< FLOAT_TYPE >( value );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	FLOAT_TYPE value = pSection->as< FLOAT_TYPE >();
	return sink.write( value );
}


template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	IF_NOT_MF_ASSERT_DEV( pSection )
	{
		return false;
	}

	FLOAT_TYPE value = 0;
	stream >> value;
	if (stream.error()) return false;

	pSection->be< FLOAT_TYPE >( value );
	return true;
}


template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool isPersistentOnly ) const
{
	FLOAT_TYPE value = pSection->as< FLOAT_TYPE >();
	stream << value;
	return true;
}


template <class FLOAT_TYPE>
void FloatDataType< FLOAT_TYPE >::addToMD5( MD5 & md5 ) const
{
	// TODO: Make this more generic!
	if (sizeof(FLOAT_TYPE) == sizeof(float))
		md5.append( "Float", sizeof( "Float" ) );
	else
		md5.append( "Float64", sizeof( "Float64" ) );
}


template <class FLOAT_TYPE>
DataType::StreamElementPtr FloatDataType< FLOAT_TYPE >::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		typedef SimpleStreamElement< FLOAT_TYPE > FloatStreamElementT;
		return StreamElementPtr( new FloatStreamElementT( *this ) );
	}
	return StreamElementPtr();
}


template <class FLOAT_TYPE>
bool FloatDataType< FLOAT_TYPE >::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const FloatDataType& otherFloat =
		static_cast< const FloatDataType& >( other );
	return defaultValue_ < otherFloat.defaultValue_;
}


/// Datatype for floats.
SIMPLE_DATA_TYPE( FloatDataType< float >, FLOAT32 )
SIMPLE_DATA_TYPE( FloatDataType< double >, FLOAT64 )

BW_END_NAMESPACE

// float_data_types.cpp
