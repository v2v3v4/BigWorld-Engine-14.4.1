#include "pch.hpp"
#include "Python.h"

#include "py_server.hpp"

#include "entity.hpp"
#include "entity_type.hpp"

#include "connection_model/server_entity_mail_box.hpp"

#include "entitydef/method_description.hpp"

#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ServerCaller
// -----------------------------------------------------------------------------

/**
 *	This class implements a simple helper Python type. Objects of this type are
 *	used to represent methods that the client can call on the server.
 */
class ServerCaller : public PyObjectPlus, public ServerEntityMailBox
{
	Py_Header( ServerCaller, PyObjectPlus );

public:
	/**
	 *	This constructor is used to initialise a ServerCaller.
	 *
	 *	@param entity		The entity the method is being called on.
	 *	@param pDescription	The description associated with the method to call.
	 */
	ServerCaller( PyEntity * pPyEntity, const MethodDescription * pDescription,
			bool isProxyCaller,
			PyTypeObject * pType = &ServerCaller::s_type_ ) :
		PyObjectPlus( pType ),
		ServerEntityMailBox( *pPyEntity->pEntity() ),
		pPyEntity_( pPyEntity ),
		pDescription_( pDescription ),
		isProxyCaller_( isProxyCaller )
	{
		Py_INCREF( pPyEntity_ );
	}

	/**
	 *	Destructor.
	 */
	~ServerCaller()
	{
		Py_DECREF( pPyEntity_ );
	}

	PY_METHOD_DECLARE( pyCall );


private:
	PyEntity * pPyEntity_;
	const MethodDescription * pDescription_;
	bool isProxyCaller_;
};


/**
 *	This method is called with this object is "called" in script. This is what
 *	makes this object a functor. It is responsible for putting the call on the
 *	bundle that will be sent to the server.
 *
 *	@param args	A tuple containing the arguments of the call.
 *
 *	@return		Py_None on success and NULL on failure. If an error occurs, the
 *				Python error string is set.
 */
PyObject * ServerCaller::pyCall( PyObject * args )
{
	if (pPyEntity_->pEntity() == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "Entity has been destroyed" );
		return NULL;
	}

	ScriptObject pArgs( args, ScriptObject::FROM_BORROWED_REFERENCE );
	if (!pDescription_->areValidArgs( false, pArgs, true ))
	{
		return NULL;	// exception already set by areValidArgs
	}

	const int methodID = pDescription_->exposedIndex();

	bool shouldDrop = false;
	BinaryOStream * pStream =
		this->startMessage( methodID, isProxyCaller_, shouldDrop );

	if (!pStream)
	{
		if (shouldDrop)
		{
			// Connection has told us to drop this call silently.
			Py_RETURN_NONE;
		}

		PyErr_SetString( PyExc_ValueError, "Could not create stream for call" );
		return NULL;
	}

	ScriptDataSource source( pArgs );
	bool ok = pDescription_->addToStream( source, *pStream );
	if (!ok)
	{
		PyErr_Format( PyExc_SystemError,
			"Unexpected error in call to %s",
			pDescription_->name().c_str() );
		return NULL;
	}

	Py_RETURN_NONE;
}

PY_TYPEOBJECT_WITH_CALL( ServerCaller )

PY_BEGIN_METHODS( ServerCaller )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ServerCaller )
PY_END_ATTRIBUTES()




// -----------------------------------------------------------------------------
// Section: PyServer
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyServer )

PY_BEGIN_METHODS( PyServer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyServer )
PY_END_ATTRIBUTES()

///	Constructor
PyServer::PyServer( PyEntity * pPyEntity,
			bool isProxyCaller,
			PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pPyEntity_( pPyEntity ),
	isProxyCaller_( isProxyCaller )
{
	Py_INCREF( pPyEntity );
}

///  Destructor
PyServer::~PyServer()
{
	MF_ASSERT( pPyEntity_ == NULL );
}

/**
 *	This method returns the attributes associated with this object. This is
 *	actually a dummy object. It returns a method object that is used to make a
 *	call to the server.
 */
ScriptObject PyServer::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();
	Entity * pEntity = this->pEntity();
	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_TypeError, "%s is no longer callable since "
			"its entity no longer exists on this client.",
			attr );
		return ScriptObject();
	}

	const EntityMethodDescriptions & rMethods = ( isProxyCaller_ ?
		pEntity->type().description().base() :
		pEntity->type().description().cell() );

	const MethodDescription * pDescription = rMethods.find( attr );

	if (pDescription != NULL)
	{
		if (!pDescription->isExposed())
		{
			PyErr_Format( PyExc_TypeError, "%s.%s is not an exposed method",
				pEntity->entityTypeName().c_str(),
				pDescription->name().c_str() );
			return ScriptObject();
		}

		return ScriptObject(
			new ServerCaller( pPyEntity_, pDescription, isProxyCaller_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->PyObjectPlus::pyGetAttribute( attrObj );
}

/**
 *	Return additional members
 */
void PyServer::pyAdditionalMembers( const ScriptList & pList ) const
{
	Entity * pEntity = this->pEntity();

	if (pEntity == NULL)
	{
		return;
	}

	const EntityMethodDescriptions & rMethods = ( isProxyCaller_ ?
		pEntity->type().description().base() :
		pEntity->type().description().cell() );

	for ( uint i = 0; i < ( uint ) rMethods.size(); i++)
	{
		MethodDescription * pMethod = rMethods.internalMethod( i );
		if (pMethod->isExposed())
		{
			pList.append( ScriptString::create( pMethod->name() ) );
		}
	}
}


/**
 *	This method is called when our pEntity_ pointer is about to become invalid
 */
void PyServer::onEntityDestroyed()
{
	MF_ASSERT( pPyEntity_ != NULL );
	Py_DECREF( pPyEntity_ );
	pPyEntity_ = NULL;
}


/**
 *	This method is called to dereference our Entity if it hasn't already been
 *	destroyed.
 */
Entity * PyServer::pEntity() const
{
	if (pPyEntity_ == NULL)
	{
		return NULL;
	}

	return pPyEntity_->pEntity();
}

BW_END_NAMESPACE

// py_server.cpp
