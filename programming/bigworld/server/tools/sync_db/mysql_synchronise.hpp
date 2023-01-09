#ifndef MYSQL_SYNCHRONISE_HPP
#define MYSQL_SYNCHRONISE_HPP

#include "db_storage_mysql/database_tool_app.hpp"
#include "db_storage_mysql/db_config.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class MySql;

class MySqlSynchronise : public DatabaseToolApp
{
public:
	MySqlSynchronise() : DatabaseToolApp() {}
	~MySqlSynchronise() {}

	bool init( bool isVerbose, bool shouldLock, bool isDryRun,
		DBConfig::ConnectionInfo & connectionInfo );

	bool run();

	MySql & connection()
		{ return this->DatabaseToolApp::connection(); }

	// Overrides from DatabaseToolApp
	MySqlLockedConnection * createMysqlConnection (
		const DBConfig::ConnectionInfo & connectionInfo ) const;

	const EntityDefs & entityDefs()
		{ return this->DatabaseToolApp::entityDefs(); }

	bool synchroniseEntityDefinitions( bool allowNew,
		const BW::string & characterSet = "",
		const BW::string & collation = "" );

private:
	bool doSynchronise();
	void createSpecialBigWorldTables();
	bool updatePasswordHash( bool wasHashed );
	bool alterDBCharSet( const BW::string & characterSet, 
		const BW::string & collation );

	bool isDryRun_;

};

BW_END_NAMESPACE

#endif // MYSQL_SYNCHRONISE_HPP
