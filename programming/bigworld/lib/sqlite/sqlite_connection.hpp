#ifndef SQLITE_CONNECTION_HPP
#define SQLITE_CONNECTION_HPP

#include "cstdmf/debug.hpp"

#include "sqlite/sqlite3.h"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class represents an SQLite database session.
 */
class SqliteConnection
{
public:
	SqliteConnection();
	~SqliteConnection();

	bool open( const BW::string & sqliteDbPath );

	bool backup( const SqliteConnection * srcDb,
		const BW::string & databaseName );

	const char * lastError() const;

	sqlite3 * get() { return pDbHandle_; }

private:
	sqlite3 * pDbHandle_;
};

BW_END_NAMESPACE

#endif // SQLITE_CONNECTION_HPP
