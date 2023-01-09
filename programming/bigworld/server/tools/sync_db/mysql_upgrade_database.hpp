#ifndef MYSQL_UPGRADE_DATABASE_HPP
#define MYSQL_UPGRADE_DATABASE_HPP

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

class MySqlSynchronise;

class MySqlUpgradeDatabase
{
public:
	MySqlUpgradeDatabase( MySqlSynchronise & synchronise );

	bool run( uint32 version );

private:
	bool upgradeDatabaseLogOnMapping2();
	bool upgradeDatabaseShouldAutoLoad();
	bool upgradeDatabaseLogOnMapping();
	bool upgradeDatabaseBinaryStrings();
	void upgradeDatabase1_9NonNull();
	void upgradeDatabase1_9Snapshot();
	void upgradeDatabase1_8();

	void upgradeVersionNumber( const uint32 newVersion );
	bool convertRecordNameToDBID( int bwEntityTypeID, int dbEntityTypeID );

// Member data

	MySqlSynchronise & synchronise_;
};

BW_END_NAMESPACE

#endif /* MYSQL_UPGRADE_DATABASE_HPP */
