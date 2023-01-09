#include "db_config.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/string_utils.hpp"

#include "db/db_config.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 );

BW_BEGIN_NAMESPACE

namespace DBConfig
{

/**
 *	This method sets the serverInfo from the given data section.
 */
void readConnectionInfo( ConnectionInfo & connectionInfo )
{
	const DBConfig::Config & config = DBConfig::get();

	connectionInfo.host 		= config.mysql.host();
	connectionInfo.port 		= config.mysql.port();
	connectionInfo.username 	= config.mysql.username();
	connectionInfo.password 	= config.mysql.password();
	connectionInfo.secureAuth 	= config.mysql.secureAuth();
	connectionInfo.database 	= config.mysql.databaseName();

	bw_searchAndReplace( connectionInfo.database,
		"{{username}}", getUsername() );
	bw_searchAndReplace( connectionInfo.database,
		"{{ username }}", getUsername() );
}


const ConnectionInfo & connectionInfo()
{
	static ConnectionInfo s_connectionInfo;
	static bool isFirst = true;

	if (isFirst)
	{

		s_connectionInfo.host = "localhost";
		s_connectionInfo.port = 0;
		s_connectionInfo.username = "bigworld";
		s_connectionInfo.password = "bigworld";
		s_connectionInfo.database = "";
		s_connectionInfo.secureAuth = false;

		isFirst = false;
		readConnectionInfo( s_connectionInfo );
	}

	return s_connectionInfo;
}

}	// namespace DBConfig

BW_END_NAMESPACE

// db_config.cpp

