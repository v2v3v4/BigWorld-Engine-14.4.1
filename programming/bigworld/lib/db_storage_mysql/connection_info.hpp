#ifndef CONNECTION_INFO_HPP
#define CONNECTION_INFO_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

namespace DBConfig
{

/**
 *	This struct contains the information to connect to a database server.
 */
class ConnectionInfo
{
public:
	ConnectionInfo(): port( 0 ) {}

	BW::string	host;
	unsigned int port;
	BW::string	username;
	BW::string	password;
	BW::string	database;
	bool 		secureAuth;

	/**
	 *	Generates a name used by all BigWorld processes to lock the database.
	 *	Only one connection can successfully obtain a lock with this name
	 *	at any one time.
	 */
	BW::string generateLockName() const
	{
		BW::string lockName( "BigWorld ");
		lockName += database;

		return lockName;
	}
};

} // namespace DBConfig

BW_END_NAMESPACE

#endif // CONNECTION_INFO_HPP
