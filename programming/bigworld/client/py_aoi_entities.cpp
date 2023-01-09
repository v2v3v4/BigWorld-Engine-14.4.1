#include "pch.hpp"

#include "py_aoi_entities.hpp"

#include "connection_control.hpp"
#include "entity.hpp"
#include "py_entity.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"

#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

PyMappingMethods g_pyAoIEntitiesMappingMethods = 
{
	PyAoIEntities::pyGetLength,			// mp_length
	PyAoIEntities::pyGetFieldByKey,		// mp_subscript
	NULL								// mp_ass_subscript
										// (we don't support item assignment)
};

} // end namespace (anonymous)

	
PY_TYPEOBJECT_SPECIALISE_ITER( PyAoIEntities,
	&PyAoIEntities::pyGetIter, &PyAoIEntities::pyIterNext )
PY_TYPEOBJECT_SPECIALISE_MAP( PyAoIEntities,
	&g_pyAoIEntitiesMappingMethods )
PY_TYPEOBJECT_SPECIALISE_FLAGS( PyAoIEntities, 
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER )
PY_TYPEOBJECT( PyAoIEntities )

PY_BEGIN_METHODS( PyAoIEntities )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( values )
	PY_METHOD( items )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAoIEntities )

PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyAoIEntities::PyAoIEntities( Entity * pWitness ) :
	PyObjectPlus( &s_type_ ),
	pWitness_( pWitness )
{}


/**
 *	Destructor.
 */
PyAoIEntities::~PyAoIEntities()
{}


/**
 *	This method returns the number of entities in the associated witness's AoI.
 *
 *	@return the number of entities in the associated witness's AoI.
 */
size_t PyAoIEntities::numInAoI() const
{
	if (!this->isValid())
	{
		return 0;
	}
	
	return pWitness_->pBWConnection()->numInAoI( pWitness_->entityID() );
}


/**
 *	This method returns true if this instance is still valid for querying.
 *	This becomes false when the associated witness entity has been destroyed.
 */
bool PyAoIEntities::isValid() const
{
	return !pWitness_->isDestroyed();
}


/**
 *	This method returns whether the given entity ID refers to an entity in the
 *	associated witness's AoI.
 */
PyObject * PyAoIEntities::py_has_key( PyObject * args )
{
	if (!this->isValid())
	{
		PyErr_SetString( PyExc_ValueError, "Associated witness has been destroyed" );
		return NULL;
	}

	EntityID entityID = 0;

	if (!PyArg_ParseTuple( args, "l", &entityID ))
	{
		return NULL;
	}

	if (pWitness_->pBWConnection()->isInAoI( entityID, pWitness_->entityID() ))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}


/**
 *	Helper method for visiting entities in the associated witness's AoI.
 */
bool PyAoIEntities::visitAoI( PyObject * args, AoIEntityVisitor & visitor )
{
	if (!this->isValid())
	{
		PyErr_SetString( PyExc_ValueError,
			"Associated witness has been destroyed" );
		return false;
	}

	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_ValueError, "Unexpected argument" );
		return false;
	}
	
	pWitness_->pBWConnection()->visitAoIEntities( pWitness_->entityID(),
		visitor );

	return true;
}


/**
 *	This method implements the Python keys() method.
 */
PyObject * PyAoIEntities::py_keys( PyObject * args )
{
	ScriptList list = ScriptList::create();

	class EntityIDAoIEntityVisitor : public AoIEntityVisitor
	{

	public:
		EntityIDAoIEntityVisitor( ScriptList list ) :
			list_( list )
		{}

		void onVisitAoIEntity( BWEntity * pBWEntity ) /* override */
		{
			Entity * pEntity = static_cast< Entity * >( pBWEntity );
			list_.append( ScriptObject::createFrom( pEntity->entityID() ) );
		}

	private:
		ScriptList list_;
	}; 

	EntityIDAoIEntityVisitor visitor( list );

	if (!this->visitAoI( args, visitor ))
	{
		return NULL;
	}

	return list.newRef();
}


/**
 *	This method implements the values() method.
 */
PyObject * PyAoIEntities::py_values( PyObject * args )
{
	ScriptList list = ScriptList::create();

	class PyEntityAoIEntityVisitor : public AoIEntityVisitor
	{

	public:
		PyEntityAoIEntityVisitor( ScriptList list ) :
			list_( list )
		{}

		void onVisitAoIEntity( BWEntity * pBWEntity ) /* override */
		{
			Entity * pEntity = static_cast< Entity * >( pBWEntity );
			list_.append( ScriptObject::createFrom( pEntity->pPyEntity() ) );
		}

	private:
		ScriptList list_;
	}; 

	PyEntityAoIEntityVisitor visitor( list );

	if (!this->visitAoI( args, visitor ))
	{
		return NULL;
	}

	return list.newRef();
}


/**
 *	This method implements the items() method.
 */
PyObject * PyAoIEntities::py_items( PyObject * args )
{
	ScriptList list = ScriptList::create();

	class EntityIDPyEntityAoIEntityVisitor : public AoIEntityVisitor
	{

	public:
		EntityIDPyEntityAoIEntityVisitor( ScriptList list ) :
			list_( list )
		{}

		void onVisitAoIEntity( BWEntity * pBWEntity ) /* override */
		{
			Entity * pEntity = static_cast< Entity * >( pBWEntity );
			list_.append( ScriptObject::createFrom(
				Py_BuildValue( "(lO)", 
					pBWEntity->entityID(), pEntity->pPyEntity() ) ) );
		}

	private:
		ScriptList list_;
	}; 

	EntityIDPyEntityAoIEntityVisitor visitor( list );

	if (!this->visitAoI( args, visitor ))
	{
		return NULL;
	}

	return list.newRef();
}


/**
 *	Implementation of the Python len() operator.
 */
/* static */
Py_ssize_t PyAoIEntities::pyGetLength( PyObject * self )
{
	return static_cast< PyAoIEntities * >( self )->numInAoI();
}


/**
 *	This method returns the AoI entity with the given entity ID.
 *
 *	@param entityID		The entity ID.
 *
 *	@return The entity pointer, or NULL if no such entity is in the AoI for
 *			the associated witness entity.
 */
Entity * PyAoIEntities::getAoIEntity( EntityID entityID )
{
	if (!this->isValid())
	{
		return NULL;
	}

	BWConnection * pConnection = pWitness_->pBWConnection();
	if (!pConnection->isInAoI( entityID, pWitness_->entityID() ))
	{
		return NULL;
	}

	Entity * pEntity = 
		static_cast< Entity * >( pConnection->entities().find( entityID ) );
	
	MF_ASSERT( pEntity ); // Otherwise isInAoI should have failed above.

	return pEntity;
}


/**
 *	Implementation of the subscript operator.
 */
/* static */
PyObject * PyAoIEntities::pyGetFieldByKey( PyObject * self, PyObject * key )
{
	PyAoIEntities * pPyAoIEntities = static_cast< PyAoIEntities * >( self );
	
	if (!pPyAoIEntities->isValid())
	{
		PyErr_SetString( PyExc_ValueError,
			"Associated witness has been destroyed" );
		return NULL;
	}

	if (!PyInt_Check( key ) && !PyLong_Check( key ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected int or long entity ID" );
		return NULL;
	}

	EntityID entityID = PyInt_AsLong( key );

	EntityPtr pEntity = pPyAoIEntities->getAoIEntity( entityID );

	if (!pEntity.hasObject())
	{
		PyErr_SetObject( PyExc_KeyError, key );
		return NULL;
	}

	return ScriptObject( pEntity->pPyEntity().get() ).newRef();
}


/**
 *	Return the Python string representation of this object.
 */
PyObject * PyAoIEntities::pyRepr()
{
	if (!this->isValid())
	{
		return PyString_FromString( 
			"<(Defunct) PyAoIEntities for destroyed entity>" );
	}

	return PyString_FromFormat( "<PyAoIEntities for entity %d (%d entities)>",
		pWitness_->entityID(), int( this->numInAoI() ) );
}


/**
 *	This method supplies the implementation of the tp_iter function for
 *	PyAoIEntities.
 */
PyObject * PyAoIEntities::pyGetIter( PyObject * self )
{
	// Essentially return iter( self.keys() )
	PyAoIEntities * pThis = static_cast< PyAoIEntities * >( self );
	ScriptList keyList( pThis->py_keys( PyTuple_New( 0 ) ) );

	ScriptObject iterator( PyObject_GetIter( keyList.get() ) );

	return iterator.newRef();
}


/**
 *	This method supplies the implementation of the tp_iternext function for
 *	PyAoIEntities.
 */
PyObject * PyAoIEntities::pyIterNext( PyObject * iteration )
{
	return PyIter_Next( iteration );
}


BW_END_NAMESPACE

// py_aoi_entities.cpp
