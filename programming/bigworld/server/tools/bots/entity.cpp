#include "script/first_include.hpp"

#include "entity.hpp"

#include "bots_config.hpp"
#include "client_app.hpp"
#include "py_entity.hpp"

#include "common/simple_client_entity.hpp"

#include "entitydef/script_data_sink.hpp"

#include "network/msgtypes.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
Entity::Entity( const ClientApp & clientApp, const EntityType & type ) :
		BWEntity( clientApp.pConnection() ),
		type_( type ),
		clientApp_( clientApp )
{
}


/**
 *	Destructor.
 */
Entity::~Entity()
{
}

/**
 *	This method is called to determine the stream size of a property update
 *	message from the server.
 *
 *	@param	propertyID	The id of the property to check.
 */
int Entity::getMethodStreamSize( int methodID ) const
{
	const EntityDescription & description = type_.description();

	const MethodDescription * pMethodDescription =
		description.client().exposedMethod( methodID );

	MF_ASSERT( pMethodDescription );

	return pMethodDescription->streamSize( true );
}

/**
 *	This method is called to determine the stream size of a property update
 *	message from the server.
 *
 *	@param	propertyID	The id of the property to check.
 */
int  Entity::getPropertyStreamSize( int propertyID ) const
{
	const EntityDescription & description = type_.description();

	return description.clientServerProperty( propertyID )->streamSize();
}


/**
 *	This method returns the type name of this entity.
 */
const BW::string  Entity::entityTypeName() const
{
	return type_.name();
}

/**
 *	This method is called when we enter the world
 */
void Entity::onEnterWorld()
{
	if (this->isPlayer() && this->isControlled())
	{
		// TODO: Get rid of this const_cast, make movement controllers
		// part of the entity itself.
		const_cast< ClientApp & >( clientApp_ ).setDefaultMovementController();
	}
}

/**
 *	This method is called when this entity changes its state of being
 *	server-controlled.
 */
void Entity::onChangeControl( bool isControlling, bool isInitialising )
{
	MF_ASSERT( isControlling == this->isControlled() );

	if (!this->isPlayer())
	{
		return;
	}

	INFO_MSG( "Bot %d %s of the player\n", this->entityID(),
		isControlling ? "is in control" : "loses control" );

}


BW_END_NAMESPACE


// entity.cpp
