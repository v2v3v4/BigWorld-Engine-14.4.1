#ifndef SQLITE_STATEMENT_HPP
#define SQLITE_STATEMENT_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include "sqlite_connection.hpp"

#include "sqlite/sqlite3.h"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This class provides a wrapper for the SQLite C API statement object.
 */
class SqliteStatement
{
public:
	SqliteStatement( SqliteConnection & connection,
			const BW::string & statement, int & result );
	~SqliteStatement();

	sqlite3_stmt * get();

	int reset();

	int step();

	const unsigned char * textColumn( int column );
	int intColumn( int column );
	int64 int64Column( int column );
	const void * blobColumn( int column, int * pSize );
	int columnBytes( int column );

private:
	sqlite3_stmt * pStmt_;
};

BW_END_NAMESPACE

#endif // SQLITE_STATEMENT_HPP
