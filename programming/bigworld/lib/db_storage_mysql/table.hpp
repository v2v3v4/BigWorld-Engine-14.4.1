#ifndef MYSQL_TABLE_HPP
#define MYSQL_TABLE_HPP

#include "column_type.hpp"


BW_BEGIN_NAMESPACE

class TableVisitor;


/**
 *	This is an interface into a table mapping i.e. something that maps to a
 * 	MySql table.
 */
class TableProvider
{
public:

	virtual const BW::string & getTableName() const = 0;
	// Visits all columns except the ID column
	virtual bool visitColumnsWith( ColumnVisitor & visitor ) = 0;
	// Visit the ID column
	virtual bool visitIDColumnWith( ColumnVisitor & visitor ) = 0;
	virtual bool visitSubTablesWith( TableVisitor & visitor ) = 0;

	// Determines whether this table has any sub-tables.
	bool hasSubTables();

	bool visitSubTablesRecursively( TableVisitor & visitor );
};


/**
 *	This class describes an interface. Classes derived from this can be used to
 *	visit tables.
 */
class TableVisitor
{
public:
	/**
	 * This method is called for each table that is being visited.
	 * 
	 * @param table  The provider to use when interfacing with the table
	 *               being visited.
	 * @returns true if the caller should continue visiting other tables,
	 *          false otherwise.
	 */
	virtual bool onVisitTable( TableProvider & table ) = 0;
};

BW_END_NAMESPACE

#endif // MYSQL_TABLE_HPP
