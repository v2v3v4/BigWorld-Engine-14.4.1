#include "script/first_include.hpp"

#include "py_entities.hpp"

#include "bots_config.hpp"
#include "entity.hpp"

#include "connection_model/bw_entities.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_entities.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: PyEntities
// -----------------------------------------------------------------------------

/**
 *	The constructor for PyEntities.
 */
PyEntities::PyEntities( const BWEntities & entities, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	entities_( entities )
{
	MF_ASSERT( BotsConfig::shouldUseScripts() );
}


/**
 *	Destructor.
 */
PyEntities::~PyEntities()
{
}


/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyEntities::s_subscript( PyObject * self, PyObject * index )
{
	return ((PyEntities *) self)->subscript( index );
}


/**
 * 	This function returns the number of entities in the system.
 */
Py_ssize_t PyEntities::s_length( PyObject * self )
{
	return ((PyEntities *) self)->length();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods g_entitiesMapping =
{
	PyEntities::s_length,		// mp_length
	PyEntities::s_subscript,	// mp_subscript
	NULL						// mp_ass_subscript
};


PY_TYPEOBJECT_WITH_MAPPING( PyEntities, &g_entitiesMapping )

/*~ function PyEntities has_key
 *  @components{ bots }
 *  Used to determine whether an entity with a specific ID
 *  is listed in this PyEntities object.
 *  @param key An integer key to look for.
 *  @return A boolean. True if the key was found, false if it was not.
 */
/*~ function PyEntities keys
 *  @components{ bots }
 *  Generates a list of the IDs of all of the entities listed in
 *  this object.
 *  @return A list containing all of the keys, represented as integers.
 */
/*~ function PyEntities items
 *  @components{ bots }
 *  Generates a list of the items (ID:entity pairs) listed in
 *  this object.
 *  @return A list containing all of the ID:entity pairs, represented as tuples
 *  containing one integer, and one entity.
 */
/*~ function PyEntities values
 *  @components{ bots }
 *  Generates a list of all the entities listed in
 *  this object.
 *  @return A list containing all of the entities.
 */
/*~ function PyEntities get
 *  @components{ bots }
 *	@param id The id of the entity to find.
 *	@param defaultValue An optional argument that is the return value if the
 *		entity cannot be found. This defaults to None.
 *	@return The entity with the input id or the default value if not found.
 */
PY_BEGIN_METHODS( PyEntities )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )
	PY_METHOD( get )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEntities )
PY_END_ATTRIBUTES()


/**
 *	This method finds the entity with the input ID.
 *
 * 	@param	entityID 	The ID of the entity to locate.
 *
 *	@return	The object associated with the given entity ID.
 */
PyObject * PyEntities::subscript( PyObject* entityID )
{
	EntityID id = PyInt_AsLong( entityID );

	if (PyErr_Occurred())
		return NULL;

	Entity * pEntity = static_cast< Entity * >( entities_.find( id ) );

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_KeyError, "%d", id );
		return NULL;
	}

	MF_ASSERT( !pEntity->isDestroyed() );

	PyObject * pPyEntity = pEntity->pPyEntity().get();
	Py_INCREF( pPyEntity );
	return pPyEntity;
}


/**
 * 	This method returns the number of entities in the system.
 */
Py_ssize_t PyEntities::length()
{
	return entities_.size();
}


/**
 * 	This method returns true if the given entity exists.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * PyEntities::py_has_key( PyObject* args )
{
	long id;

	if (!PyArg_ParseTuple( args, "i", &id ))
	{
		return NULL;
	}

	const bool hasKey = (entities_.find( id ) != NULL);

	return PyInt_FromLong( hasKey );
}


namespace {
/**
 *	This function is used py_keys. It creates an object that goes into the keys
 *	list based on an element in the Entities map.
 */
static PyObject * getEntityID( const BWEntities::const_iterator & iEntity )
{
	return PyInt_FromLong( iEntity->first );
}


/**
 *	This function is used py_values. It creates an object that goes into the
 *	values list based on an element in the Entities map.
 */
static PyObject * getEntityReference(
	const BWEntities::const_iterator & iEntity )
{
	BWEntityPtr pBWEntity = iEntity->second;

	MF_ASSERT( !pBWEntity->isDestroyed() );

	Entity * pEntity = static_cast< Entity * >( pBWEntity.get() );

	PyObject * pPyEntity = pEntity->pPyEntity().get();
	Py_INCREF( pPyEntity );
	return pPyEntity;
}


/**
 *	This function is used py_items. It creates an object that goes into the
 *	items list based on an element in the Entities map.
 */
static PyObject * getItem( const BWEntities::const_iterator & iEntity )
{
	ScriptTuple result = ScriptTuple::create( 2 );
	ScriptObject entityID( getEntityID( iEntity ), ScriptObject::FROM_NEW_REFERENCE );
	ScriptObject entityRef( getEntityReference( iEntity ), ScriptObject::FROM_NEW_REFERENCE );
	MF_VERIFY( result.setItem( 0, entityID ) );
	MF_VERIFY( result.setItem( 1, entityRef ) );
	return result.newRef();
}
} // anonymous namespace


/**
 *	This method is a helper used by py_keys, py_values and py_items. It returns
 *	a list, each element is created by using the input function.
 */
PyObject * PyEntities::makeList( GetFunc objectFunc )
{
	PyObject* pList = PyList_New( this->length() );
	int i = 0;

	BWEntities::const_iterator iEntity = entities_.begin();

	while (iEntity != entities_.end())
	{
		PyList_SET_ITEM( pList, i, objectFunc( iEntity ) );

		++i;
		++iEntity;
	}

	return pList;
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */
PyObject* PyEntities::py_keys( PyObject * /*args*/ )
{
	return this->makeList( getEntityID );
}


/**
 * 	This method returns a list of all the entity objects in the system.
 */
PyObject* PyEntities::py_values( PyObject * /*args*/ )
{
	return this->makeList( getEntityReference );
}


/**
 * 	This method returns a list of tuples of all the entity IDs
 *	and objects in the system.
 */
PyObject* PyEntities::py_items( PyObject * /*args*/ )
{
	return this->makeList( getItem );
}


/**
 *	This method returns the entity with the input index. If this entity cannot
 *	be found, the default value is returned.
 */
PyObject * PyEntities::py_get( PyObject * args )
{
	PyObject * pDefault = Py_None;
	int id = 0;
	if (!PyArg_ParseTuple( args, "i|O", &id, &pDefault ))
	{
		return NULL;
	}

	Entity * pEntity = static_cast< Entity * >( entities_.find( id ) );
	PyObject * pPyEntity = pDefault;

	if (pEntity != NULL)
	{
		MF_ASSERT( !pEntity->isDestroyed() );

		pPyEntity = pEntity->pPyEntity().get();
	}

	Py_INCREF( pPyEntity );
	return pPyEntity;
}

BW_END_NAMESPACE

// py_entities.cpp
