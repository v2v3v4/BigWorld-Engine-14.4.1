#include "query.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Query
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Query::Query( const BW::string & stmt, bool shouldPartitionArgs )
{
	if (!stmt.empty())
	{
		this->init( stmt, shouldPartitionArgs );
	}
}


/**
 *	This method initialises this query from a query string. The query string
 *	should contain '?' characters to indicate the arguments of the query.
 */
bool Query::init( const BW::string & stmt, bool shouldPartitionArgs )
{
	IF_NOT_MF_ASSERT_DEV( queryParts_.empty() )
	{
		return false;
	}

	if (!shouldPartitionArgs)
	{
		queryParts_.push_back( stmt );

		return true;
	}

	// TODO: Need better query parsing to handle '?' characters in strings that
	// should not be treated as argument placeholders. Only really an issue for
	// executeRawDatabaseCommand since everything else should not hardcode
	// strings in the query template.

	BW::string::const_iterator partStart = stmt.begin();
	BW::string::const_iterator partEnd = stmt.begin();

	while (partEnd != stmt.end())
	{
		if (*partEnd == '?')
		{
			queryParts_.push_back( BW::string( partStart, partEnd ) );
			partStart = partEnd + 1;
		}

		++partEnd;
	}

	queryParts_.push_back( BW::string( partStart, partEnd ) );

	return true;
}

BW_END_NAMESPACE


// statement.cpp
