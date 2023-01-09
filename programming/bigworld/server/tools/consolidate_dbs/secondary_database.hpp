#ifndef SECONDARY_DATABASE_HPP
#define SECONDARY_DATABASE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/shared_ptr.hpp"

#include <memory>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class ConsolidationProgressReporter;
class PrimaryDatabaseUpdateQueue;
class SecondaryDatabaseTable;
class SqliteConnection;

/**
 * 	Consolidates data from remote secondary databases.
 */
class SecondaryDatabase
{
public:
	SecondaryDatabase();
	~SecondaryDatabase();

	bool init( const BW::string & dbPath );

	bool getChecksumDigest( BW::string & digest );

	uint numEntities() const
		{ return numEntities_; }

	SqliteConnection & connection()
		{ return *pConnection_; }

	bool consolidate( PrimaryDatabaseUpdateQueue & primaryDBQueue,
			ConsolidationProgressReporter & progressReporter,
			bool shouldIgnoreErrors,
			bool & shouldAbort );

private:
	bool readTables();
	bool tableExists( const BW::string & tableName );
	void sortTablesByAge();

	BW::string							path_;

	std::auto_ptr< SqliteConnection > 	pConnection_;

	typedef BW::vector< shared_ptr< SecondaryDatabaseTable > > Tables;
	Tables 								tables_;

	uint								numEntities_;
};

BW_END_NAMESPACE

#endif // SECONDARY_DATABASE_HPP
