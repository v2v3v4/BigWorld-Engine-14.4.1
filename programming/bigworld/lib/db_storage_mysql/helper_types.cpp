#include "helper_types.hpp"

#include "wrapper.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
MySqlEscapedString::MySqlEscapedString( MySql & connection,
		const BW::string& string ) :
	escapedString_( new char[ string.length() * 2 + 1 ] )
{
	mysql_real_escape_string( connection.get(), escapedString_,
			string.data(), string.length() );
}


/**
 *	Destructor.
 */
MySqlEscapedString::~MySqlEscapedString()
{
	delete [] escapedString_;
}

BW_END_NAMESPACE

// helper_types.cpp
