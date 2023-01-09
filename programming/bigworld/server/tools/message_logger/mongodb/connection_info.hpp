#ifndef MONGODB_CONNECTION_INFO_HPP
#define MONGODB_CONNECTION_INFO_HPP

#include "constants.hpp"


BW_BEGIN_NAMESPACE

namespace MongoDB
{

/**
 *  This structure contains the information required to connect to a MongoDB
 *  server.
 */
class ConnectionInfo
{
public:
	ConnectionInfo() :
		dbPort_( 0 ), tcpTimeout_( TCP_TIME_OUT ),
		waitBetweenReconnects_( WAIT_BETWEEN_RECONNECTS )
		{}

	BW::string dbHost_;
	uint16 dbPort_;
	BW::string dbUser_;
	BW::string dbPasswd_;
	uint32 tcpTimeout_;
	uint16 waitBetweenReconnects_;
};

} // namespace MongoDB

BW_END_NAMESPACE

#endif // MONGODB_CONNECTION_INFO_HPP
