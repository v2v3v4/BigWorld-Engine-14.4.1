#include "pch.hpp"

#include "entity_udo_factory.hpp"

#include "connection_control.hpp"
#include "entity.hpp"

#include "chunk/user_data_object.hpp"
#include "chunk/user_data_object_type.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entities.hpp"
#include "connection_model/bw_entity.hpp"
#include "connection_model/bw_null_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"


BW_BEGIN_NAMESPACE

struct EntityUDOFactory::UDOHolder
{
	UserDataObjectPtr pUDO_;
};

// ----------------------------------------------------------------------------
CompiledSpace::IEntityUDOFactory::EntityID EntityUDOFactory::createEntity(
	SpaceID spaceID, const DataSectionPtr & ds )
{
	BW::string type = ds->readString( "type" );
	const EntityType * pType = EntityType::find( type );
	if (pType == NULL)
	{
		ERROR_MSG( "No such entity type '%s'\n", type.c_str() );
		return NULL_ENTITY_ID;
	}

	DataSectionPtr pTransform = ds->openSection( "transform" );
	if (!pTransform)
	{
		return NULL_ENTITY_ID;
	}

	Matrix transform = ds->readMatrix34( "transform", Matrix::identity );
	DataSectionPtr pPropertiesDS = ds->openSection( "properties" );
	if (!pPropertiesDS)
	{
		ERROR_MSG( "Entity has no 'properties' section\n" );
		return NULL_ENTITY_ID;
	}

	Entity* pEntity = EntityUDOFactory::createEntityWithType( spaceID, pType,
		transform, pPropertiesDS );
	MF_ASSERT( pEntity != NULL );

	return pEntity->entityID();
}


// ----------------------------------------------------------------------------
void EntityUDOFactory::destroyEntity( EntityID entityID )
{
	EntityUDOFactory::destroyEntityByID( entityID );
}


// ----------------------------------------------------------------------------
CompiledSpace::IEntityUDOFactory::UDOHandle EntityUDOFactory::createUDO( 
	const DataSectionPtr& ds )
{
	BW::string type = ds->readString( "type" );
	if (UserDataObjectType::knownTypeInDifferentDomain( type.c_str() ))
	{
		return 0;
	}

	UserDataObjectTypePtr pType = UserDataObjectType::getType( type.c_str() );
	if (pType == NULL)
	{
		ERROR_MSG( "EntityUDOFactory::createUDO: "
			"No such UserDataObject type '%s'\n", type.c_str() );
		return 0;
	}

	BW::string idStr = ds->readString( "guid" );
	if (idStr.empty())
	{
		ERROR_MSG( "EntityUDOFactory::createUDO: "
			" UserDataObject has no GUID" );
		return 0;
	}

	UserDataObjectInitData initData;
	initData.guid = UniqueID( idStr );

	DataSectionPtr pTransform = ds->openSection( "transform" );
	if (!pTransform)
	{
		ERROR_MSG( "EntityUDOFactory::createUDO: "
			"UserDataObject (of type '%s') has no position", type.c_str() );
		return 0;
	}

	Matrix m = ds->readMatrix34( "transform", Matrix::identity );
	initData.position = m.applyToOrigin();
	initData.direction = Direction3D(Vector3( m.roll(), m.pitch(), m.yaw() ));

	initData.propertiesDS = ds->openSection( "properties" );
	if (!initData.propertiesDS)
	{
		ERROR_MSG( "EntityUDOFactory::createUDO: "
			"UserDataObject has no 'properties' section" );
		return 0;
	}

	SmartPointer< UserDataObject > pUserDataObject =
		UserDataObject::findOrLoad( initData, pType );
	pUserDataObject->incChunkItemRefCount();
	if (!pUserDataObject)
	{
		return 0;
	}

	UDOHolder* pInst = new UDOHolder;
	pInst->pUDO_ = pUserDataObject;
	return (UDOHandle)pInst;
}


// ----------------------------------------------------------------------------
void EntityUDOFactory::destroyUDO( UDOHandle udo )
{
	UDOHolder* pInst = (UDOHolder*)udo;
	MF_ASSERT( pInst );
	if (pInst->pUDO_)
	{
		pInst->pUDO_->decChunkItemRefCount();
	}
	bw_safe_delete( pInst );
}


// ----------------------------------------------------------------------------
/* static */ Entity * EntityUDOFactory::createEntityWithType( SpaceID spaceID,
	const EntityType * pType, const Matrix & transform,
	const DataSectionPtr & pPropertiesDS )
{
	ConnectionControl & connectionControl = ConnectionControl::instance();
	SpaceManager & spaceManager = SpaceManager::instance();

	BWConnection * pConnection = spaceManager.isLocalSpace( spaceID ) ?
			connectionControl.pSpaceConnection( spaceID ) :
			connectionControl.pConnection();

	if (!pConnection)
	{
		WARNING_MSG( "EntityUDOFactory::createEntityWithType: "
				"No server connection or client space with ID %d",
			spaceID );
		return NULL;
	}

	MemoryOStream propertiesStream( 2048 );
	pType->prepareCreationStream( propertiesStream, pPropertiesDS );
	propertiesStream.rewind();

	Vector3 position = transform.applyToOrigin();
	Direction3D direction;
	direction.yaw = transform.yaw();
	direction.pitch = transform.pitch();
	direction.roll = transform.roll();

	BWEntity * pEntity = pConnection->createLocalEntity( pType->index(),
		spaceID, NULL_ENTITY_ID, position, direction, propertiesStream );

	MF_ASSERT( pEntity->isLocalEntity() );

	return static_cast< Entity * >( pEntity );
}


// ----------------------------------------------------------------------------
/* static */ void EntityUDOFactory::destroyEntityByID( EntityID id )
{
	MF_ASSERT( BWEntities::isLocalEntity( id ) );

	BWEntity * pEntity = ConnectionControl::instance().findEntity( id );

	if (!pEntity)
	{
		ERROR_MSG( "EntityUDOFactory::destroyEntityByID: "
				"No entity with ID %d known to the system.\n",
			id );
		return;
	}

	MF_VERIFY( pEntity->pBWConnection()->destroyLocalEntity( id ) );
}


/**
 *	This static method destroys the supplied local Entity. The Entity pointer
 *	itself may still be valid.
 */
/* static */ void EntityUDOFactory::destroyBWEntity( BWEntity * pEntity )
{
	// In case this is the last reference
	BWEntityPtr pEntityHolder = pEntity;

	MF_ASSERT( !pEntity->isDestroyed() );

	pEntity->pBWConnection()->destroyLocalEntity( pEntity->entityID() );

	MF_ASSERT( pEntity->isDestroyed() );
}


BW_END_NAMESPACE
