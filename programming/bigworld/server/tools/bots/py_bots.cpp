#include "script/first_include.hpp"

#include "py_bots.hpp"
#include "client_app.hpp"
#include "main_app.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyBots::s_subscript( PyObject * self, PyObject * index )
{
	return ((PyBots *) self)->subscript( index );
}


/**
 * 	This function returns the number of entities in the system.
 */
Py_ssize_t PyBots::s_length( PyObject * self )
{
	return ((PyBots *) self)->length();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods g_clientAppsMapping =
{
	PyBots::s_length,		// mp_length
	PyBots::s_subscript,	// mp_subscript
	NULL						// mp_ass_subscript
};

PY_TYPEOBJECT_WITH_MAPPING( PyBots, &g_clientAppsMapping )

PY_BEGIN_METHODS( PyBots )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBots )
PY_END_ATTRIBUTES()


/**
 *	The constructor for PyBots.
 */
PyBots::PyBots( PyTypeObject * pType ) :
	PyObjectPlus( pType )
{
}


/**
 *	This method finds the client application with the input ID.
 *
 * 	@param	entityID 	The ID of the entity to locate.
 *
 *	@return	The object associated with the given entity ID.
 */
PyObject * PyBots::subscript( PyObject* entityID )
{
	long id = PyInt_AsLong( entityID );

	if (PyErr_Occurred())
		return NULL;

	ClientAppPtr pEntity = MainApp::instance().findApp( id );
	//Py_XINCREF( pEntity ); findApp already incremented

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_KeyError, "%ld", id );
		return NULL;
	}

	return pEntity.newRef();
}


/**
 * 	This method returns the number of client apps (bots) in the system.
 */
int PyBots::length()
{
	PyObject* pList = PyList_New(0);

	// This could be done more much more efficiently by adding a method
	// to CellApp to count entities.

	MainApp::instance().appsKeys( pList );

	int len = PyList_Size(pList);
	Py_DECREF(pList);
	return len;
}


/**
 * 	This method returns true if the given entity exists.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * PyBots::py_has_key( PyObject* args )
{
	long id;

	if (!PyArg_ParseTuple( args, "i", &id ))
		return NULL;

	if (MainApp::instance().findApp( id ))
		return PyInt_FromLong( 1 );
	else
		return PyInt_FromLong( 0 );
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */
PyObject* PyBots::py_keys(PyObject* /*args*/)
{
	PyObject* pList = PyList_New( 0 );
	MainApp::instance().appsKeys( pList );
	return pList;
}


/**
 * 	This method returns a list of all the entity objects in the system.
 */
PyObject* PyBots::py_values( PyObject* /*args*/ )
{
	PyObject* pList = PyList_New( 0 );
	MainApp::instance().appsValues( pList );
	return pList;
}


/**
 * 	This method returns a list of tuples of all the entity IDs
 *	and objects in the system.
 */
PyObject* PyBots::py_items( PyObject* /*args*/ )
{
	PyObject* pList = PyList_New( 0 );
	MainApp::instance().appsItems( pList );
	return pList;
}

BW_END_NAMESPACE

// py_bots.cpp
