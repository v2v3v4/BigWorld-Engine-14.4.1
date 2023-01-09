#include "result_set.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ResultSet
// -----------------------------------------------------------------------------

/**
 *
 */
ResultSet::ResultSet() : pResultSet_( NULL )
{
}


/**
 *
 */
ResultSet::~ResultSet()
{
	this->setResults( NULL );
}


/**
 *	This method returns the number of result rows in this set.
 */
int ResultSet::numRows() const
{
	return pResultSet_ ? mysql_num_rows( pResultSet_ ) : 0;
}


/**
 *
 */
void ResultSet::setResults( MYSQL_RES * pResultSet )
{
	if (pResultSet_ != NULL)
	{
		mysql_free_result( pResultSet_ );
	}

	pResultSet_ = pResultSet;
}

BW_END_NAMESPACE

// result_set.cpp
