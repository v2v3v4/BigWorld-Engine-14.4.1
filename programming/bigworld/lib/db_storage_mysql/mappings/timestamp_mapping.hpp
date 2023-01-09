#ifndef MYSQL_TIMESTAMP_MAPPING_HPP
#define MYSQL_TIMESTAMP_MAPPING_HPP

#include "property_mapping.hpp"

#include "../column_type.hpp"
#include "../constants.hpp"
#include "../wrapper.hpp"


BW_BEGIN_NAMESPACE

class TimestampMapping : public PropertyMapping
{
public:
	TimestampMapping();

	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const
	{
		// Timestamp should not be provided for the query.
		// The ColumnDescription in visitParentColumns is created with
		// shouldIgnore = true, which means we should be skipped by the visitor
	}

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const
	{
		// The timestamp should never be read back off by BigWorld.
	}

	virtual void defaultToStream( BinaryOStream & strm ) const {}

	virtual bool visitParentColumns( ColumnVisitor & visitor );
};

BW_END_NAMESPACE

#endif // MYSQL_TIMESTAMP_MAPPING_HPP
