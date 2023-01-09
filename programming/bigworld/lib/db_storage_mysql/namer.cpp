#include <string>

#include "namer.hpp"

#include "constants.hpp"

#include "entitydef/member_description.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method replaces component name separator symbol (dot) 
 *	in given name with double underscore
 * 
 *  @param name	name to correct
 *  @returns corrected name
 */
BW::string Namer::correctName( const BW::string & name )
{
	const char toReplace = MemberDescription::COMPONENT_NAME_SEPARATOR;
	const BW::string replacement = "__";
	BW::string result = name;

	for (BW::string::size_type pos = result.find( toReplace ); 
		 pos != BW::string::npos; 
		 pos = result.find( toReplace, pos + replacement.size() ))
	{
		result.replace( pos, /*count*/1, replacement );
	}
	return result;
}

/**
 *	Constructor (1 of 2)
 */
Namer::Namer( const BW::string & entityName,
			const BW::string & tableNamePrefix ) :
		tableNamePrefix_( tableNamePrefix ),
		names_( 1, entityName ),
		tableLevels_( 1, 1 )
{
}


/**
 *	Constructor (2 of 2)
 */
Namer::Namer( const Namer & existing,
		const BW::string & propName, bool isTable ) :
	tableNamePrefix_( existing.tableNamePrefix_ ),
	names_( existing.names_ ),
	tableLevels_( existing.tableLevels_ )
{
	if (propName.empty())
	{
		names_.push_back( isTable ? DEFAULT_SEQUENCE_TABLE_NAME :
							DEFAULT_SEQUENCE_COLUMN_NAME  );
	}
	else
	{
		names_.push_back( correctName( propName ) );
	}

	if (isTable)
	{
		tableLevels_.push_back( names_.size() );
	}
}


/**
 *
 */
BW::string Namer::buildColumnName( const BW::string & prefix,
							const BW::string & propName ) const
{
	BW::string suffix =
		(propName.empty()) ? DEFAULT_SEQUENCE_COLUMN_NAME : 
							 correctName( propName );

	return this->buildName( prefix, suffix, tableLevels_.back() );
}


/**
 *
 */
BW::string Namer::buildTableName( const BW::string & propName ) const
{
	BW::string suffix =
		(propName.empty()) ? DEFAULT_SEQUENCE_TABLE_NAME : 
							 correctName( propName );

	return this->buildName( tableNamePrefix_, suffix, 0 );
}


/**
 *
 */
BW::string Namer::buildName( const BW::string & prefix,
					const BW::string & suffix,
					Strings::size_type startIdx ) const
{
	BW::string name = prefix;

	for (Strings::size_type i = startIdx; i < names_.size(); ++i)
	{
		name += '_';
		name += names_[i];
	}

	if (!suffix.empty())
	{
		name += '_';
		name += suffix;
	}

	return name;
}

BW_END_NAMESPACE

// mysql_namer.cpp
