#ifndef MYSQL_BW_META_DATA_HPP
#define MYSQL_BW_META_DATA_HPP

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class MySql;

/**
 *	This class is used to access tables that stores entity meta data.
 */
class BigWorldMetaData
{
public:
	BigWorldMetaData( MySql & connection );

	MySql & connection()		{ return connection_; }

	EntityTypeID getEntityTypeID( const BW::string & entityName );
	void setEntityTypeID( const BW::string & entityName, EntityTypeID typeID );
	void addEntityType( const BW::string & entityName, EntityTypeID typeID );
	void removeEntityType( const BW::string & entityName );

private:
	MySql &				connection_;
};

BW_END_NAMESPACE

#endif // MYSQL_BW_META_DATA_HPP
