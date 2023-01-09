#include "pch.hpp"

#include "vector_data_types.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_types.hpp"
#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"

#if defined( SCRIPT_PYTHON )
#include "pyscript/script_math.hpp"
#endif


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: VectorDataType
// -----------------------------------------------------------------------------

// This is used to implement some helper functions used to implement the
// VectorDataType template.

#define VECTOR_DATA_TYPE_HELPERS( VECTOR )									\
void fromSectionToVector( DataSectionPtr pSection, VECTOR & v )				\
{																			\
	v = pSection->as##VECTOR();												\
}																			\
																			\
void fromVectorToSection( const VECTOR & v, DataSectionPtr pSection )		\
{																			\
	pSection->set##VECTOR( v );												\
}																			\

namespace Helpers
{

VECTOR_DATA_TYPE_HELPERS( Vector2 )
VECTOR_DATA_TYPE_HELPERS( Vector3 )
VECTOR_DATA_TYPE_HELPERS( Vector4 )

} // namespace Helpers


/**
 *	Constructor.
 */
template <class VECTOR>
VectorDataType< VECTOR >::VectorDataType( MetaDataType * pMeta ) :
	DataType( pMeta, /*isConst:*/false )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
template <class VECTOR>
bool VectorDataType< VECTOR >::isSameType( ScriptObject pValue )
{
#if defined( SCRIPT_PYTHON )
	if (PyVector< VECTOR >::Check( pValue ))
	{
		return true;
	}
	else
#endif // SCRIPT_PYTHON

	if (ScriptSequence::check( pValue ))
	{
		ScriptSequence seq( pValue );

		if (seq.size() == NUM_ELEMENTS)
		{
			float f;
			ScriptErrorClear clearError;
			for (int i = 0; i < NUM_ELEMENTS; i++)
			{
				if (!seq.getItem( i, clearError ).convertTo( f, clearError ))
				{
					return false;
				}
			}

			return true;
		}
	}

	return false;
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
template <class VECTOR>
void VectorDataType< VECTOR >::setDefaultValue( DataSectionPtr pSection )
{
	if (pSection)
	{
		Helpers::fromSectionToVector( pSection, defaultValue_ );
	}
	else
	{
		defaultValue_.setZero();
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
template <class VECTOR>
bool VectorDataType< VECTOR >::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
template <class VECTOR>
int VectorDataType< VECTOR >::streamSize() const
{
	return sizeof( VECTOR );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
template <class VECTOR>
bool VectorDataType< VECTOR >::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	VECTOR value;

	if (!source.read( value ))
	{
		return false;
	}

	Helpers::fromVectorToSection( value, pSection );

	return true;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
template <class VECTOR>
bool VectorDataType< VECTOR >::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	VECTOR value;
	Helpers::fromSectionToVector( pSection, value );

	return sink.write( value );
}

template <class VECTOR>
bool VectorDataType< VECTOR >::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	VECTOR value;
	stream >> value;
	if (stream.error()) return false;

	Helpers::fromVectorToSection( value, pSection );
	return true;
}

template <class VECTOR>
void VectorDataType< VECTOR >::addToMD5( MD5 & md5 ) const
{
	md5.append( "Vector", sizeof( "Vector" ) );
	md5.append( &NUM_ELEMENTS, sizeof(int) );
}

template <class VECTOR>
DataType::StreamElementPtr VectorDataType< VECTOR >::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new VectorStreamElementT( *this ) );
	}
	return StreamElementPtr();
}


template <class VECTOR>
bool VectorDataType< VECTOR >::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const VectorDataType& otherVec =
		static_cast< const VectorDataType& >( other );
	return (defaultValue_ < otherVec.defaultValue_);
}

template <class VECTOR>
const int VectorDataType< VECTOR >::NUM_ELEMENTS =
	sizeof( VECTOR )/sizeof( float );


SIMPLE_DATA_TYPE( VectorDataType< Vector2 >, VECTOR2 )
SIMPLE_DATA_TYPE( VectorDataType< Vector3 >, VECTOR3 )
SIMPLE_DATA_TYPE( VectorDataType< Vector4 >, VECTOR4 )

BW_END_NAMESPACE

// vector_data_types.cpp
