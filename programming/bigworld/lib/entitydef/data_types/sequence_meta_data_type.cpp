#include "pch.hpp"

#include "sequence_meta_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "array_data_type.hpp"
#include "tuple_data_type.hpp"

#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SequenceMetaDataType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SequenceMetaDataType::SequenceMetaDataType( const char * name,
		SequenceTypeFactory factory ) :
	name_( name ),
	factory_( factory )
{
	MetaDataType::addMetaType( this );
}


/**
 *	Destructor.
 */
SequenceMetaDataType::~SequenceMetaDataType()
{
	MetaDataType::delMetaType( this );
}


DataTypePtr SequenceMetaDataType::getType( DataSectionPtr pSection )
{
	int size = pSection->readInt( "size", 0 );
	bool persistAsBlob = pSection->readBool( "persistAsBlob", false );

	DataTypePtr pOfType =
		DataType::buildDataType( pSection->openSection( "of" ) );

	if (pOfType)
	{
		// Store in MEDIUMBLOB column if persistAsBlob is true
		// Retaining dbLen instead of passing true/false just in case someone
		// complains and wants to control the size of the column. We can
		// implement that quite easily with another pSection->readInt() in here.
		int dbLen = persistAsBlob ? 16777215 : 0;
		return (*factory_)( this, pOfType, size, dbLen );
	}

	ERROR_MSG( "SequenceMetaDataType::getType: "
		"Unable to create sequence of '%s'\n",
			pSection->readString( "of" ).c_str() );

	return NULL;
}


static SequenceMetaDataType s_ARRAY_metaDataType( "ARRAY",
		&ArrayDataType::construct );
DATA_TYPE_LINK_ITEM( ARRAY )

static SequenceMetaDataType s_TUPLE_metaDataType( "TUPLE",
		&TupleDataType::construct );
DATA_TYPE_LINK_ITEM( TUPLE )

BW_END_NAMESPACE

// sequence_meta_data_type.cpp
