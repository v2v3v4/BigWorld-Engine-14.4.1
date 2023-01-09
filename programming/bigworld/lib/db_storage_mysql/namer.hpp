#ifndef MYSQL_NAMER_HPP
#define MYSQL_NAMER_HPP

#include "constants.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class helps to build names for table columns. Introduced when due to
 *	nested properties. It tries to achieve the following:
 *		- Table names are fully qualified.
 *		- Column names are relative to the current table.
 */
class Namer
{
public:
	Namer( const BW::string & entityName,
			const BW::string & tableNamePrefix = TABLE_NAME_PREFIX );

	Namer( const Namer & existing,
			const BW::string & propName, bool isTable );

	BW::string buildColumnName( const BW::string & prefix,
								const BW::string & propName ) const;

	BW::string buildTableName( const BW::string & propName ) const;

private:
	typedef BW::vector< BW::string >			Strings;
	typedef BW::vector< Strings::size_type >	Levels;

	BW::string buildName( const BW::string & prefix,
						const BW::string & suffix,
						Strings::size_type startIdx ) const;
	
	static BW::string correctName( const BW::string & name );

	BW::string tableNamePrefix_;
	Strings		names_;
	Levels		tableLevels_;
};

BW_END_NAMESPACE

#endif // MYSQL_NAMER_HPP
