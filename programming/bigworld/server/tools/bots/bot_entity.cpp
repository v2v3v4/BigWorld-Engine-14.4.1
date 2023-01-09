#include "script/first_include.hpp"

#include "bot_entity.hpp"

#include "bots_config.hpp"
#include "client_app.hpp"
#include "py_entity.hpp"

#include "common/simple_client_entity.hpp"

#include "entitydef/script_data_sink.hpp"

#include "network/msgtypes.hpp"

DECLARE_DEBUG_COMPONENT2( "BotEntity", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
 BotEntity:: BotEntity( const ClientApp & clientApp, const EntityType & type ):
		Entity( clientApp, type )
{
}


/**
 *	Destructor.
 */
 BotEntity::~ BotEntity()
{
}

 ScriptObject BotEntity::pPyEntity() const
 {
	 return ScriptObject();
 }

/**
 *	This method handles a call to a method of the entity sent from the server.
 */
void BotEntity::onMethod( int methodID, BinaryIStream & data )
{
	data.finish();
}


/**
 *	This method handles a property update message from the server.
 */
void BotEntity::onProperty( int propertyID, BinaryIStream & data,
	bool isInitialising )
{
	// Consume exactly the right amount of data, because we are called
	// repeatedly from onProperties
	const DataDescription * pDD =
		this->type().description().clientServerProperty( propertyID );
	if (pDD != NULL)
	{
		ScriptDataSink sink;
		MF_VERIFY( pDD->createFromStream( data, sink,
			/* isPersistentOnly */ false ) );

		ScriptObject pValue = sink.finalise();
		MF_ASSERT( pValue );
	}
}


/**
 *	This method is called when a nested property change has been sent from
 *	the server.
 */
void BotEntity::onNestedProperty( BinaryIStream & data, bool isSlice,
	bool isInitialising )
{
	data.finish();
}

bool BotEntity::initCellEntityFromStream( BinaryIStream & data )
{
	data.finish();
	return true;
}


/**
 *	This method is called when initialising the player from its base
 *	entity, and the BASE_AND_CLIENT properties are streamed.
 */
bool BotEntity::initBasePlayerFromStream( BinaryIStream & data )
{
	data.finish();
	return true;
}


/**
 *	This method is called when initialising the player from its cell
 *	entity, and OWN_CLIENT, OTHER_CLIENT and ALL_CLIENT properties are
 *	streamed.
 */
bool BotEntity::initCellPlayerFromStream( BinaryIStream & data )
{
	data.finish();
	return true;
}


/**
 *  This method is called when restoring the player from its cell entity.
 *
 *  @param data The BASE_AND_CLIENT, then OWN_CLIENT, OTHER_CLIENT and
 *  ALL_CLIENT properties as per initBasePlayerFromStream and
 *  initCellPlayerFromStream.
 */
bool BotEntity::restorePlayerFromStream( BinaryIStream & data )
{
	data.finish();
	return true;
}


/**
 *	This method is called when this entity becomes the player.
 */
void BotEntity::onBecomePlayer()
{
}


/**
 *	This method is called when this entity stops being the player.
 */
void BotEntity::onBecomeNonPlayer()
{
}


/**
 *	This method is called when this entity is about to cease to exist, to
 *	give subclasses a chance to clean up any pending reference counting or
 *	similar.
 */
void BotEntity::onDestroyed()
{
}


BW_END_NAMESPACE


// bot_entity.cpp
