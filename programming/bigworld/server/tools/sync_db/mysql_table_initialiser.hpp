#ifndef MYSQL_TABLE_INITIALISER_HPP
#define MYSQL_TABLE_INITIALISER_HPP

#include "db_storage_mysql/table_inspector.hpp"


BW_BEGIN_NAMESPACE

class AlterTableHelper;

class TableInitialiser : public TableInspector
{
public:
	TableInitialiser( MySql & con, bool allowNew,
			const BW::string & characterSet, const BW::string & collation );

	// Exposed to allow sync_db to update from 1.7 to 1.8
	void createIndex( const BW::string & tableName,
		const BW::string & colName,
		const TableMetaData::ColumnInfo & colInfo );

protected:
	// TableInspector overrides.
	virtual bool onExistingTable( const BW::string & tableName );

	virtual bool onNeedNewTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns );

	virtual bool onNeedUpdateTable( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & obsoleteColumns,
			const TableMetaData::NameToColInfoMap & newColumns,
			const TableMetaData::NameToColInfoMap & updatedColumns,
			const TableMetaData::NameToIdxColInfoMap & indexedColumns );

	virtual bool onNeedDeleteTables( const StrSet & tableNames );

private:
	void addColumns( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns,
			AlterTableHelper & helper, bool shouldPrintInfo );

	void dropColumns( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns,
			AlterTableHelper & helper, bool shouldPrintInfo );

	void updateColumns( const BW::string & tableName,
			const TableMetaData::NameToColInfoMap & columns,
			AlterTableHelper & helper, bool shouldPrintInfo );

	void indexColumns( const BW::string & tableName,
			const TableMetaData::NameToIdxColInfoMap & columns );

	const BW::string generateCreateIndexStatement( 
			const BW::string & tableName, const BW::string & colName, 
			const TableMetaData::ColumnInfo & colInfo );

	void removeIndex( const BW::string & tableName,
		const BW::string & colName,
		ColumnIndexType indexType );

	template < class COLUMN_MAP >
	void initialiseColumns( const BW::string & tableName,
		COLUMN_MAP & columns, bool shouldApplyDefaultValue );

	void initialiseColumn( const BW::string & tableName,
		const BW::string & columnName,
		const TableMetaData::ColumnInfo & columnInfo,
		bool shouldApplyDefaultValue );

	bool allowNew_;
	BW::string characterSet_;
	BW::string collation_;
};

BW_END_NAMESPACE

#endif // MYSQL_TABLE_INITIALISER_HPP
