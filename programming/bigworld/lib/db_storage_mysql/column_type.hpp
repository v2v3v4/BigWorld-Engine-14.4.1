#ifndef COLUMN_TYPE_HPP
#define COLUMN_TYPE_HPP

#include "constants.hpp" 
#include "type_traits.hpp" 
#include "wrapper.hpp"

#include "network/basictypes.hpp"

#include <mysql/mysql.h>


BW_BEGIN_NAMESPACE

enum ColumnIndexType
{
	INDEX_TYPE_NONE 		= 0,
	INDEX_TYPE_PRIMARY		= 1,
	INDEX_TYPE_PARENT_ID	= 2,
	INDEX_TYPE_UNIQUE		= 3,
	INDEX_TYPE_NON_UNIQUE	= 4,
	INDEX_TYPE_EXTERNAL		= 5,
};


/**
 *	This class is used to describe the type of a column.
 */
class ColumnType
{
public:
	enum_field_types	fieldType;
	bool				isUnsignedOrBinary;	// Dual use field
	uint				length; // In characters for text types, otherwise
								// in bytes.
	BW::string			defaultValue;
	BW::string			onUpdateCmd;
	bool				isAutoIncrement;

	/**
	 *	Constructor used when creating a new field in the database.
	 */
	ColumnType( enum_field_types type = MYSQL_TYPE_NULL,
			bool isUnsignedOrBin = false, 
			uint len = 0,
			const BW::string defVal = BW::string(),
			bool isAutoInc = false ) :
		fieldType( type ),
		isUnsignedOrBinary( isUnsignedOrBin ),
		length( len ),
		defaultValue( defVal ),
		isAutoIncrement( isAutoInc )
	{}


	/**
	 *	Constructor used when populating from a retrieved MYSQL_FIELD value.
	 */
	ColumnType( const MYSQL_FIELD & field ) :
		fieldType( field.type ),
		isUnsignedOrBinary( deriveIsUnsignedOrBinary( field ) ),
		length( field.length / MySql::charsetWidth( field.charsetnr ) ),
		defaultValue( field.def ? field.def : BW::string() ),
		isAutoIncrement( field.flags & AUTO_INCREMENT_FLAG )
	{
		if (this->fieldType == MYSQL_TYPE_BLOB)
		{
			this->fieldType = MySqlTypeTraits<BW::string>::colType( this->length );
		}

	}

	// Only applies to integer fields
	bool isUnsigned() const			{ return isUnsignedOrBinary; }
	void setIsUnsigned( bool val )	{ isUnsignedOrBinary = val; }

	// Only applies to string or blob fields
	bool isBinary()	const			{ return isUnsignedOrBinary; }
	void setIsBinary( bool val ) 	{ isUnsignedOrBinary = val; }

	BW::string getAsString( MySql& connection, ColumnIndexType idxType ) const;
	BW::string getDefaultValueAsString( MySql& connection ) const;
	bool isDefaultValueSupported() const;
	bool isStringType() const;
	bool isSimpleNumericalType() const;

	bool operator==( const ColumnType& other ) const;
	bool operator!=( const ColumnType& other ) const
	{	return !this->operator==( other );	}

	static bool deriveIsUnsignedOrBinary( const MYSQL_FIELD& field );


private:
	void adjustBlobTypeForSize();
};


/**
 *	This is a description of a column.
 */
class ColumnDescription
{
public:
	ColumnDescription( const BW::string & columnName,
			const ColumnType & type,
			ColumnIndexType indexType = INDEX_TYPE_NONE,
			bool shouldIgnore = false ) :
		columnName_( columnName ),
		columnType_( type ),
		indexType_( indexType ),
		shouldIgnore_( shouldIgnore )
	{
	}

	const BW::string & columnName() const			{ return columnName_; }
	const ColumnType & columnType() const			{ return columnType_; }
	ColumnIndexType columnIndexType() const			{ return indexType_; }

	bool shouldIgnore() const	{ return shouldIgnore_; }

private:
	const BW::string columnName_;
	ColumnType columnType_;
	ColumnIndexType indexType_;
	bool shouldIgnore_;
};


/**
 *	This class describes an interface. Classes derived from this can be used to
 *	visit columns.
 */
class ColumnVisitor
{
public:
	// NOTE: For efficiency reasons the IMySqlColumnMapping passed to the
	// visitor may be a temporary on the stack i.e. you should not store
	// its address.
	virtual bool onVisitColumn( const ColumnDescription & description ) = 0;
};


const ColumnType PARENTID_COLUMN_TYPE( MYSQL_TYPE_LONGLONG );
const ColumnType ID_COLUMN_TYPE( MYSQL_TYPE_LONGLONG, false, 
		0, BW::string(), true );	// Auto-increment column

BW_END_NAMESPACE

#endif // COLUMN_TYPE_HPP
