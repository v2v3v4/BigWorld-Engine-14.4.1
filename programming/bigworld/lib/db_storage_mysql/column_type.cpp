#include "column_type.hpp"

#include "helper_types.hpp"

#include <sstream>


BW_BEGIN_NAMESPACE

//------------------------------------------------------------------------------
// Section: class ColumnType
//------------------------------------------------------------------------------
/**
 *	This method converts the information in this ColumnType object into a
 * 	string suitable for a CREATE TABLE or ALTER TABLE statement. 
 */
BW::string ColumnType::getAsString( MySql & connection,
		ColumnIndexType idxType ) const
{
	// Builds up the type string using this object
	struct ResultStream
	{
		BW::stringstream 	ss;
		bool				isUnsignedOrBinary;

		ResultStream( bool isUnsignedOrBinaryIn ) : 
			isUnsignedOrBinary( isUnsignedOrBinaryIn )
		{}

		void addType( const char * typeName )
		{
			ss << typeName;
		}
		void addNumericalType( const char * typeName )
		{
			ss << typeName;
			if (isUnsignedOrBinary)
			{
				ss << " UNSIGNED";
			}
		}
		void addVarLenType( const char * typeName, uint length )
		{
			ss << typeName << '(' << length << ')';
		}
		void addVarLenStrType( uint length, const char * prefix = "" )
		{
			const char * charTypeStr = isUnsignedOrBinary ? "BINARY" : "CHAR";
			ss << prefix << charTypeStr << '(' << length << ')';
		}
		void addMultiLenType( const char * lenType )
		{
			const char * blobTypeStr = isUnsignedOrBinary ? "BLOB" : "TEXT";
			ss << lenType << blobTypeStr;
		}
		void addAutoIncrement()
		{
			ss << " AUTO_INCREMENT";
		}
		void addDefaultValue( const BW::string& defaultValue, 
				MySql* pConnection )
		{
			if (!defaultValue.empty())
			{
				ss << " DEFAULT ";
				if (pConnection)
				{
					ss << '\'' 
						<<	MySqlEscapedString( *pConnection, defaultValue )
						<< '\''; 
				}
				else
				{
					ss << defaultValue;
				}
			}
		}
		void addPrimaryKey()
		{
			ss << " PRIMARY KEY";
		}
		void addNotNull()
		{
			ss << " NOT NULL";
		}
		void addOnUpdate( const BW::string & cmd )
		{
			ss << " ON UPDATE " << cmd;
		}
	};
	ResultStream result( this->isUnsignedOrBinary );

	switch (fieldType)
	{
		case MYSQL_TYPE_TINY:
			result.addNumericalType( "TINYINT" );
			break;
		case MYSQL_TYPE_SHORT:
			result.addNumericalType( "SMALLINT" );
			break;
		case MYSQL_TYPE_INT24:
			result.addNumericalType( "MEDIUMINT" );
			break;
		case MYSQL_TYPE_LONG:
			result.addNumericalType( "INT" );
			break;
		case MYSQL_TYPE_LONGLONG:
			result.addNumericalType( "BIGINT" );
			break;
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
			result.addNumericalType( "DECIMAL" );
			break;
		case MYSQL_TYPE_FLOAT:
			result.addType( "FLOAT" );
			break;
		case MYSQL_TYPE_DOUBLE:
			result.addType( "DOUBLE" );
			break;
		case MYSQL_TYPE_TIMESTAMP:
			result.addType( "TIMESTAMP" );
			break;
		case MYSQL_TYPE_TIME:
			result.addType( "TIME" );
			break;
		case MYSQL_TYPE_NEWDATE:
			result.addType( "DATE" );
			break;
		case MYSQL_TYPE_YEAR:
			result.addVarLenType( "YEAR", this->length );
			break;
		case MYSQL_TYPE_VARCHAR:
			result.addVarLenType( "VARCHAR", this->length );
			break;
		case MYSQL_TYPE_TINY_BLOB:
			result.addMultiLenType( "TINY" );
			break;
		case MYSQL_TYPE_MEDIUM_BLOB:
			result.addMultiLenType( "MEDIUM" );
			break;
		case MYSQL_TYPE_LONG_BLOB:
			result.addMultiLenType( "LONG" );
			break;
		case MYSQL_TYPE_BLOB:
			result.addMultiLenType( "" );
			break;
		case MYSQL_TYPE_BIT:
			result.addVarLenType( "BIT", this->length );
			break;
		case MYSQL_TYPE_VAR_STRING:
			result.addVarLenStrType( this->length, "VAR" );
			break;
		case MYSQL_TYPE_STRING:
			result.addVarLenStrType( this->length );
			break;
//		case MYSQL_NULL:
//		case MYSQL_TYPE_SET:
//		case MYSQL_TYPE_GEOMETRY:
//		case MYSQL_TYPE_ENUM:
		default:
			MF_ASSERT( false );
			break;
	}

	if (this->isAutoIncrement)
	{
		result.addAutoIncrement();
	}
	else if (this->isDefaultValueSupported())
	{
		result.addDefaultValue( this->defaultValue, 
				this->isStringType() ? &connection : NULL );
	}

	if (!this->onUpdateCmd.empty())
	{
		result.addOnUpdate( this->onUpdateCmd );
	}

	if (idxType == INDEX_TYPE_PRIMARY)
	{
		// The primary key has to be part of the column specification because
		// we want to create it when we create the table. If we do not, then
		// MySQL will create one automatically, and it may not be what we want.
		result.addPrimaryKey();
	}

	result.addNotNull();

	return result.ss.str();
}

/**
 * 	Returns true if the type supports the default value specification.
 */
BW::string ColumnType::getDefaultValueAsString( 
		MySql& connection ) const
{
	if (this->isStringType())
	{
		BW::stringstream ss;
		ss << '\'' <<  MySqlEscapedString( connection, this->defaultValue )
				<< '\'';
		return ss.str();
	}
	else
	{
		return this->defaultValue;
	}
}

/**
 * 	Returns true if the type supports the default value specification.
 */
bool ColumnType::isDefaultValueSupported() const
{
	switch (this->fieldType)
	{
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
			return false;
		default:
			return true;
	}
}

/**
 *	Whether the type is a string-like type.
 */
bool ColumnType::isStringType() const
{
	switch (this->fieldType)
	{
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_VARCHAR:
			return true;
		default:
			return false;
	}
}

bool ColumnType::isSimpleNumericalType() const
{
	switch (this->fieldType)
	{
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_INT24:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
			return true;
		default:
			return false;
	}
}

/**
 * 	Equality operator. Need to take into account that not all members are 
 * 	relevant. 
 */
bool ColumnType::operator==( const ColumnType & other ) const
{
	struct HelperFunctions
	{
		static bool equalOptionalStuff( const ColumnType & self, const ColumnType & other )
		{
			MF_ASSERT( self.fieldType == other.fieldType );

			switch (self.fieldType)
			{
				case MYSQL_TYPE_DECIMAL:
				case MYSQL_TYPE_NEWDECIMAL:
				case MYSQL_TYPE_FLOAT:
				case MYSQL_TYPE_DOUBLE:
				case MYSQL_TYPE_TIMESTAMP:
				case MYSQL_TYPE_TIME:
				case MYSQL_TYPE_NEWDATE:
					return true;
				case MYSQL_TYPE_TINY:
				case MYSQL_TYPE_SHORT:
				case MYSQL_TYPE_INT24:
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_LONGLONG:
					return (self.isUnsigned() == other.isUnsigned())
							&& (self.isAutoIncrement == other.isAutoIncrement);
				case MYSQL_TYPE_TINY_BLOB:
				case MYSQL_TYPE_MEDIUM_BLOB:
				case MYSQL_TYPE_LONG_BLOB:
				case MYSQL_TYPE_BLOB:
					return self.isBinary() == other.isBinary();
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_STRING:
					return (self.length == other.length) && 
						(self.isBinary() == other.isBinary());
				case MYSQL_TYPE_YEAR:
				case MYSQL_TYPE_VARCHAR:
					return self.length == other.length;
				default:
					MF_ASSERT( false );
					return true;
			}
		}


		static bool equalDefaultValue( const ColumnType & self, const ColumnType & other )
		{
			if (self.isAutoIncrement || !self.isDefaultValueSupported())
			{
				return true;
			}

			if (self.defaultValue == other.defaultValue)
			{
				return true;
			}

			if (self.isSimpleNumericalType())
			{
				return (self.defaultValue.empty() &&
						(other.defaultValue == "0")) ||
					((self.defaultValue == "0") && other.defaultValue.empty());
			}

			if (self.fieldType == MYSQL_TYPE_STRING)
			{
				BW::string nullString( self.length, 
						self.isBinary() ? '\0' : ' ' );

				return (self.defaultValue.empty() && 
						(other.defaultValue == nullString)) ||
					((self.defaultValue == nullString) &&
						other.defaultValue.empty());
			}

			if (self.fieldType == MYSQL_TYPE_TIMESTAMP)
			{
				return true;
			}

			return false;
		}
	};

	return (this->fieldType == other.fieldType) &&
			HelperFunctions::equalOptionalStuff( *this, other ) &&
			(this->isAutoIncrement == other.isAutoIncrement) &&
			HelperFunctions::equalDefaultValue( *this, other );
}


/**
 *	Returns true if the field is either an unsigned field or a binary blob 
 * 	field.
 */
bool ColumnType::deriveIsUnsignedOrBinary( const MYSQL_FIELD & field )
{
	switch (field.type)
	{
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_INT24:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
			return (field.flags & UNSIGNED_FLAG);

		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:
			// charsetnr is from mysql.h and represents the character set
			// of the associated MYSQL_FIELD. The magic number number 63
			// is supposed to represent 'CHARACTER SET binary'. This
			// however may be unsafe as the value comes from the file
			// mysql/charsets/Index.xml, which may be modified in different
			// versions of MySQL
			// See: http://forge.mysql.com/wiki/MySQL_Internals_Algorithms
			return (field.charsetnr == 63);

		default:
			// Doesn't apply to other types so we can return whatever we like.
			return false;
	}
}

BW_END_NAMESPACE

// column_type.cpp
