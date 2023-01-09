#ifndef MYSQL_TABLE_META_DATA_HPP
#define MYSQL_TABLE_META_DATA_HPP

#include "table.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

typedef BW::set< BW::string > StrSet;

class MySql;
class TableIndices;

#define PARENTID_INDEX_NAME "parentIDIndex"

BW::string generateIndexName( const BW::string & colName );

namespace TableMetaData
{

class ColumnInfo
{
public:
	ColumnType		columnType;
	ColumnIndexType	indexType;

	ColumnInfo( const MYSQL_FIELD& field, const TableIndices& indices );
	ColumnInfo();

	bool isIndexEqual( const ColumnInfo & other) const;

private:
	static ColumnIndexType deriveIndexType(
			const MYSQL_FIELD& field, const TableIndices& indices );
};

// Map of column name to ColumnInfo.
typedef BW::map< BW::string, ColumnInfo > NameToColInfoMap;

struct IndexedColumnInfo : public ColumnInfo
{
	ColumnIndexType	oldIndexType;

	IndexedColumnInfo( const ColumnInfo & newCol, const ColumnInfo & oldCol ) :
	   	ColumnInfo( newCol ),
		oldIndexType( oldCol.indexType )
	{}
};

// Map of column name to IndexedColumnInfo.
typedef BW::map< BW::string, IndexedColumnInfo > NameToIdxColInfoMap;

void getEntityTables( StrSet & tables, MySql & connection );
void getTableColumns( NameToColInfoMap & columns,
		MySql & connection, const BW::string & tableName );

} // namespace TableMetaData



/**
 *	This class retrieves the fields and indexes of a table.
 */
class MySqlTableMetadata
{
public:
	MySqlTableMetadata( MySql & connection, const BW::string tableName );
	~MySqlTableMetadata();

	bool isValid() const 				{ return result_; }
	unsigned int getNumFields() const	{ return numFields_; }
	const MYSQL_FIELD& getField( unsigned int index ) const
	{ return fields_[index]; }

private:
	MYSQL_RES*		result_;
	unsigned int 	numFields_;
	MYSQL_FIELD* 	fields_;
};

BW_END_NAMESPACE

#endif // MYSQL_TABLE_META_DATA_HPP
