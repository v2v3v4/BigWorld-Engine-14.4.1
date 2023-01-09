#include "pch.hpp"

#include "tuple_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TupleDataType
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 *
 *	@param pMeta The meta type.
 *	@param elementType The type of the tuple's elements.
 *	@param size	The size of the tuple. If size is 0, the tuple is of
 *				variable size.
 *	@param dbLen	The database length of this property type.
 */
TupleDataType::TupleDataType( MetaDataType * pMeta, DataTypePtr elementType,
		int size, int dbLen ) :
	SequenceDataType( pMeta, elementType, size, dbLen,
			/*isConst:*/true )
{
}


DataType * TupleDataType::construct( MetaDataType * pMeta,
	DataTypePtr elementType, int size, int dbLen )
{
	return new TupleDataType( pMeta, elementType, size, dbLen );
}


bool TupleDataType::startSequence( DataSink & sink, size_t count ) const
{
	return sink.beginTuple( this, count );
}


int TupleDataType::compareDefaultValue( const DataType & other ) const
{
	const TupleDataType& otherTuple =
			static_cast< const TupleDataType& >( other );

	if (pDefaultSection_)
	{
		return pDefaultSection_->compare( otherTuple.pDefaultSection_ );
	}

	return (otherTuple.pDefaultSection_) ? -1 : 0;
}


void TupleDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Tuple", sizeof( "Tuple" ) );
	this->SequenceDataType::addToMD5( md5 );
}


void TupleDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}


bool TupleDataType::getDefaultValue( DataSink & output ) const
{
	if (!pDefaultSection_)
	{
		return this->createDefaultValue( output );
	}
	return this->createFromSection( pDefaultSection_, output );
}

BW_END_NAMESPACE

// tuple_data_type.cpp
