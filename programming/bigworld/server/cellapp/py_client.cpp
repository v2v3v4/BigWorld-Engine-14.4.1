#include "script/first_include.hpp"

#include "py_client.hpp"

#include "entity.hpp"

#include "cstdmf/profiler.hpp"
#include "entitydef/mailbox_base.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ClientCaller
// -----------------------------------------------------------------------------

/*~ class NoModule.ClientCaller
 *  @components{ cell }
 *  Instances of the ClientCaller class represent entity member functions which
 *  a cell application can call on one or more clients. When an object of this
 *  type is called it adds a request to call the appropriate function on to the
 *  network bundles that are sent to the appropriate clients.
 *
 *  ClientCaller objects cannot be created via python, and are available only
 *  through instances which are created by PyClient objects. Further
 *  information and an example regarding the use of ClientCaller objects can
 *  be found in the PyClient class documentation.
 */
/**
 *	This class implements a simple helper Python type. Objects of this type are
 *	used to represent methods that the server can call on the client.
 */
class ClientCaller : public PyObjectPlus
{
	Py_Header( ClientCaller, PyObjectPlus )

public:
	ClientCaller( Entity & entity, 
			const MethodDescription & methodDescription,
			bool isForOwn, 
			bool isForOthers, 
			RecordingOption recordingOption,
			const BW::string & destEntityTypeName,
			EntityID destID,
			PyTypeObject * pType = &ClientCaller::s_type_ );

	/**
	 *	Destructor.
	 */
	~ClientCaller()
	{
		Py_DECREF( &entity_ );
	}

	PY_METHOD_DECLARE( pyCall )

	PY_RO_ATTRIBUTE_DECLARE( destID_, entityID );
	PY_RO_ATTRIBUTE_DECLARE( entity_.id(), clientID );
	PY_RO_ATTRIBUTE_DECLARE( methodDescription_.name(), methodName );
	PY_RO_ATTRIBUTE_DECLARE( isForOwn_, isForOwn );
	PY_RO_ATTRIBUTE_DECLARE( isForOthers_, isForOthers );
	PY_RO_ATTRIBUTE_DECLARE( isExposedForReplay_, isRecording );
	PY_RO_ATTRIBUTE_DECLARE( isExposedForReplay_ && !isForOwn_ && !isForOthers_,
		isRecordingOnly );

	PyObject * pyRepr() const;

private:
	Entity & entity_;
	const MethodDescription & methodDescription_;

	bool isForOwn_;				///< Call is destined for our client?
	bool isForOthers_;			///< Call is destined for other clients?
	bool isExposedForReplay_; 	///< Call is exposed for replay?

	EntityID destID_;			///< ID of entity on client.
	BW::string destEntityTypeName_; ///< Name of the destination entity type.
};



/**
 *	Constructor.
 *
 *	@param methodDescription 	The method description.
 *	@param isForOwn 			Whether this will be sent to the entity's own
 *								client.
 *	@param isForOthers 			Whether this will be sent to other clients that
 *								have this entity in their AoI.
 *	@param recordingOption		See RecordingOption enum.
 *	@param destEntityTypeName 	The name of the destination entity's type.
 *	@param destID 				The destination entity's ID.
 *	@param pType 				The type object to initialise this Python
 *								object to.
 */
ClientCaller::ClientCaller( Entity & entity, 
			const MethodDescription & methodDescription,
			bool isForOwn, 
			bool isForOthers, 
			RecordingOption recordingOption,
			const BW::string & destEntityTypeName,
			EntityID destID,
			PyTypeObject * pType /* = &ClientCaller::s_type_ */ ) :
		PyObjectPlus( pType ),
		entity_( entity ),
		methodDescription_( methodDescription ),
		isForOwn_( isForOwn ),
		isForOthers_( isForOthers ),
		isExposedForReplay_( false ),
		destID_( destID ),
		destEntityTypeName_( destEntityTypeName )
{
	Py_INCREF( &entity_ );

	switch (recordingOption)
	{
	case RECORDING_OPTION_METHOD_DEFAULT:
		if (isForOwn && !isForOthers)
		{
			isExposedForReplay_ = 
				(methodDescription.replayExposureLevel() >= 
					MethodDescription::REPLAY_EXPOSURE_LEVEL_ALL_CLIENTS);
		}
		else if (isForOthers)
		{
			isExposedForReplay_ = 
				(methodDescription.replayExposureLevel() >= 
					MethodDescription::REPLAY_EXPOSURE_LEVEL_OTHER_CLIENTS);
		}
		break;

	case RECORDING_OPTION_RECORD_ONLY:
		isForOwn_ = false;
		isForOthers_ = false;
		// Fall through
	case RECORDING_OPTION_RECORD:
		isExposedForReplay_ = true;
		break;

	case RECORDING_OPTION_DO_NOT_RECORD:
	default:
		// Leave isExposedForReplay_ to false
		break;
	}
}


/**
 *	This method is called when this object is "called" in script. That is, it is
 *	like operator(). It is responsible for "sending" the method call to the
 *	appropriate clients.
 *
 *	@param args	A Python tuple containing the arguments for the method call.
 *
 *	@return		Py_None on success, NULL on failure.
 */
PyObject * ClientCaller::pyCall( PyObject * args )
{
	PROFILER_SCOPED( ClientCaller_pyCall );
	ScriptObject pArgs( args, ScriptObject::FROM_BORROWED_REFERENCE );

	if (!methodDescription_.areValidArgs( true, pArgs, true ))
	{
		return NULL;
	}

	if (entity_.isDestroyed())
	{
		PyErr_Format( PyExc_TypeError,
				"ClientCaller: Cannot call %s on a destroyed entity (%d)",
				methodDescription_.name().c_str(), (int)entity_.id() );
		return NULL;
	}

	ScriptDataSource source( pArgs );
	MemoryOStream stream;

	if (!methodDescription_.addToClientStream( source, stream, destID_ ))
	{
		return NULL;
	}

	bool succeeded = false;

	if (entity_.isReal())
	{
		succeeded = entity_.sendToClient( destID_, methodDescription_, 
			stream, isForOwn_, isForOthers_, isExposedForReplay_ );
	}
	else
	{
		succeeded = entity_.sendToClientViaReal( destID_, methodDescription_,
			stream, isForOwn_, isForOthers_, isExposedForReplay_ );
	}

	if (!succeeded)
	{
		PyErr_Format( PyExc_AttributeError,
				"%s called on invalid entity (no associated client?).",
				methodDescription_.name().c_str() );
		return NULL;
	}

	Py_RETURN_NONE;
}


/**
 *	This method returns the Python string representation for instances of this
 *	class.
 */
PyObject * ClientCaller::pyRepr() const
{
	if (entity_.id() != destID_)
	{
		return PyString_FromFormat( 
			"Remote method: Client %s(%d): %s(%d).%s( %s ).%s",
			entity_.pType()->name(), 
			entity_.id(), 
			destEntityTypeName_.c_str(),
			destID_,
			((isForOwn_ && isForOthers_) ? "allClients" :
				!isForOwn_ ? "otherClients" : "ownClient" ),
			(isExposedForReplay_ ? 
				((!isForOwn_ && !isForOthers_) ? 
					"shouldRecordOnly=True" :
					"shouldExposeForReplay=True") :
				"shouldExposeForReplay=False"),
			methodDescription_.name().c_str() );
	}
	else
	{
		return PyString_FromFormat( 
			"Remote method: %s(%d).%s( %s ).%s",
			entity_.pType()->name(), 
			entity_.id(), 
			((isForOwn_ && isForOthers_) ? "allClients" :
				!isForOwn_ ? "otherClients" : "ownClient" ),
			(isExposedForReplay_ ? 
				((!isForOwn_ && !isForOthers_) ? 
					"shouldRecordOnly=True" :
					"shouldExposeForReplay=True") :
				"shouldExposeForReplay=False"),
			methodDescription_.name().c_str() );
	}
}


namespace // (anonymous)
{
PyObject * pyReprClientCaller( PyObject * self )
{
	return reinterpret_cast< ClientCaller * >( self )->pyRepr();
}
}

PY_TYPEOBJECT_SPECIALISE_REPR_AND_STR( ClientCaller, 
	&pyReprClientCaller, &pyReprClientCaller );

PY_TYPEOBJECT_WITH_CALL( ClientCaller )

PY_BEGIN_METHODS( ClientCaller )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClientCaller )

/*~ attribute ClientCaller entityID
 *	@components{ cell }
 *	This is the entity ID of the entity on the client being called.
 */
	PY_ATTRIBUTE( entityID )

/*~ attribute ClientCaller clientID 
 *	@components{ cell }
 *	This is the ID of the client being called.
 */
	PY_ATTRIBUTE( clientID )

/*~ attribute ClientCaller methodName 
 *	@components{ cell }
 *	The name of the client entity method to call.
 */
	PY_ATTRIBUTE( methodName )

/*~ attribute ClientCaller isForOwn
 *	@components{ cell }
 *	If True, calls to this object will be broadcast to this entity's own
 *	client.
 */
	PY_ATTRIBUTE( isForOwn )

/*~ attribute ClientCaller isForOthers
 *	@components{ cell }
 *	If True, calls to this object will be broadcast to other clients that have
 *	this entity in their AoI.
 */
	PY_ATTRIBUTE( isForOthers )

/*~ attribute ClientCaller isRecording
 *	@components{ cell }
 *	Whether this call will be recorded for replay, if recording is active.
 */
	PY_ATTRIBUTE( isRecording )

/*~ attribute ClientCaller isRecordingOnly
 *	@components{ cell }
 *	Whether this call will only be recorded for replay, if recording is active.
 *	If True, this call will be recorded, but will not be propagated to clients.
 */
	PY_ATTRIBUTE( isRecordingOnly )

PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: PyClient
// -----------------------------------------------------------------------------



namespace // (anonymous)
{
PyObject * reprPyClient( PyObject * self )
{
	return reinterpret_cast< PyClient * >( self )->pyRepr();
}

} // end namespace (anonymous)

PY_TYPEOBJECT_SPECIALISE_REPR_AND_STR( PyClient, &reprPyClient, &reprPyClient );
PY_TYPEOBJECT_WITH_CALL( PyClient )

PY_BEGIN_METHODS( PyClient )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyClient )
PY_END_ATTRIBUTES()


/**
 *  Constructor for PyClient.
 *
 *	@param clientEntity	When isForOwn is true, the entity associated with the
 *						client machine the call is going to be made to.
 *	@param destEntity	The entity on the client to send to.
 *	@param isForOwn		Indicates whether the message is to sent to the client
 *						entity.
 *	@param isForOthers	Indicates whether the message should be sent to all
 *						clients who have this in their AoI.
 *	@param recordingOption
 						See the RecordingOption enum.
 *	@param pType		A pointer to the structure indicating the Python type.
 */
PyClient::PyClient( Entity & clientEntity,
		Entity & destEntity,
		bool isForOwn,
		bool isForOthers,
		RecordingOption recordingOption,
			/* = RECORDING_OPTION_METHOD_DEFAULT */
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	clientEntity_( clientEntity ),
	destEntity_( destEntity ),
	isForOwn_( isForOwn ),
	isForOthers_( isForOthers ),
	recordingOption_( recordingOption )
{
	// TODO: We probably want to do a custom allocator for this since there
	// should only really ever be one of these needed at a time so we could keep
	// reusing the same object.
	Py_INCREF( &clientEntity_ );
	Py_INCREF( &destEntity_ );
}


/**
 *	Destructor.
 */
PyClient::~PyClient()
{
	Py_DECREF( &clientEntity_ );
	Py_DECREF( &destEntity_ );
}


/**
 *	This method returns the attributes associated with this object. This is
 *	actually a dummy object. It returns a method object that is used to make a
 *	call to the server.
 */
ScriptObject PyClient::pyGetAttribute( const ScriptString & attrObj )
{
	PROFILER_SCOPED( pyClient_pyGetAttribute );
	const char * attr = attrObj.c_str();

	const MethodDescription * pDescription =
		destEntity_.pType()->description().client().find( attr );

	if (pDescription == NULL)
	{
		PyErr_Format( PyExc_AttributeError,
			"No such client method %s for type %s",
				attr, destEntity_.pType()->name() );
		return ScriptObject();
	}

	// TODO: We probably want to do a custom allocator since there should
	// only ever be one of these at a time and they are created often.

	return ScriptObject( 
		new ClientCaller( clientEntity_, *pDescription,
			isForOwn_, isForOthers_, 
			recordingOption_, 
			destEntity_.pType()->name(),
			destEntity_.id() ),
		ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method implements tp_call / __call__.
 */
PyObject * PyClient::pyCall( PyObject * args, PyObject * kwargs )
{
	RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT;
	if (!PyEntityMailBox::parseRecordingOptionFromPythonCall( args, kwargs,
			recordingOption))
	{
		return NULL;
	}

	if (recordingOption == recordingOption_)
	{
		this->incRef();
		return this;
	}

	return new PyClient( clientEntity_, destEntity_, isForOwn_, isForOthers_,
		recordingOption );
}


/**
 *	This method returns a Python string representation of instances of this
 *	class.
 */
PyObject * PyClient::pyRepr() const
{
	if (destEntity_.id() != clientEntity_.id())
	{
		return PyString_FromFormat( 
			"Client MailBox: %s(%d).clientEntity( %s %d )",
			clientEntity_.pType()->name(), clientEntity_.id(),
			destEntity_.pType()->name(), destEntity_.id() );
	}
	else if (recordingOption_ == RECORDING_OPTION_METHOD_DEFAULT)
	{
		return PyString_FromFormat( "Client MailBox: %s(%d).%s",
			clientEntity_.pType()->name(), clientEntity_.id(),
			((isForOwn_ && isForOthers_) ? 
				"allClients" : 
				(isForOwn_ ? "ownClient" : "otherClients")) );
	}
	else
	{
		bool isExposedForReplay = (recordingOption_ >= RECORDING_OPTION_RECORD);
		bool shouldRecordOnly = (recordingOption_ == 
			RECORDING_OPTION_RECORD_ONLY);

		return PyString_FromFormat( "Client MailBox: %s(%d).%s( %s )",
			clientEntity_.pType()->name(), clientEntity_.id(),
			((isForOwn_ && isForOthers_) ? 
				"allClients" : 
				(isForOwn_ ? "ownClient" : "otherClients")),
			(isExposedForReplay ? 
				(shouldRecordOnly ? 
					"shouldExposeForReplay=True, shouldRecordOnly=True": 
					"shouldExposeForReplay=True") :
				"shouldExposeForReplay=False") );
	}
}


BW_END_NAMESPACE


// py_client.cpp
