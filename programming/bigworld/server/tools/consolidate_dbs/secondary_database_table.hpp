#ifndef CONSOLIDATE_DBS__SECONDARY_DATABASE_TABLE_HPP
#define CONSOLIDATE_DBS__SECONDARY_DATABASE_TABLE_HPP

#include "network/basictypes.hpp"

#include <memory>
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class ConsolidationProgressReporter;
class PrimaryDatabaseUpdateQueue;
class SecondaryDatabase;
class SqliteStatement;

/**
 *	Class to keep track of entity data stored in a table in a secondary
 *	database.
 */
class SecondaryDatabaseTable
{
public:
	SecondaryDatabaseTable( SecondaryDatabase & database, 
			const BW::string & tableName );
	bool init();

	~SecondaryDatabaseTable();

	const BW::string & tableName() const
		{ return tableName_; }

	GameTime firstGameTime();

	int numRows();

	bool consolidate( PrimaryDatabaseUpdateQueue & primaryQueue,
			ConsolidationProgressReporter & progressReporter,
			bool & shouldAbort );

private:
	SecondaryDatabase & 				database_;
	BW::string 						tableName_;
	std::auto_ptr< SqliteStatement > 	pGetDataQuery_;
	std::auto_ptr< SqliteStatement > 	pGetNumRowsQuery_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__SECONDARY_DATABASE_TABLE_HPP

