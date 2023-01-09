#include "script/first_include.hpp"

#include "unique_id_mapping.hpp"

#include "../namer.hpp"
#include "../query.hpp"
#include "../result_set.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UniqueIDMapping::UniqueIDMapping( const Namer & namer,
		const BW::string & propName,
		DataSectionPtr pDefaultValue ) :
	PropertyMapping( propName ),
	colName_( namer.buildColumnName( "sm", propName ) )
{
	if (pDefaultValue)
	{
		defaultValue_ = UniqueID( pDefaultValue->asString() );
	}
}


/*
 *	Override from PropertyMapping.
 */
void UniqueIDMapping::fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
{
	UniqueID uniqueId;
	strm >> uniqueId;
	if (strm.error())
	{
		ERROR_MSG( "UniqueIDMapping::fromStreamToDatabase: "
					"Failed destreaming property '%s'.\n",
				this->propName().c_str() );
		return;
	}

	BW::string uniqueIdStr( (const char *)&uniqueId, sizeof( uniqueId ) );
	queryRunner.pushArg( uniqueIdStr );
}


/*
 *	Override from PropertyMapping.
 */
void UniqueIDMapping::fromDatabaseToStream( ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const
{
	BW::string data;
	results >> data;

	if (data.size() != sizeof( UniqueID ))
	{
		results.setError();
	}
	else
	{
		UniqueID uniqueId;
		memcpy( &uniqueId, data.data(), sizeof( UniqueID ) );
		strm << uniqueId;
	}
}


/*
 *	Override from PropertyMapping.
 */
void UniqueIDMapping::defaultToStream( BinaryOStream & strm ) const
{
	strm << defaultValue_;
}


/*
 *	Override from PropertyMapping.
 */
bool UniqueIDMapping::visitParentColumns( ColumnVisitor & visitor )
{
	ColumnType type( MYSQL_TYPE_STRING,
			true,
			sizeof( UniqueID ),
			// TODO: Make stingify method
			BW::string( reinterpret_cast< const char * >( &defaultValue_ ),
					sizeof( defaultValue_ ) ) );
	ColumnDescription description( colName_, type );
	return visitor.onVisitColumn( description );
}

BW_END_NAMESPACE

// unique_id_mapping.cpp
