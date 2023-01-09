#ifndef MYSQL_COMMA_SEP_COLUMN_NAMES_BUILDER_HPP
#define MYSQL_COMMA_SEP_COLUMN_NAMES_BUILDER_HPP

#include "mappings/property_mapping.hpp" // For PropertyMappings

#include "table.hpp" // For ColumnVisitor

#include <sstream>


BW_BEGIN_NAMESPACE

class PropertyMapping;

/**
 *	This helper class builds a comma separated list string of column names,
 * 	with a suffix attached to the name.
 */
class CommaSepColNamesBuilder : public ColumnVisitor
{
public:
	// Constructor for a single PropertyMapping
	CommaSepColNamesBuilder( PropertyMapping & property );

	// Constructor for many PropertyMappings
	CommaSepColNamesBuilder( const PropertyMappings & properties );

	// Constructor for a TableProvider
	CommaSepColNamesBuilder( TableProvider & table, bool visitIDCol );

	BW::string getResult() const	{ return commaSepColumnNames_.str(); }
	int 		getCount() const	{ return count_; }

	// ColumnVisitor override
	bool onVisitColumn( const ColumnDescription & column );

protected:
	BW::stringstream	commaSepColumnNames_;
	int 				count_;

	CommaSepColNamesBuilder() : count_( 0 ) {}

};

/**
 *	This helper class builds a comma separated list string of column names,
 * 	with a suffix attached to the name.
 */
class CommaSepColNamesBuilderWithSuffix : public CommaSepColNamesBuilder
{
public:
	// Constructor for many PropertyMappings
	CommaSepColNamesBuilderWithSuffix( const PropertyMappings & properties,
			const BW::string & suffix = BW::string() );

	// Constructor for a single PropertyMapping
	CommaSepColNamesBuilderWithSuffix( PropertyMapping & property,
			const BW::string & suffix = BW::string() );

	bool onVisitColumn( const ColumnDescription & column );

private:
	BW::string suffix_;
};

BW_END_NAMESPACE

#endif // MYSQL_COMMA_SEP_COLUMN_NAMES_BUILDER_HPP
