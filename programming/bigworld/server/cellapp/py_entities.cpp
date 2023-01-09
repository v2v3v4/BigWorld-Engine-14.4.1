#include "script/first_include.hpp"

#include "py_entities.hpp"

#include "cellapp.hpp"
#include "entity.hpp"
#include "entity_population.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

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

/*~ function PyEntities has_key
 *  @components{ cell }
 *  has_key reports whether an entity with a specific ID
 *  is listed in this PyEntities object.
 *  @param key key is the integer key to be searched for.
 *  @return A boolean. True if the key was found, false if it was not.
 */
/*~ function PyEntities keys
 *  @components{ cell }
 *  keys returns a list of the IDs of all of the entities listed in
 *  this object.
 *  @return A list containing all of the keys.
 */
/*~ function PyEntities items
 *  @components{ cell }
 *  items returns a list of the items, as (ID, entity) pairs, that are listed
 *  in this object.
 *  @return A list containing all of the (ID, entity) pairs, represented as
 *  tuples containing an integer ID and an entity.
 */
/*~ function PyEntities values
 *  @components{ cell }
 *  values returns a list of all the entities listed in
 *  this object.
 *  @return A list containing all of the entities.
 */
/*~ function PyEntities get
 *	@components{ cell }
 *	@param id The id of the entity to find.
 *	@param defaultValue An optional argument that is the return value if the
 *		entity cannot be found. This defaults to None.
 *	@return The entity with the input id or the default value if not found.
 */
PY_TYPEOBJECT_WITH_MAPPING( PyEntities, &g_entitiesMapping )

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
 *	The constructor for PyEntities.
 */
PyEntities::PyEntities( PyTypeObject * pType ) :
	PyObjectPlus( pType )
{
}


/**
 *	This method finds the entity with the input ID.
 *
 * 	@param	entityID 	The ID of the entity to locate.
 *
 *	@return	The object associated with the given entity ID.
 */
PyObject * PyEntities::subscript( PyObject* entityID )
{
	long id = PyInt_AsLong( entityID );

	if (PyErr_Occurred())
	{
		if (PyString_Check( entityID ))
		{
			PyErr_Clear();
			return this->findInstanceWithType( PyString_AsString( entityID ) );
		}

		return NULL;
	}

	PyObject * pEntity = CellApp::instance().findEntity( id );
	Py_XINCREF( pEntity );

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_KeyError, "%ld", id );
		return NULL;
	}

	return pEntity;
}


/**
 *	This method returns a Base entity that has a type that matches the string
 *	that is passed in.
 */
PyObject * PyEntities::findInstanceWithType( const char * typeName ) const
{
	EntityPopulation::const_iterator iter = Entity::population().begin();

	while (iter != Entity::population().end())
	{
		Entity * pEntity = iter->second;

		if (strcmp( pEntity->pType()->name(), typeName ) == 0)
		{
			Py_INCREF( pEntity );
			return pEntity;
		}

		++iter;
	}

	PyErr_Format( PyExc_KeyError, "Could not find an entry with type %s",
			typeName );
	return NULL;
}


/**
 * 	This method returns the number of entities in the system.
 */
int PyEntities::length()
{
	PyObject* pList = PyList_New(0);

	// This could be done more much more efficiently by adding a method
	// to CellApp to count entities.

	CellApp::instance().entityKeys(pList);

	int len = PyList_Size(pList);
	Py_DECREF(pList);
	return len;
}


/**
 *	This method returns True if the given entity exists.
 *
 * 	@param args		A Python tuple containing the arguments.
 */
PyObject * PyEntities::py_has_key(PyObject* args)
{
	long id;

	if (!PyArg_ParseTuple( args, "i", &id ))
		return NULL;

	return PyBool_FromLong( CellApp::instance().findEntity( id ) != NULL );
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */
PyObject* PyEntities::py_keys(PyObject* /*args*/)
{
	PyObject* pList = PyList_New( 0 );
	CellApp::instance().entityKeys( pList );
	return pList;
}


/**
 *	This method returns a list of all the entity objects in the system.
 */
PyObject* PyEntities::py_values( PyObject* /*args*/ )
{
	PyObject* pList = PyList_New( 0 );
	CellApp::instance().entityValues( pList );
	return pList;
}


/**
 * 	This method returns a list of tuples of all the entity IDs
 *	and objects in the system.
 */
PyObject* PyEntities::py_items( PyObject* /*args*/ )
{
	PyObject* pList = PyList_New( 0 );
	CellApp::instance().entityItems( pList );
	return pList;
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

	PyObject * pEntity = CellApp::instance().findEntity( id );

	if (!pEntity)
	{
		pEntity = pDefault;
	}

	Py_INCREF( pEntity );
	return pEntity;
}

BW_END_NAMESPACE

// py_entities.cpp
