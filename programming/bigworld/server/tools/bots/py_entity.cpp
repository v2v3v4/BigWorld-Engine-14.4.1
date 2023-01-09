#include "script/first_include.hpp"

#include "py_entity.hpp"

#include "py_server.hpp"
#include "client_app.hpp"


DECLARE_DEBUG_COMPONENT2( "Entity", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyEntity
// -----------------------------------------------------------------------------

PY_BASETYPEOBJECT( PyEntity )

PY_BEGIN_METHODS( PyEntity )
	/*~ function Entity.addTimer
	 *	@components{ bots }
	 *	The function addTimer registers the timer function <i>onTimer</i> (a
	 *	member function) to be called
	 *	after "initialOffset" seconds. Optionally it can be repeated every
	 *	"repeatOffset" seconds, and have optional user data (an integer that is
	 *	simply passed to the timer function) "userArg".
	 *
	 *	The <i>onTimer</i> function
	 *	must be defined in the entity's class and accept 2 arguments; the
	 *	internal id of the timer (useful to remove the timer via the "delTimer"
	 *	method), and the optional user supplied integer "userArg".
	 *
	 *	Example:
	 *	@{
	 *	<pre># this provides an example of the usage of addTimer
	 *	import BigWorld
	 *	&nbsp;
	 *	class MyBaseEntity( BigWorld.Entity ):
	 *	&nbsp;
	 *	    def __init__( self ):
	 *	        BigWorld.Entity.__init__( self )
	 *	&nbsp;
	 *	        # add a timer that is called after 5 seconds, and then every
	 *	        # second with custom user data of 9
	 *	        self.addTimer( 5, 1, 9 )
	 *	&nbsp;
	 *	        # add a timer that is called once after a second, with custom
	 *	        # user data defaulting to 0
	 *	        self.addTimer( 1 )
	 *	&nbsp;
	 *	    # timers added on a Entity have "onTimer" called as their timer function
	 *	    def onTimer( self, id, userArg ):
	 *	        print "MyBaseEntity.onTimer called: id %i, userArg: %i" % ( id, userArg )
	 *	        # if this is a repeating timer then call:
	 *	        #     self.delTimer( id )
	 *	        # to remove the timer when it is no longer needed</pre>
	 *	@}
	 *	@param initialOffset initialOffset is a float and specifies the
	 *	interval (in seconds) between registration of the timer, and the time of
	 *	first call.
	 *	@param repeatOffset=0 repeatOffset is an optional float and
	 *	specifies the interval (in seconds) between repeated executions after
	 *	the first execution. The delTimer method must be used to remove the
	 *	timer or this will repeat continuously. Values less than 0 are ignored.
	 *	@param userArg=0 userArg is an optional integer which specifies an
	 *	optional user supplied value to be passed to the "onTimer" method of
	 *	the entity.
	 *	@return Integer. This function returns the internal id of the timer.
	 *	This can be used with delTimer method to remove the timer.
	 */
	PY_METHOD( addTimer )
	/*~ function Entity.delTimer
	 *	@components{ bots }
	 *	The function delTimer removes a registered timer function so that it no
	 *	longer executes. Timers that execute only once are automatically removed
	 *	and do not have to be removed using delTimer. If delTimer is called with
	 *	an id that is no longer valid (already removed for example) then an
	 *	error will be logged to the standard log file.
	 *
	 *	See Entity.addTimer for an example of timer usage.
	 *	@param id id is an integer that specifies the internal id of the timer
	 *	to be removed.
	 */
	PY_METHOD( delTimer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEntity )

	/*~ attribute Entity id
	 *  @components{ bots }
	 *  This is the unique identifier for the Entity. It is common across
	 *	bots, cell, and base applications.
	 *
	 *  @type Read-only Integer
	 */
	PY_ATTRIBUTE( id )

	/*~ attribute Entity isDestroyed
	 *  @components{ bots }
	 *
	 *  This attribute indicates if this PyEntity's Entity has been destroyed.
	 *
	 *	Accessing position, yaw, pitch, roll, cell or base
	 *	after this becomes true will raise an exception.
	 *
	 *  @type Read-only boolean
	 */
	PY_ATTRIBUTE( isDestroyed )

	/*~ attribute Entity isInWorld
	 *  @components{ bots }
	 *
	 *  This attribute indicates if this PyEntity's Entity is in world.
	 *
	 *
	 *  @type Read-only boolean
	 */
	PY_ATTRIBUTE( isInWorld )

	/*~ attribute Entity position
	 *  @components{ bots }
	 *
	 *	Current position of Entity in world
	 *
	 *	@type Read-only Vector3
	 */
	PY_ATTRIBUTE( position )

	/*~ attribute Entity yaw
	 *  @components{ bots }
	 *
	 *	Current yaw of Entity in world
	 *
	 *	@type Read-only float
	 */
	PY_ATTRIBUTE( yaw )

	/*~ attribute Entity pitch
	 *  @components{ bots }
	 *
	 *	Current pitch of Entity in world
	 *
	 *	@type Read-only float
	 */
	PY_ATTRIBUTE( pitch )

	/*~ attribute Entity roll
	 *  @components{ bots }
	 *
	 *	Current roll of Entity in world
	 *
	 *	@type Read-only float
	 */
	PY_ATTRIBUTE( roll )

	/*~ attribute Entity cell
	 *  @components{ bots }
	 *
	 *  This attribute provides a PyServer object. This can
	 *  be used to call functions on the base Entity.
	 *
	 *  @type Read-only PyServer
	 */
	PY_ATTRIBUTE( cell )

	/*~ attribute Entity base
	 *  @components{ bots }
	 *
	 *  This attribute provides a PyServer object. This can
	 *  be used to call functions on the base Entity.
	 *
	 *  @type Read-only PyServer
	 */
	PY_ATTRIBUTE( base )

	/*~ attribute Entity clientApp
	 *  @components{ bots }
	 *
	 *  This attribute provides access to the ClientApp object the Entity was
	 *	created from, or None if it has already been destroyed.
	 *
	 *  @type Read-only ClientApp
	 */
	PY_ATTRIBUTE( clientApp )

PY_END_ATTRIBUTES()


// Callbacks
/*~ callback Entity.onBecomePlayer
 *	@components{ bots }
 *
 *	This callback method signals that an entity has become the player, and its
 *	script has become an instance of the player class.
 *
 *	@see ClientApp.player
 *	@see Entity.onBecomeNonPlayer
 */
/*~ callback Entity.onBecomeNonPlayer
 *	@components{ bots }
 *
 *	This callback method signals that an entity is no longer the player, and
 *	its script is just about to become an instance of its original class.
 *
 *	@see ClientApp.player
 *	@see Entity.onBecomePlayer
 */
/*~	callback Entity.onTimer
 *	@components{ bots }
 *	This method is called when a timer associated with this entity is triggered.
 *	A timer can be added with the Entity.addTimer method.
 *
 *	@param timerHandle	The id of the timer.
 *	@param userData	The integer user data passed into Entity.addTimer.
 *
 *	@see Entity.addTimer
 */
/*~ callback Entity.onStreamComplete
 *	@components{ bots }
 *	This is a callback function that can be implemented by the user.
 *
 *	If present, this method is called when a download from the proxy triggered
 *  with either streamStringToClient() or streamFileToClient() has completed.
 * 
 *	See Proxy.streamStringToClient in BaseApp Python API documentation
 *	See Proxy.streamFileToClient in BaseApp Python API documentation
 *
 *	@param	id		A unique ID associated with the download.
 *  @param  desc	A short string description of the download.
 *	@param	data	The downloaded data, as a string.
 */



/**
 *	Constructor.
 */
PyEntity::PyEntity( Entity & entity ) :
		PyObjectPlus( (PyTypeObject*) entity.type().type().get(),
			/* isInitialised */ true ),
		pEntity_( &entity ),
		entityID_( NULL_ENTITY_ID ),
		wpClientApp_( &const_cast< ClientApp & >( pEntity_->getClientApp() ) ),
		cell_(
			new PyServer( this,  /* isProxyCaller */ false ),
				ScriptObject::FROM_NEW_REFERENCE ),
		base_(
			new PyServer( this, /* isProxyCaller */ true ),
				ScriptObject::FROM_NEW_REFERENCE ),
		pyTimer_( this, entityID_ )
{
}


/**
 *	Destructor.
 */
PyEntity::~PyEntity()
{
}


/**
 *	This method returns the EntityID of the Entity we are or were
 *	associated with
 */
EntityID PyEntity::entityID() const
{
	return entityID_;
}


/**
 *	This method returns whether or not our associated entity has been destroyed.
 *
 *	@return bool	True if our associated entity has been destroyed (i.e.
 *					onEntityDestroyed() has been called)
 */
bool PyEntity::isDestroyed() const
{
	return pEntity_ == NULL;
}


/**
 *	This method returns our ClientApp if it is still available.
 *
 *	@return ClientApp *	The ClientApp we are part of, or NULL if it has already
 *						been cleaned up.
 */
ClientApp * PyEntity::pClientApp() const
{
	return wpClientApp_.get();
}


/**
 *  This method is called when the entity is destroyed.
 */
void PyEntity::onEntityDestroyed()
{
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

	pyTimer_.cancelAll();

	pEntity_ = NULL;
	MF_ASSERT( this->isDestroyed() );
}


bool PyEntity::initCellEntityFromStream( BinaryIStream & data )
{
	MF_ASSERT( pEntity_ != NULL );

	entityID_ = pEntity_->entityID();
	MF_ASSERT( entityID_ != NULL_ENTITY_ID );

	pyTimer_.ownerID( entityID_ );

	// Start with all of the default values.
	PyObject * pNewDict = pEntity_->type().newDictionary( /* pSection */ NULL,
		/* isPlayer */ false );
	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if ( !pNewDict || !pCurrDict ||
		PyDict_Update( pCurrDict, pNewDict ) < 0 )
	{
		PY_ERROR_CHECK();
		return false;
	}

	Py_XDECREF( pNewDict );
	Py_XDECREF( pCurrDict );

	pEntity_->onProperties( data, /* isInitialising */ true );

	// Now that everything is in working order, create the script object
	PyEntityPtr pThis( this );
	pThis.callMethod( "__init__", ScriptErrorPrint(),
		/* allowNullMethod */ true );

	return true;
}


bool PyEntity::initBasePlayerFromStream( BinaryIStream & data )
{
	MF_ASSERT( pEntity_ != NULL );

	entityID_ = pEntity_->entityID();
	MF_ASSERT( entityID_ != NULL_ENTITY_ID );

	pyTimer_.ownerID( entityID_ );

	PyObject * pNewDict = pEntity_->type().newDictionary( data,
		EntityType::BASE_PLAYER_DATA );

	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if ( !pNewDict || !pCurrDict ||
		PyDict_Update( pCurrDict, pNewDict ) < 0 )
	{
		PY_ERROR_CHECK();
		return false;
	}

	Py_XDECREF( pNewDict );
	Py_XDECREF( pCurrDict );

	// Now that everything is in working order, create the script object
	PyEntityPtr pThis( this );
	pThis.callMethod( "__init__", ScriptErrorPrint(),
		/* allowNullMethod */ true );

	return true;
}


/**
 *	This method reads the player data sent from the cell. This is called on the
 *	player entity when it gets a cell entity associated with it.
 */
bool PyEntity::initCellPlayerFromStream( BinaryIStream & stream )
{
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


/**
 *	This method is exposed to scripting. It is used by an entity to register a
 *	timer function to be called with a given period.
 */
PyObject * PyEntity::py_addTimer( PyObject * args )
{
	if (this->isDestroyed())
	{
		PyErr_Format( PyExc_TypeError, "Entity.addTimer: "
			"Entity %d has already been destroyed", this->entityID() );
		return NULL;
	}

	return pyTimer_.addTimer( args );
}


/**
 *	This method is exposed to scripting. It is used by an entity to remove a timer
 *	from the time queue.
 */
PyObject * PyEntity::py_delTimer( PyObject * args )
{
	return pyTimer_.delTimer( args );
}


/* Macros to raise exceptions if accessing attributes of a destroyed entity */

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyEntity::

#define PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( MEMBER, NAME )				\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
{																			\
	if (this->isDestroyed())												\
	{																		\
		PyErr_Format( PyExc_TypeError, "Sorry, the attribute " #NAME " in "	\
		"%s is no longer available as its entity no longer exists on this "	\
		"client.", this->typeName() );										\
		return NULL;														\
	}																		\
	return Script::getReadOnlyData( MEMBER );								\
}

#define PY_RO_PENTITY_ATTRIBUTE( MEMBER, NAME )								\
	PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( pEntity_->MEMBER, NAME )			\
	PY_RO_ATTRIBUTE_SET( NAME )												\


PY_RO_PENTITY_ATTRIBUTE( position(), position )
PY_RO_PENTITY_ATTRIBUTE( direction().yaw, yaw )
PY_RO_PENTITY_ATTRIBUTE( direction().pitch, pitch )
PY_RO_PENTITY_ATTRIBUTE( direction().roll, roll )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( cell_, cell )
PY_RO_ATTRIBUTE_SET( cell )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( base_, base )
PY_RO_ATTRIBUTE_SET( base )

#undef PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED
#undef PY_RO_PENTITY_ATTRIBUTE
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE

/**
 *	This method sets an attribute associated with this object.
 */
bool PyEntity::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
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


BW_END_NAMESPACE

// py_entity.cpp
