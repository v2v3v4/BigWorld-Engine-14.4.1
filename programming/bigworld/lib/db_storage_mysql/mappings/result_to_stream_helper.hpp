#ifndef RESULT_TO_STREAM_HELPER_HPP
#define RESULT_TO_STREAM_HELPER_HPP

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class MySql;

/**
 *	This class is used as a helper when running
 *	PropertyMapping::fromStreamToDatabase.
 */
class ResultToStreamHelper
{
public:
	ResultToStreamHelper( MySql & connection, DatabaseID parentID = 0 ) :
		connection_( connection ),
		parentID_( parentID )
	{
	}

	MySql & connection()				{ return connection_; }

	DatabaseID parentID() const			{ return parentID_; }
	void parentID( DatabaseID id )		{ parentID_ = id; }

private:
	MySql & connection_;
	DatabaseID parentID_;
};

BW_END_NAMESPACE

#endif // RESULT_TO_STREAM_HELPER_HPP
