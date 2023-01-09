#include "pch.hpp"

#include "method_description.hpp"

#include "data_description.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"

#if defined( SCRIPT_PYTHON )
#include "pyscript/pyobject_plus.hpp"
#endif // SCRIPT_PYTHON

#ifdef MF_SERVER
#include "return_values_handler.hpp"
#endif // MF_SERVER

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"
#include "network/bundle.hpp"
#include "network/exposed_message_range.hpp"
#include "network/network_interface.hpp"
#include "network/smart_bundles.hpp"

#include "script/pickler.hpp"

#include <float.h>
#include <memory>

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "method_description.ipp"
#endif

namespace
{

ScriptObject s_pCreateErrorFunc;

}

#if defined( SCRIPT_PYTHON )

// -----------------------------------------------------------------------------
// Section: PyDeferredResponse
// -----------------------------------------------------------------------------

/**
 *	This class is used to send a response to a two-way script call when the
 *	response is delayed by using a Twisted Deferred.
 */
class PyDeferredResponse : public PyObjectPlus
{
	Py_Header( PyDeferredResponse, PyObjectPlus )

public:
	PyDeferredResponse( const MethodDescription & methodDescription,
			int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface ) :
		PyObjectPlus( &PyDeferredResponse::s_type_ ),
		methodDescription_( methodDescription ),
		replyID_( replyID ),
		replyAddr_( replyAddr ),
		networkInterface_( networkInterface )
	{
	}

	void callback( ScriptTuple value );
	void errback( ScriptObject fail );

	PY_AUTO_METHOD_DECLARE( RETVOID, callback, ARG( ScriptTuple, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, errback, ARG( ScriptObject, END ) )

private:
	const MethodDescription & methodDescription_;
	int replyID_;
	Mercury::Address replyAddr_;
	Mercury::NetworkInterface & networkInterface_;
};

PY_TYPEOBJECT( PyDeferredResponse )

PY_BEGIN_METHODS( PyDeferredResponse )
	PY_METHOD( callback )
	PY_METHOD( errback )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyDeferredResponse )
PY_END_ATTRIBUTES()

void PyDeferredResponse::callback( ScriptTuple values )
{
	if (!methodDescription_.areValidReturnValues( values ))
	{

		methodDescription_.sendReturnValuesError(
				methodDescription_.createErrorObject( "BWInvalidArgsError",
					"Invalid return arguments" ),
				replyID_, replyAddr_, networkInterface_ );

		WARNING_MSG( "PyDeferredResponse::callback: "
					"Sent failure response for %s\n",
				methodDescription_.name().c_str() );
		return;
	}

	ScriptDataSource source( values );
	
	if (!methodDescription_.sendReturnValues( source, replyID_, replyAddr_,
		networkInterface_ ))
	{
		WARNING_MSG( "PyDeferredResponse::callback: "
					"Failed to send response for %s\n",
				methodDescription_.name().c_str() );
	}
}

void PyDeferredResponse::errback( ScriptObject failure )
{
	ScriptObject value = failure.getAttribute( "value", ScriptErrorPrint(
		"PyDeferredResponse::errback: Invalid failure object\n" ) );

	if (value)
	{
		methodDescription_.sendReturnValuesError( value,
				replyID_, replyAddr_, networkInterface_ );
	}
}

#endif // SCRIPT_PYTHON


// -----------------------------------------------------------------------------
// Section: Method Description
// -----------------------------------------------------------------------------

/**
 *	Default constructor.
 */
MethodDescription::MethodDescription() :
	MemberDescription( NULL ),
	flags_( 0 ),
	args_(),
	returnValues_(),
	hasReturnValues_( false ),
	internalIndex_( -1 ),
	exposedIndex_( -1 ),
	exposedMsgID_( -1 ),
	exposedSubMsgID_( -1 ),
	priority_( FLT_MAX ),
	replayExposureLevel_( REPLAY_EXPOSURE_LEVEL_OTHER_CLIENTS ),
	components_(),
	isComponentised_( false )
#if ENABLE_WATCHERS
	,timeSpent_( 0 ),
	timeSpentMax_( 0 ),
	timesCalled_( 0 )
#endif
{
}

/**
 *	The copy constructor.
 */
MethodDescription::MethodDescription( const MethodDescription & description ) :
	BW::MemberDescription( description )
{
	(*this) = description;
}


/**
 *	Destructor.
 */
MethodDescription::~MethodDescription()
{
}


/**
 *	The assignment operator.
 */
MethodDescription & MethodDescription::operator=(
				const MethodDescription & description )
{
	// The assignment operator used to be necessary when we were storing the
	// function pointer which needed to be reference counted.

	if (this != &description)
	{
		*(MemberDescription *)this = description;
		// this->MemberDescription::operator=( description );

		flags_			= description.flags_;
		args_			= description.args_;
		returnValues_	= description.returnValues_;
		hasReturnValues_ = description.hasReturnValues_;
		internalIndex_	= description.internalIndex_;
		exposedIndex_	= description.exposedIndex_;
		exposedMsgID_	= description.exposedMsgID_;
		exposedSubMsgID_= description.exposedSubMsgID_;
		priority_		= description.priority_;
		replayExposureLevel_ = description.replayExposureLevel_;
		components_		= description.components_;
		isComponentised_ = description.isComponentised_;
#if ENABLE_WATCHERS
		timeSpent_		= description.timeSpent_;
		timeSpentMax_	= description.timeSpentMax_;
		timesCalled_	= description.timesCalled_;
#endif
	}

	return *this;
}


#ifdef SCRIPT_PYTHON
/**
 *	This method initialises the static CreateError function used by
 *	MethodDescription
 */
bool MethodDescription::staticInitTwoWay()
{
	static bool hasFailed = false;

	if (hasFailed)
	{
		return false;
	}

	if (s_pCreateErrorFunc)
	{
		return true;
	}

	ScriptModule pModule = ScriptModule::import( "BWTwoWay",
		ScriptErrorPrint( "MethodDescription::staticInitTwoWay: "
				"Failed to import BWTwoWay\n" ) );

	if (!pModule)
	{
		hasFailed = true;

		return false;
	}

	const char * funcName = "createError";

	s_pCreateErrorFunc = pModule.getAttribute( funcName,
		ScriptErrorRetain() );

	if (!s_pCreateErrorFunc)
	{
		ERROR_MSG( "MethodDescription::staticInitTwoWay: "
				"Failed to get '%s' from module BWTwoWay\n", funcName );
		Script::printError();
		Script::clearError();

		hasFailed = true;

		return false;
	}

	return true;
}


/**
 *	This method creates a JSON object representing an exception object. It is
 *	made up of a string containing the error type, and an object containing
 *	the arguments.
 */
ScriptObject MethodDescription::createErrorObject( const char * excType,
		ScriptObject args )
{
	if (!s_pCreateErrorFunc)
	{
		return ScriptObject();
	}

	ScriptObject pExc( PyObject_CallFunction( s_pCreateErrorFunc.get(), "sO",
						excType, args.get() ),
					ScriptObject::STEAL_REFERENCE );

	if (!pExc)
	{
		Script::printError();
	}

	return pExc;
}

#endif // SCRIPT_PYTHON

/**
 *	This method is a convenience for calling createErrorObject with a single
 *	string argument.
 */
ScriptObject MethodDescription::createErrorObject( const char * excType,
		const char * msg )
{
	ScriptTuple args = ScriptTuple::create( 1 );

	ScriptObject pMsgStr = ScriptObject::createFrom( msg );
	args.setItem( 0, pMsgStr );

	return MethodDescription::createErrorObject( excType, args );
}


/**
 *	This method parses a method description.
 *
 *	@param interfaceName
 *						The name of the interface that this member belongs to.
 *	@param componentName
 *						The name of the component to which this member belongs
 *						(if any), eg. the Despair component.
 *	@param pSection		Datasection from which to parse the description.
 *	@param component	Indicates which component this method belongs to.
 *	@param pNumLatestEventMembers
 *						This is a pointer to the value that stores 
 *						numLatestEventMembers. If not NULL, this is incremented
 *						if this is a SendLatestOnly property.
 *
 *	@return true if the description was parsed successfully, false otherwise.
 */
bool MethodDescription::parse( const BW::string & interfaceName,
		DataSectionPtr pSection, Component component,
		unsigned int * pNumLatestEventMembers )
{
	bool result = this->MemberDescription::parse( interfaceName,
			pSection, /* isForClient */ (component == CLIENT),
			pNumLatestEventMembers );

	name_ = pSection->sectionName();

	if (name_ == "")
	{
		WARNING_MSG( "MethodDescription::parse: Missing <Name> tag\n" );
		return false;
	}

	DataSectionPtr pExposed = pSection->findChild( "Exposed" );
	if (pExposed)
	{
		if (component == CLIENT)
		{
			ERROR_MSG( "MethodDescription::parse: "
				"Unable to use <Exposed> tag in client method %s.\n",
				name_.c_str() );
			return false;
		}

		BW::string exposedFlag = pExposed->asString();
		if (component == CELL && exposedFlag == "ALL_CLIENTS")
		{
			this->setExposedToAllClients();
		}
		else if (exposedFlag == "OWN_CLIENT")
		{
			this->setExposedToOwnClient();
		}
		else if (exposedFlag.empty())
		{
			this->setExposedToDefault();
		}
		else
		{
			ERROR_MSG( "MethodDescription::parse: "
						"Could not recognise %s flag in <Exposed> tag in %s. "
						"Use <Exposed> [%s OWN_CLIENT ] </Exposed>"
						"\n", exposedFlag.c_str(), name_.c_str(),
						component == CELL ? " ALL_CLIENTS |" : "");
			return false;
		}
	}


	DataSectionPtr pReplayExposureLevel = 
		pSection->findChild( "ReplayExposureLevel" );

	if (pReplayExposureLevel)
	{
		BW::string replayExposureLevelString = pReplayExposureLevel->asString();

		if (replayExposureLevelString == "NONE")
		{
			replayExposureLevel_ = REPLAY_EXPOSURE_LEVEL_NONE;
		}
		else if (replayExposureLevelString == "OTHER_CLIENTS")
		{
			replayExposureLevel_ = REPLAY_EXPOSURE_LEVEL_OTHER_CLIENTS;
		}
		else if (replayExposureLevelString == "ALL_CLIENTS" )
		{
			replayExposureLevel_ = REPLAY_EXPOSURE_LEVEL_ALL_CLIENTS;
		}
		else
		{
			ERROR_MSG( "MethodDescription::parse: "
					"Could not recognise %s flag in <ReplayExposureLevel> tag "
					"in %s. Use <ReplayExposureLevel> "
					"NONE | OTHER_CLIENTS | ALL_CLIENTS </ReplayExposureLevel>\n",
				replayExposureLevelString.c_str(), name_.c_str() );

			return false;
		}
	}

	this->component( component );

	DataSectionPtr pArgs = pSection->findChild( "Args" );
	bool hasOldStyleArgs = false;

	if (!pArgs)
	{
		pArgs = pSection;
		hasOldStyleArgs = true;
	}

	if (!args_.parse( pArgs, hasOldStyleArgs ))
	{
		ERROR_MSG( "MethodDescription::parse: "
					"Failed to parse arguments of %s\n",
				name_.c_str() );

		return false;
	}

	DataSectionPtr pReturnValues = pSection->findChild( "ReturnValues" );

	if (pReturnValues)
	{
		hasReturnValues_ = true;

		if (!returnValues_.parse( pReturnValues ))
		{
			ERROR_MSG( "MethodDescription::parse: "
					"Failed to parse return values of %s\n",
				name_.c_str() );
			return false;
		}

#ifdef MF_SERVER
		if (!ReturnValuesHandler::staticInit())
		{
			// TODO: Put this back. Removed for WebIntegration.
			// return false;
		}
#endif // MF_SERVER
	}

	priority_ = pSection->readFloat( "DetailDistance", FLT_MAX );

	if (!isEqual( priority_, FLT_MAX ))
	{
		priority_ *= priority_;
	}

	return result;
}


#if defined( SCRIPT_PYTHON )
/**
 *	This method checks whether the input tuple of arguments are valid
 *	to call this method with (by adding them to a stream)
 *	@see addToStream
 *
 *	@param args		A Python tuple object containing the arguments.
 *	@param exposedExplicit		Flag indicating whether or not the
 *				source for exposed methods is specified explicitly
 *	@param generateException	Flag indicating whether or not to
 *				generate a python exception describing the problem
 *				if the arguments are not valid.
 *
 *	@return	True if the arguments are valid.
 */
bool MethodDescription::areValidArgs( bool exposedExplicit, ScriptObject args,
	bool generateException ) const
{
	// see what the first ordinary argument index should be
	int firstOrdinaryArg =
		(exposedExplicit && this->isExposed() && this->component() == CELL);

	bool result = args_.checkValid( ScriptTuple( args ),
		name_.c_str(), firstOrdinaryArg );

	if (!result && !generateException)
	{
		PyErr_Clear();
	}

	return result;
}
#endif // defined( SCRIPT_PYTHON )


/**
 *	This method adds a client-server method call to the input stream. It assumes
 *	that the arguments are valid, according to 'areValidArgs'. The method header
 *	will already have been put onto the stream by the time this is called, so we
 *	have to complete the message even if we put on garbage.
 *	@see areValidArgs
 *
 *	@param source	A DataSource containing the method arguments
 *	@param stream	The stream to which the method call should be added.
 *
 *	@return	True if successful.
 */
bool MethodDescription::addToStream( DataSource & source,
	BinaryOStream & stream ) const
{
	MF_ASSERT( this->component() != MethodDescription::CLIENT );

	// Note: Calls from client to the server have the sub-message id added in
	// the ServerConnection::start*EntityMessage() methods.

	return args_.addToStream( source, stream );
}


/**
 *	This method adds a server-server method call to the input stream. It assumes
 *	that the arguments are valid, according to 'areValidArgs'. The method header
 *	will already have been put onto the stream by the time this is called, so we
 *	have to complete the message even if we put on garbage.
 *	@see areValidArgs
 *
 *	@param source	A DataSource containing the method arguments
 *	@param stream	The stream to which the method call should be added.
 *	@param sourceEntityID
 *					The entity ID calling the method. This is used when a
 *					client-exposed method is called from another server
 *					component, and should be NULL_ENTITY_ID otherwise.
 *
 *	@return	True if successful.
 */
bool MethodDescription::addToServerStream( DataSource & source,
	BinaryOStream & stream, EntityID sourceEntityID ) const
{
	MF_ASSERT( this->component() != MethodDescription::CLIENT );

	if (this->isExposed() && this->component() == MethodDescription::CELL)
	{
		stream << sourceEntityID;
	}

	return args_.addToStream( source, stream );
}


/**
 *	This method adds a server-client method call to the input stream. It assumes
 *	that the arguments are valid, according to 'areValidArgs'. The method header
 *	will already have been put onto the stream by the time this is called, so we
 *	have to complete the message even if we put on garbage.
 *	@see areValidArgs
 *
 *	@param source	A DataSource containing the method arguments
 *	@param stream	The stream to which the method call should be added.
 *	@param targetEntityID
 *					The EntityID of the entity on the client receiving the
 *					method call. This is required for warning output on oversize
 *					messages when we're sending to client.
 *
 *	@return	True if successful.
 */
bool MethodDescription::addToClientStream( DataSource & source,
	BinaryOStream & stream, EntityID targetEntityID ) const
{
	MF_ASSERT( this->component() == MethodDescription::CLIENT );
	MF_ASSERT( this->isExposed() );

	std::auto_ptr< MemoryOStream > pLengthStream( new MemoryOStream );

	this->addSubMessageIDToStream( *pLengthStream );

	bool isOkay = args_.addToStream( source, *pLengthStream );

	size_t length = static_cast< size_t >( pLengthStream->size() );

	if (!this->checkForOversizeLength( length, targetEntityID ))
	{
		return false;
	}

	stream.transfer( *pLengthStream, static_cast<int>(length) );

	return isOkay;
}


/**
 *	This method extracts the sourceEntityID preprended to the args tuple
 *	by the BigWorld Script Server->Server calling convention, for passing
 *	to MethodDescription::addToServerStream.
 *
 *	@param	args			The original tuple from the Script call
 *	@param	sourceEntityID	The extracted source Entity ID
 *	@return	A ScriptTuple which should be provided to
 *							MethodDescription::addToServerStream
 */
ScriptTuple MethodDescription::extractSourceEntityID( ScriptTuple args,
	EntityID & sourceEntityID ) const
{
	if (this->component() != MethodDescription::CELL || !this->isExposed())
	{
		// No sourceEntityID needed
		sourceEntityID = NULL_ENTITY_ID;
		return args;
	}

	if (args.size() == 0)
	{
		ERROR_MSG( "MethodDescription::extractSourceEntityID: Attempting to "
				"call exposed CELL method without a source entity ID\n" );
		return ScriptTuple();
	}

	ScriptObject sourceObj = args.getItem( 0 );
	if (sourceObj.isNone())
	{
		sourceEntityID = NULL_ENTITY_ID;
	}
	else if (!sourceObj.convertTo( sourceEntityID, ScriptErrorClear() ))
	{
		ScriptObject sourceID =
			sourceObj.getAttribute( "id", ScriptErrorClear() );
		if (!sourceID.exists() || 
			!sourceID.convertTo( sourceEntityID, 
			ScriptErrorPrint( "MethodDescription::extractSourceEntityID" ) ))
		{
			ERROR_MSG( "MethodDescription::extractSourceEntityID: Could not "
					"get an EntityID from the source entity description\n" );
			return ScriptTuple();
		}
	}

	// We've extracted sourceEntityID, now we need to recreate the tuple
	// without the first argument.

	ScriptTuple newArgs = ScriptTuple::create( args.size() - 1 );

	for (size_t i = 1; i < (size_t)args.size(); ++i)
	{
		newArgs.setItem( i - 1, args.getItem( i ) );
	}

	return newArgs;
}


/**
 *	This method returns the number of bytes used to stream methods invocations.
 *
 *	@param isFromServer 	If true, the method is being called from the server
 *							to another component, otherwise false.
 *
 *	@return 				Fixed-length sized messages are indicated by the
 *							return of a non-negative lengths. For
 *							variable-length sized messages, these are indicated
 *							by a negative number, which is the negative of the
 *							preferred number of bytes used in the nominal case.
 */
int MethodDescription::streamSize( bool isFromServer ) const
{
	int result = args_.streamSize();

	if ((result < 0) || (this->isExposed() && this->hasSubMsgID()))
	{
		return -static_cast< int >( this->getVarLenHeaderSize() );
	}

	// add the id of the object if it's not the client talking to the cell
	if (this->isExposed() &&
			isFromServer &&
			(this->component() == CELL))
	{
		result += sizeof( EntityID );
	}

#if 0
	// TODO: Check this. Will never get here because sub-indexed methods
	// are always variable-size.

	// Server to server method calls don't expect the sub-index to be on the
	// stream. Only Client-Server method calls use sub-indexing.
	bool isToClient = (this->component() == CLIENT);

	if (isFromServer == isToClient)
	{
		if (this->isExposed() && this->hasSubMsgID())
		{
			result += sizeof( uint8 );
		}
	}
#endif

	return result;
}


#if defined( SCRIPT_PYTHON )
/**
 *	This method converts keyword arguments into positional arguments.
 */
ScriptTuple MethodDescription::convertKeywordArgs( ScriptTuple args,
		ScriptDict kwargs ) const
{
	return ScriptTuple( args_.convertKeywordArgs( args, kwargs ) );
}


/**
 *	This method returns the header size for a variable size message taking into
 *	account the special case for sub-indexed methods.
 */
uint MethodDescription::getVarLenHeaderSize() const
{
	if (this->isExposed() && this->hasSubMsgID())
	{
		// Sub-indexed methods are always variable-size and must use a fixed
		// header size so the receiving side is able to determine message size
		// without reading the sub message ID.
		return Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE;
	}
	else
	{
		return this->MemberDescription::getVarLenHeaderSize();
	}
}


/**
 *	This method handles the case where a called method raises an exception.
 *	If this method is two-way, it sends back the error.
 */
void MethodDescription::handleException( int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface,
	   		bool shouldPrintError ) const
{
	if (this->hasReturnValues())
	{
		INFO_MSG( "MethodDescription::handleException: "
					"Two-way script call to '%s' is returning an error\n",
				name_.c_str() );
		MethodDescription::sendCurrentExceptionReturnValuesError(
				replyID, replyAddr, networkInterface );
	}

	if (shouldPrintError)
	{
		WARNING_MSG( "MethodDescription::handleException: "
				"Script call to '%s' raised an exception. Please check SCRIPT log "
				"output for script errors:\n",
			name_.c_str() );
		PyErr_Print();
	}
	else
	{
		PyErr_Clear();
	}
}


/**
 *	This method sends the current Python exception as a ReturnValues error.
 */
void MethodDescription::sendCurrentExceptionReturnValuesError( int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface )
{
	MF_ASSERT( PyErr_Occurred() );

	PyObject * errtype = NULL;
	PyObject * errvalue = NULL;
	PyObject * traceback = NULL;

	PyErr_Fetch( &errtype, &errvalue, &traceback );
	PyErr_NormalizeException( &errtype, &errvalue, &traceback );

	MethodDescription::sendReturnValuesError( 
			ScriptObject( errvalue, ScriptObject::FROM_BORROWED_REFERENCE),
			replyID, replyAddr, networkInterface );

	// The error may be printed in MethodDescription::handleException
	PyErr_Restore( errtype, errvalue, traceback );
}



/**
 *	This method calls the method described by this object.
 *
 *	@param self the object whose method is called
 *	@param data the incoming data stream
 *	@param sourceID the source object ID if it came from a client
 *	@param replyID the reply ID, -1 if none
 *	@param pReplyAddr the reply address or NULL if none
 *	@param pInterface the nub to use, or NULL if none
 *
 *	@return		True if successful, otherwise false.
 */
bool MethodDescription::callMethod( ScriptObject self,
	BinaryIStream & data,
	EntityID sourceID,
	int replyID,
	const Mercury::Address * pReplyAddr,
	Mercury::NetworkInterface * pInterface ) const
{
//	TRACE_MSG( "MethodDescription::callMethod: %s\n", name_.c_str() );

#if ENABLE_WATCHERS
	this->stats().countReceived( data.remainingLength() );
#endif // ENABLE_WATCHERS

	ScriptObject spTuple( this->getArgsAsTuple( data, sourceID ) );

	// If we couldn't stream off the args correctly, abort this method call
	if (spTuple.get() == NULL)
	{
		ERROR_MSG( "MethodDescription::callMethod: "
				   "Couldn't stream off args for %s correctly, "
				   "aborting method call!\n", name_.c_str() );
		return false;
	}

	ScriptObject pFunction = this->getMethodFrom( self );

	if (!pFunction)
	{
		// This warning is disable because it generates loads of spew in 'bots'
		// for all the unimplemented methods.  If you really have an
		// unimplemented method in a non-bots process you will get a warning on
		// startup so you should be able to live without this extra warning
		// anyway.
//  		ERROR_MSG( "MethodDescription::callMethod: "
// 			"No such method %s in object at 0x%08X\n", name_.c_str(), self );

		this->handleException( replyID, *pReplyAddr, *pInterface,
				/* shouldPrintError */ false );

		return false;
	}

#if ENABLE_WATCHERS
	uint64 curTime = timestamp();
#endif

	ScriptObject returnValues(
			PyObject_CallObject( pFunction.get(), spTuple.get() ),
			PyObjectPtr::STEAL_REFERENCE );

	if (!returnValues)
	{
		this->handleException( replyID, *pReplyAddr, *pInterface,
				/* shouldPrintError */ true );
		return false;
	}

	if (this->hasReturnValues())
	{
		MF_ASSERT( pReplyAddr );
		MF_ASSERT( replyID != Mercury::REPLY_ID_NONE );

		ScriptObject pAddCallbacks =
			returnValues.getAttribute( "addCallbacks", ScriptErrorClear() );

		if (pAddCallbacks)
		{
			ScriptObject pDelayedReturn( new PyDeferredResponse( *this, replyID,
						*pReplyAddr, *pInterface ),
				   ScriptObject::STEAL_REFERENCE );

			ScriptObject pCallback = pDelayedReturn.getAttribute( 
				"callback", ScriptErrorPrint() );
			ScriptObject pErrback =  pDelayedReturn.getAttribute( 
				"errback", ScriptErrorPrint() );

			MF_ASSERT( pCallback && pErrback );

			PyObjectPtr pResult(
				PyObject_CallFunctionObjArgs( pAddCallbacks.get(),
						pCallback.get(), pErrback.get(),
						NULL ),
					PyObjectPtr::STEAL_REFERENCE );

			if (!pResult)
			{
				Script::printError();
			}
		}
		else
		{
			ScriptTuple returnTuple = ScriptTuple::create( returnValues );
			if (!returnTuple || !this->areValidReturnValues( returnTuple ))
			{
				this->sendReturnValuesError(
						this->createErrorObject( "BWInvalidArgsError",
							"Invalid return arguments" ),
						replyID, *pReplyAddr, *pInterface );

				WARNING_MSG( "PyDeferredResponse::callback: "
							"Sent failure response for %s\n",
						this->name().c_str() );
				return false;
			}

			ScriptDataSource returnSource( returnValues );
			this->sendReturnValues( returnSource, replyID, *pReplyAddr,
					*pInterface );
		}
	}

#if ENABLE_WATCHERS
	uint64 currTimeSpent = timestamp() - curTime;
	timeSpent_ += currTimeSpent;
	timeSpentMax_ = std::max( (uint64)timeSpentMax_, currTimeSpent );
	timesCalled_ ++;
#endif

	return true;
}


/**
 *	This method returns whether a tuple of values is an appropriate return
 *	value.
 *	@see addReturnValues
 */
bool MethodDescription::areValidReturnValues( ScriptTuple values ) const
{
	bool result = returnValues_.checkValid( values, name_.c_str() );

	if (!result)
	{
		WARNING_MSG( "MethodDescription::areValidReturnValues: "
			"Invalid return values for method %s\n",
			this->name().c_str() );
		Script::printError();
	}

	return result;
}


/**
 *	This method sends return values for a two-way call. It assumes that the
 *	arguments are valid, according to 'areValidReturnValues'.
 *	@see areValidReturnValues
 *	@see addReturnValues
 *
 *	@param source			A DataSource containing the method arguments
 *	@param replyID			The ID of the original method call's request
 *	@param replyAddr		The Address to send the reply to
 *	@param networkInterface	The NetworkInterface to send the reply via
 *
 *	@return	True if successful.
 */
bool MethodDescription::sendReturnValues( DataSource & source,
	int replyID,
	const Mercury::Address & replyAddr,
	Mercury::NetworkInterface & networkInterface ) const
{
	Mercury::AutoSendBundle spBundle( networkInterface, replyAddr );
	spBundle->startReply( replyID );

	return this->addReturnValues( source, *spBundle );
}


/**
 *	This method adds a tuple of return values to a stream. It assumes that the
 *	arguments are valid, according to 'areValidReturnValues'.
 *	@see areValidReturnValues
 *
 *	@param source	A DataSource containing the method arguments
 *	@param stream	The stream to which the method call should be added.
 *
 *	@return	True if successful.
 */
bool MethodDescription::addReturnValues( DataSource & source,
		BinaryOStream & stream ) const
{
	// Zero for success.
	stream << uint8( 0 );

	return returnValues_.addToStream( source, stream );
}


/**
 *	This method is a convenience for sending a standard Exception to the caller.
 */
void MethodDescription::sendReturnValuesError( const char * excType,
			const char * msg,
			int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface )
{
#if defined( SCRIPT_PYTHON )
	ScriptObject pError = MethodDescription::createErrorObject( excType,
			msg );

	MethodDescription::sendReturnValuesError( pError, replyID,
			replyAddr, networkInterface );
#else // !SCRIPT_PYTHON
	MF_ASSERT( !"Not implemented" );
#endif // SCRIPT_PYTHON
}


/**
 *	This method sends an errback response to the caller. The error data is sent
 *	as a string.
 */
void MethodDescription::sendReturnValuesError( ScriptObject pFailure,
			int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface )
{
	Mercury::AutoSendBundle pBundle( networkInterface, replyAddr );
	pBundle->startReply( replyID );

	MethodDescription::addReturnValuesError( pFailure, *pBundle );
}


/**
 *
 */
void MethodDescription::addReturnValuesError( ScriptObject pFailure,
		BinaryOStream & stream )
{
#if defined( SCRIPT_PYTHON )

	// One for failure
	stream << uint8( 1 );

	stream << pFailure.get()->ob_type->tp_name;

	ScriptObject args = pFailure.getAttribute( "args",
		ScriptErrorClear() );

	if (!args)
	{
		ScriptObject pFailureRepr( PyObject_Repr( pFailure.get() ),
				ScriptObject::STEAL_REFERENCE );
		ERROR_MSG( "MethodDescription::addReturnValuesError: "
					"Error has no args member.\n"
					"Error contents: %s",
				PyString_AsString( pFailureRepr.get() ) );
		args = ScriptTuple::create( 0 );
	}

	stream << Pickler::pickle( args );

#else
	MF_ASSERT( !"Not implemented" );
#endif // SCRIPT_PYTHON
}


/**
 *	This method returns true if recording should occur for the given recording
 *	option.
 */
bool MethodDescription::shouldRecord( RecordingOption recordingOption ) const
{
	return (recordingOption >= RECORDING_OPTION_RECORD) || 
		(recordingOption == RECORDING_OPTION_METHOD_DEFAULT && 
			(replayExposureLevel_ >=
				MethodDescription::REPLAY_EXPOSURE_LEVEL_ALL_CLIENTS)); 
}


/**
 *
 */
ScriptObject MethodDescription::getMethodFrom( ScriptObject object ) const
{
	return ScriptObject( PyObject_GetAttrString( object.get(),
				const_cast< char * >( this->name().c_str() ) ),
			ScriptObject::STEAL_REFERENCE );
}


/**
 *
 */
bool MethodDescription::isMethodHandledBy(
		const ScriptObject & classObject ) const
{
	ScriptObject method = this->getMethodFrom( classObject );

	if (!method)
	{
		Script::clearError();
		return false;
	}

	return true;
}


/**
 *	This returns a tuple containing the arguments of the method described by
 *  this class.
 *
 *	@param data	A stream containing the arguments for the call.
 *	@param sourceID	The ID to pass as the source of exposed method calls.
 *	@return	A ScriptObject containing the arguments as a tuple.
 */
ScriptObject MethodDescription::getArgsAsTuple( BinaryIStream & data,
	EntityID sourceID ) const
{
	ScriptDataSink sink;

	if (!this->createArgsFromStream( data, sink, sourceID ))
	{
		return ScriptObject();
	}

	return sink.finalise();
}


/**
 *	This method outputs the arguments for this method from the given stream
 *	into the given DataSink as a tuple.
 *	@param data	A stream containing the arguments for the call.
 *	@param sink	The DataSink to output the argument tuple to.
 *	@param sourceID	The ID to pass as the source of exposed method calls.
 *	@return	true unless output failed.
 */
bool MethodDescription::createArgsFromStream( BinaryIStream & data,
	DataSink & sink, EntityID sourceID ) const
{
	bool hasImplicitSourceID = this->isExposed() && (this->component() == CELL);

	return args_.createFromStream( data, sink, name_,
		hasImplicitSourceID ? &sourceID : NULL );
}


/**
 *	This method outputs the arguments for this method from the given stream
 *	into the given DataSink as a tuple.
 *	@param data	A stream containing the arguments for the call.
 *	@param sink	The DataSink to output the argument tuple to.
 *	@return	true unless output failed.
 */
bool MethodDescription::createArgsFromStream( BinaryIStream & data,
	DataSink & sink ) const
{
	return args_.createFromStream( data, sink, name_, NULL );
}

/**
 *	This method creates either the return values or a failure object.
 *
 *	@return 0 if return values, non zero for failure object.
 */
uint8 MethodDescription::createReturnValuesOrFailureFromStream(
		BinaryIStream & data, ScriptObject & pResult ) const
{
	uint8 isError = 0;
	data >> isError;

	if (data.error())
	{
		isError = 1;
	}

	if (isError)
	{
		BW::string errorType;
		data >> errorType;

		BW::string errorArgsStr;
		data >> errorArgsStr;

		ScriptObject args;

		if (!errorArgsStr.empty())
		{
			args = Pickler::unpickle( errorArgsStr );
		}
		else
		{
			args = ScriptTuple::create( 0 );
		}

		pResult =
			MethodDescription::createErrorObject( errorType.c_str(), args );
	}
	else
	{
		ScriptDataSink sink;
		if (this->createReturnValuesFromStream( data, sink ))
		{
			pResult = sink.finalise();
		}
		else
		{
			pResult = ScriptObject();
		}
	}

	return isError;
}
#endif // SCRIPT_PYTHON


/**
 *	This method creates a tuple of return values from a stream.
 */
bool MethodDescription::createReturnValuesFromStream(
	BinaryIStream & data, DataSink & sink ) const
{
	return returnValues_.createFromStream( data, sink, name_ );
}


/**
 *	This method creates a dictionary from a tuple of return values.
 */
ScriptDict MethodDescription::createDictFromTuple( ScriptTuple args ) const
{
	return returnValues_.createDictFromTuple( args );
}


/**
 *	This method adds the sub message id to the stream, if necessary.
 */
void MethodDescription::addSubMessageIDToStream( BinaryOStream & stream ) const
{
	// If this is triggered, potentially have not called
	// MethodDescription::setExposedMsgID correctly. This is triggered by
	// passing the appropriate ExposedMethodMessageRange into
	// EntityDescriptionMap::parse().
	MF_ASSERT( exposedMsgID_ != -1 );

	if (exposedSubMsgID_ != -1)
	{
		stream << uint8( exposedSubMsgID_ );
	}
}


/**
 *	This method sets the exposed message and sub message ids.
 */
void MethodDescription::setExposedMsgID( int exposedID, int numExposed,
		const ExposedMethodMessageRange * pRange )
{
	exposedIndex_ = exposedID;

	if (pRange)
	{
		pRange->msgIDFromExposedID( exposedID, numExposed,
				exposedMsgID_, exposedSubMsgID_ );
		const uint varHdrSize = this->MemberDescription::getVarLenHeaderSize();
		if (exposedSubMsgID_ != -1 &&
			varHdrSize != Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE)
		{
			// All methods that need a sub-index must use variable size messages
			// with the default header length.
			WARNING_MSG( "MethodDescription::setExposedMsgID: "
					"VariableLengthHeaderSize value (%u) will be ignored "
					"for %s because a sub-index is required for this method, "
					"the default %u-byte(s) header will be used instead.\n",
				varHdrSize, this->toString().c_str(),
				Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE );
		}
	}
}


/**
 *	This method adds this object to the input MD5 object.
 */
void MethodDescription::addToMD5( MD5 & md5, int legacyExposedIndex ) const
{
	this->MemberDescription::addToMD5( md5 );

	md5.append( &flags_, sizeof( flags_ ) );
	args_.addToMD5( md5 );

	md5.append( &legacyExposedIndex, sizeof( legacyExposedIndex ) );
}


/**
 *	This method returns the safety level of arguments:
 *	PYTHON = unsafe
 *	MAILBOX = unusable
 */
int MethodDescription::clientSafety() const
{
	return args_.clientSafety();
}


/*
 *	Override from MemberDescription.
 */
BW::string MethodDescription::toString() const
{
	BW::ostringstream oss;
	oss << "Method " << interfaceName_ << "." << name_ << "()";
	return oss.str();
}


#if ENABLE_WATCHERS
/**
 * 	This method returns the generic watcher for Method Descriptions.
 */
WatcherPtr MethodDescription::pWatcher()
{
	static WatcherPtr watchMe = NULL;

	if (watchMe == NULL)
	{
		MethodDescription *pNull = NULL;
		watchMe = new DirectoryWatcher();

		watchMe->addChild( "name", makeWatcher( pNull->name_ ) );
		watchMe->addChild( "interfaceName",
			makeWatcher( pNull->interfaceName_) );
		watchMe->addChild( "variableLengthHeaderSize",
			makeWatcher( pNull->varLenHeaderSize_ ) );
		watchMe->addChild( "oversizeWarnLevel",
			makeNonRefWatcher( *(static_cast< MemberDescription * >(pNull)),
				&MemberDescription::oversizeWarnLevelAsString,
				&MemberDescription::watcherOversizeWarnLevelFromString ) );
		watchMe->addChild( "priority", makeWatcher( pNull->priority_ ) );
		watchMe->addChild( "internalIndex", 
						   makeWatcher( pNull->internalIndex_ ) );
		watchMe->addChild( "exposedIndex", 
						   makeWatcher( pNull->exposedIndex_ ) );
		watchMe->addChild( "exposedMsgID", 
						   makeWatcher( pNull->exposedMsgID_ ) );
		watchMe->addChild( "exposedSubMsgID", 
						   makeWatcher( pNull->exposedSubMsgID_ ) );
		watchMe->addChild( "timeSpent", makeWatcher( pNull->timeSpent_ ) );	
		watchMe->addChild( "timeSpentMax", makeWatcher( pNull->timeSpentMax_ ) );	
		watchMe->addChild( "timesCalled", makeWatcher( pNull->timesCalled_ ) );	
		watchMe->addChild( "latestEventIndex", 
						   makeWatcher( pNull->latestEventIndex_ ));
		watchMe->addChild( "isReliable", 
						   makeWatcher( pNull->isReliable_ ));
		watchMe->addChild( "stats", EntityMemberStats::pWatcher(),
						   &pNull->stats_ );
	}
	return watchMe;
}

#endif

/**
 *	This method adds a component name to the list of components the method is
 *	implemented by. Returns false if the component is already in the list or 
 *	conflicts with previously added components.
 */
bool MethodDescription::addImplementingComponent( 
	const BW::string & componentName )
{
	if (!componentName.empty()) // it's a component
	{
		if (components_.find( BW::string() ) != components_.end())
		{
			ERROR_MSG( "MethodDescription::addImplementingComponent: "
				"Method '%s' of component '%s' already defined in entity scope",
				name_.c_str(), componentName.c_str() );
			return false;
		}
		isComponentised_ = true;
	}
	else if (isComponentised_) // it's entity but method already delegated to component
	{
		ERROR_MSG( "MethodDescription::addImplementingComponent: "
			"Entity method '%s' already defined by one or more components",
			name_.c_str() );
		return false;
	}

	if (!components_.insert( componentName ).second)
	{
		ERROR_MSG( "MethodDescription::addImplementingComponent: "
			"Method '%s' already defined by %s",
			name_.c_str(), 
			componentName.empty() ? "entity" : 
			("'" + componentName + "' component").c_str() );

		return false;
	}

	return true;
}

/**
 *	This method checks if the method is implemented by component with given name.
 */
bool MethodDescription::isImplementedBy( 
	const BW::string & componentName ) const
{
	return components_.find( componentName ) != components_.end();
}


/**
 *	This method checks if the method is implemented by delegate.
 */
bool MethodDescription::isComponentised() const
{
	return isComponentised_;
}

bool MethodDescription::isSignatureEqual( const MethodDescription & other ) const
{
	return flags_ == other.flags_ &&
		   args_ == other.args_ &&
		   returnValues_ == other.returnValues_;
}

BW_END_NAMESPACE

// method_description.cpp
