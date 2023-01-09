#ifndef SERVER_CONNECTION_HANDLER_HPP
#define SERVER_CONNECTION_HANDLER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class LogOnStatus;

/**
 *	This interface is used to pass event about connection to the server.
 */
class ServerConnectionHandler
{
public:
	virtual ~ServerConnectionHandler() {}

	virtual void onLoggedOff() = 0;
	virtual void onLoggedOn() = 0;
	virtual void onLogOnFailure( const LogOnStatus & status,
		const BW::string & message ) = 0;
};

BW_END_NAMESPACE

#endif // SERVER_CONNECTION_HANDLER_HPP
