#include "pch.hpp"
#include "Python.h"

#include "entity.hpp"
#include "entity_type.hpp"
#include "py_entity.hpp"
#include "py_server.hpp"

DECLARE_DEBUG_COMPONENT2( "PyEntity", 0 )


BW_BEGIN_NAMESPACE

// scripting declarations
PY_BASETYPEOBJECT( PyEntity )

PY_BEGIN_METHODS( PyEntity )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEntity )
	PY_ATTRIBUTE( position )
	PY_ATTRIBUTE( yaw )
	PY_ATTRIBUTE( pitch )
	PY_ATTRIBUTE( roll )
	PY_ATTRIBUTE( cell )
	PY_ATTRIBUTE( base )
	PY_ATTRIBUTE( id )
	PY_ATTRIBUTE( spaceID )
	PY_ATTRIBUTE( isDestroyed )
PY_END_ATTRIBUTES()


PyEntity::PyEntity( Entity & entity ) :
	PyObjectPlus( entity.type().pType(), true ),
	pEntity_( &entity ),
	pPyCell_( NULL ),
	pPyBase_( NULL )
{
	// Start with all of the default values.
	PyObject * pNewDict = entity.type().newDictionary();
	PyObject * pCurrDict = this->pyGetAttribute( 
		ScriptString::create( "__dict__" ) ).newRef();

	if ( !pNewDict || !pCurrDict ||
		PyDict_Update( pCurrDict, pNewDict ) < 0 )
	{
		PY_ERROR_CHECK();
	}

	Py_XDECREF( pNewDict );
	Py_XDECREF( pCurrDict );

	pPyCell_ = new PyServer( this, /* isProxyCaller */ false );
	pPyBase_ = new PyServer( this, /* isProxyCaller */ true );
}


PyEntity::~PyEntity()
{
	MF_ASSERT( pPyBase_ == NULL );
	MF_ASSERT( pPyCell_ == NULL );
	MF_ASSERT( pEntity_ == NULL );
}


/**
 *	This method is called when our pEntity_ pointer is about to become invalid
 */
void PyEntity::onEntityDestroyed()
{
	((PyServer * )pPyBase_)->onEntityDestroyed();
	Py_XDECREF( pPyBase_ );
	pPyBase_ = NULL;

	((PyServer * )pPyCell_)->onEntityDestroyed();
	Py_XDECREF( pPyCell_ );
	pPyCell_ = NULL;

	pEntity_ = NULL;
}

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyEntity::

#define PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( MEMBER, NAME )				\
	PyObject * PY_ATTR_SCOPE pyGet_##NAME()									\
{																			\
	if (pEntity_ == NULL)													\
	{																		\
		PyErr_Format( PyExc_TypeError, "Sorry, the attribute " #NAME " in "	\
		"%s is no longer available as its entity no longer exists on this "	\
		"client.", this->typeName() );										\
		return NULL;														\
	}																		\
	return Script::getReadOnlyData( MEMBER );								\
}

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( pPyCell_, cell )
PY_RO_ATTRIBUTE_SET( cell )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( pPyBase_, base )
PY_RO_ATTRIBUTE_SET( base )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->position(), position )
PY_RO_ATTRIBUTE_SET( position )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->direction().yaw, yaw )
PY_RO_ATTRIBUTE_SET( yaw )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->direction().pitch,
	pitch )
PY_RO_ATTRIBUTE_SET( pitch )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->direction().roll, roll )
PY_RO_ATTRIBUTE_SET( roll )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->entityID(), id )
PY_RO_ATTRIBUTE_SET( id )

PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED( this->pEntity_->spaceID(), spaceID )
PY_RO_ATTRIBUTE_SET( spaceID )

#undef PY_RO_ATTRIBUTE_GET_IF_NOT_DESTROYED
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE


void PyEntity::callInit()
{
	BW::string callContext = this->pEntity()->entityTypeName() + ".__init__";

	Script::call( PyObject_GetAttrString( this, "__init__" ),
		PyTuple_New( 0 ), callContext.c_str(), true );
}


Entity * PyEntity::pEntity() const
{
	return static_cast< Entity * >( pEntity_ );
}


bool PyEntity::initFromStream( BinaryIStream & stream )
{
	if (stream.remainingLength() == 0)
	{
		this->callInit();
		return true;
	}

	PyObject * pNewDict = this->pEntity()->type().newDictionary( stream );
	PyObject * pCurrDict = PyObject_GetAttrString( this, "__dict__" );

	if ( !pNewDict || !pCurrDict ||
		PyDict_Update( pCurrDict, pNewDict ) < 0 )
	{
		PY_ERROR_CHECK();
		return false;
	}

	if (stream.error())
	{
		return false;
	}

	Py_XDECREF( pNewDict );
	Py_XDECREF( pCurrDict );

	this->callInit();

	return true;
}


bool PyEntity::initBasePlayerFromStream( BinaryIStream & stream )
{
	PyObject * pNewDict = this->pEntity()->type().newDictionary( stream,
		EntityDescription::FROM_BASE_TO_CLIENT_DATA );
	PyObject * pCurrDict = PyObject_GetAttrString( this, "__dict__" );

	if ( !pNewDict || !pCurrDict ||
		PyDict_Update( pCurrDict, pNewDict ) < 0 )
	{
		PY_ERROR_CHECK();
		return false;
	}

	if (stream.error())
	{
		return false;
	}

	Py_XDECREF( pNewDict );
	Py_XDECREF( pCurrDict );

	this->callInit();

	return true;
}


/**
 *	This method reads the player data send from the cell. This is called on the
 *	player entity when it gets a cell entity associated with it.
 */
bool PyEntity::initCellPlayerFromStream( BinaryIStream & stream )
{
	PyObject * pCellData = this->pEntity()->type().newDictionary( stream,
		EntityDescription::FROM_CELL_TO_CLIENT_DATA );

	PyObject * pDict = PyObject_GetAttrString( this, "__dict__" );

	if ((pCellData == NULL) || (pDict == NULL) || stream.error())
	{
		ERROR_MSG( "Entity::initCellPlayerFromStream: Could not create dict\n" );
		Py_XDECREF( pCellData );
		Py_XDECREF( pDict );
		return false;
	}

	PyDict_Update( pDict, pCellData );
	Py_DECREF( pCellData );

	std::cout << "Entity::initCellPlayerFromStream:" << std::endl;
	PyObject * pStr = PyObject_Str( pDict );
	std::cout << PyString_AsString( pStr ) << std::endl;
	Py_DECREF( pStr );

	Py_DECREF( pDict );

	return true;
}

BW_END_NAMESPACE

// py_entity.cpp

