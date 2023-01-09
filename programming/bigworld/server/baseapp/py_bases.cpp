#include "script/first_include.hpp"

#include "py_bases.hpp"

#include "base.hpp"
#include "bases.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyBases::s_subscript( PyObject * self, PyObject * index )
{
	return ((PyBases *) self)->subscript( index );
}


/**
 * 	This function returns the number of bases in this manager.
 */
Py_ssize_t PyBases::s_length( PyObject * self )
{
	return ((PyBases *) self)->length();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods g_basesMapping =
{
	PyBases::s_length,		// mp_length
	PyBases::s_subscript,	// mp_subscript
	NULL						// mp_ass_subscript
};

/*~ class NoModule.PyBases
 *  @components{ base }
 *  An instance of PyBases emulates a dictionary of Base Entity objects,
 *  indexed by their id attribute.  It does not support item assignment, but can
 *  be used with the subscript operator.  Note that the key must be an integer,
 *  and that a key error will be thrown if the key does not exist in the
 *  dictionary.  Instances of this class are used by the engine to make
 *  lists of Base Entities available to Python.  They cannot be created via script,
 *  nor can they be modified.
 *
 *  Code Example (on baseapp machine, eg, telnet 192.168.0.3:40001):
 *
 *		e = BigWorld.entities		# this is a PyBases object
 *		print "The entity with ID 100 is", e[ 100 ]
 *		print "There are", len( e ), "entities"
 */
PY_TYPEOBJECT_WITH_MAPPING( PyBases, &g_basesMapping )

PY_BEGIN_METHODS( PyBases )

	/*~ function PyBases has_key
	 *  @components{ base }
	 *  has_key reports whether a Base Entity with a specific ID
	 *  is listed in this PyBases object.
	 *  @param key key is the integer key to be searched for.
	 *  @return A boolean. True if the key was found, false if it was not.
	 */
	PY_METHOD( has_key )

	/*~ function PyBases keys
	 *  @components{ base }
	 *  keys returns a list of the IDs of all of the Base Entities listed in
	 *  this object.
	 *  @return A list containing all of the keys.
	 */
	PY_METHOD( keys )

	/*~ function PyBases items
	 *  @components{ base }
	 *  items returns a list of the items, as (ID, Base Entity) pairs, that are listed
	 *  in this object.
	 *  @return A list containing all of the (ID, Base Entity) pairs, represented as
	 *  tuples containing an integer ID and a Base Entity.
	 */
	PY_METHOD( items )

	/*~ function PyBases values
	 *  @components{ base }
	 *  values returns a list of all the Base Entities listed in
	 *  this object.
	 *  @return A list containing all of the Base Entities.
	 */
	PY_METHOD( values )

	/*~ function PyBases get
	 *  @components{ base }
	 *	@param id The id of the entity to find.
	 *	@param defaultValue An optional argument that is the return value if the
	 *		entity cannot be found. This defaults to None.
	 *	@return The entity with the input id or the default value if not found.
	 */
	PY_METHOD( get )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBases )
PY_END_ATTRIBUTES()


/**
 *	The constructor for PyBases.
 */
PyBases::PyBases( const Bases & bases, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	bases_( bases )
{
}


/**
 *	This method finds the entity with the input ID.
 *
 * 	@param	entityID 	The ID of the entity to locate.
 *
 *	@return	The object associated with the given entity ID.
 */
PyObject * PyBases::subscript( PyObject* entityID )
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

	PyObject * pBase = bases_.findEntity( id );
	Py_XINCREF( pBase );

	if (pBase == NULL)
	{
		PyErr_Format( PyExc_KeyError, "%d", int(id) );
		return NULL;
	}

	return pBase;
}


/**
 *	This method returns a Base entity that has a type that matches the string
 *	that is passed in.
 */
PyObject * PyBases::findInstanceWithType( const char * typeName ) const
{
	Base * pBase = bases_.findInstanceWithType( typeName );

	if (!pBase)
	{
		PyErr_Format( PyExc_KeyError, "Could not find an entry with type %s",
				typeName );
		return NULL;
	}

	Py_INCREF( pBase );
	return pBase;
}


/**
 * 	This method returns the number of entities in the system.
 */
int PyBases::length()
{
	return bases_.size();
}


/**
 * 	This method returns true if the given entity exists.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * PyBases::py_has_key( PyObject* args )
{
	long id;

	if (!PyArg_ParseTuple( args, "i", &id ))
		return NULL;

	return PyInt_FromLong( bases_.findEntity( id ) != NULL );
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */
PyObject* PyBases::py_keys(PyObject* /*args*/)
{
	int size = bases_.size();
	PyObject * pList = PyList_New( size );

	Bases::const_iterator iter = bases_.begin();
	int i = 0;

	while (iter != bases_.end())
	{
		// This steals a reference.
		PyList_SetItem( pList, i, PyInt_FromLong( iter->first ) );

		i++;
		iter++;
	}

	return pList;
}


/**
 * 	This method returns a list of all the entity objects in the system.
 */
PyObject* PyBases::py_values(PyObject* /*args*/)
{
	int size = bases_.size();
	PyObject * pList = PyList_New( size );

	Bases::const_iterator iter = bases_.begin();
	int i = 0;

	while (iter != bases_.end())
	{
		// This steals a reference.
		Py_XINCREF( iter->second );
		PyList_SetItem( pList, i, iter->second );

		i++;
		iter++;
	}

	return pList;
}


/**
 * 	This method returns a list of tuples of all the base IDs and objects on the
 *	BaseApp.
 */
PyObject* PyBases::py_items( PyObject* /*args*/ )
{
	int size = bases_.size();
	PyObject * pList = PyList_New( size );

	Bases::const_iterator iter = bases_.begin();
	int i = 0;

	while (iter != bases_.end())
	{
		PyObject * pTuple = PyTuple_New( 2 );
		PyObject * pValue = iter->second;
		Py_INCREF( pValue );

		PyTuple_SetItem( pTuple, 0, PyInt_FromLong( iter->first ) );
		PyTuple_SetItem( pTuple, 1, pValue );

		// This steals a reference.
		PyList_SetItem( pList, i, pTuple );

		i++;
		iter++;
	}

	return pList;
}


/**
 *	This method returns the entity with the input index. If this entity cannot
 *	be found, the default value is returned.
 */
PyObject * PyBases::py_get( PyObject * args )
{
	PyObject * pDefault = Py_None;
	int id = 0;
	if (!PyArg_ParseTuple( args, "i|O", &id, &pDefault ))
	{
		return NULL;
	}

	PyObject * pEntity = bases_.findEntity( id );

	if (!pEntity)
	{
		pEntity = pDefault;
	}

	Py_INCREF( pEntity );
	return pEntity;
}

BW_END_NAMESPACE

// py_bases.cpp
