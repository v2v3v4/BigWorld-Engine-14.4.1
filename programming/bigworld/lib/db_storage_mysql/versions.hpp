#ifndef MYSQL_VERSIONS_HPP
#define MYSQL_VERSIONS_HPP

BW_BEGIN_NAMESPACE

// 1.7.
const uint32	DBAPP_VERSION_1_7				= 1;
// 1.8.
const uint32	DBAPP_VERSION_1_8				= 2;
// 1.9 snapshot.
const uint32	DBAPP_VERSION_1_9_SNAPSHOT		= 3;
// 1.9 non-null columns.
const uint32	DBAPP_VERSION_1_9_NON_NULL		= 4;
// 1.9 secondary DBs.
const uint32	DBAPP_VERSION_SECONDARYDB  		= 5;
// STRING type treated as binary.
const uint32	DBAPP_VERSION_BINARY_STRINGS 	= 6;
// Changing bigworldLogOnMapping from recordName to entity database ID.
const uint32	DBAPP_VERSION_LOGON_MAPPING 	= 7;
// Adding shouldAutoLoad column to bigworldLogOns.
const uint32	DBAPP_VERSION_SHOULD_AUTO_LOAD 	= 8;
// Adding NULL support for entityType and entityID. NOT NULL for password
const uint32	DBAPP_VERSION_LOGON_MAPPING_2 	= 9;


const uint32	DBAPP_CURRENT_VERSION	= DBAPP_VERSION_LOGON_MAPPING_2;
const uint32	DBAPP_OLDEST_SUPPORTED_VERSION	= 1;

BW_END_NAMESPACE

#endif // MYSQL_VERSIONS_HPP
