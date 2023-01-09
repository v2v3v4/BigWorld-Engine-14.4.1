#include "script/first_include.hpp"

#include "timestamp_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
TimestampMapping::TimestampMapping() :
	PropertyMapping( TIMESTAMP_COLUMN_NAME )
{
}


/*
 *	Override from PropertyMapping.
 */
bool TimestampMapping::visitParentColumns( ColumnVisitor & visitor )
{
	ColumnType type( MYSQL_TYPE_TIMESTAMP );
	type.defaultValue = "CURRENT_TIMESTAMP";
	type.onUpdateCmd = "CURRENT_TIMESTAMP";

	ColumnDescription description( TIMESTAMP_COLUMN_NAME_STR,
			type,
			INDEX_TYPE_NONE,
			/* shouldIgnore: */ true );

	return visitor.onVisitColumn( description );
}

BW_END_NAMESPACE

// timestamp_mapping.cpp
