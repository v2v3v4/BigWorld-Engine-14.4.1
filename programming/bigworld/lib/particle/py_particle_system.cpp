#include "pch.hpp"

#include "py_particle_system.hpp"
#include "model/model.hpp"
#include "moo/render_context.hpp"
#include "moo/draw_context.hpp"
#include "cstdmf/string_builder.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_particle_system.ipp"
#endif

/**
 *	This is the constructor for PyParticleSystem.
 *
 *	@param pSystem	The particle system that we are wrappiong with a python api
 *					It must never be NULL.
 *	@param pType	Parameters passed to the parent PyObject class.
 */
PyParticleSystem::PyParticleSystem( ParticleSystemPtr pSystem, PyTypeObject *pType ) :
	PyAttachment( pType ),
	pSystem_( pSystem ),
	lod_( 0.0f ),
	actionsHolder_( pSystem->actionSet(), this, true )
{	
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pSystem_.getObject() )
	{
		MF_EXIT( "PyParticleSystem: failed to allocate memory for particle system." );
	}
}


/**
 *	This is the destructor for PyParticleSystem.
 */
PyParticleSystem::~PyParticleSystem()
{
	BW_GUARD;
	pSystem_ = NULL;
}


/**
 *	This method updates the state of the particles for the time given.
 *
 *	@param dTime	Time since last update in seconds.
 */
void PyParticleSystem::tick( float dTime )
{
	BW_GUARD;
	pSystem_->tick( dTime );
}


/**
 *	Check if this if visible or lodded out.
 *	@return true if this object is visible, false if lodded out.
 */
bool PyParticleSystem::isLodVisible() const
{
	return pSystem_->isLodVisible( lod_ );
}


/**
 *	Re-calculate LOD and apply animations to this PyParticleSystem for this
 *	frame. Should be called once per frame.
 *	@param worldTransform the world transform.
 *	@param lod the lod to use for drawing.
 */
void PyParticleSystem::updateAnimations( const Matrix & worldTransform,
	float lod )
{
	BW_GUARD;
	lod_ = lod;
}


/**
 *	This methods tells the particle system to draw itself.
 */
void PyParticleSystem::draw( Moo::DrawContext& drawContext, const Matrix &world )
{
	BW_GUARD;
	pSystem_->draw( drawContext, world, lod_ );
}


/**
 *	This accumulates our bounding box into the given variable
 */
void PyParticleSystem::localBoundingBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;
	pSystem_->localBoundingBox( bb );
}


/**
 *	This accumulates our visibility bounding box into the given variable
 */
void PyParticleSystem::localVisibilityBox( BoundingBox & vbb, bool skinny )
{
	BW_GUARD;
	pSystem_->localVisibilityBoundingBox( vbb );
}


/**
 *	This accumulates our bounding box into the given variable
 */
void PyParticleSystem::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny )
{
	BW_GUARD;
	pSystem_->worldBoundingBox( bb, world );
}


/**
 *	This accumulates our visibility bounding box into the given variable
 */
void PyParticleSystem::worldVisibilityBox( BoundingBox & wvbb, const Matrix& world, bool skinny )
{
	BW_GUARD;
	pSystem_->worldVisibilityBoundingBox( wvbb );
}


/**
 *	We have been attached. Give ourselves a ground specifier
 */
bool PyParticleSystem::attach( MatrixLiaison * pOwnWorld )
{
	BW_GUARD;
	bool ret = this->PyAttachment::attach( pOwnWorld );
	ret &= pSystem_->attach( pOwnWorld );
	return ret;	
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyParticleSystem::detach()
{
	BW_GUARD;
	this->PyAttachment::detach();
	pSystem_->detach();
}


/**
 *	Make sure u convert back to world coordinates when entering world.
 */
void PyParticleSystem::enterWorld()
{
	BW_GUARD;
	this->PyAttachment::enterWorld();
	pSystem_->enterWorld();	
}


/**
 *	Make sure to convert back to local coordinates when leaving world.
 */
void PyParticleSystem::leaveWorld()
{
	BW_GUARD;
	this->PyAttachment::leaveWorld();
	pSystem_->leaveWorld();
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyParticleSystem.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyParticleSystem::

PY_TYPEOBJECT( PyParticleSystem )

PY_BEGIN_METHODS( PyParticleSystem )
	/*~ function PyParticleSystem.addAction
	 *	Adds a particle system action to the particle system.
	 *	@param action A reference to an instance of PyParticleSystemAction.
	 */
	PY_METHOD( addAction )
	/*~ function PyParticleSystem.removeAction
	 *	Removes a particle system action from the particle system.
	 *	@param action Either an index to the type of action to be removed (the
	 *	first will be removed if there is more than one of that type) or a
	 *	reference to an instance of ParticleSystemAction. The index is the id
	 *	of the type of ParticleSystemAction you want to remove (see
	 *	ParticleSystemAction for these values).
	 */
	PY_METHOD( removeAction )
	/*~ function PyParticleSystem.action
	 *	Returns an action from the particle system.
	 *	@param index An index to the type of action. This is the id of the type
	 *	of ParticleSystemAction you are looking for (see ParticleSystemAction
	 *	for these values).
	 *	@return A reference to the action if found (to the first created if
	 *	multiple actions of that type exisit in the particle system) or None if
	 *	none of that type of action belongs in the particle system.
	 */
	PY_METHOD( action )
	/*~ function PyParticleSystem.clear
	 *	Removes all particles from the particle system.
	 */
	PY_METHOD( clear )
	/*~ function PyParticleSystem.update
	 *	Updates the particle system by the specified time slice dt. You do
	 *	not need to call this method.
	 *	@param dt Time interval to update particle system by (in seconds).
	 */
	PY_METHOD( update )
	/*~ function PyParticleSystem.render
	 *	Draws the particle system. You do not need to call this method.
	 */
	PY_METHOD( render )
	/*~ function PyParticleSystem.load
	 *	Loads a particle system definition from an XML file.
	 *	@param filename name of the XML file to read from.
	 */
	PY_METHOD( load )
	/*~ function PyParticleSystem.save
	 *	Saves a particle system definition to an XML file.
	 *	@param filename name of the XML file to write to.
	 */
	PY_METHOD( save )
	/*~ function PyParticleSystem.size
	 *	Retrieves the number of currently active particles.
	 *	@return The number of currently active particles.
	 */
	PY_METHOD( size )
	/*~ function PyParticleSystem.force
	 *	Forces particle system to spawn num particles.
	 *  @param num The number of particle to spawn (default num = 1).
	 */
	PY_METHOD( force )
	/*~ function PyParticleSystem.duration
	 *	This function looks for a sink in each contained ParticleSystem and
	 *	returns the maximum age of each particle system, returns -1 if no
	 *	sinks are found
	 */
	 PY_METHOD( duration )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyParticleSystem )
	/*~ attribute PyParticleSystem.actions
	 *	This contains a list of actions acting on the Particle System.
	 *	@type List of ParticleSystemAction.
	 */
	PY_ATTRIBUTE( actions )
	/*~ attribute PyParticleSystem.capacity
	 *	This specifies the maximum number of particles that can be present in
	 *	the particle system.
	 */
	PY_ATTRIBUTE( capacity )
	/*~ attribute PyParticleSystem.windFactor
	 *	Specifies how much effect the wind has on the particles velocity.
	 *	Best values are from 0 to 1 and any negative value is treated as
	 *	the wind having no effect on the particles. Values are treated as
	 *	an exponential decay to the wind speed.
	 *	@type Float value containing the windHalfLife.
	 */
	PY_ATTRIBUTE( windFactor )
	/*~ attribute PyParticleSystem.renderer
	 *	Renderer for the ParticleSystem. This can be used to set the
	 *	ParticleSystemRenderer for the ParticleSystem or to access it.
	 *	@type ParticleSystemRenderer.
	 */
	PY_ATTRIBUTE( renderer )
	/*~ attribute PyParticleSystem.explicitPosition
	 *	Set the particle system to always use this location as its position
	 *	(ignore world space). Default is ( 0, 0, 0 ) and is not active. Setting
	 *	either explicitPosition or explicitDirection sets a flag internal to the
	 *	particle system to use the explicit values (this is off by default, and
	 *	cannot be turned off once turned on).
	 *	@type Tuple of 3 float values.
	 */
	PY_ATTRIBUTE( explicitPosition )
	/*~ attribute PyParticleSystem.explicitDirection
	 *	This sets the particle system to always use this direction as its
	 *	direction (ignore world transform). Default is ( 0, 0, 1 ) and is not
	 *	active. Setting either explicitPosition or explicitDirection sets a flag
	 *	internal to the particle system to use the explicit values (this is off
	 *	by default, and cannot be turned off once turned on).
	 *	@type Tuple of 3 float values.
	 */
	PY_ATTRIBUTE( explicitDirection )
	/*~ attribute PyParticleSystem.maxLod
	 *	This specifies the maxLod property. The distance from the camera beyond
	 *	which the particle system will not be drawn.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( maxLod )
	/*~	attribute PyParticleSystem.fixedFrameRate
	 *
	 *	Simulates a fixed frame rate for particle systems.  If the value is > 0,
	 *	this indicates the desired frame rate.  If &lt;=0 then the current frame rate is used.
	 *	@type Float
	 */
	PY_ATTRIBUTE( fixedFrameRate )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyParticleSystem )

/*~ function Pixie.ParticleSystem
 *	Factory function to create and return a PyParticleSystem object. The
 *	PyParticleSystem class provides a base particle system to manage
 *	particles.
 *	@return A new ParticleSystem object.
 */
PY_FACTORY_NAMED( PyParticleSystem, "ParticleSystem", Pixie )


/**
 *	Python-accessible update method
 */
PyObject * PyParticleSystem::py_update( PyObject *args )
{
	BW_GUARD;
	float dTime = 0.03f;

	if ( !PyArg_ParseTuple( args, "f", &dTime ) )
	{
		PyErr_SetString( PyExc_TypeError, "PyParticleSystem::py_update expects a dTime" );
		return NULL;
	}

	this->tick( dTime );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible render method
 */
PyObject * PyParticleSystem::py_render( PyObject *args )
{
	BW_GUARD;
	this->updateAnimations( Matrix::identity, Model::LOD_MIN );
	// INVESTIGATE
	// why would you render something directly from python ????
	// can we get rid of this function
	// temporary solution is to create temp draw context, draw to it and flush immediately
	Moo::DrawContext tempDrawContext( Moo::RENDERING_PASS_COLOR );
	tempDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
	this->draw( tempDrawContext, Matrix::identity );
	tempDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
	tempDrawContext.flush( Moo::DrawContext::ALL_CHANNELS_MASK );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible load method
 */
PyObject * PyParticleSystem::py_load( PyObject *args )
{
	BW_GUARD;
	char * name;

	if ( !PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_SetString( PyExc_TypeError, "PyParticleSystem::py_load expects a filename" );
		return NULL;
	}

	char msgBuffer[ 512 ];
	StringBuilder errorMsg( msgBuffer, sizeof(msgBuffer) );
	pSystem_->load( name, "", &errorMsg );
	if (errorMsg.length() != 0)
	{
		ERROR_MSG( "%s\n", errorMsg.string() );
	}

	Py_RETURN_NONE;
}


/**
 *	Python-accessible save method
 */
PyObject * PyParticleSystem::py_save( PyObject *args )
{
	BW_GUARD;
	char * name;

	if ( !PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_SetString( PyExc_TypeError, "PyParticleSystem::py_save expects a filename" );
		return NULL;
	}

	pSystem_->save( name );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible size method (number of particles active)
 */
PyObject * PyParticleSystem::py_size( PyObject *args )
{
	BW_GUARD;
	return Script::getData( pSystem_->size() );
}


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should be
 *					a number (capacity.)
 */
PyObject *PyParticleSystem::pyNew( PyObject *args )
{
	BW_GUARD;
	int initialCapacity = 100;

	if ( !PyArg_ParseTuple( args, "|i", &initialCapacity ) )
	{
		PyErr_SetString( PyExc_TypeError, "ParticleSystem: "
			"Optional argument must be an integer.\n" );
		return NULL;
	}

	ParticleSystemPtr pSystem = new ParticleSystem( initialCapacity );
	PyParticleSystem* pPPS = new PyParticleSystem( pSystem );
	return pPPS;
}


/**
 *	This Python method allows the script to add an action to the particle
 *	system.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a reference to an instance of ParticleSystemAction.
 */
PyObject *PyParticleSystem::py_addAction( PyObject *args )
{
	BW_GUARD;
	PyObject *pNewAction;

	if ( PyArg_ParseTuple( args, "O", &pNewAction ) )
	{
		if ( PyParticleSystemAction::Check( pNewAction ) )
		{
			pSystem_->addAction( (static_cast<PyParticleSystemAction*>( pNewAction ))->pAction() );
			Py_RETURN_NONE;
		}
	}

	PyErr_SetString( PyExc_TypeError, "PyParticleSystem.addAction: "
		"PyParticleSystemAction expected." );
	return NULL;
}


/**
 *	This Python method allows the script to remove an action from the particle
 *	system.
 *
 *	@param args		The list of parameters passed from Python. This could
 *					either be an index to the type of Action to be removed
 *					or a reference to an instance of ParticleSystemAction.
 */
PyObject *PyParticleSystem::py_removeAction( PyObject *args )
{
	BW_GUARD;
	if ( PyTuple_Size( args ) == 1 )
	{
		// The following is a borrowed reference so there is no need to
		// decrement the reference count for the pointer.
		PyObject * pFirstItem = PyTuple_GetItem( args, 0 );

		if ( PyParticleSystemAction::Check( pFirstItem ) )
		{
			pSystem_->removeAction( (static_cast<PyParticleSystemAction *>(
				pFirstItem ))->pAction());
			Py_RETURN_NONE;
		}
		else
		{
			int actionTypeID;
			if ( Script::setData( pFirstItem, actionTypeID ) == 0 )
			{
				pSystem_->removeAction( actionTypeID );
				Py_RETURN_NONE;
			}
		}
	}

	PyErr_SetString( PyExc_TypeError, "PyParticleSystem.removeAction: "
		"actionTypeID (int) or PyParticleSystemAction expected." );
	return NULL;
}


/**
 *	This Python method allows the script to access an action from the particle
 *	system.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be an index to the type of Action.
 *
 *	@return	A reference to the Action if found or None if none of that type of
 *			action belongs in the particle system.
 */
PyObject *PyParticleSystem::py_action( PyObject *args )
{
	BW_GUARD;
	int actionTypeID;

	if ( PyArg_ParseTuple( args, "i", &actionTypeID ) )
	{
		ParticleSystemActionPtr pAction = pSystem_->pAction( actionTypeID );
		if (pAction.hasObject())
		{
			PyParticleSystemActionPtr pPSA = PyParticleSystemAction::createPyAction(&*pAction);
			return Script::getData( pPSA );
		}
		else
		{
			PyErr_SetString( PyExc_ValueError, "ParticleSystem.action: "
				"action does not exist." );
			return NULL;
		}
	}

	PyErr_SetString( PyExc_TypeError, "ParticleSystem.action: "
		"actionTypeID (int) expected." );
	return NULL;
}


/**
 *	This Python method allows the script to tell the particle system to
 *	remove all particles from itself.
 *
 *	@param args		The list of parameters passed from Python. None are
 *					expected or checked.
 */
PyObject *PyParticleSystem::py_clear( PyObject *args )
{
	BW_GUARD;
	pSystem_->clear();
	Py_RETURN_NONE;
}


/**
 *	Python-accessible wrapper for the spawn method
 */
PyObject * PyParticleSystem::py_force( PyObject* args )
{
	BW_GUARD;
	int num=1;
	if (!PyArg_ParseTuple( args, "|i", &num ))
	{
		PyErr_SetString( PyExc_TypeError, "PyParticleSystem.Force: "
			"Argument parsing error: Expected an optional number of units to force" );
		return NULL;
	}
	pSystem_->spawn(num);
	Py_RETURN_NONE;
}


/**
 *	This is the release method for particle system actions.
 *
 *	@param pAction		The particle action to be released.
 */
void PyParticleSystemAction_release( PyParticleSystemActionPtr pAction )
{	
	BW_GUARD;
	Py_XDECREF( pAction.getObject() );
}

BW_END_NAMESPACE

// py_particle_system.cpp
