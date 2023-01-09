#include "script/first_include.hpp"

#include "class_mapping.hpp"

#include "../namer.hpp"
#include "../query.hpp"
#include "../result_set.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ClassMapping::ClassMapping( const Namer & namer, const BW::string & propName,
			bool allowNone ) :
	CompositePropertyMapping( propName ),
	allowNone_( allowNone ),
	colName_( allowNone_ ? namer.buildColumnName( "fm", propName ) : "" ),
	hasProps_( 1 )
{
}


/*
 *	Override from PropertyMapping.
 */
void ClassMapping::fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
{
	uint8 hasProps = 1;

	if (allowNone_)
	{
		strm >> hasProps;
		queryRunner.pushArg( hasProps );
	}

	if (hasProps)
	{
		this->CompositePropertyMapping::fromStreamToDatabase( helper,
				strm, queryRunner );
	}
	else
	{
		MemoryOStream stream;
		this->CompositePropertyMapping::defaultToStream( stream );
		this->CompositePropertyMapping::fromStreamToDatabase( helper,
				stream, queryRunner );
	}
}


/*
 *	Override from PropertyMapping.
 */
void ClassMapping::fromDatabaseToStream( ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const
{
	uint8 hasProps = 1;

	if (allowNone_)
	{
		results >> hasProps;
		strm << hasProps;
	}

	if (hasProps)
	{
		this->CompositePropertyMapping::fromDatabaseToStream( helper,
				results, strm );
	}
	else
	{
		MemoryOStream tempStream;
		this->CompositePropertyMapping::fromDatabaseToStream( helper,
				results, tempStream );
	}
}


/*
 *	Override from PropertyMapping.
 */
void ClassMapping::defaultToStream( BinaryOStream & strm ) const
{
	if (allowNone_)
	{
		strm << uint8(0) ;
	}
	else
	{
		CompositePropertyMapping::defaultToStream( strm );
	}
}


/*
 *	Override from PropertyMapping.
 */
bool ClassMapping::visitParentColumns( ColumnVisitor & visitor )
{
	if (allowNone_)
	{
		ColumnDescription description( colName_,
				ColumnType( MYSQL_TYPE_TINY,
				/*isUnsigned*/true,
				0,
				/* defaultValue: */ "1" ) );

		if (!visitor.onVisitColumn( description ))
		{
			return false;
		}
	}

	return CompositePropertyMapping::visitParentColumns( visitor );
}

BW_END_NAMESPACE

// class_mapping.cpp
