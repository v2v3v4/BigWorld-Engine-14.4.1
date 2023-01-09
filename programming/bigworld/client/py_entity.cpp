#include "pch.hpp"

#include "py_entity.hpp"

#include "camera_app.hpp"
#include "connection_control.hpp"
#include "entity.hpp"
#include "entity_manager.hpp"
#include "physics.hpp"
#include "py_aoi_entities.hpp"
#include "py_filter.hpp"
#include "py_server.hpp"
#include "py_dumb_filter.hpp"

#include "connection_model/bw_connection.hpp"

#include "space/client_space.hpp"

#if ENABLE_CONSOLES
#include "romp/console_manager.hpp"
#include "romp/xconsole.hpp"
#endif // ENABLE_CONSOLES

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )

BW_BEGIN_NAMESPACE

typedef WeakPyPtr<PyEntity> PyEntityWPtr;

// -----------------------------------------------------------------------------
// Section: EntityMProv
// -----------------------------------------------------------------------------

/**
 *	Entity matrix provider class
 */
class EntityMProv : public MatrixProvider
{
	Py_Header( EntityMProv, MatrixProvider )

public:
	EntityMProv( PyEntity * pPyEntity, PyTypeObject * pType = &s_type_ ) :
		MatrixProvider( false, pType ),
		wpPyEntity_( pPyEntity ),
		notModel_( false )
	{
		BW_GUARD;
	}

	~EntityMProv()
	{
		BW_GUARD;
	}

	virtual void matrix( Matrix & m ) const
	{
		BW_GUARD;
		if (!wpPyEntity_ || !wpPyEntity_->pEntity())
		{
			m.setIdentity();
		}
		else if (!notModel_)
		{
			m = wpPyEntity_->pEntity()->fallbackTransform();
		}
		else
		{
			m.setRotate(	wpPyEntity_->pEntity()->direction().yaw,
							-wpPyEntity_->pEntity()->direction().pitch,
							wpPyEntity_->pEntity()->direction().roll );
			m.translation( wpPyEntity_->pEntity()->position() );
		}
	}

	PY_RW_ATTRIBUTE_DECLARE( notModel_, notModel )

private:
	PyEntityWPtr	wpPyEntity_;
	bool			notModel_;
};

/*~	class BigWorld.EntityMProv
 *
 *	This is a special MatrixProvider for an Entity.
 *	It is useful if the Entity and Model matrix differ.
 */
PY_TYPEOBJECT( EntityMProv )

PY_BEGIN_METHODS( EntityMProv )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( EntityMProv )

	/*~	attribute EntityMProv.notModel
	 *
	 *	If the Model and entity position are expected to be different throughout the course of their respective
	 *	lifetimes, then this attribute can be used to flag which should be regarded as the authoritative in-game
	 *	position.  If true, this MatrixProvider will track the Entity's matrix;  If false, it will always provide
	 *	the Model's.  It can be changed at any time to switch between the two and defaults to false, since the
	 *	Model is the visual representation of the Entity on the Client.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( notModel )

PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: PyEntity
// -----------------------------------------------------------------------------

PY_BASETYPEOBJECT( PyEntity )

PY_BEGIN_METHODS( PyEntity )
	PY_METHOD( prints )
	PY_METHOD( addModel )
	PY_METHOD( delModel )
	PY_METHOD( addTrap )
	PY_METHOD( delTrap )
	PY_METHOD( hasTargetCap )
	PY_METHOD( setInvisible )
	PY_METHOD( setPortalState )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEntity )

	/*~ attribute Entity.id
	 *	Unique Entity identifier, shared across Cell, Base and Client
	 *	@type Read-Only float
	 */
	PY_ATTRIBUTE( id )

	/*~ attribute Entity.isClientOnly
	 *	This property is True if it is a client-only entity.
	 *	@type Read-Only Boolean
	 */
	PY_ATTRIBUTE( isClientOnly )

	/**	attribute Entity.isDestroyed
	 *	This property will be True if the entity has been destroyed.
	 *	For example when an entity leaves the AoI. Accessing properties
	 *	other than 'Entity.isDestroyed' or 'Entity.id' will raise an
	 *	EntityIsDestroyedException.
	 *	@type Read-Only Boolean
	 */
	PY_ATTRIBUTE( isDestroyed )

	/*~ attribute Entity.position
	 *	Current position of Entity in world
	 *	@type Read-Only Vector3
	 */
	PY_ATTRIBUTE( position )

	/*~ attribute Entity.velocity
	 *	Current velocity of Entity in world, if the current filter
	 *	calculates it, or (0,0,0) otherwise.
	 *	@type Read-Only Vector3
	 */
	PY_ATTRIBUTE( velocity )

	/*~ attribute Entity.yaw
	 *	Current yaw of Entity
	 *	@type Read-Only float
	 */
	PY_ATTRIBUTE( yaw )

	/*~ attribute Entity.pitch
	 *	Current pitch of Entity
	 *	@type Read-Only float
	 */
	PY_ATTRIBUTE( pitch )

	/*~ attribute Entity.roll
	 *	Current roll of Entity
	 *	@type Read-Only float
	 */
	PY_ATTRIBUTE( roll )

	// TODO: Remove this.
	PY_ATTRIBUTE_ALIAS( cell, server )

	/*~ attribute Entity.cell
	 *	Communicates with Cell component of this Entity.
	 *	@type Read-Only PyServer
	 */
	PY_ATTRIBUTE( cell )

	/*~ attribute Entity.base
	 *	Communicates with Base component if Base is Proxy, otherwise this attribute is not available.
	 *	@type Read-Only PyBase
	 */
	PY_ATTRIBUTE( base )

	/*~ attribute Entity.model
	 *	Current Model visible to user
	 *	@type Read-Only PyModel
	 */
	PY_ATTRIBUTE( model )

	/*~ attribute Entity.models
	 *	List of PyModels attached to Entity
	 *	@type list of PyModels
	 */
	PY_ATTRIBUTE( models )

	/*~ attribute Entity.filter
	 *	Movement interpolation filter for this Entity.
	 *	@type Filter
	 */
	PY_ATTRIBUTE( filter )

	/*~ attribute Entity.isPoseVolatile
	 *	On initialisation, this attribute will be False. If the server starts
	 *	sending volatile position updates, then this value will become True.
	 *	Additionally, there is a callback when this occurs,
	 *	Entity.onPoseVolatile.
	 *
	 *	@type Read-Only bool
	 *
	 *	@see Entity.onPoseVolatile
	 */
	PY_ATTRIBUTE( isPoseVolatile )

	/*~ attribute Entity.targetCaps
	 *	Stores the current targeting capabilities for this Entity.  These will be compared against
	 *	another set of targeting capabilities to determine whether this Entity should be included
	 *	in a list of potential targets.
	 *	@type list of integers
	 */
	PY_ATTRIBUTE( targetCaps )

	/*~ attribute Entity.inWorld
	 *	True when Entity is in the world (ie, added to a BigWorld Space), false when it is not
	 *	@type Read-Only bool
	 */
	PY_ATTRIBUTE( inWorld )

	/*~ attribute Entity.spaceID
	 *	Stores ID of BigWorld Space in which this Entity resides
	 *	@type Read-Only int
	 */
	PY_ATTRIBUTE( spaceID )

	/*~ attribute Entity.vehicle
	 *	Stores the vehicle Entity to which this Entity is attached.  None if not on vehicle.
	 *	@type Read-Only Entity
	 */
	PY_ATTRIBUTE( vehicle )

	/*~ attribute Entity.physics
	 *	When this Entity becomes controlled, this attribute becomes available to allow a Physics
	 *	object to be attached to this Entity.  The physics attribute is not available if this
	 *	Entity is not controlled.  See BigWorld.controlEntity().
	 *
	 *  Assigning an int representing one of the supported physics styles to the
	 *  physics property of an entity creates and attaches a Physics object to
	 *  the entity.  The constants for the different physics styles are
	 *  accessible in the BigWorld module.
	 *
	 *	Current physics styles are as follows:
	 *
	 *		STANDARD_PHYSICS:	Suitable for player, other controlled animals, etc.
	 *		HOVER_PHYSICS:		Hovering vehicle physics.
	 *		CHASE_PHYSICS:		Causes controlled Entity to follow exact movements
	 *							of Entity being chased. Has no effect unless
	 *							Physics::chase() activated.
	 *		TURRET_PHYSICS:		Useful for controlled turrets, guns, etc.
	 *
	 *	Other custom physics can be integrated into BigWorld and assigned their own integer
	 *	creation flag.  Once set, the appropriate physics module will be created and assigned to
	 *	this attribute.  The appropriate Physics style can then be accessed through this attribute.
	 *
	 *	@type Write-Only int/Physics
	 */
	PY_ATTRIBUTE( physics )

	/*~ attribute Entity.matrix
	 *	Current matrix representation of this Entity in the world.
	 *	@type Read-Only EntityMProv
	 */
	PY_ATTRIBUTE( matrix )

	/*~	attribute Entity.targetFullBounds
	 *	If true, targeting uses the full bounding box of this Entity. This, however,
	 *	will allow the player to target an entity using the extreme corners of the
	 *	bounding box (which may be out in the middle of open space). Setting this to
	 *	false will shrink the bounding box used for testing, and will also only select
	 *	the entity if two points on the bounding box (one at the top and one at the
	 *	bottom) are visible.
	 *
	 *	See the watcher values "Client Settings/Entity Picker/Lower Pct." and
	 *	"Client Settings/Entity Picker/Sides Pct." for the shrink amount control
	 *	factors.
	 *
	 *	@type bool
	 */
	PY_ATTRIBUTE( targetFullBounds )

	/*~ attribute Entity.isPlayer
	 *	If true, this entity is considered to be the player, false otherwise.
	 *
	 *	@type Read-Only bool
	 */
	PY_ATTRIBUTE( isPlayer )

	 /*~ attribute Entity.isWitness
	 *	If true, this entity is considered to have an AoI.
	 *
	 *	In general, this typically is the player entity., however, for replay
	 *	connections, multiple entities in the space may have this attribute set
	 *	to True, which allows for the aoiEntities attribute to be accessed.
	 *
	 *	@type Read-Only bool
	 */
	PY_ATTRIBUTE( isWitness )

	/*~ attribute Entity.aoiEntities
	 *	If this entity is a witness, this is a mapping of the entities in its 
	 *	AoI. The mapping keys are the entity IDs, and the mapped values are 
	 *	the entity references.
	 */
	PY_ATTRIBUTE( aoiEntities )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyEntity )


///	Constructor
PyEntity::PyEntity( Entity & entity ) :
	PyObjectPlus( entity.type().pClass(), /* isInitialised */ true ),
	pEntity_( &entity ),
	auxModelsHolder_( entity.auxiliaryEmbodiments(), this, true ),
	cell_(),
	base_(),
	pyAoIEntities_(),
	pPyFilter_( new PyDumbFilter(), PyFilterPtr::FROM_NEW_REFERENCE )
{
	BW_GUARD;
}


/// Destructor
PyEntity::~PyEntity()
{
	BW_GUARD;
	// TODO: See end of onEntityDestroyed()
	pEntity_ = NULL;
}


/**
 *	This method returns the EntityID of the Entity we are or were
 *	associated with
 */
EntityID PyEntity::entityID() const
{
	BW_GUARD;
	MF_ASSERT( pEntity_ );

	return pEntity_->entityID();
}


/**
 *	This method returns whether or not our associated entity has been destroyed.
 *
 *	@return bool	True if our associated entity has been destroyed (i.e.
 *					onEntityDestroyed() has been called)
 */
bool PyEntity::isDestroyed() const
{
	BW_GUARD;

	return pEntity_ == NULL || pEntity_->isDestroyed();
}


/**
 * This method is called when the entity is destroyed.
 */
void PyEntity::onEntityDestroyed()
{
	BW_GUARD;

	// Sanity check to avoid calling this twice
	MF_ASSERT( pEntity_ != NULL );

	if (cell_ != NULL)
	{
		cell_ = NULL;
	}

	if (base_ != NULL)
	{
		base_ = NULL;
	}

	// Clear out the __dict__ entries to release any objects that might be
	// referring back to this entity.
	PyObject * pDict = PyObject_GetAttrString( this, "__dict__" );
	if (pDict)
	{
		PyDict_Clear( pDict );
		Py_DECREF( pDict );
	}
	else
	{
		PyErr_Clear();
	}

	// TODO: We can't do this here because auxModelsHolder_ may still be holding
	// us and reference pEntity_'s auxiliaryEmbodiments()
	// pEntity_ = NULL;

	MF_ASSERT( this->isDestroyed() );
}


/**
 *	This method applies our PyFilter to our entity, setting up its
 *	Filter instance.
 */
void PyEntity::setFilterOnEntity()
{
	MF_ASSERT( !pEntity_->isDestroyed() );
	MF_ASSERT( pEntity_->isInAoI() );
	pEntity_->pFilter( pPyFilter_->getFilter() );
}


bool PyEntity::initCellEntityFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( pEntity_ != NULL );

	bool isReplay = ConnectionControl::instance().pReplayConnection() != NULL;

	EntityType::StreamContents contents = isReplay ?
		EntityType::TAGGED_CELL_PLAYER_DATA :
		EntityType::TAGGED_CELL_ENTITY_DATA;

	PyObject * pInitDict = pEntity_->type().newDictionary( data, contents );

	// Make sure that every property is set. If we are at a low LoD level, not
	// all properties will be set. Set them to their default value.
	for (uint i=0; i < pEntity_->type().description().propertyCount(); i++)
	{
		DataDescription * pDD = pEntity_->type().description().property(i);

		if (pDD->isOtherClientData())
		{
			if (PyDict_GetItemString( pInitDict,
				const_cast<char*>( pDD->name().c_str() ) ) == NULL)
			{
				PyObjectPtr pValue = pDD->pInitialValue();

				PyDict_SetItemString( pInitDict,
					const_cast<char*>( pDD->name().c_str() ), pValue.getObject() );
			}
		}
	}

	// Update our dictionary with the one passed in
	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if (pInitDict == NULL || pCurrDict == NULL ||
		PyDict_Update( pCurrDict, pInitDict ) < 0)
	{
		PY_ERROR_CHECK();
		return false;
	}

	Py_XDECREF( pInitDict );
	Py_XDECREF( pCurrDict );

	// Now that everything is in working order, create the script object
	PyEntityPtr pThis( this );
	pThis.callMethod( "__init__", ScriptErrorPrint(),
		/* allowNullMethod */ true );

	return true;
}


bool PyEntity::initBasePlayerFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( pEntity_ != NULL );

	PyObject * pInitDict = pEntity_->type().newDictionary( data,
		EntityType::BASE_PLAYER_DATA );

	// Make sure that every property is set. If we are at a low LoD level, not
	// all properties will be set. Set them to their default value.
	for (uint i=0; i < pEntity_->type().description().propertyCount(); i++)
	{
		DataDescription * pDD = pEntity_->type().description().property(i);

		if (pDD->isOtherClientData())
		{
			if (PyDict_GetItemString( pInitDict,
				const_cast<char*>( pDD->name().c_str() ) ) == NULL)
			{
				PyObjectPtr pValue = pDD->pInitialValue();

				PyDict_SetItemString( pInitDict,
					const_cast<char*>( pDD->name().c_str() ), pValue.getObject() );
			}
		}
	}


	// Update our dictionary with the one passed in
	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if (pInitDict == NULL || pCurrDict == NULL ||
		PyDict_Update( pCurrDict, pInitDict ) < 0)
	{
		PY_ERROR_CHECK();
		return false;
	}

	Py_XDECREF( pInitDict );
	Py_XDECREF( pCurrDict );

	// Now that everything is in working order, create the script object
	PyEntityPtr pThis( this );
	pThis.callMethod( "__init__", ScriptErrorPrint(),
		/* allowNullMethod */ true );

	return true;
}


/**
 *	This method read in the cell data that is associated with the player.
 */
bool PyEntity::initCellPlayerFromStream( BinaryIStream & stream )
{
	BW_GUARD;

	MF_ASSERT( pEntity_ != NULL );

	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if (pCurrDict == NULL)
	{
		ERROR_MSG( "PyEntity::readCellPlayerData: Could not get __dict__\n" );
		PyErr_PrintEx(0);
		return false;
	}

	PyObject * pCellData = pEntity_->type().newDictionary( stream,
		EntityType::CELL_PLAYER_DATA );

	PyDict_Update( pCurrDict, pCellData );

	Py_DECREF( pCellData );
	Py_DECREF( pCurrDict );
	return true;
}


// -----------------------------------------------------------------------------
// Section: Script stuff
// -----------------------------------------------------------------------------
/*~ callback Entity.onBecomePlayer
 *
 *	This callback method is called in response to a call to BigWorld.player()
 *	It signals that an entity has become the player, and its script has become
 *	an instance of the player class, instead of its original avatar class.
 *
 *	@see BigWorld.player
 *	@see Entity.onBecomeNonPlayer
 */
/**
 *	This method changes the Python class of this object to be the player class.
 */
bool PyEntity::makePlayer()
{
	BW_GUARD;

	MF_ASSERT( pEntity_ != NULL );

	IF_NOT_MF_ASSERT_DEV( this->ob_type == pEntity_->type().pClass() )
	{
		// most likely already a player - presume we've already been called
		return true;
	}

	if (pEntity_->type().pPlayerClass() == NULL)
	{
		ERROR_MSG( "PyEntity::makePlayer: "
			"Entity of type %s cannot be made a player.\n",
			this->typeName() );
		return false;
	}

	this->setClass( pEntity_->type().pPlayerClass() );
	Script::call( PyObject_GetAttrString( this, "onBecomePlayer" ),
		PyTuple_New(0), "ScriptPlayer::onBecomePlayer: ", true );

	return true;
}


/*~ callback Entity.onBecomeNonPlayer
 *
 *	This callback method is called in response to a call to BigWorld.player()
 *	It signals that an entity is no longer the player, and its script is just
 *	about to become an instance its original class, instead of the player
 *	avatar class.
 *
 *	@see BigWorld.player
 *	@see Entity.onBecomePlayer
 */
/**
 *	This method changes the Python class of this object to be the non-player
 *	class.
 */
bool PyEntity::makeNonPlayer()
{
	BW_GUARD;

	MF_ASSERT( pEntity_ != NULL );

	IF_NOT_MF_ASSERT_DEV( this->ob_type == pEntity_->type().pPlayerClass() )
	{
		// most likely no longer a player - presume we've already been called
		return true;
	}

	Script::call( PyObject_GetAttrString( this, "onBecomeNonPlayer" ),
		PyTuple_New(0), "ScriptPlayer::onBecomeNonPlayer: ", true );
	this->setClass( pEntity_->type().pClass() );

	return true;
}


/*~ callback Entity.reload
 *
 *	This callback method is called in response to reloading scripts.
 *	It allows the entity to reload itself, and attach itself to its
 *	associated objects again.
 */
/**
 *	Method to swizzle instances of pOldClass into pNewClass when
 *	reloading script.
 */
void PyEntity::swizzleClass( PyTypeObject * pOldClass, PyTypeObject * pNewClass )
{
	BW_GUARD;
	if (this->ob_type == pOldClass)
	{
		this->setClass( pNewClass );

		Script::call( PyObject_GetAttrString( this, "reload" ), PyTuple_New(0),
			"PyEntity::swizzleClass (reload): ", true );
	}
}

/*~ function Entity.prints
*
*	This function writes the Entity's ID, followed by the given string to the BigWorld console
*
*	@param	string			(string)			The string to display in the console
*
*	@return					(none)
*/
/**
 * This method prints the message appended to the entity's id
 */
bool PyEntity::prints( BW::string msg )
{
	BW_GUARD;
	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method prints in %s is no longer available as its "
			"entity no longer exists on this client.", this->typeName() );
		return false;
	}

	char fullMessage[BUFSIZ];
	bw_snprintf( fullMessage, sizeof(fullMessage), "%d> ",
		pEntity_->entityID() );

	char * restMessage = fullMessage + strlen(fullMessage);
	strncpy( restMessage, msg.c_str(), BUFSIZ-16 );
	restMessage[ BUFSIZ-16 ] = '\0';

	strcat( fullMessage, "\n" );

	INFO_MSG( "%s", fullMessage );
#if ENABLE_CONSOLES
	ConsoleManager::instance().find( "Python" )->print( fullMessage );
#endif // ENABLE_CONSOLES

	return true;
}


/*~ function Entity.addModel
*
*	This function adds a PyModel to this Entity's list of usable Models
*
*	@param	model			(PyModel)			The PyModel to add to the list
*
*	@return					(none)
*/
/**
 *	This method forwards to our Entity unless it is destroyed
 */
bool PyEntity::addModel( IEntityEmbodimentPtr pCA )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method addModel in %s is no longer available as its "
				"entity no longer exists on this client.", this->typeName() );
		return false;
	}

	pEntity_->addModel( pCA );
	return true;
}


/*~ function Entity.delModel
*
*	This function removes a PyModel to this Entity's list of usable Models
*
*	@param	model			(PyModel)			The PyModel to remove from the list
*
*	@return					(none)
*/
/**
 *	This method forwards to our Entity unless it is destroyed
 */
bool PyEntity::delModel( PyObjectPtr pEmbodimentPyObject )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method delModel in %s is no longer available as its "
				"entity no longer exists on this client.", this->typeName() );
		return false;
	}

	return pEntity_->delModel( pEmbodimentPyObject );
}


/*~ function Entity.addTrap
*
*	This function adds a sphere of awareness around this Entity, invoking a callback method whenever the
*	contents of the Trap changes.  The callback method is provided upon creation of the Trap and should have
*	the following prototype;
*
*		def trapCallback( self, entitiesInTrap ):
*
*	The "entitiesInTrap" argument is a list containing references to the entities contained inside the trap.
*
*	BigWorld.addPot is a cheaper method that only works for players.
*
*	@see BigWorld.addPot
*
*	@param	radius			(float)				The radius of the sphere around this Entity
*	@param	function		(callable)			The callback function
*
*	@return					(integer)			The ID of the newly created Trap
*/
/**
 *	This method forwards to our Entity unless it is destroyed
 */
PyObject * PyEntity::addTrap( float radius, ScriptObject callback )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method addTrap in %s is no longer available as its "
				"entity no longer exists on this client.", this->typeName() );
		return NULL;
	}

	return pEntity_->addTrap( radius, callback );
}


/*~ function Entity.delTrap
*
*	This function removes a Trap (sphere of Entity awareness) from around the player.
*
*	@param	trapID			(float)				The ID of the Trap to remove
*
*	@return					(none)
*/
/**
 *	This method forwards to our Entity unless it is destroyed
 */
bool PyEntity::delTrap( int trapID )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method delTrap in %s is no longer available as its "
				"entity no longer exists on this client.", this->typeName() );
		return false;
	}

	return pEntity_->delTrap( trapID );
}


/*~ function Entity.hasTargetCap
*
*	This function checks whether this Entity has the given target capability.
*
*	@param	targetCap		(int)				The targeting capability to check against
*
*	@return					(bool)				Returns true if capability matches, false otherwise
*/
/**
 *	This method forwards to our Entity unless it is destroyed
 */
PyObject * PyEntity::hasTargetCap( uint i ) const
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method hasTargetCap in %s is no longer available as "
				"its entity no longer exists on this client.", this->typeName() );
		return NULL;
	}

	bool result = pEntity_->hasTargetCap( i );

	return PyBool_FromLong( result );
}


/*~ function Entity.setInvisible
 *
 *	This function fades the visibility of the player in and out. It can only be
 *	called on a Player entity.
 *
 *	Raises a TypeError if this method is not called on the player entity.
 *
 *	@param	invisible	(bool)	True to make Entity invisible, False to
 *								make it reappear
 */
/**
 *	This method forwards to our Entity unless it is destroyed
 */
bool PyEntity::setInvisible( bool invis )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method setInvisible in %s is no longer available as "
				"its entity no longer exists on this client.", this->typeName() );
		return false;
	}

	return pEntity_->setInvisible( invis );
}

/*~ function Entity.setPortalState
 *  @components{ client }
 *  The setPortalState function allows an entity to control the state of a
 *	portal. If the portal is not permissive, it is added to the collision scene.
 *
 *	The position of the entity must be within 1 metre of the portal that is
 *	to be configured, otherwise the portal will probably not be found. No
 *	other portal may be closer.
 *
 *	The cell entity has an identical method. This also adds an object into the
 *	collision scene and disables navigation through the doorway when the portal
 *	is closed.
 *
 *	@param permissive	Whether the portal is open or not.
 *  @param triFlags Optional argument that sets the flags on the portal's
 *		triangles as returned from collision tests.
 */
/**
 *	This method forwards to our Entity unless it is destroyed
 */
bool PyEntity::setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the method setPortalState in %s is no longer available as "
				"its entity no longer exists on this client.", this->typeName() );
		return false;
	}

	pEntity_->setPortalState( isOpen, collisionFlags );

	return true;
}


/**
 *	This method returns the entities in the AoI of this entity, if it is a 
 *	player.
 */
PyObject * PyEntity::aoiEntities()
{
	BW_GUARD;

	if (!this->pEntity()->pBWConnection()->isWitness(
			this->pEntity()->entityID() ))
	{
		pyAoIEntities_ = NULL;
		return ScriptDict::create().newRef();
	}
	
	if (!pyAoIEntities_)
	{
		pyAoIEntities_ = ScriptObject( 
			new PyAoIEntities( this->pEntity().get() ) );
	}

	return pyAoIEntities_.newRef();
}


/* Macros for accessing simple attributes, raising an exception if destroyed */
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyEntity::

#define PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( MEMBER, NAME )				\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
{																			\
	if (this->isDestroyed())												\
	{																		\
		PyErr_Format(														\
			EntityManager::instance().entityIsDestroyedExceptionType(),		\
			"Sorry, the attribute " #NAME " in %s is no longer available "	\
				"as its entity no longer exists on this client.",			\
			this->typeName() );												\
		return NULL;														\
	}																		\
	return Script::getReadOnlyData( MEMBER );								\
}

#define PY_READABLE_ATTRIBUTE_GET_IF_NOT_DESTROYED( MEMBER, NAME )			\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
{																			\
	if (this->isDestroyed())												\
	{																		\
		PyErr_Format(														\
			EntityManager::instance().entityIsDestroyedExceptionType(),		\
			"Sorry, the attribute " #NAME " in %s is no longer available "	\
				"as its entity no longer exists on this client.",			\
			this->typeName() );												\
		return NULL;														\
	}																		\
	return Script::getData( MEMBER );										\
}

#define PY_WRITABLE_ATTRIBUTE_SET_IF_NOT_DESTROYED( MEMBER, NAME )			\
	int PY_ATTR_SCOPE pySet_##NAME( PyObject * value )						\
{																			\
	if (this->isDestroyed())												\
	{																		\
		PyErr_Format(														\
			EntityManager::instance().entityIsDestroyedExceptionType(),		\
			"Sorry, the attribute " #NAME " in %s is no longer available "	\
				"as its entity no longer exists on this client.",			\
			this->typeName() );												\
		return NULL;														\
	}																		\
	return Script::setData( value, MEMBER, #NAME );							\
}

#define PY_RO_PENTITY_ATTRIBUTE( MEMBER, NAME )								\
	PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( pEntity_->MEMBER, NAME )			\
	PY_RO_ATTRIBUTE_SET( NAME )												\

#define PY_RW_PENTITY_ATTRIBUTE( MEMBER, NAME )								\
	PY_READABLE_ATTRIBUTE_GET_IF_NOT_DESTROYED( pEntity_->MEMBER, NAME )	\
	PY_WRITABLE_ATTRIBUTE_SET_IF_NOT_DESTROYED( pEntity_->MEMBER, NAME )	\


PY_RO_PENTITY_ATTRIBUTE( isPlayer(), isPlayer )
PY_RO_PENTITY_ATTRIBUTE( isWitness(), isWitness )
PY_RO_PENTITY_ATTRIBUTE( pPyEntity()->aoiEntities(), aoiEntities )
PY_RO_PENTITY_ATTRIBUTE( isLocalEntity(), isClientOnly )
PY_RO_PENTITY_ATTRIBUTE( position(), position )
PY_RO_PENTITY_ATTRIBUTE( localVelocity(), velocity )
PY_RO_PENTITY_ATTRIBUTE( direction().yaw, yaw )
PY_RO_PENTITY_ATTRIBUTE( direction().pitch, pitch )
PY_RO_PENTITY_ATTRIBUTE( direction().roll, roll )

/**
 *	Get method for the 'cell' attribute
 */
PyObject * PyEntity::pyGet_cell()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute cell in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	if (cell_ == NULL)
	{
		cell_ = ScriptObject(
			new PyServer( this, pEntity_->type().description().cell(), false ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return Script::getData( cell_ );
}

PY_RO_ATTRIBUTE_SET( cell )


/**
 *	Get method for the 'base' attribute
 */
PyObject * PyEntity::pyGet_base()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute base in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	if (!pEntity_->isPlayer())
	{
		PyErr_SetString( PyExc_TypeError,
			"Entity.base is only available on the connected entity" );
		return NULL;
	}

	if (base_ == NULL)
	{
		base_ = ScriptObject(
			new PyServer( this, pEntity_->type().description().base(), true ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return Script::getData( base_ );
}

PY_RO_ATTRIBUTE_SET( base )

/**
 *	Specialised get method for the 'model' attribute
 */
PyObject * PyEntity::pyGet_model()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute model in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	IEntityEmbodiment * pEmbodiment = pEntity_->pPrimaryEmbodiment();

	if (pEmbodiment == NULL)
	{
		Py_RETURN_NONE;
	}

	return Script::getData( pEmbodiment->scriptObject() );
}


/**
 *	Specialised set method for the 'model' attribute
 */
int PyEntity::pySet_model( PyObject * value )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute model in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return -1;
	}

	IEntityEmbodimentPtr pNewEmbodiment;
	int ret = Script::setData( value, pNewEmbodiment, "Entity.model" );
	if (ret == 0)
	{
		pEntity_->pPrimaryEmbodiment( pNewEmbodiment );
	}
	return ret;
}


PY_READABLE_ATTRIBUTE_GET_IF_NOT_DESTROYED( auxModelsHolder_, models )
PY_WRITABLE_ATTRIBUTE_SET_IF_NOT_DESTROYED( auxModelsHolder_, models )

PY_READABLE_ATTRIBUTE_GET_IF_NOT_DESTROYED( pPyFilter_, filter )

/**
 *	Specialised set method for the 'filter' attribute
 */
int PyEntity::pySet_filter( PyObject * value )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute filter in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return -1;
	}

	// make sure it's a filter
	if (!PyFilter::Check( value ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Entity.filter must be set to a Filter" );
		return -1;
	}

	PyFilterPtr pNewFilter;

	if (value == Py_None)
	{
		// We don't support clearing the filter, so make it a DumbFilter
		pNewFilter =
			PyFilterPtr( new PyDumbFilter(), PyFilterPtr::FROM_NEW_REFERENCE );
	}
	else
	{
		pNewFilter = PyFilterPtr(
			ScriptObject( value, ScriptObject::FROM_BORROWED_REFERENCE ) );
	}

	if (pNewFilter == pPyFilter_)
	{
		return 0;
	}

	if (pNewFilter->isAttached())
	{
		PyErr_SetString( PyExc_TypeError,
			"Entity.filter must be set to an unattached Filter" );
		return -1;
	}

	if (pPyFilter_ != NULL)
	{
		pPyFilter_->isAttached( false );
	}

	pPyFilter_ = pNewFilter;

	pPyFilter_->isAttached( true );

	if (pEntity_->isInAoI())
	{
		this->setFilterOnEntity();
	}

	return 0;
}

PY_RO_PENTITY_ATTRIBUTE( isReceivingVolatileUpdates(), isPoseVolatile )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( pEntity_->targetCaps(), targetCaps )

/**
 *	Specialised set method for the 'targetCaps' attribute
 */
int PyEntity::pySet_targetCaps( PyObject * value )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute NAME in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return -1;
	}

	int ret = Script::setData( value, pEntity_->targetCaps(), "targetCaps" );
	if (ret == 0 && pEntity_->isATarget())
	{
		CameraApp::instance().entityPicker().check();
	}
	return ret;
}


PY_RO_PENTITY_ATTRIBUTE( isInWorld(), inWorld )

PY_RO_PENTITY_ATTRIBUTE( spaceID(), spaceID )

/**
 *	Specialised get method for the 'vehicle' attribute
 */
PyObject * PyEntity::pyGet_vehicle()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute physics in %s is no longer available "
			"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	Entity * pVehicle = static_cast< Entity * >( pEntity_->pVehicle() );

	if (pEntity_->pVehicle() == NULL || pEntity_->pVehicle()->isDestroyed())
	{
		Py_RETURN_NONE;
	}

	return Script::getData( pVehicle->pPyEntity() );
}

PY_RO_ATTRIBUTE_SET( vehicle )


/**
 *	Specialised get method for the 'physics' attribute
 */
PyObject * PyEntity::pyGet_physics()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute physics in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	if (!pEntity_->isControlled())
	{
		PyErr_SetString( PyExc_AttributeError,
			"Only controlled entities have a 'physics' attribute" );
		return NULL;
	}

	return Script::getData( pEntity_->pPhysics() );
}

/**
 *	Specialised set method for the 'physics' attribute
 */
int PyEntity::pySet_physics( PyObject * value )
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute physics in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return -1;
	}

	if (!pEntity_->isControlled())
	{
		PyErr_SetString( PyExc_AttributeError,
			"Only controlled entities have a 'physics' attribute" );
		return -1;
	}

	if( value == Py_None )
	{
		pEntity_->setPhysics( NULL );
		return 0;
	}

	int style;
	int ret = Script::setData( value, style, "physics" );

	if (ret == 0)
	{
		pEntity_->setPhysics( &style );
	}

	return ret;
}


/**
 *	Specialised get method for the 'matrix' attribute
 */
PyObject * PyEntity::pyGet_matrix()
{
	BW_GUARD;

	if (this->isDestroyed())
	{
		PyErr_Format(
			EntityManager::instance().entityIsDestroyedExceptionType(),
			"Sorry, the attribute matrix in %s is no longer available "
				"as its entity no longer exists on this client.",
			this->typeName() );
		return NULL;
	}

	return new EntityMProv( this );
}

PY_RO_ATTRIBUTE_SET( matrix )

PY_RW_PENTITY_ATTRIBUTE( targetFullBounds(), targetFullBounds )


#undef PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED
#undef PY_READABLE_ATTRIBUTE_GET_IF_NOT_DESTROYED
#undef PY_WRITABLE_ATTRIBUTE_SET_IF_NOT_DESTROYED
#undef PY_RO_PENTITY_ATTRIBUTE
#undef PY_RW_PENTITY_ATTRIBUTE
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE

/**
 *	This method returns an attribute associated with this object.
 */
ScriptObject PyEntity::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	// Nasty hack, see getChunkSpace in duplo/chunk_embodiment.cpp
	// See BWT-19950 for more details.
	if (!strcmp( attrObj.c_str(), ".space" ) && pEntity_ != NULL)
	{
		return ScriptObject::createFrom( (intptr)(pEntity_->pSpace().get()) );
	}

	// Do the default then
	return PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	This method sets an attribute associated with this object.
 */
bool PyEntity::pySetAttribute( const ScriptString & attrObj, 
	const ScriptObject & value )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();
	if (pEntity_ != NULL)
	{
		DataDescription * pProperty = pEntity_->type().description()
			.findProperty(attr);
		if (pProperty != NULL &&
			pProperty->isClientServerData() &&
			!pProperty->dataType()->isSameType( value ) )
		{
			PyErr_Format( PyExc_NameError, "Attempted to set attribute %s on %s "
				"to an invalid value.", attr,
				pEntity_->type().name().c_str() );
			return false;
		}
	}

	// Do the default then
	return PyObjectPlus::pySetAttribute( attrObj, value );
}


/**
 *	This method sets the Python class that is associated with this object.
 */
void PyEntity::setClass( PyTypeObject * pClass )
{
	BW_GUARD;
	MF_ASSERT_DEV( PyObject_IsSubclass( (PyObject *)pClass,
		(PyObject *)&PyEntity::s_type_ ) );
	MF_VERIFY( this->pySetAttribute( ScriptString::create( "__class__" ),
		ScriptObject( (PyObject *)pClass, 
			ScriptObject::FROM_BORROWED_REFERENCE ) ) );
}


/*~ function BigWorld controlEntity
 *	Sets whether the movement of an entity should be controlled locally by
 *	physics.
 *
 *	When shouldBeControlled is set to True, the entity's physics attribute
 *	becomes accessible. Each time this is called with its shouldBeControlled
 *	attribute as True, the entity's physics attribute is set to None. As this
 *	is also made accessible, it can then be set to a different value.
 *
 *	When shouldBeControlled is set to False, attempts to access the entity's
 *	physics object raise an AttributeError.
 *
 *	This function only works for client-side entities. The server decides
 *	who controls server-side entities. A TypeError is raised if the given
 *	entity is not a client-side entity.
 *
 *	@param entity The entity to control/uncontrol.
 *	@param beControlled Whether the entity should be controlled.
 */
/**
 *	Controls or uncontrols this entity
 */
static bool controlEntity( PyEntityPtr pPyEntity, bool shouldBeControlled )
{
	BW_GUARD;

	if (pPyEntity->isDestroyed())
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.controlEntity: "
			"This function does not work on destroyed entities" );
		return false;
	}

	EntityPtr pEntity = pPyEntity->pEntity();

	if (!pEntity->isLocalEntity())
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.controlEntity: "
			"This function only works on client-side entities" );
		return false;
	}

	// TODO: Test that pEntity is inWorld here. Or inAoI, I dunno exactly.
	// This should not be possible to fail, anyway.
	if (!pEntity->isInWorld())
	{
		PyErr_Format( PyExc_TypeError, "BigWorld.controlEntity: "
			"Entity %d could not be controlled (probably because "
			"it is not in the world)", pEntity->entityID() );
		return false;
	}

	if (pEntity->isControlled() != shouldBeControlled)
	{
		// TODO: Should call through BWEntities, like all local-entity methods.
		pEntity->triggerOnChangeControl( shouldBeControlled,
			/* isInitialising */ false );
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, controlEntity,
	NZARG( PyEntityPtr, ARG( bool, END ) ), BigWorld )


BW_END_NAMESPACE

/* py_entity.cpp */
