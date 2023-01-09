#include "script/first_include.hpp"

#include "string_like_mapping.hpp"

#include "../column_type.hpp"
#include "../namer.hpp"
#include "../query.hpp"
#include "../result_set.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/value_or_null.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor for string-like types where the character length and the
 *	byte length are equivalent.
 */
StringLikeMapping::StringLikeMapping( const Namer & namer,
			const BW::string & propName,
			ColumnIndexType indexType,
			uint charLength,
	   		uint byteLength ):
		PropertyMapping( propName, indexType ),
		colName_( namer.buildColumnName( "sm", propName ) ),
		charLength_( charLength ),
		byteLength_( (byteLength != 0) ? byteLength : charLength ),
		defaultValue_()
{
}


/*
 *	Override from PropertyMapping.
 */
void StringLikeMapping::fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
{
	BW::string value;
	strm >> value;
	if (strm.error())
	{
		ERROR_MSG( "StringLikeMapping::fromStreamToDatabase: "
					"Failed destreaming property '%s'.\n",
				this->propName().c_str() );
		return;
	}

	if (value.size() > byteLength_)
	{
		WARNING_MSG( "StringLikeMapping::fromStreamToDatabase: "
					"Truncating string property '%s' from %zd to %u.\n",
				this->propName().c_str(), value.size(), byteLength_ );
	}

	queryRunner.pushArg( value );
}


/*
 *	Override from PropertyMapping.
 */
void StringLikeMapping::fromDatabaseToStream( ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const
{
	ValueOrNull< BW::string > value;

	results >> value;

	if (!value.isNull())
	{
		strm << *value.get();
	}
	else
	{
		strm << defaultValue_;
	}
}


/*
 *	Override from PropertyMapping.
 */
void StringLikeMapping::defaultToStream( BinaryOStream & strm ) const
{
	strm << defaultValue_;
}


/**
 * This method retrieves the MySQL field type of the column based on the
 * length of each character being represented by the string type.
 *
 * @returns The MySQL field type of the associated string column.
 */
enum_field_types StringLikeMapping::getColumnType() const
{
	return MySqlTypeTraits< BW::string >::colType( charLength_ );
}


/*
 *	Override from PropertyMapping.
 */
bool StringLikeMapping::visitParentColumns( ColumnVisitor & visitor )
{
	ColumnType type( 
			this->getColumnType(),
			this->isBinary(),
			charLength_,
			defaultValue_ );

	if (type.fieldType == MYSQL_TYPE_LONG_BLOB)
	{
		// Can't put string > 16MB onto stream.
		CRITICAL_MSG( "StringLikeMapping::StringLikeMapping: "
				"Property '%s' has DatabaseLength %u that exceeds the maximum "
				"supported byte length of 16777215\n",
			this->propName().c_str(),
			charLength_ );
	}

	ColumnDescription description( colName_, type, this->indexType() );

	return visitor.onVisitColumn( description );
}

BW_END_NAMESPACE

// string_like_mapping.cpp
