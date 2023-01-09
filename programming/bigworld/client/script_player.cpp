#include "pch.hpp"

#include "script_player.hpp"

#include "entity_flare_collider.hpp"

#include "py_entity.hpp"

#include "chunk/chunk_light.hpp"

#include "duplo/pymodelnode.hpp"

#include "romp/lens_effect_manager.hpp"
#include "romp/z_buffer_occluder.hpp"

#include "space/client_space.hpp"

DECLARE_DEBUG_COMPONENT2( "App", 0 )


BW_BEGIN_NAMESPACE

BW_INIT_SINGLETON_STORAGE( ScriptPlayer )

/// Constructor
ScriptPlayer::ScriptPlayer() :
	pEntity_( NULL ),
	playerFlareCollider_( NULL )
{
}


// Destructor
ScriptPlayer::~ScriptPlayer()
{
	BW_GUARD;

	MF_ASSERT(pEntity_ == NULL);
}

/**
 *	ScriptPlayer singleton initialisation.
 */
bool ScriptPlayer::doInit()
{
	BW_GUARD;
	return true;
}

/**
 *	ScriptPlayer singleton fini.
 */
bool ScriptPlayer::doFini()
{
	BW_GUARD;
	return true;
}


/**
 *	Change the entity that the client controls.
 *	A NULL entity pointer is permitted to indicate no entity.
 *
 *	@return True on success, otherwise false.
 */
bool ScriptPlayer::setPlayer( Entity * newPlayer )
{
	BW_GUARD;

	MF_ASSERT( Py_IsInitialized() );

	if (newPlayer != NULL && newPlayer->type().pPlayerClass() == NULL)
	{
		ERROR_MSG( "ScriptPlayer::setPlayer: "
			"newPlayer id %d of type %s does not have an associated player class.\n",
			newPlayer->entityID(), newPlayer->type().name().c_str() );

		return false;
	}

	if (newPlayer == pEntity_)
	{
		return true;
	}

	// if there's already a player tell it its time has come
	if (pEntity_ != NULL && !pEntity_->isDestroyed())
	{
		pEntity_->pPyEntity()->makeNonPlayer();

		for (uint i = 0; i < attachedPSs_.size(); i++)
		{
			attachedPSs_[i].pNode_->detach( 
				PyAttachmentPtr( attachedPSs_[i].pAttachment_ ) );
		}

		attachedPSs_.clear();

		LensEffectManager::instance().delPhotonOccluder( playerFlareCollider_ );
		playerFlareCollider_ = NULL;
	}

	// we are back to no player now
	pEntity_ = NULL;

	// set a new player if instructed
	if (newPlayer != NULL && newPlayer->type().pPlayerClass() != NULL)
	{
		// make the player script object
		pEntity_ = newPlayer;

		MF_ASSERT( !pEntity_->isDestroyed() );
		MF_VERIFY( pEntity_->pPyEntity()->makePlayer() );

		// adorn the model with weather particle systems
		if (pEntity_->pSpace())
		{
			updateWeatherParticleSystems(
				pEntity_->pSpace()->enviro().playerAttachments() );
		}

		if (!ZBufferOccluder::isAvailable())
		{		
			playerFlareCollider_ = new EntityPhotonOccluder( *pEntity_ );
			LensEffectManager::instance().addPhotonOccluder( playerFlareCollider_ );
		}
	}

	return true;
}


/**
 *	Grabs the particle systems that want to be attached to the player,
 *	and attaches them to its model.
 */
void ScriptPlayer::updateWeatherParticleSystems( PlayerAttachments & pa )
{
	BW_GUARD;
	for (uint i = 0; i < attachedPSs_.size(); i++)
	{
		attachedPSs_[i].pNode_->detach( 
			PyAttachmentPtr( attachedPSs_[i].pAttachment_ ) );
	}

	attachedPSs_.clear();

	if (pa.empty()) return;
	if (pEntity_ == NULL || pEntity_->pPrimaryModel() == NULL) return;

	for (uint i = 0; i < pa.size(); i++)
	{
		PyObject * pPyNode = pEntity_->pPrimaryModel()->node( pa[i].onNode );
		if (!pPyNode || !PyModelNode::Check( pPyNode ))
		{
			PyErr_Clear();
			continue;
		}

		SmartPointer<PyModelNode> pNode( (PyModelNode*)pPyNode, true );

		if (!pNode->attach( PyAttachmentPtr( pa[i].pSystem ) ))
		{
			ERROR_MSG( "ScriptPlayer::updateWeatherParticleSystems: Could not attach "
				"weather particle system to new Player Model\n" );

			PyErr_Clear();
			continue;
		}

		AttachedPS aps;
		aps.pNode_ = pNode;
		aps.pAttachment_ = pa[i].pSystem;
		attachedPSs_.push_back( aps );
	}
}


/*~ function BigWorld player
 *  This sets the player entity, or returns the current player entity if the 
 *  entity argument is not provided. The BigWorld engine assumes that there is 
 *  no more than one player entity at any time. Changing whether an entity is 
 *  the current player entity involves changing whether it is an instance of 
 *  its normal class, or its player class. This is a class whose name equals 
 *  the entity's current class name, prefixed with the word "Player". As an 
 *  example, the player class for Biped would be PlayerBiped.
 *  
 *
 *  The following occurs if a new entity is specified:
 *  
 *  The onBecomeNonPlayer function is called on the old player entity.
 *  
 *  The old player entity becomes an instance of its original class, rather 
 *  than its player class.
 *  
 *  The reference to the current player entity is set to None.
 *  
 *  A player class for the new player entity is sought out.
 *  If the player class for the new player entity is found, then the entity 
 *  becomes an instance of this class. Otherwise, the function immediately 
 *  returns None.
 *  
 *  The onBecomePlayer function is called on the new player entity.
 *  
 *  The reference to the current player entity is set the new player entity.
 *  
 *  @param entity An optional entity. If supplied, this entity becomes the 
 *  current player.
 *  @return If the entity argument is supplied then this is None, otherwise 
 *  it's the current player entity.
 */
/**
 *	Returns the player entity.
 */
static PyObject * player( PyEntityPtr pNewPlayer )
{
	BW_GUARD;
	// get the player if desired
	if (pNewPlayer == NULL)
	{
		Entity * pPlayer = ScriptPlayer::entity();

		if (pPlayer == NULL)
		{
			Py_RETURN_NONE;
		}
		return pPlayer->pPyEntity().newRef();
	}

	// otherwise switch player control to this entity
	if (!pNewPlayer->pEntity())
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.player: Expected a non-destroyed entity\n" );
		return NULL;
	}

	ScriptPlayer::instance().setPlayer( pNewPlayer->pEntity().get() );
	Py_RETURN_NONE;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, player,
	OPTARG( PyEntityPtr, PyEntityPtr(), END ), BigWorld )



/*~ class BigWorld.PlayerMProv
 *
 *	This class inherits from MatrixProvider, and can be used to access the 
 *	position of the current player entity.  Multiple PlayerMProvs can be
 *	created, but they will all have the same value.
 *
 *	There are no attributes or functions available on a PlayerMProv, but it
 *	can be used to construct a PyMatrix from which the position can be read.
 *
 *	A new PlayerMProv is created using BigWorld.PlayerMatrix function.
 */
/**
 *	Helper class to access the matrix of the current player position
 */
class PlayerMProv : public MatrixProvider
{
	Py_Header( PlayerMProv, MatrixProvider )

public:
	PlayerMProv() : MatrixProvider( false, &s_type_ ) {}

	virtual void matrix( Matrix & m ) const
	{
		BW_GUARD;
		Entity * pEntity = ScriptPlayer::entity();
		if (pEntity != NULL)
		{
			m = pEntity->fallbackTransform();
		}
		else
		{
			m.setIdentity();
		}
	}

	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PlayerMProv, END )
};


PY_TYPEOBJECT( PlayerMProv )

PY_BEGIN_METHODS( PlayerMProv )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PlayerMProv )
PY_END_ATTRIBUTES()

/*~ function BigWorld.PlayerMatrix
 *
 *	This function returns a new PlayerMProv which is a MatrixProvider
 *	that can be used to access the player entity's position.
 */
PY_FACTORY_NAMED( PlayerMProv, "PlayerMatrix", BigWorld )

/**
 *	Get the matrix provider for the player's transform
 */
/* static */ MatrixProviderPtr ScriptPlayer::camTarget()
{
	BW_GUARD;
	return MatrixProviderPtr( new PlayerMProv(), true );
}


/*~ function BigWorld.playerDead
 *
 *	This method sets whether or not the player is dead.  This information is
 *	used by the snow system to stop snow falling if the player is dead.
 *
 *	@param isDead	an integer as boolean. This is 0 if the player is alive, otherwise
 *	it is non-zero.
 */
/**
 *	Set whether or not the player is dead
 */
static void playerDead( bool isDead )
{
	BW_GUARD;
	Entity * pE = ScriptPlayer::entity();
	if (pE == NULL || !pE->pSpace()) return;
	pE->pSpace()->enviro().playerDead( isDead );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, playerDead, ARG( bool, END ), BigWorld )


bool ScriptPlayer::drawPlayer( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (pEntity_ == NULL)
	{
		return false;
	}

	PyModel * pPrimaryModel = pEntity_->pPrimaryModel();

	if (pPrimaryModel == NULL)
	{
		return false;
	}

	const Vector3 & loc = pEntity_->fallbackTransform().applyToOrigin();
	BoundingBox bbQuery( loc, loc );

	Moo::LightContainerPtr allLights;
	Moo::LightContainerPtr specularLights;
	entity()->pSpace()->lightsInArea( bbQuery, &allLights );

	Moo::rc().lightContainer( allLights );
	Moo::rc().specularLightContainer( specularLights );

	pPrimaryModel->draw( drawContext, pEntity_->fallbackTransform() );

	return true;
}

BW_END_NAMESPACE

// player.cpp
