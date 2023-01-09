#include "pch.hpp"

#include "py_entities.hpp"

#include "entity_manager.hpp"
#include "py_entity.hpp"

#include "connection_model/bw_connection.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_entities.ipp"
#endif

/*~ attribute BigWorld entities
 *  This lists all of the entities currently in the world. It does not include
 *	entities the server has informed the client about but which the client does
 *	not have sufficient information about to bring into the world.
 *
 *  @type Read-Only PyEntities.
 */
PY_MODULE_ATTRIBUTE( BigWorld, entities, new PyEntities )

// -----------------------------------------------------------------------------
// Section: PyEntities
// -----------------------------------------------------------------------------

/**
 *	The constructor for PyEntities.
 */
PyEntities::PyEntities( PyTypeObject * pType ) :
	PyObjectPlus( pType )
{
	BW_GUARD;	
}


/**
 *	Destructor.
 */
PyEntities::~PyEntities()
{
	BW_GUARD;	
}


/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyEntities::s_subscript( PyObject * self, PyObject * index )
{
	BW_GUARD;
	return ((PyEntities *) self)->subscript( index );
}


/**
 * 	This function returns the number of entities in the system.
 */
Py_ssize_t PyEntities::s_length( PyObject * self )
{
	BW_GUARD;
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
 *  Used to determine whether an entity with a specific ID
 *  is listed in this PyEntities object.
 *  @param key An integer key to look for.
 *  @return A boolean. True if the key was found, false if it was not.
 */
/*~ function PyEntities keys
 *  Generates a list of the IDs of all of the entities listed in
 *  this object.
 *  @return A list containing all of the keys, represented as integers.
 */
/*~ function PyEntities items
 *  Generates a list of the items (ID:entity pairs) listed in
 *  this object.
 *  @return A list containing all of the ID:entity pairs, represented as tuples
 *  containing one integer, and one entity.
 */
/*~ function PyEntities values
 *  Generates a list of all the entities listed in
 *  this object.
 *  @return A list containing all of the entities.
 */
/*~ function PyEntities get
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
	BW_GUARD;
	long id = PyInt_AsLong( entityID );

	if (PyErr_Occurred())
		return NULL;

	Entity * pEntity = this->findEntity( id );
	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_KeyError, "%d", id );
		return NULL;
	}

	PyObject * pPyEntity = pEntity->pPyEntity().newRef();
	MF_ASSERT( pPyEntity != NULL );
	return pPyEntity;
}


/**
 *	This method returns the entity with the input id.
 */
Entity * PyEntities::findEntity( EntityID id ) const
{
	BW_GUARD;

	Entity * pEntity = ConnectionControl::instance().findEntity( id );

	MF_ASSERT( pEntity == NULL || !pEntity->isDestroyed() );

	return pEntity;
}


/**
 * 	This method returns the number of entities in the system.
 */ 
Py_ssize_t PyEntities::length()
{
	BW_GUARD;

	return ConnectionControl::instance().numEntities();
}


/**
 * 	This method returns true if the given entity exists.
 * 
 * 	@param args		A python tuple containing the arguments.
 */ 
PyObject * PyEntities::py_has_key(PyObject* args)
{
	BW_GUARD;
	long id;

	if (!PyArg_ParseTuple( args, "i", &id ))
		return NULL;

	return PyInt_FromLong( this->findEntity( id ) != NULL );
}


/**
 *	This function is used py_keys. It creates an object that goes into the keys
 *	list based on an element in the Entities map.
 */
PyObject * getKey( Entity * pEntity )
{
	BW_GUARD;
	return PyInt_FromLong( pEntity->entityID() );
}


/**
 *	This function is used py_values. It creates an object that goes into the
 *	values list based on an element in the Entities map.
 */
PyObject * getValue( Entity * pEntity )
{
	BW_GUARD;
	PyObject * pPyEntity = pEntity->pPyEntity().newRef();
	MF_ASSERT( pPyEntity != NULL );
	return pPyEntity;
}


/**
 *	This function is used py_items. It creates an object that goes into the
 *	items list based on an element in the Entities map.
 */
PyObject * getItem( Entity * pEntity )
{
	BW_GUARD;
	PyObject * pTuple = PyTuple_New( 2 );

	PyTuple_SET_ITEM( pTuple, 0, getKey( pEntity ) );
	PyTuple_SET_ITEM( pTuple, 1, getValue( pEntity ) );

	return pTuple;
}


namespace // (anonymous)
{

class EntityListEntityVisitor : public EntityVisitor
{
public:
	typedef PyObject * (*GetFunc)( Entity * pEntity );
	
	/** Constructor. */
	EntityListEntityVisitor( PyObject * pList, GetFunc objectFunc ) :
			pList_( pList ),
			numEntities_( 0 ),
			objectFunc_( objectFunc )
	{}

	/* Override from EntityVisitor. */
	virtual bool onVisitEntity( Entity * pEntity )
	{
		PyList_SET_ITEM( pList_, numEntities_, objectFunc_( pEntity ) );
		++numEntities_;

		return true;
	}

private:
	PyObject * pList_;
	size_t numEntities_;
	GetFunc objectFunc_;
};

} // end namespace (anonymous)

/**
 *	This method is a helper used by py_keys, py_values and py_items. It returns
 *	a list, each element is created by using the input function.
 */
PyObject * PyEntities::makeList( GetFunc objectFunc )
{
	BW_GUARD;

	PyObject* pList = PyList_New( this->length() );

	EntityListEntityVisitor visitor( pList, objectFunc );
	ConnectionControl::instance().visitEntities( visitor );

	return pList;
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */ 
PyObject* PyEntities::py_keys(PyObject* /*args*/)
{
	BW_GUARD;
	return this->makeList( getKey );
}


/**
 * 	This method returns a list of all the entity objects in the system.
 */ 
PyObject* PyEntities::py_values( PyObject* /*args*/ )
{
	BW_GUARD;
	return this->makeList( getValue );
}


/**
 * 	This method returns a list of tuples of all the entity IDs 
 *	and objects in the system.
 */ 
PyObject* PyEntities::py_items( PyObject* /*args*/ )
{
	BW_GUARD;
	return this->makeList( getItem );
}


/**
 *	This method returns the entity with the input index. If this entity cannot
 *	be found, the default value is returned.
 */
PyObject * PyEntities::py_get( PyObject * args )
{
	BW_GUARD;
	PyObject * pDefault = Py_None;
	int id = 0;
	if (!PyArg_ParseTuple( args, "i|O", &id, &pDefault ))
	{
		return NULL;
	}

	Entity * pEntity = this->findEntity( id );
	PyObject * pPyEntity = pDefault;

	if (pEntity)
	{
		pPyEntity = pEntity->pPyEntity().get();
	}

	Py_INCREF( pPyEntity );
	return pPyEntity;
}


int PyEntities_token = 0;

BW_END_NAMESPACE

// py_entities.cpp
