#include "stream_to_query_helper.hpp"

#include "../query_runner.hpp"
#include "../wrapper.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChildQuery
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChildQuery::ChildQuery( MySql & connection, const Query & query ) :
	queryRunner_( connection, query ),
	helper_( connection, 0 )
{
}


/**
 *	This method places the ID of the parent DB record into the query string
 *	and executes the delayed query, calling any other buffered queries when
 *	complete.
 *
 *	@param parentID  The ID of the record in the parent table this query is
 *	                 associated with.
 */
void ChildQuery::execute( DatabaseID parentID )
{
	queryRunner_.pushArg( parentID );
	queryRunner_.execute( NULL );

	if (helper_.hasBufferedQueries())
	{
		helper_.executeBufferedQueries( queryRunner_.connection().insertID() );
	}
}


// -----------------------------------------------------------------------------
// Section: StreamToQueryHelper
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StreamToQueryHelper::StreamToQueryHelper( MySql & connection,
		DatabaseID parentID ) :
	connection_( connection ),
	parentID_( parentID )
{
}


/**
 *	Destructor.
 */
StreamToQueryHelper::~StreamToQueryHelper()
{
	BufferedQueries::iterator iter = bufferedQueries_.begin();

	while (iter != bufferedQueries_.end())
	{
		delete *iter;

		++iter;
	}
}


/**
 *	This method executes any queries that have been delayed until a parent
 *	record ID had been produced.
 *
 *	@param parentID  The ID of the parent record to associate with the buffered
 *	                 queries.
 */
void StreamToQueryHelper::executeBufferedQueries( DatabaseID parentID )
{
	BufferedQueries::iterator iter = bufferedQueries_.begin();

	while (iter != bufferedQueries_.end())
	{
		(*iter)->execute( parentID );

		++iter;
	}
}


/**
 *	This method creates a child query for the query helper and buffers it to be
 *	executed when the parent record has been written.
 *
 *	@param query  The parent query for which a child will be created.
 */
ChildQuery & StreamToQueryHelper::createChildQuery( const Query & query )
{
	bufferedQueries_.push_back( new ChildQuery( connection_, query ) );

	return *bufferedQueries_.back();
}

BW_END_NAMESPACE

// stream_to_query_helper.cpp
