#include "pch.hpp"

#include "mailbox_base.hpp"
#include "method_description.hpp"
#include "remote_entity_method.hpp"
#include "return_values_handler.hpp"
#include "script_data_source.hpp"

#include "cstdmf/memory_stream.hpp"
#include "pyscript/keyword_parser.hpp"

#include <arpa/inet.h>

DECLARE_DEBUG_COMPONENT2( "EntityDef", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyEntityMailBox
// -----------------------------------------------------------------------------

/*~ class NoModule.PyEntityMailBox
 *	@components{ base, cell }
 *
 *	This is the base class for all mailboxes.
 */
PY_TYPEOBJECT( PyEntityMailBox )

PY_BEGIN_METHODS( PyEntityMailBox )
	PY_PICKLING_METHOD()
	/*~ function PyEntityMailBox callMethod
	 *	@components{ base, cell }
	 *
	 *	This method is used to call a method
	 *	which name is defined at run-time
	 */
	PY_METHOD( callMethod )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEntityMailBox )
	/*~ attribute PyEntityMailBox address
	 *	@components{ base, cell }
	 *
	 *	The address of the mailbox as a tuple consisting of the IP
	 *	dotted-decimal string and port number.
	 */
	PY_ATTRIBUTE( address )
	/*~ attribute PyEntityMailBox id
	 *	@components{ base, cell }
	 *
	 *	The id of the associated Entity.
	 *	@type EntityID
	 */
	PY_ATTRIBUTE( id )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyEntityMailBox )

PyEntityMailBox::Population PyEntityMailBox::s_population_;

/**
 *	This method overrides the virtual pyGetAttribute function.
 */
ScriptObject PyEntityMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();

	const MethodDescription * pDescription = this->findMethod( attr );
	if (pDescription)
	{
		return ScriptObject(
			new RemoteEntityMethod( this, pDescription ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}



typedef BW::map<
	EntityMailBoxRef::Component,PyEntityMailBox::FactoryFn> Fabricators;
typedef BW::vector< std::pair<
	PyEntityMailBox::CheckFn,PyEntityMailBox::ExtractFn> > Interpreters;
typedef BW::vector< PyTypeObject * >  MailBoxTypes;


/**
 *	MailBoxRef helper struct
 */
static struct MailBoxRefRegistry
{
	Fabricators		fabs_;
	Interpreters	inps_;
	MailBoxTypes	mailBoxTypes_;
} * s_pRefReg = NULL;


/**
 *	Constructor.
 */
PyEntityMailBox::PyEntityMailBox( 
			PyTypeObject * pType /* = &PyEntityMailBox::s_type_ */ ) :
		PyObjectPlus( pType ),
		populationIter_()
{
	populationIter_ = s_population_.insert( s_population_.end(), this );
}


/**
 *	Destructor.
 */
PyEntityMailBox::~PyEntityMailBox()
{
	s_population_.erase( populationIter_ );
}


/**
 *	This method calls a method on a given mailbox
 */
PyObject * PyEntityMailBox::callMethod(
		const ScriptString & methodName, const ScriptTuple & arguments  )
{
	const MethodDescription * pDescription = this->findMethod( 
		methodName.c_str() );
	if (!pDescription)
	{
		PyErr_Format( PyExc_TypeError, 
			"Unable to find method %s",	methodName.c_str() );
		return NULL;
	}
	
	return this->callMethod( pDescription, arguments );
}


PyObject * PyEntityMailBox::callMethod( 
		const MethodDescription * pMethodDescription,
		const ScriptTuple & pArgs )
{
	if (!pMethodDescription->areValidArgs( true, pArgs, true ))
	{
		return NULL;
	}

	ReturnValuesHandler * pReplyHandler = NULL;
	PyObjectPtr pDeferred;

	if (pMethodDescription->hasReturnValues())
	{
		pReplyHandler = new ReturnValuesHandler( *pMethodDescription );
		pDeferred = pReplyHandler->getDeferred();
	}

	BinaryOStream * pBOS = this->getStream( *pMethodDescription,
			std::auto_ptr< Mercury::ReplyMessageHandler >( pReplyHandler ) );

	if (pBOS == NULL)
	{
		MF_ASSERT( PyErr_Occurred() );

		PyErr_Print();
		WARNING_MSG( "PyEntityMailBox::callMethod: "
				" Could not get stream to call %s\n",
			pMethodDescription->name().c_str() );
		Py_RETURN_NONE;
	}

#if ENABLE_WATCHERS
	uint32 startingSize = pBOS->size();
#endif

	// Add the arguments to the stream.
	if (pMethodDescription->component() == MethodDescription::CLIENT)
	{
		ScriptDataSource source( pArgs );
		if (!pMethodDescription->addToClientStream( source, *pBOS, this->id() ))
		{
			return NULL;
		}
	}
	else
	{
		EntityID sourceEntityID;
		ScriptTuple remainingArgs =
			pMethodDescription->extractSourceEntityID( pArgs, sourceEntityID );

		ScriptDataSource source( remainingArgs );
		if (!pMethodDescription->addToServerStream( source, *pBOS,
				sourceEntityID ))
		{
			return NULL;
		}
	}

#if ENABLE_WATCHERS
	EntityMailBoxRef ref;
	PyEntityMailBox::reduceToRef( this, &ref );
	int bytesSent = pBOS->size() - startingSize;
	switch(ref.component())
	{
	case EntityMailBoxRef::CELL:
	case EntityMailBoxRef::BASE_VIA_CELL:
	case EntityMailBoxRef::CLIENT_VIA_CELL:
		pMethodDescription->stats().countSentToGhosts( bytesSent );
		break;
	case EntityMailBoxRef::BASE:
	case EntityMailBoxRef::SERVICE:
	case EntityMailBoxRef::CELL_VIA_BASE:
	case EntityMailBoxRef::CLIENT_VIA_BASE:
		pMethodDescription->stats().countSentToBase( bytesSent );
		break;
	case EntityMailBoxRef::CLIENT:
		pMethodDescription->stats().countSentToOwnClient( bytesSent );
		break;
	}
#endif // ENABLE_WATCHERS

	this->sendStream();

	if (pDeferred)
	{
		Py_INCREF( pDeferred.get() );
		return pDeferred.get();
	}
	else
	{
		Py_RETURN_NONE;
	}
}


/**
 *	Construct a PyEntityMailBox or equivalent from an EntityMailBoxRef.
 *	Returns Py_None on failure.
 */
PyObject * PyEntityMailBox::constructFromRef(
	const EntityMailBoxRef & ref )
{
	if (ref.id == 0) Py_RETURN_NONE;

	if (s_pRefReg == NULL) Py_RETURN_NONE;

	Fabricators::iterator found = s_pRefReg->fabs_.find( ref.component() );
	if (found == s_pRefReg->fabs_.end()) Py_RETURN_NONE;

	PyObject * pResult = (*found->second)( ref );

	if (pResult)
	{
		return pResult;
	}
	else
	{
		WARNING_MSG( "PyEntityMailBox::constructFromRef: "
				"Could not create mailbox from id %d. addr %s. component %d\n",
				ref.id, ref.addr.c_str(), ref.component() );
		Py_RETURN_NONE;
	}
}

/**
 *	Register a PyEntityMailBox factory
 */
void PyEntityMailBox::registerMailBoxComponentFactory(
	EntityMailBoxRef::Component c, FactoryFn fn, PyTypeObject * pType )
{
	if (s_pRefReg == NULL) s_pRefReg = new MailBoxRefRegistry();
	s_pRefReg->fabs_.insert( std::make_pair( c, fn ) );
	s_pRefReg->mailBoxTypes_.push_back( pType );
}


/**
 *	Reduce the given python object to an EntityMailBoxRef.
 *	Return false if it cannot be reduced.
 *
 *	@param pObject 			The given Python object to reduce to a mailbox
 *							reference.
 *	@param pRefOutput 		If non-NULL, this is filled with the reduced mailbox
 *							reference.
 *
 *	@return 	Whether the given Python object is reducible to a valid mailbox
 *				reference.
 */
bool PyEntityMailBox::reduceToRef( PyObject * pObject, 
		EntityMailBoxRef * pRefOutput )
{
	if (pObject == Py_None)
	{
		if (pRefOutput)
		{
			pRefOutput->init();
		}
		return true;	
	}

	if (s_pRefReg != NULL)
	{
		for (Interpreters::iterator it = s_pRefReg->inps_.begin();
			it != s_pRefReg->inps_.end();
			it++)
		{
			if ((*it->first)( pObject ))
			{
				if (pRefOutput != NULL)
				{
					*pRefOutput = (*it->second)( pObject );
				}
				return true;
			}
		}
	}

	return false;
}

/**
 *	Register a Python object reducible to an EntityMailBoxRef
 */
void PyEntityMailBox::registerMailBoxRefEquivalent( CheckFn cf, ExtractFn ef )
{
	if (s_pRefReg == NULL) s_pRefReg = new MailBoxRefRegistry();
	s_pRefReg->inps_.push_back( std::make_pair( cf, ef ) );
}


/**
 *	Visit every mail box in the population.
 *
 *	@param visitor 	The PyEntityMailBoxVisitor instance.
 */
void PyEntityMailBox::visit( PyEntityMailBoxVisitor & visitor )
{
	Population::iterator iPopulation = s_population_.begin();

	while (iPopulation != s_population_.end())
	{
		PyEntityMailBox * pMailBox = *iPopulation;
		++iPopulation;
		visitor.onMailBox( pMailBox );
	}
}


// -----------------------------------------------------------------------------
// Section: Pickling and unpickling routines
// -----------------------------------------------------------------------------

/**
 *  This method reduces this mailbox to something that can be pickled
 */
PyObject * PyEntityMailBox::pyPickleReduce()
{
	EntityMailBoxRef embr;
	PyEntityMailBox::reduceToRef( this, &embr );

	PyObject * pConsArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pConsArgs, 0,
		PyString_FromStringAndSize( (char*)&embr, sizeof(embr) ) );

	return pConsArgs;
}

/**
 *	This static function unpickles a previously pickled mailbox
 *	(possibly from a different component)
 */
static PyObject * PyEntityMailBox_pyPickleResolve( const BW::string & str )
{
	if (str.size() != sizeof(EntityMailBoxRef))
	{
		PyErr_SetString( PyExc_ValueError, "PyEntityMailBox_pyPickleResolve: "
			"wrong length string to unpickle" );
		return NULL;
	}

	return PyEntityMailBox::constructFromRef( *(EntityMailBoxRef*)str.data() );
}
PY_AUTO_UNPICKLING_FUNCTION( RETOWN, PyEntityMailBox_pyPickleResolve,
	ARG( BW::string, END ), MailBox )


/**
 *	This method is used to implement the str and repr methods called on an
 *	entity mailbox.
 */
PyObject * PyEntityMailBox::pyRepr()
{
	EntityMailBoxRef embr;
	PyEntityMailBox::reduceToRef( this, &embr );
	const char * location =
		(embr.component() == EntityMailBoxRef::CELL)   ? "Cell" :
		(embr.component() == EntityMailBoxRef::BASE)   ? "Base" :
		(embr.component() == EntityMailBoxRef::CLIENT) ? "Client" :
		(embr.component() == EntityMailBoxRef::BASE_VIA_CELL)   ? "BaseViaCell" :
		(embr.component() == EntityMailBoxRef::CLIENT_VIA_CELL) ? "ClientViaCell" :
		(embr.component() == EntityMailBoxRef::CELL_VIA_BASE)   ? "CellViaBase" :
		(embr.component() == EntityMailBoxRef::CLIENT_VIA_BASE) ? "ClientViaBase" :
		(embr.component() == EntityMailBoxRef::SERVICE) ? "Service" : "???";

	return PyString_FromFormat( "%s mailbox id: %d type: %d addr: %s",
			location, embr.id, embr.type(), embr.addr.c_str() );
}


/**
 *	This method implements the accessor for the address attribute.
 */
PyObject * PyEntityMailBox::pyGet_address()
{
	Mercury::Address address = this->address();
	return Py_BuildValue( "(sH)", address.ipAsString(), 
		ntohs( address.port ) );
}


/**
 *	This method parses the arguments given to a Python call when a mailbox is
 *	called to supply the recording option.
 *
 *	@param recordingOption 	If successful, this is filled with the parsed
 *							recording option.
 *
 *	@return true on success, false otherwise (and an exception is raised)
 */
/*static*/
bool PyEntityMailBox::parseRecordingOptionFromPythonCall(
		PyObject * args, PyObject * kwargs, RecordingOption & recordingOption )
{
	if (PySequence_Size( args ) > 0)
	{
		PyErr_SetString( PyExc_TypeError, 
				"Cannot call a mailbox object with positional parameters" );
		return false;
	}

	bool shouldExposeForReplay = false;
	bool shouldRecordOnly = false;

	KeywordParser options;
	options.add( "shouldExposeForReplay", shouldExposeForReplay );
	options.add( "shouldRecordOnly", shouldRecordOnly );

	KeywordParser::ParseResult parseResult = options.parse( kwargs,
		/* shouldRemove */ true, /* allowOtherArguments */ false );

	if (parseResult == KeywordParser::EXCEPTION_RAISED)
	{
		return false;
	}

	if (parseResult == KeywordParser::NONE_FOUND)
	{
		// No arguments, just return the defaults.
		recordingOption = RECORDING_OPTION_METHOD_DEFAULT;
		return true;
	}

	recordingOption = (shouldRecordOnly ? 
			RECORDING_OPTION_RECORD_ONLY :
			(shouldExposeForReplay ?
				RECORDING_OPTION_RECORD :
				RECORDING_OPTION_DO_NOT_RECORD));
	return true;
}


// -----------------------------------------------------------------------------
// Section: Script converters for EntityMailBoxRef
// -----------------------------------------------------------------------------

/**
 *	setData function
 */
int Script::setData( PyObject * pObj, EntityMailBoxRef & mbr,
		const char * varName )
{
	if (!PyEntityMailBox::reduceToRef( pObj, &mbr ))
	{
		PyErr_Format( PyExc_TypeError, "%s must be set to "
			"a type reducible to an EntityMailBox", varName );
		return -1;
	}

	return 0;
}

/**
 *	getData function
 */
PyObject * Script::getData( const EntityMailBoxRef & mbr )
{
	return PyEntityMailBox::constructFromRef( mbr );
}

BW_END_NAMESPACE

// mailbox_base.cpp
