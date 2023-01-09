#ifndef BW_CONNECTION_HELPER_HPP
#define BW_CONNECTION_HELPER_HPP

#include "bw_connection.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "connection/condemned_interfaces.hpp"
#include "connection/login_challenge_factory.hpp"


BW_BEGIN_NAMESPACE

class BWEntityFactory;
class BWSpaceDataStorage;
class BWConnection;
class BWServerConnection;
class BWNullConnection;
class BWReplayConnection;
class CondemnedInterfaces;
class LoginChallengeFactories;
class EntityDefConstants;

/**
 * This class is used to help creating connection to the BigWorld server.
 *
 *	Applications should create a single instance of this class. Instances can
 *	be used for:
 *
 * - creating connections 
 * - calling update and updateServer on connection
 */


class BWConnectionHelper
{
public:
	BWConnectionHelper( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage, 
		const EntityDefConstants & entityDefConstants );
	~BWConnectionHelper();
	
	// Connection ticking
	void update( BWConnection* pConnection, float dt );
	void updateServer ( BWConnection* pConnection );

	BWServerConnection* createServerConnection(  
		double initialClientTime );

	BWNullConnection * createNullConnection(
		double initialClientTime, SpaceID spaceID );

	BWReplayConnection * createReplayConnection(
		double initialClientTime );


private:
	BWEntityFactory & entityFactory_;
	BWSpaceDataStorage & spaceDataStorage_;
	const EntityDefConstants & entityDefConstants_;
	CondemnedInterfaces condemnedInterfaces_;
	LoginChallengeFactories loginChallengeFactories_;
};


BW_END_NAMESPACE

#endif // BW_CONNECTION_HELPER_HPP

