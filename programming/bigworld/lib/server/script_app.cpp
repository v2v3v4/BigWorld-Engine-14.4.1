#include "script/first_include.hpp"

#include "script_app.hpp"

#include "script_frequent_tasks.hpp"
#include "server_app_config.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/py_deferred.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/py_traceback.hpp"

#include "server/bwconfig.hpp"
#include "server/python_server.hpp"

DECLARE_DEBUG_COMPONENT2( "Server", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ScriptApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ScriptApp::ScriptApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface ) :
	ServerApp( mainDispatcher, interface ),
	pPythonServer_( NULL )
{
	ScriptFrequentTasks::init( &updatables_ );

	scriptEvents_.createEventType( "onInit" );
	scriptEvents_.createEventType( "onFini" );
}


/**
 *	Destructor.
 */
ScriptApp::~ScriptApp()
{
	bw_safe_delete( pPythonServer_ );

	scriptEvents_.clear();
	Script::fini();
}


/**
 * 	Initialisation function.
 *
 *	This must be called from subclasses's init().
 */
bool ScriptApp::init( int argc, char * argv[] )
{
	if (!this->ServerApp::init( argc, argv ))
	{
		return false;
	}

	return true;
}


/**
 * 	Finalization function.
 *
 *	This must be called from subclasses's fini().
 */
void ScriptApp::fini()
{
	this->triggerDelayableEvent( "onFini", PyTuple_New( 0 ) );
}


/**
 *	This method loads the personality script associated with this application.
 */
bool ScriptApp::initPersonality()
{
	if (!Personality::import( ServerAppConfig::personality() ))
	{
		WARNING_MSG( "ScriptApp::initPersonality: "
					"No personality script '%s.py'\n",
				ServerAppConfig::personality().c_str() );
		return false;
	}

	scriptEvents_.initFromPersonality( Personality::instance() );

	return true;
}


/**
 *	This class is used to receive the responses from any Twisted Deferred
 *	objects returned by onInit or onFini listeners.
 */
class DelayedEvent : public PyObjectPlus
{
	Py_Header( DelayedEvent, PyObjectPlus )

public:
	DelayedEvent( Mercury::EventDispatcher & dispatcher ) :
		PyObjectPlus( &DelayedEvent::s_type_ ),
		dispatcher_( dispatcher ),
		outstandingDelays_( 0 )
	{
		dispatcher_.breakProcessing( false );
	}

	void addTo( ScriptObject pAddCallbacks )
	{
		this->increment();

		ScriptObject thisScript = ScriptObject( this,
			ScriptObject::FROM_BORROWED_REFERENCE );
		ScriptObject pCallback = thisScript.getAttribute( "callback",
			ScriptErrorPrint() );
		ScriptObject pErrback = thisScript.getAttribute( "errback",
			ScriptErrorPrint() );

		MF_ASSERT( pCallback && pErrback );

		PyObjectPtr pResult(
				PyObject_CallFunctionObjArgs( pAddCallbacks.get(),
						pCallback.get(), pErrback.get(), NULL ),
					PyObjectPtr::STEAL_REFERENCE );

		if (!pResult)
		{
			Script::printError();
		}
	}

	void increment()
	{
		++outstandingDelays_;
	}

	void decrement()
	{
		MF_ASSERT( outstandingDelays_ > 0 );

		if (--outstandingDelays_ == 0)
		{
			dispatcher_.breakProcessing();
		}
	}

	void callback( ScriptObject value )
	{
		this->decrement();
	}

	void errback( ScriptObject value )
	{
		WARNING_MSG( "DelayedEvent::errback: Deferred had errback called.\n" );
		this->decrement();
	}

	PY_AUTO_METHOD_DECLARE( RETVOID, callback, ARG( ScriptObject, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, errback, ARG( ScriptObject, END ) )

private:
	Mercury::EventDispatcher & dispatcher_;
	int outstandingDelays_;
};


PY_TYPEOBJECT( DelayedEvent )

PY_BEGIN_METHODS( DelayedEvent )
	PY_METHOD( callback )
	PY_METHOD( errback )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( DelayedEvent )
PY_END_ATTRIBUTES()


/**
 *	This method initialises the Python interpreter.
 */
bool ScriptApp::initScript( const char * componentName,
		const char * scriptPath1, const char * scriptPath2 )
{
	PyImportPaths paths;

	paths.addResPath( scriptPath1 );

	if (scriptPath2)
	{
		paths.addResPath( scriptPath2 );
	}

	paths.addResPath( EntityDef::Constants::serverCommonPath() );

	if (!Script::init( paths, componentName ))
	{
		return false;
	}

	ScriptModule bigWorld = ScriptModule::getOrCreate( "BigWorld",
		ScriptErrorPrint( "Failed to create BigWorld module" ) );
	/*~ attribute BigWorld.serverMode
	 *	current mode of the BW server -
	 *	'center' / 'periphery' / 'standalone'
	 */
	bigWorld.setAttribute( "serverMode",
		ScriptString::create( ServerAppConfig::serverMode() ),
		ScriptErrorPrint() );

	if (!PyDeferred::staticInit())
	{
		ERROR_MSG( "ScriptApp::initScript: Initialising PyDeferred failed.\n" );
		return false;
	}

	return true;
}


/**
 *	This method triggers an event that can be delayed by the listeners returning
 *	Twisted Deferred instances.
 */
bool ScriptApp::triggerDelayableEvent( const char * eventName,
		PyObject * pArgs )
{
	ScriptList resultsList = ScriptList::create();
	bool result =
		this->scriptEvents().triggerEvent( eventName, pArgs, resultsList );

	int size = resultsList.size();
	DelayedEvent * pDelayedEvent = NULL;

	for (int i = 0; i < size; ++i)
	{
		ScriptObject currResult = resultsList.getItem( i );

		ScriptObject addCallbacks = currResult.getAttribute( "addCallbacks",
			ScriptErrorClear() );

		if (addCallbacks)
		{
			if (!pDelayedEvent)
			{
				pDelayedEvent = new DelayedEvent( mainDispatcher_ );
				// Need a reference here as first call to addCallbacks may
				// trigger its 'callback' immediately causing breakProcessing.
				pDelayedEvent->increment();
			}

			pDelayedEvent->addTo( addCallbacks );
		}
	}

	if (pDelayedEvent)
	{
		pDelayedEvent->decrement();
		Py_DECREF( pDelayedEvent ); 
		if (!mainDispatcher_.processingBroken())
		{
			mainDispatcher_.processContinuously();
		}
	}

	return result;
}


/**
 *	This method triggers the onInit event.
 */
bool ScriptApp::triggerOnInit( bool isReload )
{
	return this->triggerDelayableEvent( "onInit",
			PyTuple_Pack( 1, isReload ? Py_True : Py_False ) );

}


/**
 *	This method starts the Python server.
 */
void ScriptApp::startPythonServer( uint16 port, int appID )
{
	BW::stringstream stream;

	stream << "Welcome to " << this->getAppName();

	if (appID != 0)
	{
		stream << " " << appID;
	}

	stream << "\r\n";
	stream << "Version: " << BWVersion::versionString() << "\r\n";
	stream << "Built: " << this->buildDate();

	pPythonServer_ = new PythonServer( stream.str().c_str() );
	pPythonServer_->startup( mainDispatcher_,
			interface_.address().ip, port,
			BWConfig::get( "monitoringInterface" ).c_str() );
	PyRun_SimpleString( "import BigWorld" );

	MF_WATCH( "pythonServerPort", *pPythonServer_, &PythonServer::port );
}



#if ENABLE_MEMORY_DEBUG
/*~	function BigWorld.saveAllocationsToFile
 *
 *  Save all recorded memory allocations into the text file for further analysis
 *  File format description:
 *
 *	number_of_callstack_hashes
 *  64bit hash;callstack
 *  64bit hash;callstack
 *  ....
 *  64bit hash;callstack
 *  slotId;64bit callstack hash;allocation size
 *  ...
 *  slotId;64bit callstack hash;allocation size
 *
 *  Use condense_mem_allocs.py script to convert saved file into the following format:
 *  slot id;allocationSize;number of allocations;callstack
 *  ...
 *
 *	@param filename 	The name of file to save data
 */
void saveAllocationsToFile( const BW::string& filename )
{
	BW::Allocator::saveAllocationsToFile( filename.c_str() );
}

PY_AUTO_MODULE_FUNCTION( RETVOID, saveAllocationsToFile, ARG( BW::string, END ), BigWorld );
#endif





BW_END_NAMESPACE

// script_app.hpp
