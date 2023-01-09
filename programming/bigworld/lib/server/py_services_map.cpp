#include "script/first_include.hpp"

#include "entitydef/mailbox_base.hpp"

#include "py_services_map.hpp"


BW_BEGIN_NAMESPACE


namespace // (anonymous)
{


/**
 *	This function implements subscript operator.
 */
PyObject * s_subscript( PyObject * self, PyObject * index )
{
	PyServicesMap * pPyServicesMap = 
		reinterpret_cast< PyServicesMap * >( self );

	BW::string serviceName;

	if (!PyString_Check( index ))
	{
		PyErr_SetString( PyExc_TypeError, 
			"Invalid type for subscript index, expected string or unicode" );
		return NULL;
	}

	serviceName.assign( PyString_AS_STRING( index ),
		PyString_Size( index ) );

	EntityMailBoxRef mailBoxRef;
	if (!pPyServicesMap->map().chooseFragment( serviceName, mailBoxRef ))
	{
		PyErr_SetString( PyExc_KeyError, serviceName.c_str() );
		return NULL;
	}

	return Script::getData( mailBoxRef );
}


/**
 *	This function implements the len() script operator.
 */
Py_ssize_t s_length( PyObject * self )
{
	PyServicesMap * pPyServicesMap = 
		reinterpret_cast< PyServicesMap * >( self );
	return pPyServicesMap->map().size();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
PyMappingMethods s_PyServicesMapMethods=
{
	&s_length,					// mp_length
	&s_subscript,				// mp_subscript
	NULL						// mp_ass_subscript
};

} // end namespace (anonymous)


PY_TYPEOBJECT_WITH_MAPPING( PyServicesMap, &s_PyServicesMapMethods )

PY_BEGIN_METHODS( PyServicesMap )


/*~ function PyServicesMap has_key
 *  @components{ base, cell }
 *  has_key reports whether a service with the given name exists.
 *
 *  @param key 	key is the string key to be searched for.
 *  @return		A boolean. True if the key was found, false if it was not.
 */
	PY_METHOD( has_key )


/*~ function PyServicesMap keys
 *  @components{ base, cell }
 *  keys returns a list of the names of all of the services.
 *  @return A list containing all of the service names.
 */
	PY_METHOD( keys )

/*~ function PyServicesMap items
 *  @components{ base, cell }
 *
 *  items returns a list of the items, as (label, base) pairs, that are listed
 *  in this object.
 *
 *  @return A list containing all of the (serviceName, fragment) pairs,
 *  represented as tuples containing an string label and a mailbox to the
 *  service fragment.
 */
	PY_METHOD( items )


/*~ function PyServicesMap get
 *  @components{ base, cell }
 *
 *  get returns a randomly chosen mailbox to a service fragment with the
 *  given service name, or the default value if none is found.
 *
 *	@param serviceName 		The service name.
 *	@param defaultValue 	Default value to return if the service is not
 *							found, defaults to None.
 *  @return A mailbox to a service fragment, or the default value if not found.
 */
	PY_METHOD( get )

/*~ function PyServicesMap.allFragmentsFor
 *	@components{ base, cell }
 *
 *	allFragmentsFor returns a list of all registered fragments for the given
 *	service.
 *
 *	@param serviceName 		The service name.
 *
 *	@return a list of mailboxes pointing to the service fragments.
 */
	PY_METHOD( allFragmentsFor )

PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( PyServicesMap )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	The constructor for PyServicesMap.
 */
PyServicesMap::PyServicesMap( PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		map_()
{
}


/**
 *	Destructor.
 */
PyServicesMap::~PyServicesMap()
{
}


// -----------------------------------------------------------------------------
// Section: Script Mapping Related
// -----------------------------------------------------------------------------


/**
 * 	This method implements the has_key() script method.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * PyServicesMap::py_has_key( PyObject * args )
{
	char * serviceName = NULL;

	if (!PyArg_ParseTuple( args, "s", &serviceName ))
	{
		return NULL;
	}

	ServicesMap::Names names;
	map_.getServiceNames( names );

	bool hasKey = names.count( serviceName );

	return PyBool_FromLong( hasKey );
}


/**
 * 	This method implements the get() script method.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * PyServicesMap::py_get( PyObject * args )
{
	PyObject * key;
	PyObject * defaultObject = Py_None;

	if (!PyArg_UnpackTuple( args, "get", 1, 2, &key, &defaultObject ))
	{
		return NULL;
	}

	PyObject * pResult = s_subscript( this, key );

	if (!pResult)
	{
		PyErr_Clear();

		Py_INCREF( defaultObject );
		return defaultObject;
	}

	return pResult;
}


/**
 * 	This method returns a list of the names of all registered services.
 */
PyObject * PyServicesMap::py_keys( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "keys() takes no arguments" );
		return NULL;
	}

	ServicesMap::Names names;
	map_.getServiceNames( names );

	PyObject * pList = PyList_New( names.size() );

	Py_ssize_t i = 0;
	ServicesMap::Names::const_iterator iServiceName = names.begin();

	while (iServiceName != names.end())
	{
		PyList_SET_ITEM( pList, i, 
			PyString_FromStringAndSize( 
				iServiceName->data(),
				iServiceName->size() ) );

		++iServiceName;
		++i;
	}

	return pList;
}


/**
 * 	This method returns a list of tuples of all the service names and one of
 * 	its fragments, randomly chosen.
 */
PyObject* PyServicesMap::py_items( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "items() takes no arguments" );
		return NULL;
	}

	ServicesMap::Names names;
	map_.getServiceNames( names );

	PyObject * pList = PyList_New( names.size() );

	Py_ssize_t i = 0;
	ServicesMap::Names::const_iterator iServiceName = names.begin();

	while (iServiceName != names.end())
	{
		EntityMailBoxRef mailBoxRef;
		MF_VERIFY( map_.chooseFragment( *iServiceName, mailBoxRef ) );

		// see BWT-27448
		//ScriptObject mailBox = ScriptObject::createFrom( mailBoxRef );

		ScriptObject mailBox( Script::getData( mailBoxRef ),
			ScriptObject::FROM_NEW_REFERENCE );
		MF_ASSERT( mailBox.exists() );

		ScriptString serviceName = ScriptString::create( iServiceName->data(),
			iServiceName->size() );

		ScriptTuple servicePair = ScriptTuple::create( 2 );
		MF_VERIFY( servicePair.setItem( 0, serviceName ) );
		MF_VERIFY( servicePair.setItem( 1, mailBox ) );

		// PyList_SET_ITEM steals a reference to its argument
		PyList_SET_ITEM( pList, i, servicePair.newRef() );

		++iServiceName;
		++i;
	}

	return pList;
}


/**
 *	This method implements the script call allFragmentsFor().
 */
PyObject * PyServicesMap::py_allFragmentsFor( PyObject * args )
{
	char * serviceName = NULL;
	if (!PyArg_ParseTuple( args, "s", &serviceName ))
	{
		return NULL;
	}

	ServicesMap::FragmentMailBoxes mailBoxes;

	map_.getFragmentsForService( serviceName, mailBoxes );

	PyObject * pList = PyList_New( mailBoxes.size() );

	ServicesMap::FragmentMailBoxes::const_iterator iMailBox = mailBoxes.begin();
	Py_ssize_t i = 0;
	while (iMailBox != mailBoxes.end())
	{
		PyList_SET_ITEM( pList, i, Script::getData( *iMailBox ) );

		++iMailBox;
		++i;
	}

	return pList;
}


BW_END_NAMESPACE


// py_services_map.cpp
