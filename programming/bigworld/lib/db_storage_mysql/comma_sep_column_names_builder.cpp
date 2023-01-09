#include "script/first_include.hpp"

#include "comma_sep_column_names_builder.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

class PropertyMapping;

	// Constructor for a single PropertyMapping
CommaSepColNamesBuilder::CommaSepColNamesBuilder( PropertyMapping & property ) :
	count_( 0 )
{
	property.visitParentColumns( *this );
}

// Constructor for many PropertyMappings
CommaSepColNamesBuilder::CommaSepColNamesBuilder(
		const PropertyMappings & properties ) : count_( 0 )
{
	for (PropertyMappings::const_iterator it = properties.begin();
			it != properties.end(); ++it)
	{
		(*it)->visitParentColumns( *this );
	}
}

/**
 *	Constructor for a TableProvider.
 */
CommaSepColNamesBuilder::CommaSepColNamesBuilder( TableProvider & table,
		bool visitIDCol ) :
	count_( 0 )
{
	if (visitIDCol)
	{
		table.visitIDColumnWith( *this );
	}

	table.visitColumnsWith( *this );
}


// ColumnVisitor override
bool CommaSepColNamesBuilder::onVisitColumn( const ColumnDescription & column )
{
	// This is so that timestamp will be ignored.
	if (!column.shouldIgnore())
	{
		if (count_ > 0)
		{
			commaSepColumnNames_ << ',';
		}

		commaSepColumnNames_ << column.columnName();

		++count_;
	}

	return true;
}


// Constructor for many PropertyMappings
CommaSepColNamesBuilderWithSuffix::CommaSepColNamesBuilderWithSuffix(
		const PropertyMappings & properties,
		const BW::string & suffix ) : suffix_( suffix )
{
	for ( PropertyMappings::const_iterator it = properties.begin();
			it != properties.end(); ++it )
	{
		(*it)->visitParentColumns( *this );
	}
}

	// Constructor for a single PropertyMapping
CommaSepColNamesBuilderWithSuffix::CommaSepColNamesBuilderWithSuffix(
		PropertyMapping & property,
		const BW::string & suffix ) : suffix_( suffix )
{
	property.visitParentColumns( *this );
}


bool CommaSepColNamesBuilderWithSuffix::onVisitColumn(
		const ColumnDescription & column )
{
	bool shouldContinue = CommaSepColNamesBuilder::onVisitColumn( column );

	if (!column.shouldIgnore())
	{
		commaSepColumnNames_ << suffix_;
	}

	return shouldContinue;
}

BW_END_NAMESPACE

// comma_sep_column_names_builder.cpp
