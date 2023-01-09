#include "sqlite_statement.hpp"

#include "sqlite_connection.hpp"


#include "sqlite/sqlite3.h"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
SqliteStatement::SqliteStatement( SqliteConnection & connection,
		const BW::string & statement, int & result ) :
	pStmt_( NULL )
{
	result = sqlite3_prepare_v2( connection.get(), statement.c_str(), 
		-1, &pStmt_, NULL );
}


/**
 *	Destructor.
 */
SqliteStatement::~SqliteStatement()
{
	MF_VERIFY( sqlite3_finalize( pStmt_ ) == SQLITE_OK );
}


sqlite3_stmt * SqliteStatement::get()
{
	return pStmt_;
}


int SqliteStatement::reset()
{
	return sqlite3_reset( pStmt_ );
}


int SqliteStatement::step()
{
	return sqlite3_step( pStmt_ );
}


const unsigned char * SqliteStatement::textColumn( int column )
{
	return sqlite3_column_text( pStmt_, column );
}


int SqliteStatement::intColumn( int column )
{
	return sqlite3_column_int( pStmt_, column );
}


int64 SqliteStatement::int64Column( int column )
{
	return sqlite3_column_int64( pStmt_, column );
}


const void * SqliteStatement::blobColumn( int column, int * pSize ) 
{ 
	if (pSize)
	{
		*pSize = this->columnBytes( column );
	}

	return sqlite3_column_blob( pStmt_, column );
}


int SqliteStatement::columnBytes( int column )
{
	return sqlite3_column_bytes( pStmt_, column );
}

BW_END_NAMESPACE

// sqlite_statement.cpp
