#ifndef MYSQL_TYPE_TRAITS_HPP
#define MYSQL_TYPE_TRAITS_HPP

#include "cstdmf/stdmf.hpp"

#include <mysql/mysql.h>

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

// This traits class is used for mapping C types to MySQL types.
template < class CTYPE >
struct MySqlTypeTraits
{
	// static const enum enum_field_types	colType = MYSQL_TYPE_LONG;
};

#define MYSQL_TYPE_TRAITS( FROM_TYPE, TO_TYPE ) 					\
template <>															\
struct MySqlTypeTraits< FROM_TYPE >									\
{																	\
	static const enum enum_field_types	colType = TO_TYPE;			\
};

MYSQL_TYPE_TRAITS( int8, MYSQL_TYPE_TINY )
MYSQL_TYPE_TRAITS( uint8, MYSQL_TYPE_TINY )
MYSQL_TYPE_TRAITS( int16, MYSQL_TYPE_SHORT )
MYSQL_TYPE_TRAITS( uint16, MYSQL_TYPE_SHORT )
MYSQL_TYPE_TRAITS( int32, MYSQL_TYPE_LONG )
MYSQL_TYPE_TRAITS( uint32, MYSQL_TYPE_LONG )
MYSQL_TYPE_TRAITS( int64, MYSQL_TYPE_LONGLONG )
MYSQL_TYPE_TRAITS( uint64, MYSQL_TYPE_LONGLONG )
MYSQL_TYPE_TRAITS( float, MYSQL_TYPE_FLOAT )
MYSQL_TYPE_TRAITS( double, MYSQL_TYPE_DOUBLE )

// Slightly dodgy specialisation for BW::string. Basically it maps to
// different sized BLOBs. colType and colTypeStr are actually functions
// instead of constants.
template <>
struct MySqlTypeTraits< BW::string >
{
	typedef MySqlTypeTraits< BW::string > THIS;

	static enum enum_field_types colType( uint maxColWidthBytes )
	{
		if (maxColWidthBytes < 1<<8)
		{
			return MYSQL_TYPE_TINY_BLOB;
		}
		else if (maxColWidthBytes < 1<<16)
		{
			return MYSQL_TYPE_BLOB;
		}
		else if (maxColWidthBytes < 1<<24)
		{
			return MYSQL_TYPE_MEDIUM_BLOB;
		}
		else
		{
			return MYSQL_TYPE_LONG_BLOB;
		}
	}

	static const BW::string TINYBLOB;
	static const BW::string BLOB;
	static const BW::string MEDIUMBLOB;
	static const BW::string LONGBLOB;

	static BW::string colTypeStr( uint maxColWidthBytes )
	{
		switch (THIS::colType( maxColWidthBytes ))
		{
			case MYSQL_TYPE_TINY_BLOB:
				return TINYBLOB;
			case MYSQL_TYPE_BLOB:
				return BLOB;
			case MYSQL_TYPE_MEDIUM_BLOB:
				return MEDIUMBLOB;
			case MYSQL_TYPE_LONG_BLOB:
				return LONGBLOB;
			default:
				break;
		}
		return NULL;
	}
};

BW_END_NAMESPACE

#endif // MYSQL_TYPE_TRAITS_HPP
