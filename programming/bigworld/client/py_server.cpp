#include "pch.hpp"

#include "py_server.hpp"

#include "entity_manager.hpp"
#include "entity_type.hpp"
#include "py_entity.hpp"

#include "connection/smart_server_connection.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entities.hpp"

#include "entitydef/method_description.hpp"
#include "entitydef/script_data_source.hpp"

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
class ServerCaller : public PyObjectPlus
{
	Py_Header( ServerCaller, PyObjectPlus );

public:
	/**
	 *	This constructor is used to initialise a ServerCaller.
	 *
	 *	@param entity		The entity the method is being called on.
	 *	@param pDescription	The description associated with the method to call.
	 *	@param isProxyCaller True for calling the base, false for the cell.
	 *	@param pType		The derived Python type.
	 */
	ServerCaller( PyEntity * pPyEntity, const MethodDescription * pDescription,
			bool isProxyCaller,
			PyTypeObject * pType = &ServerCaller::s_type_ ) :
		PyObjectPlus( pType ),
		wpPyEntity_( pPyEntity ),
		pDescription_( pDescription ),
		isProxyCaller_( isProxyCaller )
	{
		BW_GUARD;
	}

	/**
	 *	Destructor.
	 */
	~ServerCaller()
	{
		BW_GUARD;
	}

	PY_METHOD_DECLARE( pyCall );


private:
	PyEntityWPtr wpPyEntity_;
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
	BW_GUARD;

	if (!wpPyEntity_.good() || wpPyEntity_->isDestroyed())
	{
		PyErr_Format( PyExc_TypeError, "Entity.%s object cannot be used since "
			"its Entity is no longer on this client",
			(!isProxyCaller_) ? "cell" : "base" );
		return NULL;
	}

	Entity * pEntity = wpPyEntity_->pEntity().get();

	if (!BWEntities::isLocalEntity( pEntity->entityID() ))
	{
		ScriptObject pArgs( args, ScriptObject::FROM_BORROWED_REFERENCE );

		if (!pDescription_->areValidArgs( /* exposedExplicit */ false, pArgs,
				/* generateException */ true ))
		{
			return NULL;	// exception already set by areValidArgs
		}
		
		if (isProxyCaller_ && !pEntity->isPlayer())
		{
			PyErr_SetString( PyExc_TypeError,
				"Can only call base methods on the player" );
			return NULL;
		}
				
		BWConnection * pConnection = pEntity->pBWConnection();

		bool shouldDrop = false;

		BinaryOStream * pStream = pConnection->addServerMessage(
				pEntity->entityID(),
				pDescription_->exposedIndex(),
				/* isForBaseEntity */ isProxyCaller_,
				shouldDrop );

		if (!pStream)
		{
			if (shouldDrop)
			{
				// The connection has told us to drop the call silently.
				Py_RETURN_NONE;
			}

			PyErr_Format( PyExc_ValueError, "Could not get stream to call %s.%s",
				pEntity->type().name().c_str(),
				pDescription_->name().c_str() );

			return NULL;
		}

		ScriptDataSource source( pArgs );
		if (!pDescription_->addToStream( source, *pStream ))
		{
			PyErr_Format( PyExc_SystemError,
				"Unexpected error in call to %s",
				pDescription_->name().c_str() );
			return NULL;
		}
	}
	// we don't actually throw an error if there's no server... or else
	// we'd have to have a whole collection of 'offline' scripts as well.

	Py_RETURN_NONE;
}

/*~ class BigWorld.ServerCaller
 *
 *	These objects are automatically created by BigWorld to populate a PyServer object.  Each ServerCaller object
 *	provides the link to a single method on the component that the PyServer object represents (ie, Cell or Base).
 *	These methods are defined in the .def file and can only exist if the server component exists.  A ServerCaller
 *	object is accessed by invoking the method that it represents in the PyServer object, eg;
 *
 *		self.cell.method( args )
 *
 *	where self is an Entity, cell is the PyObject created by BigWorld (which is not available if there is no Cell
 *	component for this Entity) and method( args ) is the ServerCaller object that represents the method on the Cell
 *	that was defined in this Entity's .def file
 *
 *  This is not supported for licensees who have not licensed the BigWorld
 *  server technology.
 */
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
		const EntityMethodDescriptions & methods,
		bool isProxyCaller,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	wpPyEntity_( pPyEntity ),
	methods_( methods ),
	isProxyCaller_( isProxyCaller )
{
	BW_GUARD;	
}

///  Destructor
PyServer::~PyServer()
{
	BW_GUARD;	
}

/**
 *	This method returns the attributes associated with this object. This is
 *	actually a dummy object. It returns a method object that is used to make a
 *	call to the server.
 */
ScriptObject PyServer::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;
	const char * attr = attrObj.c_str();

	const MethodDescription * pDescription = methods_.find( attr );

	if (pDescription == NULL)
	{
		ScriptObject result = this->PyObjectPlus::pyGetAttribute( attrObj );
		if (!result)
		{
			const char * type = wpPyEntity_->pEntity()->type().name().c_str();
			PyErr_Format( PyExc_AttributeError, "%s.%s needs to be "
				"added to <%s> of %s.def", type, attr, 
				isProxyCaller_ ? "BaseMethods" : "CellMethods", type );
			return ScriptObject();
		}
		return result;
	}

	if (!wpPyEntity_.good() || wpPyEntity_->isDestroyed())
	{
		PyErr_Format( PyExc_TypeError, "Entity.%s object cannot be used since "
			"its Entity is no longer on this client",
			(!isProxyCaller_) ? "cell" : "base" );
		return ScriptObject();
	}

	if (!pDescription->isExposed())
	{
		PyErr_Format( PyExc_TypeError, "%s.%s is not an exposed method",
			wpPyEntity_->pEntity()->type().name().c_str(),
			pDescription->name().c_str() );
		return ScriptObject();
	}

	return ScriptObject(
		new ServerCaller( wpPyEntity_.get(), pDescription, isProxyCaller_ ),
		ScriptObject::FROM_NEW_REFERENCE );
}

/**
 *	Return additional members
 */
void PyServer::pyAdditionalMembers( const ScriptList & pList ) const
{
	BW_GUARD;
	for (uint i = 0; i < methods_.size(); i++)
	{
		MethodDescription * pMethod = methods_.internalMethod( i );
		if (isProxyCaller_ || pMethod->isExposed())
		{
			pList.append( ScriptString::create( pMethod->name() ) );
		}
	}
}

BW_END_NAMESPACE

// py_server.cpp
