#ifndef MYSQL_TABLE_INSPECTOR_HPP
#define MYSQL_TABLE_INSPECTOR_HPP

#include "bw_meta_data.hpp"
#include "table.hpp"
#include "table_meta_data.hpp"

#include "mappings/entity_type_mappings.hpp"

#include "db_storage/db_entitydefs.hpp"


BW_BEGIN_NAMESPACE

class TableInspector;

uint32 getBigWorldDBVersion( MySql & connection );
bool getIsPasswordHashed( MySql & connection );
uint32 numSecondaryDBs( MySql& connection );
bool isEntityTablesInSync( MySql& connection, const EntityDefs& entityDefs );
bool isSpecialBigWorldTablesInSync( MySql & connection, bool isPasswordHashed );



/**
 * 	This visitor collects information about the tables required by an
 * 	entity type and checks whether they match the tables in the database.
 */
class TableInspector: public TableVisitor
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param connection  The MySql connection to use when inspecting the DB.
	 */
	TableInspector( MySql & connection ) :
		connection_( connection ),
		isSynced_( true )
	{}

	/**
	 *	This method returns the MySql connection being used by the inspector.
	 *
	 *	@returns The MySql connection used by the inspector.
	 */
	MySql & connection()		{ return connection_; }

	bool deleteUnvisitedTables();

	/**
	 *	This method returns whether the tables required by the entity
	 *	definitions match the tables in the database.
	 *
	 *	@returns true if the tables are synchronised with the entity definition,
	 *	         false otherwise.
	 */
	bool isSynced() const	{ return isSynced_; }

	// TableVisitor overrides
	virtual bool onVisitTable( TableProvider & table );

protected:
	// Interface for derived class

	/**
	 *	This method is called for all tables that existed before and still
	 *	exist.
	 *
	 *	@param tableName The name of the table.
	 *
	 *	@return true on success otherwise false.
	 */
	virtual bool onExistingTable( const BW::string & tableName )
		{ return true; }

	/**
	 *	This method is called when a new table is required in order to
	 *	synchronise with the Entity definitions.
	 *
	 *	@param tableName  The name of the table to create.
	 *	@param columns    The column descriptions to use when creating the
	 *	                  table.
	 *
	 *	@returns true when the table was created, false otherwise.
	 */
	virtual bool onNeedNewTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns ) = 0;

	/**
	 *	This method is called when an existing table needs to be modified in
	 *	order to synchronise it with the Entity definitions.
	 *
	 *	@param tableName        The name of the table to modify.
	 *	@param obsoleteColumns  Column descriptions for any columns which are
	 *	                        no longer required from the table.
	 *	@param newColumns       Column descriptions for any new columns that
	 *	                        need to be added to the table.
	 *	@param updatedColumns   Column descriptions for any columns that need
	 *	                        to be modified (eg, column type changes).
	 *	@param indexedColumns   Column descriptions for any columns that need
	 *	                        to be indexed.
	 *
	 *	@returns true when all requested column modifications have been
	 *	         performed, false otherwise.
	 */
	virtual bool onNeedUpdateTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & obsoleteColumns,
			const TableMetaData::NameToColInfoMap & newColumns,
			const TableMetaData::NameToColInfoMap & updatedColumns,
			const TableMetaData::NameToIdxColInfoMap & indexedColumns ) = 0;

	/**
	 *	This method is called if there are any tables that are no longer
	 *	required that existed in a previous Entity definition.
	 *
	 *	@param tableNames  The set of tables which can be deleted from the
	 *	                   database.
	 *
	 *	@returns true when the entire set of tables has been delete, false
	 *	         otherwise.
	 */
	virtual bool onNeedDeleteTables( const StrSet & tableNames ) = 0;

protected:
	MySql & 	connection_;

private:
	void classifyColumns( TableMetaData::NameToColInfoMap & oldColumns,
		TableMetaData::NameToColInfoMap & newColumns,
		TableMetaData::NameToColInfoMap & updatedColumns,
		TableMetaData::NameToIdxColInfoMap & indexedColumns );

	bool	isSynced_;
	StrSet 	visitedTables_;
};


/**
 *	This specialisation of TableInspector simply prints out the differences
 * 	between the required tables and the tables in the database.
 */
class TableValidator : public TableInspector
{
public:
	TableValidator( MySql& connection ) : TableInspector( connection ) {}

protected:
	// Override interface methods
	virtual bool onNeedNewTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns );

	virtual bool onNeedUpdateTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & obsoleteColumns,
			const TableMetaData::NameToColInfoMap & newColumns,
			const TableMetaData::NameToColInfoMap & updatedColumns,
			const TableMetaData::NameToIdxColInfoMap & indexedColumns );

	virtual bool onNeedDeleteTables( const StrSet & tableNames );
};


/**
 *	This class collects the names and ID of entity types and updates
 * 	the bigworldEntityTypes table.
 */
class TypesCollector
{
public:
	TypesCollector( MySql& connection ) : metaData_( connection ), types_()
	{}

	void addType( EntityTypeID bigworldID, const BW::string& name );

	void deleteUnwantedTypes();

private:
	BigWorldMetaData	metaData_;
	StrSet 				types_;
};


/**
 * 	This visitor collects information about the columns of a table.
 */
class ColumnsCollector : public ColumnVisitor
{
public:
	// ColumnVisitor overrides
	virtual bool onVisitColumn( const ColumnDescription & column )
	{
		TableMetaData::ColumnInfo & colInfo = columns_[ column.columnName() ];
		colInfo.columnType = column.columnType();
		colInfo.indexType = column.columnIndexType();

		return true;
	}

	TableMetaData::NameToColInfoMap & getColumnsInfo()	{ return columns_; }

private:
	TableMetaData::NameToColInfoMap	columns_;
};

BW_END_NAMESPACE

#endif /* MYSQL_TABLE_INSPECTOR_HPP */
