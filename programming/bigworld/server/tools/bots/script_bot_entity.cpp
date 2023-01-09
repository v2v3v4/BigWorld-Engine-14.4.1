#include "script/first_include.hpp"

#include "script_bot_entity.hpp"

#include "bots_config.hpp"
#include "client_app.hpp"
#include "py_entity.hpp"

#include "common/simple_client_entity.hpp"

#include "entitydef/script_data_sink.hpp"

#include "network/msgtypes.hpp"

DECLARE_DEBUG_COMPONENT2( "ScriptBotEntity", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
 ScriptBotEntity:: ScriptBotEntity( const ClientApp & clientApp, const EntityType & type ):
		Entity( clientApp, type ),
		pPyEntity_()
{
	MF_ASSERT( BotsConfig::shouldUseScripts() );
	BW_STATIC_ASSERT( std::tr1::is_polymorphic< PyEntity >::value == false,
		PyEntity_is_virtual_but_uses_PyType_GenericAlloc );

	PyObject * pObject = type.type().genericAlloc( ScriptErrorPrint() );

	MF_ASSERT( pObject != NULL );

	pPyEntity_ = PyEntityPtr( new (pObject) PyEntity( *this ),
		PyEntityPtr::FROM_NEW_REFERENCE );
	MF_ASSERT( pPyEntity_ );
}


/**
 *	Destructor.
 */
 ScriptBotEntity::~ ScriptBotEntity()
{
}

ScriptObject ScriptBotEntity::pPyEntity() const
{
	return pPyEntity_;
}

/**
 *	This method handles a call to a method of the entity sent from the server.
 */
void  ScriptBotEntity::onMethod( int methodID, BinaryIStream & data )
{
	SimpleClientEntity::methodEvent( pPyEntity_, this->type().description(),
		methodID, data );
}


/**
 *	Thid method handles a property update message from the server.
 */
void  ScriptBotEntity::onProperty( int propertyID, BinaryIStream & data,
	bool isInitialising )
{
	SimpleClientEntity::propertyEvent( pPyEntity_, this->type().description(),
		propertyID, data, /* callSetForTopLevel */ !isInitialising );
}


/**
 *	This method is called when a nested property change has been sent from
 *	the server.
 */
void  ScriptBotEntity::onNestedProperty( BinaryIStream & data, bool isSlice,
	bool isInitialising )
{
	SimpleClientEntity::nestedPropertyEvent( pPyEntity_, this->type().description(),
		data, /* callSetForTopLevel */ !isInitialising, isSlice );
}


bool  ScriptBotEntity::initCellEntityFromStream( BinaryIStream & data )
{
	return pPyEntity_->initCellEntityFromStream( data );
}


/**
 *	This method is called when initialising the player from its base
 *	entity, and the BASE_AND_CLIENT properties are streamed.
 */
bool  ScriptBotEntity::initBasePlayerFromStream( BinaryIStream & data )
{
	return pPyEntity_->initBasePlayerFromStream( data );
}


/**
 *	This method is called when initialising the player from its cell
 *	entity, and OWN_CLIENT, OTHER_CLIENT and ALL_CLIENT properties are
 *	streamed.
 */
bool  ScriptBotEntity::initCellPlayerFromStream( BinaryIStream & data )
{
	return pPyEntity_->initCellPlayerFromStream( data );
}


/**
 *  This method is called when restoring the player from its cell entity.
 *
 *  @param data The BASE_AND_CLIENT, then OWN_CLIENT, OTHER_CLIENT and
 *  ALL_CLIENT properties as per initBasePlayerFromStream and
 *  initCellPlayerFromStream.
 */
bool  ScriptBotEntity::restorePlayerFromStream( BinaryIStream & data )
{
	return SimpleClientEntity::resetPropertiesEvent( pPyEntity_,
		this->type().description(), data );
}


/**
 *	This method is called when this entity becomes the player.
 */
void  ScriptBotEntity::onBecomePlayer()
{
	pPyEntity_.callMethod( "onBecomePlayer", ScriptErrorPrint(),
		/* allowNullMethod */ true );
}


/**
 *	This method is called when this entity stops being the player.
 */
void  ScriptBotEntity::onBecomeNonPlayer()
{
	pPyEntity_.callMethod( "onBecomeNonPlayer", ScriptErrorPrint(),
			/* allowNullMethod */ true );
}


/**
 *	This method is called when this entity is about to cease to exist, to
 *	give subclasses a chance to clean up any pending reference counting or
 *	similar.
 */
void  ScriptBotEntity::onDestroyed()
{
	pPyEntity_->onEntityDestroyed();

	pPyEntity_ = PyEntityPtr();

	MF_ASSERT( !pPyEntity_ );
}

BW_END_NAMESPACE


// script_bot_entity.cpp
