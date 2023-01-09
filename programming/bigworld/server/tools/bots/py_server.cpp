#include "script/first_include.hpp"

#include "py_server.hpp"

#include "entity.hpp"
#include "entity_type.hpp"
#include "py_entity.hpp"

#include "connection_model/server_entity_mail_box.hpp"

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
class ServerCaller : public PyObjectPlus, public ServerEntityMailBox
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
		ServerEntityMailBox( *pPyEntity->pEntity() ),
		wpPyEntity_( pPyEntity ),
		pDescription_( pDescription ),
		isProxyCaller_( isProxyCaller )
	{
	}

	/**
	 *	Destructor.
	 */
	~ServerCaller()
	{
	}

	PY_METHOD_DECLARE( pyCall );

private:
	Entity * pEntity() const;

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
	Entity * pEntity = this->pEntity();

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_TypeError, "Entity.%s object cannot be used since "
			"its Entity is no longer on this client",
			(!isProxyCaller_) ? "cell" : "base" );
		return NULL;
	}

	EntityID entityID = this->pEntity()->entityID();
	bool isClientEntity = entityID >= BW::FIRST_LOCAL_ENTITY_ID;

	if (isClientEntity)
	{
		Py_RETURN_NONE;
	}

	ScriptObject pArgs( args, ScriptObject::FROM_BORROWED_REFERENCE );

	if (!pDescription_->areValidArgs( /* exposedExplicit */ false, pArgs,
			/* generateException */ true ))
	{
		return NULL;	// exception already set by areValidArgs
	}

	const int methodID = pDescription_->exposedIndex();
	bool shouldDrop = false;
	BinaryOStream * pStream = this->startMessage( methodID, isProxyCaller_,
		shouldDrop );

	if (shouldDrop)
	{
		Py_RETURN_NONE;
	}

	if (!pStream)
	{
		PyErr_SetString( PyExc_ValueError, "Could not create stream for call" );
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

	Py_RETURN_NONE;
}


/**
 *	This method gets an Entity from our weak PyEntity pointer if it is valid.
 */
Entity * ServerCaller::pEntity() const
{
	if (!wpPyEntity_.good() || wpPyEntity_->isDestroyed())
	{
		return NULL;
	}
	return wpPyEntity_->pEntity().get();
}


/*~ class BigWorld.ServerCaller
 *	@components{ bots }
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
		bool isProxyCaller,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	wpPyEntity_( pPyEntity ),
	isProxyCaller_( isProxyCaller )
{
}

///  Destructor
PyServer::~PyServer()
{
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
		PyErr_Format( PyExc_TypeError, "Entity.%s object cannot be used since "
			"its Entity is no longer on this client",
			(!isProxyCaller_) ? "cell" : "base" );
		return ScriptObject();
	}

	const EntityMethodDescriptions & methods = ( isProxyCaller_ ?
		pEntity->type().description().base() :
		pEntity->type().description().cell() );

	const MethodDescription * pMethod = methods.find( attr );

	if (pMethod == NULL || !pMethod->isExposed())
	{
		return this->PyObjectPlus::pyGetAttribute( attrObj );
	}

	return ScriptObject(
		new ServerCaller( wpPyEntity_.get(), pMethod, isProxyCaller_ ),
		ScriptObject::FROM_NEW_REFERENCE );
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

	const EntityMethodDescriptions & methods = ( isProxyCaller_ ?
		pEntity->type().description().base() :
		pEntity->type().description().cell() );

	for (uint i = 0; i < methods.size(); i++)
	{
		MethodDescription * pMethod = methods.internalMethod( i );
		if (pMethod->isExposed())
		{
			pList.append( ScriptString::create( pMethod->name() ) );
		}
	}
}

/**
 *	This method gets an Entity from our weak PyEntity pointer if it is valid.
 */
Entity * PyServer::pEntity() const
{
	if (!wpPyEntity_.good() || wpPyEntity_->isDestroyed())
	{
		return NULL;
	}
	return wpPyEntity_->pEntity().get();
}


BW_END_NAMESPACE

// py_server.cpp
