#include "pch.hpp"

#include "cstdmf/debug.hpp"

#include "data_type.hpp"
#include "data_types.hpp"


DECLARE_DEBUG_COMPONENT2( "DataTypes", 0 )


BW_BEGIN_NAMESPACE

// token to force linking this file in
int DATA_TYPES_TOKEN = 0;

class ForceLink
{
public:
	ForceLink( int linkVar )
	{
		DATA_TYPES_TOKEN |= linkVar;
	}
};

#define FORCE_LINK( NAME ) 											\
	extern int force_link_##NAME;									\
	static ForceLink local_force_link_##NAME( force_link_##NAME );	\

/**
 *	This is a stand-in data type that is used on platforms that do not support
 *	particular data types.
 */
class UnsupportedDataType : public DataType
{
public:
	UnsupportedDataType( MetaDataType * pMetaDataType ) :
		DataType( pMetaDataType ) {}

	virtual void setDefaultValue( DataSectionPtr pSection ) {}
	virtual bool getDefaultValue( DataSink & output ) const { return false; }
	virtual bool isSameType( ScriptObject pValue ) { return false; }
	virtual bool addToStream( DataSource & source, BinaryOStream & stream,
		bool isPersistentOnly )	const { return false; }
	virtual bool createFromStream( BinaryIStream & stream, DataSink & sink,
		bool isPersistentOnly ) const { return false; }
	virtual int streamSize() const { return -1; }
	virtual bool addToSection( DataSource & source,
		DataSectionPtr pSection ) const { return false; }
	virtual bool createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const { return false; }

	virtual void addToMD5( MD5 & md5 ) const
	{
		ERROR_MSG( "UnsupportedDataType::addToMD5: "
						"%s used in external interface\n",
					this->pMetaDataType()->name() );
	}

	virtual StreamElementPtr getStreamElement( size_t /* index */,
		size_t & /* size */, bool & /* isNone */,
		bool /* isPersistentOnly */ ) const
	{
		ERROR_MSG( "UnsupportedDataType::getStreamElement: Called\n " );
		return StreamElementPtr();
	}

	virtual int clientSafety() const { return CLIENT_UNUSABLE; }
};

#define UNSUPPORTED_TYPE( NAME ) \
	SimpleMetaDataType< UnsupportedDataType > s_##NAME##_metaDataType( #NAME );

#if ! defined( SCRIPT_PYTHON )
#define CONDITIONAL_FORCE_LINK( NAME ) UNSUPPORTED_TYPE( NAME )
#else
#define CONDITIONAL_FORCE_LINK( NAME ) FORCE_LINK( NAME )
#endif


FORCE_LINK( FLOAT32 )
FORCE_LINK( FLOAT64 )

FORCE_LINK( INT8 )
FORCE_LINK( INT16 )
FORCE_LINK( INT32 )
FORCE_LINK( INT64 )

FORCE_LINK( UINT8 )
FORCE_LINK( UINT16 )
FORCE_LINK( UINT32 )
FORCE_LINK( UINT64 )

FORCE_LINK( STRING )

FORCE_LINK( BLOB )

FORCE_LINK( UNICODE_STRING )

FORCE_LINK( UDO_REF )

FORCE_LINK( VECTOR2 )
FORCE_LINK( VECTOR3 )
FORCE_LINK( VECTOR4 )

FORCE_LINK( ARRAY )
FORCE_LINK( TUPLE )

FORCE_LINK( FIXED_DICT )

#if defined( MF_SERVER )
FORCE_LINK( MAILBOX )
#else
UNSUPPORTED_TYPE( MAILBOX )
#endif

CONDITIONAL_FORCE_LINK( PYTHON )
CONDITIONAL_FORCE_LINK( CLASS )
CONDITIONAL_FORCE_LINK( USER_TYPE )

BW_END_NAMESPACE

// data_types.cpp
