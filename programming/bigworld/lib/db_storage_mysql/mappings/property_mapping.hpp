#ifndef MYSQL_PROPERTY_MAPPING_HPP
#define MYSQL_PROPERTY_MAPPING_HPP

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"

#include "entitydef/data_description.hpp"

#include "../column_type.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

class DataSection;
typedef SmartPointer< DataSection > DataSectionPtr;

class DataType;

class ColumnVisitor;
class TableVisitor;

class MySql;
class Namer;

class PropertyMapping;
class QueryRunner;
class ResultStream;

class ResultToStreamHelper;
class StreamToQueryHelper;

typedef SmartPointer< PropertyMapping > PropertyMappingPtr;
typedef BW::vector< PropertyMappingPtr > PropertyMappings;


/**
 *	This is the base class for classes that maps BigWorld types to MySQL tables
 * 	and columns.
 */
class PropertyMapping : public SafeReferenceCount
{
public:
	PropertyMapping( const BW::string & propName,
			ColumnIndexType indexType = INDEX_TYPE_NONE ) :
		propName_( propName ),
		indexType_( indexType )
	{}
	virtual ~PropertyMapping() {}

	static PropertyMappingPtr create( const Namer & namer,
			const BW::string & propName, const DataType & type,
			int databaseLength, DataSectionPtr pDefaultValue,
			DatabaseIndexingType indexingType = DATABASE_INDEXING_NONE );

	/**
	 * This method puts data from the stream into a database column or in the
	 * case of collection types, passes the stream to separate table handlers
	 * which will place the data in the appropriate table.
	 *
	 * @param helper      The helper class containing information associated
	 *                    with the query being performed.
	 * @param strm        The input stream containing the data to extract.
	 * @param queryRunner The query runner class where destreamed data is
	 *                    placed to be eventually pushed to the database.
	 */
	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const = 0;

	/**
	 * This method puts data that has already been queried from the database
	 * and places it into the stream.
	 *
	 * @param helper  The helper class containing information associated
	 *                with the query being performed.
	 * @param results The stream containing results from a query that has
	 *                already been executed on the database. 
	 * @param strm    The output stream where results should be placed after
	 *                any required modification.
	 */
	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
			ResultStream & results,
			BinaryOStream & strm ) const = 0;


	/**
	 * This method is invoked from EntityTypeMapping to perform any
	 * supplimental SQL statement preparation, such as that required by
	 * sequences.
	 */
	virtual void prepareSQL() {}

	/**
	 * This method returns the name of the property the instance maps to
	 * in a DataSection.
	 *
	 * @returns The name of the property this instance maps to.
	 */
	const BW::string& propName() const	{ return propName_; }


	/**
	 *	This method returns the indexing type used on this mapping's property.
	 *
	 *	@returns The indexing type used on this mapping's property.
	 */
	ColumnIndexType indexType() const { return indexType_; }

	/**
	 * This method places the default value of the properties data type
	 * onto the provided stream.
	 *
	 * @param strm  The output stream to place the default value onto.
	 */
	virtual void defaultToStream( BinaryOStream & strm ) const = 0;

	/**
	 * This method returns whether or not the property type or any of its
	 * children store data in additional table(s).
	 *
	 * @returns true if the property type or children requires sub-tables,
	 *          false otherwise.
	 */
	virtual bool hasTable() const { return false; }

	/**
	 * This method lets the visitor visit all the columns that are being
	 * added to the parent table.
	 *
	 * @param visitor  The visitor to call for each parent column.
	 *
	 * @returns true on success, false on error.
	 */
	virtual bool visitParentColumns( ColumnVisitor & visitor ) = 0;

	/**
	 * This method lets the visitor visit all of the parent's sub-tables
	 * that are being added.
	 *
	 * @returns true on success, false on error.
	 */
	virtual bool visitTables( TableVisitor & visitor )
		{ return true; }

	/**
	 * This method performs a cascading delete on any child tables.
	 *
	 * @param connection 
	 * @param parentID   
	 */
	virtual void deleteChildren( MySql & connection,
			DatabaseID parentID ) const {}

private:

	static ColumnIndexType getColumnIndexType( 
		DatabaseIndexingType databaseIndexingType );

	BW::string 			propName_;
	ColumnIndexType			indexType_;
};

BW_END_NAMESPACE

#endif // MYSQL_PROPERTY_MAPPING_HPP
