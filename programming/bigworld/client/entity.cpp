#include "pch.hpp"

#include "entity.hpp"

#include "action_matcher.hpp"
#include "app.hpp"
#include "camera_app.hpp"
#include "connection_control.hpp"
#include "dumb_filter.hpp"
#include "entity_manager.hpp"
#include "entity_picker.hpp"
#include "entity_type.hpp"
#include "filter.hpp"
#include "portal_state_changer.hpp"
#include "py_entity.hpp"
#include "script_player.hpp"

#include "common/simple_client_entity.hpp"

#include "camera/base_camera.hpp"

#include "space/client_space.hpp"
#include "space/space_manager.hpp"

#include "chunk/chunk_loader.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_terrain.hpp"

#include "connection/smart_server_connection.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entities.hpp"
#include "connection_model/entity_entry_blocker.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"

#include "duplo/pymodel.hpp"

#include "pyscript/pyobject_base.hpp"
#include "pyscript/pywatcher.hpp"
#include "pyscript/script.hpp"

#if ENABLE_CONSOLES
#include "romp/console_manager.hpp"
#endif // ENABLE_CONSOLES
#include "romp/enviro_minder.hpp"
#include "romp/py_resource_refs.hpp"
#include "romp/resource_ref.hpp"
#if ENABLE_CONSOLES
#include "romp/xconsole.hpp"
#endif // ENABLE_CONSOLES

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Entity
// -----------------------------------------------------------------------------

// Static variable initialisation
//ModelManager & Entity::modelManager_ = ModelManager::instance();
Entity::Census Entity::census_;

#define ENTITY_COUNTERS

#ifdef ENTITY_COUNTERS
static uint32 s_entitiesCurrent = 0;
static uint32 s_entitiesEver = 0;
typedef BW::map<BW::string, std::pair<uint32, uint32> > ETypes;
static const std::auto_ptr<ETypes> s_entitiesOfType( new ETypes );
#endif

///	Constructor
Entity::Entity( EntityType & type, BWConnection * pBWConnection ) :
	BWEntity( pBWConnection ),
	pPyEntity_(),
	loadingPrerequisites_( false ),
	type_( type ),
	primaryEmbodiment_( NULL ),
	auxiliaryEmbodiments_(),
	pSpace_( NULL ),
	pPhysics_( (Physics*)-1 ),
	tickAdvanced_( false ),
	nextTrapID_( 0 ),
	nextLeft_( NULL ),
	nextRight_( NULL ),
	nextUp_( NULL ),
	nextDown_( NULL ),
	prerequisitesOrder_( NULL ),
	invisible_( false ),
	visibilityTime_( 0 ),
	condition_( NULL ),
	targetFullBounds_( false )
{
	BW_GUARD;

	BW_STATIC_ASSERT( std::tr1::is_polymorphic< PyEntity >::value == false,
		PyEntity_is_virtual_but_uses_PyType_GenericAlloc );

	PyObject * pObject = PyType_GenericAlloc( type.pClass(), 0 );

	MF_ASSERT( pObject != NULL );

	pPyEntity_ = PyEntityPtr( new (pObject) PyEntity( *this ),
		PyEntityPtr::FROM_NEW_REFERENCE );

	MF_ASSERT( pPyEntity_ );

	census_.insert( this );

#ifdef ENTITY_COUNTERS
	if (s_entitiesEver == 0)
	{
		MF_WATCH( "PyObjCounts/Entities",
			s_entitiesCurrent,
			Watcher::WT_READ_WRITE,
			"Current number of entities constructed and not yet destructed." );

		MF_WATCH( "PyObjCounts/EntitiesEver",
			s_entitiesEver,
			Watcher::WT_READ_WRITE,
			"Total number of entities constructed." );
	}
	s_entitiesCurrent++;
	s_entitiesEver++;

	const BW::string & typeName = this->entityTypeName();
	ETypes::iterator it = s_entitiesOfType->find( typeName );
	if (it == s_entitiesOfType->end())
	{
		(*s_entitiesOfType)[typeName] = std::make_pair( 1,  1 );
		it = s_entitiesOfType->find( typeName );
#ifdef ENABLE_WATCHERS
		// Having the string built in the watch call broke VS2003 with watches disabled.
		BW::string watchName = BW::string("Entities/Counts/") + typeName;
		BW::string watchName2 = BW::string("Entities/Totals/") + typeName;
		MF_WATCH( watchName.c_str(), it->second.first );
		MF_WATCH( watchName2.c_str(), it->second.second );
#endif
	}
	else
	{
		it->second.first++;
		it->second.second++;
	}
#endif
}


/// Destructor
Entity::~Entity()
{
	BW_GUARD;
}


bool Entity::initBasePlayerFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );
	MF_ASSERT( type_.index() == this->entityTypeID() );

	return pPyEntity_->initBasePlayerFromStream( data );
}


bool Entity::initCellEntityFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );
	MF_ASSERT( type_.index() == this->entityTypeID() );

	return pPyEntity_->initCellEntityFromStream( data );
}


bool Entity::restorePlayerFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );

	TRACE_MSG( "Entity::restorePlayerFromStream: id = %d. vehicleID = %d.\n",
		this->entityID(), this->vehicleID() );

	return SimpleClientEntity::resetPropertiesEvent( pPyEntity_,
		this->type().description(), data );
}


// -----------------------------------------------------------------------------
// Section: Entry and exit
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
PrerequisitesOrder::PrerequisitesOrder(
		const BW::vector<BW::string> & resourceIDs,
		const ResourceRefs & resourceRefs ) :
	BackgroundTask( "PrerequisitesOrder" ),
	loaded_( false ),
	resourceIDs_( resourceIDs ),
	resourceRefs_( resourceRefs )
{
	BW_GUARD;	
}


/**
 *	This method loads all the prerequisites described in the given order.
 *
 *	Note: This method runs in a loading thread, and thus may not modify
 *	the entity at all (not even its reference count)
 */
void PrerequisitesOrder::doBackgroundTask( TaskManager & mgr )
{
	BW_GUARD;
	for (uint i = 0; i < resourceIDs_.size(); i++)
	{
		if (TaskManager::shouldAbortTask())
		{
			// Leaves loaded_ as false.
			return;
		}

		resourceRefs_.push_back( ResourceRef::getOrLoad( resourceIDs_[i] ) );
	}

	loaded_ = true;
}

/*~ callback Entity.prerequisites
 *
 *	This callback method is called before the entity enters the world.  It asks
 *	the entity for a list of resources that are required by the entity.  The
 *	entity is not added to the world until its prerequisite resources are
 *	loaded.
 *	The loading of prerequisite resources is done in a background thread,
 *	meaning that entities can thus enter the world without causing a loading
 *	pause to the main rendering thread.
 *
 *	The entity should return a tuple of resource IDs that represent its list
 *	of prerequisites.
 */
/**
 *	This function checks whether the prerequisites for this entity entering the
 *	world are satisfied. It retrieves this list from a Python callback
 *	Entity.prerequisites. If they are, it returns true. Otherwise, it starts
 *	the process of satisfying them (if not yet started) and returns false.
 */
bool Entity::checkPrerequisites()
{
	BW_GUARD;
	// if we already have some resources then we are fine
	if (!resourceRefs_.empty())
	{
		WARNING_MSG( "Entity::checkPrerequisites: "
			"Unexpectedly called for %d which already has resource refs\n",
			this->entityID() );
		loadingPrerequisites_ = false;
		condition_.release();
		return true;
	}

	// if there's one in progress see if it's done
	if (prerequisitesOrder_ != NULL)
	{
		if (!prerequisitesOrder_->loaded())
			return false;

		// keep the resources around
		resourceRefs_ = prerequisitesOrder_->resourceRefs();

		// it's done! now never call us again...
		prerequisitesOrder_ = NULL;
		loadingPrerequisites_ = false;
		condition_.release();
		return true;
	}

	MF_ASSERT( !this->isDestroyed() );

	// otherwise ask it for its prerequisites
	ScriptObject res = pPyEntity_.callMethod( "prerequisites",
		ScriptErrorRetain(), /* allowNullMethod */ true );

	// and find out who is there
	ResourceRefs presentPrereqs;
	BW::vector< BW::string > missingPrereqs;

	bool good = false;
	if (res != NULL && ScriptSequence::check( res ))
	{
		BW::string prereq;
		ScriptSequence seq( res );
		good = true;
		Py_ssize_t sz = seq.size();

		for (Py_ssize_t i = 0; i < sz; i++)
		{
			ScriptObject item = seq.getItem( i, ScriptErrorClear() );
			bool itemOk = (Script::setData( item.get(), prereq ) == 0);

			if (!itemOk)
			{
				good = false;
				break;
			}

			if (prereq.empty())
			{
				WARNING_MSG( "Entity::checkPrerequisites: "
					"Skipping empty prerequisite.\n" );
				continue;
			}

			ResourceRef rr = ResourceRef::getIfLoaded( prereq );
			if (rr)
			{
				presentPrereqs.push_back( rr );
			}
			else
			{
				missingPrereqs.push_back( prereq );
			}
		}
	}

	if (!good)
	{
		if (res != NULL)
		{
			// If not good but res returned something, then it failed the
			// above block, so wasn't a ScriptSequence of strings.
			PyErr_SetString( PyExc_TypeError, "prerequisites response: "
				"expected a sequence of strings" );
		}

		// If res is NULL, and no error has been set, then we just have no
		// prerequisites method. That's not an error.
		if (PyErr_Occurred())
		{
			PyErr_PrintEx(0);
			PyErr_Clear();

			presentPrereqs.clear();
			missingPrereqs.clear();
		}
	}

	// see if we have them all
	if (missingPrereqs.empty())
	{
		// keep the resources around
		resourceRefs_ = presentPrereqs;

		loadingPrerequisites_ = false;
		condition_.release();
		return true;
	}

	// otherwise start loading them
	prerequisitesOrder_ =
		new PrerequisitesOrder( missingPrereqs, presentPrereqs );

	FileIOTaskManager::instance().addBackgroundTask( prerequisitesOrder_ );

	loadingPrerequisites_ = true;
	return false;
}



/*~ callback Entity.onEnterWorld
 *
 *	This method is called when the entity enters into the player's AoI. On the
 *	client, this means that the entity has just been placed in the world.
 *	Most of the entity's initialisation code should be placed in this method
 *	instead of the entity's __init__ method because at the time that the __init__
 *	method is called the entity has not been placed in the world. Furthermore,
 *	the enterWorld method can be called multiple times for the same entity.
 *
 *	It has a PyResourceRefs object passed in as the first parameter.  This
 *	object holds onto the prerequisites that were loaded from the prerequisites
 *	method.  It is up to the script programmer to manage the lifetime of the
 *	prerequisites.
 *
 *	The leaveWorld method is called when the entity leaves the AoI.
 */
/*~ callback Entity.enterWorld
 *
 *	This method is called when the entity enters into the player's AoI. On the
 *	client, this means that the entity has just been placed in the world.
 *	Most of the entity's initialisation code should be placed in this method
 *	instead of the entity's __init__ method because at the time that the __init__
 *	method is called the entity has not been placed in the world. Furthermore,
 *	the enterWorld method can be called multiple times for the same entity
 *	because the entity is reused if it exits the player's AoI and re-enters
 *	it within a short period of time.
 *
 *	This callback has been deprecated, instead use onEnterWorld.
 *
 *	The leaveWorld method is called when the entity leaves the AoI.
 */
/*
 *	This method is called when the entity enters the client's world-space.
 *
 *	Note that at the time that an entity's script's __init__ function is
 *	called, the entity will not be in the world.
 */
void Entity::onEnterWorld()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( !pSpace_ )
	{
		this->onLeaveWorld();
	}

	IF_NOT_MF_ASSERT_DEV( this->spaceID() != NULL_SPACE_ID )
	{
		return;
	}

	this->onEnterSpace( /* transient */ false );

	BW::string desc ("Entity ");
	desc += this->entityTypeName();
	// Match server assumptions in RealEntity::enableWitness.
	// TODO: If our Player is controlled by another client before we
	// attached it it, we are now out-of-sync with the server.
	// Also, client-only entities always get Physics when entering the
	// world
	MF_ASSERT( (this->isPlayer() || this->isLocalEntity())
		== this->isControlled() );
	if (this->isPlayer() || this->isLocalEntity())
	{
		pPhysics_ = NULL;
	}

	//Entity now prefers onEnterWorld( self, prerequisites ) to
	//enterWorld( self )
	if (pPyEntity_.hasAttribute( "onEnterWorld" ))
	{
		ScriptArgs args = ScriptArgs::create( ScriptObject(
			new PyResourceRefs( resourceRefs_ ),
			ScriptObject::FROM_NEW_REFERENCE ) );
		pPyEntity_.callMethod( "onEnterWorld", args, ScriptErrorPrint() );
		//If onEnterWorld is used, then the C++ entity no longer manages
		//the lifetime of its prerequisites, it is up to the script (they
		//can hold onto the PyResourceRefs as long as they like.)
		resourceRefs_.clear();
	}
	else
	{
		pPyEntity_.callMethod( "enterWorld", ScriptErrorPrint(),
			/* allowNullMethod */ true );

		//If the deprecated enterWorld is used, then the C++ entity
		//manages the lifetime of its prerequisites, they exist as long
		//as the entity remains in the world.

		WARNING_MSG( "Entity::enterWorld: "
			"(%s) The use of Entity.enterWorld() is deprecated.  Please "
			"use onEnterWorld( prerequisites ) instead, where "
			"prerequisites is an instance of PyResourceRefs.\n",
			this->entityTypeName().c_str() );
	}

	if (this == ScriptPlayer::entity())
	{
		ScriptPlayer::instance().updateWeatherParticleSystems(
			pSpace_->enviro().playerAttachments() );
	}
}


/**
 *	This method is called when our position in the world is updated by a filter.
 */
void Entity::onPositionUpdated()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( -100000.f < this->localPosition().x &&
		this->localPosition().x < 100000.f &&
		-100000.f < this->localPosition().z &&
		this->localPosition().z < 100000.f )
	{
		ERROR_MSG( "Entity::onPositionUpdated: "
				"Tried to move entity %u to bad translation: ( %f, %f, %f )\n",
			this->entityID(), this->localPosition().x, this->localPosition().y,
			this->localPosition().z );
		return;
	}

	this->shuffle();

	// Filters must set velocity after setting position
	this->localVelocity( Vector3::zero() );
}


/*~ callback Entity.onLeaveSpace
 *
 *	This function is called when the entity changes space. This can occur, for
 *	example, when the player entity teleports between spaces.
 *
 *	The old space and its entities are still visible, but the entity's spaceID
 *	has already been updated to the new SpaceID.
 */
/*~ callback Entity.onEnterSpace
 *
 *	This function is called when the entity changes space. This can occur, for
 *	example, when the player entity teleports between spaces.
 *
 *	At this point, the entity has entered the new space, and the old space and
 *	its entities are no longer active.
 */
/*
 *	This method is called when the entity changes which space it's in. At
 *	this point, the space ID, vehicle and position have all been updated
 *	for the new space.
 */
void Entity::onChangeSpace()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( this->spaceID() != NULL_SPACE_ID )
	{
		return;
	}

	MF_ASSERT( this->isPlayer() );

	// TODO: This used to be called before we changed spaceID.
	// That might be okay, as long as the client is interested in
	// space, not spaceID? Or stuff done in onLeaveSpace()
	pPyEntity_.callMethod( "onLeaveSpace", ScriptErrorPrint(),
		/* allowNullMethod */ true );

	this->onLeaveSpace( /* transient */ true );

	MF_ASSERT_DEV( pSpace_ );

	pSpace_ = NULL;

	this->onEnterSpace( /* transient */ true );

	if (this->isATarget())
	{
		CameraApp::instance().entityPicker().check();
	}

	pPyEntity_.callMethod( "onEnterSpace", ScriptErrorPrint(),
		/* allowNullMethod */ true );
}


/*~ callback Entity.onLeaveWorld
 *
 *	This function is called when the entity leaves the player's AoI. On the
 *	client, this means that the entity is about to be removed from the world.
 *	The entity may not be destroyed immediately after this call. The same
 *	entity may be re-used if it re-enters the player's AoI a short time after
 *	leaving.
 */
/*~ callback Entity.leaveWorld
 *
 *	This function is called when the entity leaves the player's AoI. On the
 *	client, this means that the entity is about to be removed from the world.
 *	The entity may not be destroyed immediately after this call. The same
 *	entity may be re-used if it re-enters the player's AoI a short time after
 *	leaving.
 *
 *	This callback has been deprecated, instead use onLeaveWorld.
 */
/*
 *	This method is called when the entity leaves the client's world-space.
 */
void Entity::onLeaveWorld()
{
	BW_GUARD;
	// first tell the entity about it
	if (pPyEntity_.hasAttribute( "onLeaveWorld" ))
	{
		pPyEntity_.callMethod( "onLeaveWorld", ScriptErrorPrint() );
	}
	else
	{
		pPyEntity_.callMethod( "leaveWorld", ScriptErrorPrint(),
			/* allowNullMethod */ true );

		WARNING_MSG("(%s) The use of leaveWorld is deprecated.  Please use"
			" onLeaveWorld instead.\n", this->entityTypeName().c_str() );
	}

	// let the picker know we're no longer available
	if (this->isATarget())
	{
		CameraApp::instance().entityPicker().clear();
	}

	// release all our resource refs
	resourceRefs_.clear();

	this->onLeaveSpace( /* transient */ false );

	// clear all traps
	for (uint i=0; i < traps_.size(); i++)
	{
		Py_DECREF( traps_[i].function );
	}
	traps_.clear();

	// get rid of physics if we have any
	if (this->isControlled() && pPhysics_ != NULL)
	{
		MF_ASSERT( pPhysics_ != (Physics *)-1 );
		pPhysics_->disown();
		Py_DECREF( pPhysics_ );
		pPhysics_ = NULL;
	}

	pPhysics_ = (Physics*)-1;

	MF_ASSERT_DEV( pSpace_ );

	pSpace_ = NULL;
}


/**
 * This method clears the dictionary of the entity.
 */
void Entity::onDestroyed()
{
	BW_GUARD;

	MF_ASSERT( pPyEntity_ );

	this->abortLoadingPrerequisites();

	if (ScriptPlayer::instance().entity() == this)
	{
		ScriptPlayer::instance().setPlayer( NULL );
	}

	pPyEntity_.get()->onEntityDestroyed();

	pPyEntity_ = PyEntityPtr();

	MF_ASSERT( !pPyEntity_ );

	// Once destroyed, we're no longer interesting to the census or the
	// current entities collections.
	// This also avoids shutdown ordering problems if something holds a
	// reference to this instance after EntityTypes::fini is called.
	census_.erase( this );

#ifdef ENTITY_COUNTERS
	s_entitiesCurrent--;
	ETypes::iterator it = s_entitiesOfType->find( this->entityTypeName() );
	MF_ASSERT_DEV(it != s_entitiesOfType->end());
	it->second.first--;
#endif
}


/**
 *	This method aborts prerequisites loading, usually because we have
 *	left the AoI or similar but had never completed loading.
 */
void Entity::abortLoadingPrerequisites()
{
	if (!loadingPrerequisites_)
	{
		return;
	}

	MF_ASSERT( prerequisitesOrder_ != NULL );

	this->checkPrerequisites();

	if (this->loadingPrerequisites())
	{
		prerequisitesOrder_->stop();
		prerequisitesOrder_ = NULL;
		loadingPrerequisites_ = false;
	}
}


// -----------------------------------------------------------------------------
// Section: Regular timekeeping
// -----------------------------------------------------------------------------

typedef VectorNoDestructor<PyObject*> StolenPyObjects;
template <>
void PySTLSequenceHolder<StolenPyObjects>::commit()
{
	// this method doesn't compile 'coz there's no erase
	// using iterators in VectorNoDestructor
}

/**
 *	This function is called periodically by the EntityManager to give
 *	time to this entity.
 */
void Entity::tick( double timeNow, double timeLast )
{
	BW_GUARD;
	if (tickAdvanced_)
	{
		// tick is already called by a passenger
		// clear the flag.
		tickAdvanced_ = false;
		return;
	}

	// TODO: Do we need to do this still? This method is now just trap
	// checking and embodiment ticking
	Entity * pVehicle = static_cast< Entity * >( this->pVehicle() );
	if (pVehicle != NULL && pVehicle->isInWorld() && !pVehicle->tickCalled())
 	{ 
		pVehicle->tick( timeNow, timeLast );
		pVehicle->setTickAdvanced();
	}

	static DogWatch dwTick("Tick");
	ScopedDogWatch sdwTick( dwTick );

	Position3D trapPosition = this->position();

	EntityManager & rEntityManager = EntityManager::instance();

	// go through all our traps and call any that have gone off
	for (uint i = 0; i < traps_.size(); i++)
	{
		TrapRecord & tr = traps_[i];

		int	numIn = 0;

		static StolenPyObjects trapped;
		static PySTLSequenceHolder<StolenPyObjects>
			trappedHolder( trapped, NULL, false );
		trapped.clear();

		// find out how many are inside
		const BWEntities & entities = pBWConnection()->entities();

		for (BWEntities::const_iterator iEntity = entities.begin();
			iEntity != entities.end(); ++iEntity )
		{
			if (iEntity->second == this)
			{
				continue;
			}

			if (!iEntity->second->isInWorld())
			{
				continue;
			}

			float distSq = (iEntity->second->position() -
				trapPosition).lengthSquared();
			if (distSq <= tr.radiusSquared)
			{
				Entity * pEntity = static_cast< Entity * >(
					iEntity->second.get() );
				trapped.push_back( pEntity->pPyEntity().get() );
				numIn++;
			}

			// we do this loop around this way for pure convenience of
			//  coding. multiple traps are not expected (or very useful)
		}

		// is its state different?
		if (numIn != tr.num)
		{
			tr.num = numIn;

			Py_INCREF( tr.function );
			PyObject * pTuple = PyTuple_New(1);
			PyTuple_SetItem( pTuple, 0, Script::getData( trappedHolder ) );
			Script::call( tr.function, pTuple, "Entity::tick.trap: " );

			// Note: anything might have happened to traps_ now.
			// We attempt to continue nevertheless, but some might be missed
			// It should be rare for an entity to have a trap anyway...
			// and very rare for it to have more than one, so this is OK.
		}
	}

	// rev the motors of our models
	float dTime = float(timeNow - timeLast);

	if (primaryEmbodiment_)
	{
		primaryEmbodiment_->move( dTime );
	}

	for (EntityEmbodiments::iterator iter = auxiliaryEmbodiments_.begin();
		iter != auxiliaryEmbodiments_.end();
		iter++)
	{
		(*iter)->move( dTime );
	}
}


/**
 *	This method adds the given model to our list of models
 */
void Entity::addModel( const IEntityEmbodimentPtr& pCA )
{
	BW_GUARD;
	auxiliaryEmbodiments_.push_back( pCA );
	if (pSpace_)
	{
		pCA->enterSpace( pSpace_ );
	}
}


/**
 *	This method removes the given model from our list of models
 */
bool Entity::delModel( PyObjectPtr pEmbodimentPyObject )
{
	BW_GUARD;
	EntityEmbodiments::iterator iter;
	for (iter = auxiliaryEmbodiments_.begin();
		iter != auxiliaryEmbodiments_.end();
		iter++)
	{
		if ((*iter)->scriptObject() == pEmbodimentPyObject) 
		{
			break;
		}
	}

	if (iter == auxiliaryEmbodiments_.end())
	{
		PyErr_SetString( PyExc_ValueError, "Entity.delModel: "
			"Embodiment not added to this Entity." );
		return false;
	}

	if (pSpace_)
	{
		(*iter)->leaveSpace();
	}
	auxiliaryEmbodiments_.erase( iter );

	return true;
}


/**
 *	This method registers a trap function to be called when an entity
 *	(not us) enters it
 */
PyObject * Entity::addTrap( float radius, ScriptObject callback )
{
	BW_GUARD;

	if (!pSpace_)
	{
		PyErr_SetString( PyExc_EnvironmentError, "Entity.addTrap: "
			"Traps not allowed when not in world" );
		return NULL;
	}

	if (!callback.isCallable())
	{
		PyErr_SetString( PyExc_TypeError, "Entity.addTrap: "
			"callback object not callable" );
		return NULL;
	}

	TrapRecord	newTR = { nextTrapID_++, 0, radius*radius, callback.newRef() };
	traps_.push_back( newTR );

	return PyInt_FromLong( newTR.id );
}


/**
 *	This method deregisters a trap function
 */
bool Entity::delTrap( int trapID )
{
	BW_GUARD;

	for (uint i=0; i < traps_.size(); i++)
	{
		if (traps_[i].id == trapID)
		{
			Py_DECREF( traps_[i].function );
			traps_.erase( traps_.begin() + i );
			return true;
		}
	}

	PyErr_SetString( PyExc_ValueError, "Entity.delTrap: "
		"No such trap." );
	return false;
}

/*~ callback Entity.targetModelChanged
 *
 *	This callback method is invoked on the player entity when the primary
 *	model of currently targeted entity has changed.  This allows the player
 *	script to reset any trackers, gui components or special effects referring to
 *	the targets old model.
 *
 *	The currently targeted entity is passed as a parameter.
 */
/**
 *	This method sets our primary embodiment
 */
void Entity::pPrimaryEmbodiment( const IEntityEmbodimentPtr& pNewEmbodiment )
{
	BW_GUARD;
	// take the old one out of the space if it was in
	if (primaryEmbodiment_ && pSpace_)
	{
		primaryEmbodiment_->leaveSpace();
	}

	primaryEmbodiment_ = pNewEmbodiment;

	// fix up the new primary embodiment
	if (primaryEmbodiment_)
	{
		if (pSpace_)
		{
			primaryEmbodiment_->enterSpace( pSpace_ );
		}

		// give primary models their own motors.
		// we don't attempt to retrieve the ActionMatcher from the
		//  old model, so scripts will have to watch out for this,
		//  (as any settings made in it will be lost)
		PyModel * pModel = this->pPrimaryModel();
		if (pModel != NULL)
		{
			PyObject * pTuple = PyTuple_New(1);
			PyTuple_SetItem( pTuple, 0, new ActionMatcher( pPyEntity_.get() ) );
			pModel->pySet_motors( pTuple );		// I'm too lazy to write
			Py_DECREF( pTuple );				// a C++ function for this :)

			// re-attach rain particles if this model belongs to the player
			if (this == ScriptPlayer::entity())
			{
				MF_ASSERT_DEV( pSpace_ != NULL );

				if (pSpace_ != NULL)
					ScriptPlayer::instance().updateWeatherParticleSystems(
						pSpace_->enviro().playerAttachments() );
			}
		}
	}

	// and let the player know if it has us targeted
	if (this->isATarget() && ScriptPlayer::entity())
	{
		ScriptPlayer::entity()->pPyEntity_.callMethod( "targetModelChanged",
			ScriptArgs::create( pPyEntity_ ), ScriptErrorPrint(),
			/* allowNullMethod */ true );
	}
}


/**
 *	This method changes our physics system.
 *
 *	@param pStyle	A pointer to a physics system index, or NULL to disable
 *					physics.
 */
void Entity::setPhysics( int * pStyle )
{
	BW_GUARD;

	if (!this->isControlled())
	{
		ERROR_MSG( "Entity::setPhysics: Attempted to set physics for a server-"
			"controlled entity %d\n", this->entityID() );
		return;
	}

	if (this->pPhysics() != NULL)
	{
		MF_ASSERT( pPhysics_ != (Physics *)-1 );
		pPhysics_->disown();
		Py_DECREF( pPhysics_ );
	}

	if (pStyle == NULL)
	{
		pPhysics_ = NULL;
	}
	else
	{
		pPhysics_ = new Physics( this, *pStyle );
	}
}


/**
 *	This method is used to add a local movement, usually from the attached
 *	physics system.
 *
 *	It checks whether the movement is close enough to the ground to snap to
 *	the terrain, and then calls BWEntity::onMoveLocally.
 */
void Entity::physicsMovement( double time, const Position3D & position,
	EntityID vehicleID, const Direction3D & direction )
{
	BW_GUARD;
	MF_ASSERT( this->isPhysicsAllowed() );

	static const float GROUND_TOLERANCE = 0.1f; // 10cm

	bool isOnGround = false;

	ClientSpacePtr pSpace;

	if ((vehicleID == NULL_ENTITY_ID) && ((pSpace = this->pSpace()) != NULL))
	{
		Position3D globalPos = this->position();
		float terrainHeight = pSpace->findTerrainHeight( globalPos );

		if (terrainHeight != FLT_MIN)
		{
			isOnGround = fabs( globalPos.y - terrainHeight ) < GROUND_TOLERANCE;
		}
	}

	this->onMoveLocally( time, position, vehicleID, isOnGround, direction);
}


/**
 *	This method returns true if we are not physics-corrected, and not waiting
 *	for a vehicle to enter the world.
 */
bool Entity::isPhysicsAllowed() const
{
	BW_GUARD;

	return !this->isPhysicsCorrected() && !this->waitingForVehicle();
}


/**
 * This method is called to get the visibility level of the player.
 */
float Entity::transparency() const
{
	BW_GUARD;
	float tdiff = float( max( 0.0, 
		(App::instance().getGameTimeFrameStart() - visibilityTime_) ) );
	return tdiff >= 2.0 ? (invisible_ ? 0.8f : -1.f)
						: (invisible_ ? 0.0f + tdiff / 2.5f : 0.8f - tdiff / 2.5f);
}

/**
 * This method is called when the player becomes invisible.
 */
bool Entity::setInvisible( bool invis )
{
	BW_GUARD;
	if (this != ScriptPlayer::entity())
	{
		PyErr_SetString( PyExc_TypeError,
			"You can only call this method on a player" );
		return false;
	}
	else if (invisible_ != invis)
	{
		invisible_ = invis;

		// If the previous setting hadn't finished fading in, we need to
		// account for it in the visibility time so the reverse fade is smooth.
		double timeNow = App::instance().getGameTimeFrameStart();
		double tdiff = min( 2.0, max( 0.0, timeNow - visibilityTime_ ) );
		visibilityTime_ = timeNow - (2.0 - tdiff);
	}

	return true;
}

/**
 *	This method implements the Python method to allow changing the state of a
 *	nearby portal.
 */
void Entity::setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags )
{
	BW_GUARD;
	PortalStateChanger::changePortalCollisionState( this,
		isOpen, collisionFlags );
}


/**
 *	This method handles a change to a property of the entity sent
 *  from the server.
 */
void Entity::onProperty( int propertyID, BinaryIStream & data,
	bool isInitialising )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );

	SimpleClientEntity::propertyEvent( pPyEntity_, this->type().description(),
		propertyID, data, /*shouldUseCallback:*/ !isInitialising );
}


/**
 *	This method handles a call to a method of the entity sent
 *  from the server.
 */
void Entity::onMethod( int methodID, BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );

	SimpleClientEntity::methodEvent( pPyEntity_, this->type().description(),
		methodID, data );
}


/**
 *	This method handles a change to a nested property of the entity sent
 *	from the server.
 */
void Entity::onNestedProperty( BinaryIStream & data, bool isSlice,
	bool isInitialising )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );

	SimpleClientEntity::nestedPropertyEvent( pPyEntity_,
		this->type().description(), data,
		/*shouldUseCallback:*/ !isInitialising, isSlice );
}


/**
 *	This method returns the message size of a particular method update.
 */
int Entity::getMethodStreamSize( int methodID ) const
{
	BW_GUARD;
	const EntityDescription & description = this->type().description();

	const MethodDescription * pMethodDescription =
		description.client().exposedMethod( methodID );

	MF_ASSERT( pMethodDescription );

	return pMethodDescription->streamSize( true );
}


/**
 *	This method returns the message size of a particular property update.
 */
int Entity::getPropertyStreamSize( int propertyID ) const
{
	BW_GUARD;
	const EntityDescription & description = this->type().description();

	return description.clientServerProperty( propertyID )->streamSize();
}


/**
 *	This method returns the type name of this entity.
 */
const BW::string Entity::entityTypeName() const
{
	return type_.name();
}


// -----------------------------------------------------------------------------
// Section: Targeting methods
// -----------------------------------------------------------------------------

/**
 *	This method returns whether or not this entity is a target.
 */
bool Entity::isATarget() const
{
	BW_GUARD;
	return CameraApp::instance().entityPicker().pGeneralTarget() == this;
}


/*~ callback Entity.targetFocus
 *
 *	This callback method is called in response to a change in the targeted
 *	entity.  It notifies the player that an entity has just become the current
 *	target.
 *
 *	@param The newly targeted entity.
 */
/**
 *	Called when we become a target
 */
void Entity::targetFocus()
{
	BW_GUARD;

	Entity * pPlayer = ScriptPlayer::entity();
	// If the Player Entity exists, inform this Entity that it has been targeted
	// by the Player.
	// The Player may not exist if logging off whilst targeting an Entity,
	// however, this does then provide a warning that the Entity Picker is being
	// destructed with a non-NULL current entity...

	if (pPlayer != NULL && !pPlayer->isDestroyed())
	{
		pPlayer->pPyEntity().callMethod( "targetFocus",
			ScriptArgs::create( pPyEntity_ ), ScriptErrorPrint(),
			/* allowNullMethod */ true );
	}

	// possibly call our own script here too
}


/*~ callback Entity.targetBlur
 *
 *	This callback method is called in response to a change in the targeted
 *	entity.  It notifies the player that an entity has is no longer the current
 *	target.
 *
 *	@param The originally targeted entity.
 */
/**
 *	Called when we stop being a target
 */
void Entity::targetBlur()
{
	BW_GUARD;

	Entity * pPlayer = ScriptPlayer::entity();
	if (pPlayer != NULL && !pPlayer->isDestroyed())
	{
		pPlayer->pPyEntity().callMethod( "targetBlur",
			ScriptArgs::create( pPyEntity_ ), ScriptErrorPrint(),
			/* allowNullMethod */ true );
	}

	// possibly call our own script here too
}


// -----------------------------------------------------------------------------
// Section: Miscellaneous
// -----------------------------------------------------------------------------

/**
 *	This method returns the space that currently hosts the entity.
 */
ClientSpacePtr Entity::pSpace() const				
{
	return pSpace_; 
}


/*~ callback Entity.onPoseVolatile
 *
 *	This callback is triggered when this entity receives a volatile position
 *	update when it has been receiving non-volatile position updates, or
 *	vice-versa.
 *	It is also triggered when client-side control of this entity has been
 *	granted by the server, but it has been previously been receiving volatile
 *	position updates.
 *
 *	It receives one Boolean parameter, whether the pose is now volatile.
 */
/**
 *	This method calls the onPoseVolatile callback with the current state.
 *
 *	@see Entity::isReceivingVolatileUpdates
 *	@see Entity.isPoseVolatile (in py_entity.cpp)
 */
void Entity::onChangeReceivingVolatileUpdates()
{
	BW_GUARD;

	pPyEntity_.callMethod( "onPoseVolatile",
		ScriptArgs::create( this->isReceivingVolatileUpdates() ),
		ScriptErrorPrint(), /* allowNullMethod */ true );
}


/**
 *	This function returns a transform for things that absolutely MUST
 *	know a transform for every entity. Its use is discouraged.
 */
const Matrix & Entity::fallbackTransform() const
{
	BW_GUARD;
	static Matrix		fakeTransform;

	PyModel * pModel = this->pPrimaryModel();
	if (pModel != NULL)
	{
		return pModel->worldTransform();
	}
	else
	{
		Position3D worldPosition = this->position();
		Direction3D worldDirection = this->direction();
		fakeTransform.setRotate(	worldDirection.yaw,
									worldDirection.pitch,
									worldDirection.roll );
		fakeTransform.translation( worldPosition );
		return fakeTransform;
	}
}


#if ENABLE_WATCHERS
/**
 *	Static function to return a watcher for entities
 */
/* static */ Watcher & Entity::watcher()
{
	BW_GUARD;
	static DirectoryWatcherPtr	watchMe = NULL;

	if (!watchMe)
	{
		watchMe = new DirectoryWatcher;


		watchMe->addChild( "pos",
			new ReadOnlyMemberWatcher< const Position3D, BWEntity >( 
				&BWEntity::position ) );
		
#if 0
		Watcher * pDictWatcher = new DataWatcher<PyObject*>( *(PyObject**)NULL );
		watchMe->addChild( "dict", &PyMapping_Watcher(), &pNull->in_dict );
#endif
	}

	return *watchMe;
}
#endif


/**
 *	This method makes sure that this entity is in the correct
 *	place in the global sorted position lists.
 */
void Entity::shuffle()
{
	BW_GUARD;
	// Shuffle to the left...
	Position3D worldPosition = this->position();
	while (nextLeft_ != NULL && worldPosition.x < nextLeft_->position().x)
	{
		// unlink us
		nextLeft_->nextRight_ = nextRight_;
		if (nextRight_ != NULL) nextRight_->nextLeft_ = nextLeft_;
		// fix our pointers
		nextRight_ = nextLeft_;
		nextLeft_ = nextLeft_->nextLeft_;
		// relink us
		if (nextLeft_ != NULL) nextLeft_->nextRight_ = this;
		nextRight_->nextLeft_ = this;
	}

	// Shuffle to the right...
	while (nextRight_ != NULL && worldPosition.x > nextRight_->position().x)
	{
		// unlink us
		if (nextLeft_ != NULL) nextLeft_->nextRight_ = nextRight_;
		nextRight_->nextLeft_ = nextLeft_;
		// fix our pointers
		nextLeft_ = nextRight_;
		nextRight_ = nextRight_->nextRight_;
		// relink us
		nextLeft_->nextRight_ = this;
		if (nextRight_ != NULL) nextRight_->nextLeft_ = this;
	}

	// Shuffle to the front...
	while (nextUp_ != NULL && worldPosition.z < nextUp_->position().z)
	{
		// unlink us
		nextUp_->nextDown_ = nextDown_;
		if (nextDown_ != NULL) nextDown_->nextUp_ = nextUp_;
		// fix our pointers
		nextDown_ = nextUp_;
		nextUp_ = nextUp_->nextUp_;
		// relink us
		if (nextUp_ != NULL) nextUp_->nextDown_ = this;
		nextDown_->nextUp_ = this;
	}

	// Shuffle to the back...
	while (nextDown_ != NULL && worldPosition.z > nextDown_->position().z)
	{
		// unlink us
		if (nextUp_ != NULL) nextUp_->nextDown_ = nextDown_;
		nextDown_->nextUp_ = nextUp_;
		// fix our pointers
		nextUp_ = nextDown_;
		nextDown_ = nextDown_->nextDown_;
		// relink us
		nextUp_->nextDown_ = this;
		if (nextDown_ != NULL) nextDown_->nextUp_ = this;
	}

	// Ayeeeeee, Macarena!
}


/**
 *	Get the next entity of the specified chain
 */
Entity * Entity::nextOfChain( int chain )
{
	BW_GUARD;
	switch (chain)
	{
		case 0:		return nextLeft_;	break;
		case 1:		return nextRight_;	break;
		case 2:		return nextUp_;		break;
		case 3:		return nextDown_;	break;
	}

	return NULL;
}


/**
 *	This method reads in the cell data that is associated with the player.
 */
bool Entity::initCellPlayerFromStream( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( !this->isDestroyed() );

	return pPyEntity_->initCellPlayerFromStream( data );
}


void Entity::onBecomePlayer()
{
	BW_GUARD;
	ScriptPlayer::instance().setPlayer( this );
	ConnectionControl::instance().onBasePlayerCreated();
}


void Entity::onBecomeNonPlayer()
{
	BW_GUARD;
	// ScriptPlayer may have been redirected to another Entity, but
	// we still clear it.
	ScriptPlayer::instance().setPlayer( NULL );
}


void Entity::onEnterAoI( const EntityEntryBlocker & rBlocker )
{
	pPyEntity_->setFilterOnEntity();

	if (this->checkPrerequisites())
	{
		return;
	}

	condition_ = rBlocker.blockEntry();

	EntityManager::instance().addPendingEntity( this );
}


/*~ callback Entity.onControlled
 *
 *	This callback method is called when the local entity control by the client
 *	has been enabled or disabled. See the Entity.controlledBy() method in the
 *	CellApp Python API for more information.
 *
 *	@param isControlled	Whether the entity is now controlled locally.
 */
/**
 *	This method sets whether or not this entity should be controlled
 */
void Entity::onChangeControl( bool isControlling, bool isInitialising )
{
	BW_GUARD;

	// TODO: Get rid of isControlling in this callback...
	MF_ASSERT( this->isControlled() == isControlling );

	if (!this->isInWorld())
	{
		// Physics is not set up until we enter the world, so nothing
		// interesting to do here.
		MF_ASSERT( isInitialising );
		MF_ASSERT( !this->isReceivingVolatileUpdates() );
		return;
	}

	if (this->isControlled())
	{
		MF_ASSERT( pPhysics_ == (Physics*)-1 );

		pPhysics_ = NULL;
	}
	else
	{
		MF_ASSERT( pPhysics_ != (Physics*)-1 );

		if (pPhysics_ != NULL)
		{
			pPhysics_->disown();
			Py_DECREF( pPhysics_ );
		}
		pPhysics_ = (Physics*)-1;
	}

	if (!isInitialising)
	{
		pPyEntity_.callMethod( "onControlled",
			ScriptArgs::create( this->isControlled() ), ScriptErrorPrint(),
			/* allowNullMethod */ true );
	}
}


/**
 *	This method performs actions common to both onEnterWorld and onChangeSpace.
 *
 *	@param transient	true if this is being called immediately after
 *						onLeaveSpace( true ), i.e. from onChangeSpace.
 */
void Entity::onEnterSpace( bool transient )
{
	BW_GUARD;

	MF_ASSERT_DEV( this->spaceID() != NULL_SPACE_ID )
	MF_ASSERT( pSpace_ == NULL );

	pSpace_ = SpaceManager::instance().space( this->spaceID() );

	// It should not be possible to create an Entity into a space that we didn't
	// know about already.
	MF_ASSERT( pSpace_ != NULL );

	if (primaryEmbodiment_)
	{
		primaryEmbodiment_->enterSpace( pSpace_, transient );
	}

	for (EntityEmbodiments::iterator iter = auxiliaryEmbodiments_.begin();
		iter != auxiliaryEmbodiments_.end();
		iter++)
	{
		(*iter)->enterSpace( pSpace_, transient );
	}

	// Add ourselves to Entity::Neighbour iteration
	Entity * pNeighbour = NULL;

	// Find an entity in our space
	const BWEntities & entities = pBWConnection()->entities();

	for (BWEntities::const_iterator iEntity = entities.begin();
		iEntity != entities.end(); ++iEntity )
	{
		BWEntity * pOther = iEntity->second.get();

		if (pOther == this)
		{
			continue;
		}

		if (pOther->isInWorld() && pOther->spaceID() == this->spaceID())
		{
			pNeighbour = static_cast< Entity * >( pOther );
			break;
		}
	}

	MF_ASSERT( nextLeft_ == NULL );
	MF_ASSERT( nextRight_ == NULL );
	MF_ASSERT( nextUp_ == NULL );
	MF_ASSERT( nextDown_ == NULL );

	if (pNeighbour != NULL)
	{
		// If we have a neighbour, insert ourselves to its left and its up
		nextRight_ = pNeighbour;
		nextLeft_ = nextRight_->nextLeft_;
		nextRight_->nextLeft_ = this;

		if (nextLeft_ != NULL)
		{
			nextLeft_->nextRight_ = this;
		}

		nextDown_ = pNeighbour;
		nextUp_ = nextDown_->nextUp_;
		nextDown_->nextUp_ = this;

		if (nextUp_ != NULL)
		{
			nextUp_->nextDown_ = this;
		}

		// Move to the correct place in the left-right and up-down lists.
		this->shuffle();
	}
}


/**
 *	This method performs actions common to both onLeaveWorld and onChangeSpace.
 *
 *	@param transient	true if this is being called immediately before
 *						onEnterSpace( true ), i.e. from onChangeSpace.
 */
void Entity::onLeaveSpace( bool transient )
{
	BW_GUARD;

	// remove ourselves from Entity::Neighbours iteration
	if (nextLeft_ != NULL)
	{
		nextLeft_->nextRight_ = nextRight_;
	}

	if (nextRight_ != NULL)
	{
		nextRight_->nextLeft_ = nextLeft_;
	}

	if (nextUp_ != NULL)
	{
		nextUp_->nextDown_ = nextDown_;
	}

	if (nextDown_ != NULL)
	{
		nextDown_->nextUp_ = nextUp_;
	}

	nextLeft_ = NULL;
	nextRight_ = NULL;
	nextUp_ = NULL;
	nextDown_ = NULL;

	// remove auxiliary models
	for (EntityEmbodiments::iterator iter = auxiliaryEmbodiments_.begin();
		iter != auxiliaryEmbodiments_.end();
		iter++)
	{
		(*iter)->leaveSpace( transient );
	}

	// remove primary model
	if (primaryEmbodiment_)
	{
		primaryEmbodiment_->leaveSpace( transient );
	}
}


// -----------------------------------------------------------------------------
// Section: Neighbour
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Entity::Neighbour::Neighbour( Entity * pSource, float radius, bool end ) :
	pSource_( pSource ),
	radius_( radius ),
	chain_( end ? 4 : 0 ),
	pNext_( pSource )
{
	BW_GUARD;
	if (!end)
	{
		this->nextInterestingEntity();
	}
}


/**
 *	Update pNext_ to be the next interesting entity
 */
void Entity::Neighbour::nextInterestingEntity()
{
	BW_GUARD;
	//MF_ASSERT( chain_ < 4 );
	//MF_ASSERT( pNext_ != NULL );

	do
	{
		pNext_ = pNext_->nextOfChain( chain_ );

		// End of the chain
		if (pNext_ == NULL)
		{
			chain_++;
			pNext_ = pSource_;
			continue;
		}

		Position3D high;
		Position3D low;
		bool firstAxis;

		switch( chain_ )
		{
		case 0: // left
			low = pNext_->position();
			high = pSource_->position();
			firstAxis = true;
			break;
		case 1: // right
			low = pSource_->position();
			high = pNext_->position();
			firstAxis = true;
			break;
		case 2: // up
			low = pNext_->position();
			high = pSource_->position();
			firstAxis = false;
			break;
		case 3: // down
			low = pSource_->position();
			high = pNext_->position();
			firstAxis = false;
			break;
		}

		const int primaryAxis = firstAxis ? 0 : 2;
		const int otherAxis = firstAxis ? 2 : 0;

		//MF_ASSERT( high[ primaryAxis ] >= low[ primaryAxis ] );

		// Too far along the chain's primary axis
		if (high[ primaryAxis ] - low[ primaryAxis ] > radius_)
		{
			chain_++;
			pNext_ = pSource_;
		}
		// Close enough along the other axis
		else if (fabs( high[ otherAxis ] - low[ otherAxis ] ) <= radius_)
		{
			break;
		}
	}
	while (chain_ < 4);
}


/**
 *	Get the begin iterator for this entity of the specified radius.
 */
Entity::Neighbour Entity::begin( float radius )
{
	BW_GUARD;
	return Neighbour( this, radius, false );
}


/**
 *	Get the end iterator for this entity of the specified radius.
 */
Entity::Neighbour Entity::end( float radius )
{
	BW_GUARD;
	return Neighbour( this, radius, true );
}

BW_END_NAMESPACE

/* entity.cpp */
