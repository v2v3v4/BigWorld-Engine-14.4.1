#include "pch.hpp"
#include "bw_connection_helper.hpp"
#include "bw_connection.hpp"
#include "bw_entity_factory.hpp"
#include "bw_null_connection.hpp"
#include "bw_replay_connection.hpp"
#include "bw_server_connection.hpp"
#include "bw_space_data_storage.hpp"

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param entityFactory 		The BWEntityFactory reference to use when 
 *								requested to create entities. 
 *	@param spaceDataStorage		The BWSpaceDataStorage reference for storing 
 *								space data. 
 *	@param entityDefConstants	The constant entity definition values to use
 *								during initialisation.
 */
BWConnectionHelper::BWConnectionHelper( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage, 
		const EntityDefConstants & entityDefConstants ):
	entityFactory_( entityFactory ),
	spaceDataStorage_( spaceDataStorage ),
	entityDefConstants_( entityDefConstants )
{
}


/**
 *	Destructor.
 */
BWConnectionHelper::~BWConnectionHelper()
{
}


/**
 *	This method ticks the connection and condemned interfaces
 *
 *	@param pConnection 	The connection instance.
 *
 *	@param dt 	The time delta.
 *
 */
void BWConnectionHelper::update( BWConnection * pConnection, float dt )
{ 
	if (pConnection)
	{
		pConnection->update( dt );
	}
	condemnedInterfaces_.processOnce();
}


/**
 *	This method sends any updated state to the server after entities have
 *	updated their state.
 *
 *	@param pConnection 	The connection instance.
 *
 */
void BWConnectionHelper::updateServer( BWConnection * pConnection )
{
	if (pConnection)
	{
		pConnection->updateServer();
	}
}


/**
 *  This method creates a BWServerConnection.
 *
 *	@param initialClientTime	The initial client time of connection creation
 *
 */
BWServerConnection * BWConnectionHelper::createServerConnection(
	double initialClientTime )
{
	return new BWServerConnection( 
		entityFactory_,
		spaceDataStorage_, 
		loginChallengeFactories_,
		condemnedInterfaces_,
		entityDefConstants_,
		initialClientTime );
}


/**
 *  This method create a BWNullConnection.
 *
 *	@param initialClientTime	The initial client time of connection creation
 *
 *	@param	spaceID				The SpaceID to tell the client it is in.
 *
 */
BWNullConnection * BWConnectionHelper::createNullConnection(
	double initialClientTime, SpaceID spaceID )
{
	return new BWNullConnection( 
		entityFactory_,
		spaceDataStorage_, 
		entityDefConstants_,
		initialClientTime, spaceID );
}


/**
 *  This method create a BWReplayConnection.
 *
 *	@param initialClientTime	The initial client time of connection creation
 *
 */
BWReplayConnection * BWConnectionHelper::createReplayConnection(
	double initialClientTime )
{
	return new BWReplayConnection( 
		entityFactory_,
		spaceDataStorage_, 
		entityDefConstants_,
		initialClientTime );
}

BW_END_NAMESPACE
// bw_connection_helper.cpp
