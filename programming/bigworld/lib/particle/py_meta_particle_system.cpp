#include "pch.hpp"
#include "moo/render_context.hpp"
#include "moo/draw_context.hpp"
#include "py_meta_particle_system.hpp"
#include "py_particle_system.hpp"
#include "pyscript/py_data_section.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/string_builder.hpp"

// -----------------------------------------------------------------------------
// Section: The Python Interface to the Meta Particle System.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyMetaParticleSystem::


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyMetaParticleSystem )


PY_BEGIN_METHODS( PyMetaParticleSystem )
	/*~ function PyMetaParticleSystem.addSystem
	 *	This function adds a ParticleSystem to the MetaParticleSystem container.
	 *	@param particle_system The ParticleSystem to add to this container.
	 */
	PY_METHOD( addSystem )
	/*~ function PyMetaParticleSystem.removeSystem
	 *	This function removes a ParticleSystem from the MetaParticleSystem
	 *	container.
	 *	@param particle_system Either a string containing the name of the
	 *	ParticleSystem or a reference to the ParticleSystem to be removed.
	 */
	PY_METHOD( removeSystem )
	/*~ function PyMetaParticleSystem.removeAllSystems
	 *	This function removes all ParticleSystem@@s from the MetaParticleSystem
	 *	container.
	 */
	PY_METHOD( removeAllSystems )
	/*~ function PyMetaParticleSystem.system
	 *	This function returns a reference to a ParticleSystem within the
	 *	container, or None if no matching ParticleSystem is found.
	 *	@param particle_system String containing the name of the ParticleSystem
	 *	to return.
	 */
	PY_METHOD( system )
	/*~ function PyMetaParticleSystem.update
	 *	Updates all particle systems contained within this MetaParticleSystem by
	 *	the specified time slice dt. <i>You do not need to call this method.</i>
	 *	@param dt Time interval to update particle system by (in seconds).
	 */
	PY_METHOD( update )
	/*~ function PyMetaParticleSystem.render
	 *	Draws all ParticleSystem@@s contained within this MetaParticleSystem.
	 *	<i>You do not need to call this method.</i>
	 */
	PY_METHOD( render )
	/*~ function PyMetaParticleSystem.load
	 *	This function loads a saved MetaParticleSystem definition from an XML
	 *	file.
	 *	@param name Name of the XML file to load.
	 */
	PY_METHOD( load )
	/*~ function PyMetaParticleSystem.save
	 *	This functions saves a MetaParticleSystem definition to an XML file.
	 *	@param name Name of the XML file to save to.
	 */
	PY_METHOD( save )
	/*~ function PyMetaParticleSystem.force
	 *	This method forces each ParticleSystem contained within the
	 *	MetaParticleSystem to forcibly create num units of particles. The
	 *	number of particles created by each unit are dependant upon the
	 *	individual ParticleSystem@@s forcedUnitSize in an attached source
	 *	ParticleSystemAction (see SourcePSA).
	 *	@param num An integer representing the number of units to force on
	 *	the SourcePSA of all contained ParticleSystem@@s.
	 */
	PY_METHOD( force )
	/*~ function PyMetaParticleSystem.clear
	 *	This function calls the clear method of each contained ParticleSystem
	 *	(effectively removes all particles from all contained
	 *	ParticleSystem@@s).
	 */
	PY_METHOD( clear )
	/*~ function PyMetaParticleSystem.duration
	 *	This function looks for a sink in each contained ParticleSystem and
	 *	returns the maximum age of each particle system, returns -1 if no
	 *	sinks are found
	 */
	PY_METHOD( duration )
	/*~ function PyMetaParticleSystem.nSystems
	 *
	 *	This function returns the number of ParticleSystems contained within this MetaParticleSystem.
	 */
	PY_METHOD( nSystems )
PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( PyMetaParticleSystem )
PY_END_ATTRIBUTES()


/*~ function Pixie.MetaParticleSystem
 *	Factory function to create and return a PyMetaParticleSystem object.
 *	@return A new PyMetaParticleSystem object.
 */
PY_FACTORY_NAMED( PyMetaParticleSystem, "MetaParticleSystem", Pixie )


PyObject* PyMetaParticleSystem::create( const BW::string& filename )
{
	BW_GUARD;
	PyObject * ret = NULL;

	DataSectionPtr pDS = BWResource::openSection(filename);
	if (pDS)
	{
		char msgBuffer[ 512 ];
		StringBuilder errorMsg( msgBuffer, sizeof(msgBuffer) );
		if ( pDS->readString( ParticleSystem::VERSION_STRING, "" ) != "" )
		{
			//is a particle system
			ParticleSystem* ps = new ParticleSystem();
			errorMsg.clear();
			ps->load( filename, "", &errorMsg );
			MetaParticleSystemPtr mps = new MetaParticleSystem();
			mps->addSystem(ps);
			PyMetaParticleSystem* pmps = new PyMetaParticleSystem(mps);
			ret = pmps;
		}
		else
		{
			//is a meta-particle system
			MetaParticleSystem* mps = new MetaParticleSystem();
			errorMsg.clear();
			mps->load( filename, "", &errorMsg );
			PyMetaParticleSystem* pmps = new PyMetaParticleSystem(mps);
			ret = pmps;			
		}
		if (errorMsg.length() != 0)
		{
			ERROR_MSG( "%s\n", errorMsg.string() );
		}
	}
	else
	{
		PyErr_Format( PyExc_ValueError, "Pixie.create: No such resource %s", filename.c_str() );
	}

	return ret;
}


/*~ function Pixie.create
 *	This function creates either a PyParticleSystem or PyMetaParticleSystem by
 *	loading an definition for the particle system from an XML file.
 *
 *	If the XML file defines a ParticleSystem then the returned object is a
 *	PyParticleSystem, and if it defines a MetaParticleSystem then the returned
 *	object is a PyMetaParticleSystem.
 *	@param name Name of the XML file to load.
 */
PY_MODULE_STATIC_METHOD( PyMetaParticleSystem, create, Pixie )


/**
 *	Constructor for MPSBackgroundTask
 */
MPSBackgroundTask::MPSBackgroundTask( const BW::string& filename, PyObject * callback ):
	BackgroundTask( "MPSBackgroundTask" ),
	filename_( filename ),
	pCallback_( callback ),
	pSystem_( NULL )
{
}


/**
 *	This method is called in the background thread by the task manager.
 *	It creates and loads the C++ particle system class, but does not
 *	consruct the python wrapper.
 *
 *	@param	mgr		The background task manager instance
 */
void MPSBackgroundTask::doBackgroundTask( TaskManager & mgr )
{
	BW_GUARD;
	char msgBuffer[ 512 ];
	StringBuilder errorMsg( msgBuffer, sizeof(msgBuffer) );
	pSystem_ = new MetaParticleSystem();
	pSystem_->load( filename_, "", &errorMsg );
	if (errorMsg.length() != 0)
	{
		ERROR_MSG( "%s\n", errorMsg.string() );
	}

	mgr.addMainThreadTask( this );
}


/**
 *	This method is called in the main thread by the task manager.
 *	It creates the python wrapper class and calls the callback
 *	function with it as the first argument.
 *
 *	@param	mgr		The background task manager instance
 */
void MPSBackgroundTask::doMainThreadTask( TaskManager & mgr )
{
	BW_GUARD;
	PyMetaParticleSystem* pMPS = new PyMetaParticleSystem( pSystem_ );
	Py_INCREF( pCallback_.getObject() );		
	Script::call( &*(pCallback_),
		Py_BuildValue("(O)", (PyObject *)pMPS),
		"MPSBackgroundTask callback: " );		
	Py_DECREF( pMPS );
}


/**
 *	This method implements the python Pixie.createBG method, it creates a
 *	particle system in the background thread and then calls a callback function
 *	with the results.
 *
 *	@param	filename		name of the particle system to load.
 *	@param	callback		callback function that takes one parameter
 *							(a PyMetaParticleSystem)
 *
 *	@return NULL
 *
 */
PyObject* PyMetaParticleSystem::createBG( const BW::string& filename, PyObjectPtr callback )
{
	BW_GUARD;
	MPSBackgroundTaskPtr pTask = new MPSBackgroundTask( filename, callback.get() );
	FileIOTaskManager::instance().addBackgroundTask(pTask);
	Py_RETURN_NONE;
}


/*~ function Pixie.createBG
 *	This function creates a MetaParticleSystem in the background thread
 *	by loading an definition for the particle system from an XML file. 
 *
 *	Unlike the create function, it always return a MetaParticleSystem. If 
 *	the XML file defines a ParticleSystem, this will be returned as the only
 *	system in the new MetaParticleSystem create.
 *	
 *	The callable object provided as the second argument will be called 
 *	after the particle system has been created. This call will always
 *	occur in the same thread as the original call (the main thread).
 *
 *	@param name		Name of the XML file to load.
 *	@param callback	Callable object to be invoked when the new system is ready.
 */
PY_MODULE_STATIC_METHOD( PyMetaParticleSystem, createBG, Pixie )


/**
 *	This is the constructor for PyMetaParticleSystem.
 *
 *	@param pSystem		The meta particle system we are wrapping with python.
 *						It must never be NULL
 *	@param pType		Parameters passed to the parent PyObject class.
 */
PyMetaParticleSystem::PyMetaParticleSystem( MetaParticleSystemPtr pSystem, PyTypeObject *pType ) :
	PyAttachment( pType ),
	pSystem_( pSystem ),
	dirty_( true ),
	lod_( 0.0f )
{
	BW_GUARD;
	//since PyMetaParticleSystem is a wrapper around a MetaParticleSystem,
	//it must always have a valid object to delegate to.
	IF_NOT_MF_ASSERT_DEV( pSystem.hasObject() )
	{
		MF_EXIT( "PyMetaParticleSystem: failed to allocate memory for particle system." );
	}
}


/**
 *	Python-accessible duration method
 */
float PyMetaParticleSystem::duration() const
{
	BW_GUARD;
	return pSystem_->duration();
}


/**
 *	Python-accessible nSystems method
 */
uint32 PyMetaParticleSystem::nSystems() const
{
	BW_GUARD;
	return pSystem_->nSystems();
}


/**
 *	Python-accessible update method
 */
PyObject * PyMetaParticleSystem::py_update( PyObject *args )
{
	BW_GUARD;
	float dTime = 0.02f;

	if ( !PyArg_ParseTuple( args, "f", &dTime ) )
	{
		PyErr_SetString( PyExc_TypeError, "MetaParticleSystem::py_update expects a dTime" );
		return NULL;
	}

	pSystem_->tick( dTime );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible render method
 */
PyObject * PyMetaParticleSystem::py_render( PyObject *args )
{
	BW_GUARD;
	// FIXME: yet another render from python function
	// create a temporary draw context, pass it in and flush after draw
	Moo::DrawContext tempDrawContext( Moo::RENDERING_PASS_COLOR );
	tempDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
	pSystem_->draw( tempDrawContext, Matrix::identity, 0.f );
	tempDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
	tempDrawContext.flush( Moo::DrawContext::ALL_CHANNELS_MASK );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible load method
 */
PyObject * PyMetaParticleSystem::py_load( PyObject *args )
{
	BW_GUARD;
	char * name;

	if ( !PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_SetString( PyExc_TypeError, "MetaParticleSystem::py_load expects a filename" );
		return NULL;
	}
	char msgBuffer[ 512 ];
	StringBuilder errorMsg( msgBuffer, sizeof(msgBuffer) );
	pSystem_->load( name, "particles", &errorMsg );
	if (errorMsg.length() != 0)
	{
		ERROR_MSG( "%s\n", errorMsg.string() );
	}

	Py_RETURN_NONE;
}


/**
 *	Python-accessible save method
 */
PyObject * PyMetaParticleSystem::py_save( PyObject *args )
{
	BW_GUARD;
	char * name;

	if ( !PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_SetString( PyExc_TypeError, "MetaParticleSystem::py_save expects a filename" );
		return NULL;
	}

	pSystem_->save( name, "particles" );
	Py_RETURN_NONE;
}


/**
 *	Python-accessible wrapper for the spawn method
 */
PyObject * PyMetaParticleSystem::py_force( PyObject* args )
{
	BW_GUARD;
	int num=1;
	if (!PyArg_ParseTuple( args, "|i", &num ))
	{
		PyErr_SetString( PyExc_TypeError, "Pixie.Force: "
			"Argument parsing error: Expected an optional number of units to force" );
		return NULL;
	}
	pSystem_->spawn(num);
	Py_RETURN_NONE;
}


/**
 *	Python-accessible wrapper for the clear method
 */
PyObject * PyMetaParticleSystem::py_clear( PyObject* args )
{
	BW_GUARD;
	pSystem_->clear();
	Py_RETURN_NONE;
}


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. None are
 *					expected or checked.
 */
PyObject *PyMetaParticleSystem::pyNew( PyObject *args )
{
	BW_GUARD;
	MetaParticleSystemPtr pSystem = new MetaParticleSystem;
	return new PyMetaParticleSystem( pSystem );
}


/**
 *	This Python method allows the script to add a system to the meta particle
 *	system.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a reference to an instance of ParticleSystem.
 */
PyObject *PyMetaParticleSystem::py_addSystem( PyObject *args )
{
	BW_GUARD;
	PyObject *pNewSystem;

	if ( PyArg_ParseTuple( args, "O", &pNewSystem ) )
	{
		if ( PyParticleSystem::Check( pNewSystem ) )
		{
			PyParticleSystem* pPPS = static_cast<PyParticleSystem*>( pNewSystem );			
			pSystem_->addSystem( pPPS->pSystem() );
			Py_RETURN_NONE;
		}
	}

	PyErr_SetString( PyExc_TypeError, "PyMetaParticleSystem.addSystem: "
		"PyParticleSystem expected." );
	return NULL;
}


/**
 *	This Python method allows the script to remove a system from the meta
 *	particle system.
 *
 *	@param args		The list of parameters passed from Python. This could
 *					either be the name of the system to be removed
 *					or a reference to an instance of ParticleSystem.
 */
PyObject *PyMetaParticleSystem::py_removeSystem( PyObject *args )
{
	BW_GUARD;
	if ( PyTuple_Size( args ) == 1 )
	{
		// The following is a borrowed reference so there is no need to
		// decrement the reference count for the pointer.
		PyObject * pFirstItem = PyTuple_GetItem( args, 0 );

		ParticleSystemPtr systemToRemove = NULL;

		if ( PyParticleSystem::Check( pFirstItem ) )
		{
			PyParticleSystem* pPPS = static_cast<PyParticleSystem*>( pFirstItem );
			systemToRemove = pPPS->pSystem();
		}
		else
		{
			BW::string systemNameArray;
			if ( Script::setData( pFirstItem, systemNameArray ) == 0 )
			{
				systemToRemove = pSystem_->system( systemNameArray );
			}
		}

		pSystem_->removeSystem(systemToRemove);
		Py_RETURN_NONE;
	}

	PyErr_SetString( PyExc_TypeError, "PyParticleSystem.removeSystem: "
		"system name (char *) or PyParticleSystem reference expected." );
	return NULL;
}


/**
 *	This Python method allows the script to access a system in the meta particle
 *	system.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a string name of the system.
 *
 *	@return	A reference to the ParticleSystem if found or None if none of that type of
 *			action belongs in the particle system.
 */
PyObject *PyMetaParticleSystem::py_system( PyObject *args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) == 1)
	{
		ParticleSystemPtr pSys = NULL;

		PyObject * pItem = PyTuple_GetItem( args, 0 );
		if (PyString_Check( pItem ))
		{
			pSys = pSystem_->system(PyString_AsString(pItem));
		}

		if (PyInt_Check( pItem ))
		{
			int val = PyInt_AsLong( pItem );
			if (val >= 0 && val < int(pSystem_->nSystems()))
				pSys = pSystem_->systemFromIndex(val);
		}

		if (pSys)
		{			
			SmartPointer<PyParticleSystem> pPySys( new PyParticleSystem(pSys),
				SmartPointer<PyParticleSystem>::STEAL_REFERENCE );
			return Script::getData(pPySys.get());
		}
		else
		{
			return Script::getData( (PyParticleSystem*)NULL );
		}
	}


	PyErr_SetString( PyExc_TypeError, "PyParticleSystem.system: "
		"system name (char *) or system index (int) expected." );

	return NULL;
}


/**
 *	This Python method allows the script to tell the meta particle system to
 *	remove all particle systems from itself.
 *
 *	@param args		The list of parameters passed from Python. None are
 *					expected or checked.
 */
PyObject *PyMetaParticleSystem::py_removeAllSystems( PyObject *args )
{
	BW_GUARD;
	pSystem_->removeAllSystems();
	Py_RETURN_NONE;
}


/**
 *	Tick the particle systems
 *
 *	@param dTime	Time since last update in seconds.
 */
void PyMetaParticleSystem::tick( float dTime )
{
	pSystem_->tick( dTime );
	dirty_ = true;
}


/**
 *	Check if this PyMetaParticleSystem needs to be updated this frame.
 *	@return true if we have not been updated this frame or false if
 *		we don't need to. Eg. if we were already updated or have lodded.
 */
bool PyMetaParticleSystem::needsUpdate() const
{
	BW_GUARD;
	return dirty_;
}


#if ENABLE_TRANSFORM_VALIDATION
/**
 *	Check if this system's animations have been updated this frame.
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this system needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool PyMetaParticleSystem::isTransformFrameDirty( uint32 frameTimestamp ) const
{
	BW_GUARD;
	return dirty_;
}


/**
 *	Check if this system's animations have been updated this frame.
 *
 *	Use this check when checking for if we need to update.
 *	ie. want to update lods.
 *
 *	@param frameTimestamp the timestamp for this frame.
 *	@return true if this system needs animating this frame,
 *		false if it has already been marked as animated.
 */
bool PyMetaParticleSystem::isVisibleTransformFrameDirty(
	uint32 frameTimestamp ) const
{
	BW_GUARD;
	return dirty_;
}
#endif // ENABLE_TRANSFORM_VALIDATION


/**
 *	Check if this if visible or lodded out.
 *	@return true if this object is visible, false if lodded out.
 */
bool PyMetaParticleSystem::isLodVisible() const
{
	return pSystem_->isLodVisible( lod_ );
}


/**
 *	Re-calculate LOD and apply animations to this PyMetaParticleSystem for this
 *	frame. Should be called once per frame.
 *	@param worldTransform the world transform.
 *	@param lod the lod to use for drawing.
 */
void PyMetaParticleSystem::updateAnimations( const Matrix & worldTransform,
	float lod )
{
	BW_GUARD;
	lod_ = lod;
	dirty_ = false;
}


/**
 *	This methods tells the particle system to draw itself.
 *
 *	Particles are stored in world coordinates so we do not need
 *	the attachment transform which is passed in.
 */
void PyMetaParticleSystem::draw( Moo::DrawContext& drawContext, const Matrix &world )
{
	BW_GUARD;
	TRANSFORM_ASSERT( !this->isVisibleTransformFrameDirty(
		Moo::rc().frameTimestamp() ) );
	pSystem_->draw( drawContext, world, lod_ );
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::localBoundingBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;
	pSystem_->localBoundingBox( bb );	
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::localVisibilityBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;
	pSystem_->localVisibilityBoundingBox( bb );
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny )
{
	BW_GUARD;
	pSystem_->worldBoundingBox( bb, world );
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::worldVisibilityBox( BoundingBox & wvbb, const Matrix& world, bool skinny )
{
	BW_GUARD;
	BoundingBox vbb;
	pSystem_->worldVisibilityBoundingBox( wvbb );
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
bool PyMetaParticleSystem::attach( MatrixLiaison * pOwnWorld )
{
	BW_GUARD;
	bool ret = this->PyAttachment::attach( pOwnWorld );
	ret &= pSystem_->attach( pOwnWorld );
	return ret;
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::detach()
{
	BW_GUARD;
	this->PyAttachment::detach();
	pSystem_->detach();
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::tossed( bool outside )
{
	BW_GUARD;
	this->PyAttachment::tossed(outside);
	pSystem_->tossed(outside);
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::enterWorld()
{
	BW_GUARD;
	this->PyAttachment::enterWorld();
	pSystem_->enterWorld();
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::leaveWorld()
{
	BW_GUARD;
	this->PyAttachment::leaveWorld();
	pSystem_->leaveWorld();	
}


/**
 *	This method overrides PyAttachment and delegates directly to our system.
 */
void PyMetaParticleSystem::move( float dTime )
{
	BW_GUARD;
	this->PyAttachment::move(dTime);
	pSystem_->move(dTime);
}


/**
 *	This method is python API and is described in the python API document.
 */
static PyObject* py_prerequisites( PyObject* args )
{
	BW_GUARD;
	PyObject * pPyDS;
	if (!PyArg_ParseTuple( args, "O", &pPyDS) ||
		!PyDataSection::Check(pPyDS))
	{
		PyErr_SetString( PyExc_TypeError, "Expected a PyDataSection.");
		return NULL;
	}

	DataSectionPtr pSection = static_cast<PyDataSection*>(pPyDS)->pSection();

	PyObject * ret = NULL;
	BW::set<BW::string> prereqs;

	MetaParticleSystem::prerequisites(pSection, prereqs);

	PyObject * result = PyTuple_New( prereqs.size() );
	size_t i = 0;
	BW::set<BW::string>::iterator it = prereqs.begin();
	BW::set<BW::string>::iterator end = prereqs.end();
	while (it != end)
	{
		PyTuple_SetItem( result, i, Script::getData( *it++ ) );
		i++;
	}

	return result;
}


/*~ function Pixie.prerequisites
 *	This function returns a tuple of all the resources required to load
 *	a MetaParticleSystem.
 *
 *	The XML passed in should be either a ParticleSystem or a Meta-
 *	ParticleSystem.
 *	@param pSection Data Section of the particle system.
 */
PY_MODULE_FUNCTION( prerequisites, Pixie )


BW_END_NAMESPACE

// py_meta_particle_system.cpp
