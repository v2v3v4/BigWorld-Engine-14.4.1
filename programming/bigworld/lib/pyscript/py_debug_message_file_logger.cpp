#include "pch.hpp"

#include "py_debug_message_file_logger.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/datasection.hpp"
#include "script.hpp"


BW_BEGIN_NAMESPACE


/*~ class BigWorld.PyDebugMessageFileLogger
 *	@components{ client, tools }
 *
 *	This class represents an file logger which 
 *	stores the debug message into a file.
 */
PY_TYPEOBJECT( PyDebugMessageFileLogger )

PY_BEGIN_METHODS( PyDebugMessageFileLogger )
	/*~	function PyDebugMessageFileLogger.config
	 *	@components{ client, tools }
	 *
	 *	Set configuration for logging into disk.
	 *
	 *	@{
	 *	>>>	PyDebugMessageFileLogger.config( "outputfile.log", 
	 *			severities = "ERROR, CRITICAL, HACK", 
	 *			categories = "Chunk", 
	 *			sources = "SCRIPT, CPP") 
	 *			openMode = "w") 
	 *	@}
	 *
	 *	@param fileName					String,the file name to write the log into, 
	 *									if "",then executable_file_name.log 
	 *									will be used.
	 *							
	 *	@param severities (optional)	String, combination of: TRACE,DEBUG,INFO,
	 *									NOTICE,WARNING,ERROR,CRITICAL,HACK,ASSET, 
	 *									if not set, then ALL above will be included.
	 *
	 *	@param category (optional)		String, only one category is allowed, 
	 *									ie.: Chunk, if not set, then all categories
	 *									will be included.
	 *
	 *	@param sources (optional)		String, combination of: CPP, SCRIPT, 
	 *									if not set, then ALL above will be included.
	 *
	 *	@param openMode (optional)		String, either "a"(append) or "w"(overwrite), 
	 *									if not set, then "a" will be used.
	 *
	*/
	PY_METHOD( config )

PY_END_METHODS()
	
PY_BEGIN_ATTRIBUTES( PyDebugMessageFileLogger )
	/*~ attribute PyDebugMessageFileLogger.enable
	 *
	 *	This enable or disable logging into disk
	 *	@type	bool
	 */
	PY_ATTRIBUTE( enable )

	/*~ attribute PyDebugMessageFileLogger.fileName
	 *
	 *	This is the file name that this file logger writes into
	 *	@type	BW::string
	 */
	PY_ATTRIBUTE( fileName )

	/*~ attribute PyDebugMessageFileLogger.openMode
	 *
	 *	This is the write mode (append or overwrite) that this file logger uses
	 *	@type	BW::string
	 */
	PY_ATTRIBUTE( openMode )

	/*~ attribute PyDebugMessageFileLogger.severities
	 *
	 *	This is the severities used for this file logger
	 *	@type	BW::string
	 */
	PY_ATTRIBUTE( severities )

	/*~ attribute PyDebugMessageFileLogger.sources
	 *
	 *	This is the sources used for this file logger
	 *	@type	BW::string
	 */
	PY_ATTRIBUTE( sources )

	/*~ attribute PyDebugMessageFileLogger.category
	 *
	 *	This is the category used for this file logger
	 *	@type	BW::string
	 */
	PY_ATTRIBUTE( category )

PY_END_ATTRIBUTES()

	/*~	function BigWorld.FileLogger
	 *	@components{ client, tools }
	 *
	 *	Create a file logger with specified configuration.
	 *
	 *	@{
	 *	>>>	x = BigWorld.FileLogger( "outputfile.log", 
	 *			severities = "ERROR, CRITICAL, HACK", 
	 *			categories = "Chunk", 
	 *			sources = "SCRIPT, CPP") 
	 *			openMode = "w") 
	 *	@}
	 *
	 *	@param fileName					String,the file name to write the log into, 
	 *									if "",then executable_file_name.log 
	 *									will be used.
	 *							
	 *	@param severities (optional)	String, combination of: TRACE,DEBUG,INFO,
	 *									NOTICE,WARNING,ERROR,CRITICAL,HACK,ASSET, 
	 *									if not set, then ALL above will be included.
	 *
	 *	@param category (optional)		String, only one category is allowed, 
	 *									ie.: Chunk, if not set, then all categories
	 *									will be included.
	 *
	 *	@param sources (optional)		String, combination of: CPP, SCRIPT, 
	 *									if not set, then ALL above will be included.
	 *
	 *	@param openMode (optional)		String, either "a"(append) or "w"(overwrite), 
	 *									if not set, then "a" will be used.
	 *  @return The new FileLogger.
	 *
	*/
PY_FACTORY_NAMED( PyDebugMessageFileLogger, "FileLogger", BigWorld )

PyDebugMessageFileLogger::PyDebugMessageFileLogger():
	PyObjectPlus( &PyDebugMessageFileLogger::s_type_ )
{
	BW_GUARD;
}


/**
 *	This method read from configSection and set configuration.
 *	
 *  @param configSection	The data section associated to engine_config.xml
 */
void PyDebugMessageFileLogger::configFromDataSection( DataSectionPtr fileNameSection )
{
	uint16 severities = DebugMessageFileLogger::SERVERITY_ALL;
	uint8 sources = DebugMessageFileLogger::SOURCE_ALL;
	BW::string category = "";
	DebugMessageFileLogger::FileOpenMode openMode = DebugMessageFileLogger::APPEND;

	// Read log file
	BW::string logFileName = fileNameSection->asString();

	char buffer[ 4096 ];
	StringBuilder errorMsg( buffer, 4096 );

	// Read severities
	BW::vector<BW::string> stringArray;
	fileNameSection->readStrings( "severity", stringArray );
	if (stringArray.size() > 0)
	{
		errorMsg.clear();
		severities = DebugMessageFileLogger::convertSeverities( stringArray, errorMsg );
		if (severities == 0)
		{
			errorMsg.append( "PyDebugMessageFileLogger::configFromDataSection: "
				"no valid severities are specified, using ALL instead\n" );
			severities = DebugMessageFileLogger::SERVERITY_ALL;
		}
		if (errorMsg.length() > 0)
		{
			ERROR_MSG( "%s\n", errorMsg.string() );
		}
	}

	// Read sources
	stringArray.clear();
	fileNameSection->readStrings( "source", stringArray );
	if (stringArray.size() > 0)
	{
		errorMsg.clear();
		sources = DebugMessageFileLogger::convertSources( stringArray, errorMsg );
		if (sources == 0)
		{
			errorMsg.append( "PyDebugMessageFileLogger::configFromDataSection: "
				"no valid sources are specified, using ALL instead\n" );
			sources = DebugMessageFileLogger::SOURCE_ALL;
		}
		if (errorMsg.length() > 0)
		{
			ERROR_MSG( "%s\n", errorMsg.string() );
		}
	}

	// Read category
	category = fileNameSection->readString( "category" , "" );

	// File open mode
	errorMsg.clear();
	BW::string openModeVal = fileNameSection->readString( "openMode");
	if (!openModeVal.empty())
	{
		openMode = DebugMessageFileLogger::convertOpenMode( 
			openModeVal, errorMsg );
		if (errorMsg.length() > 0)
		{
			ERROR_MSG( "%s\n", errorMsg.string() );
		}
	}
	// Set config
	this->config( logFileName, severities, category, sources, openMode );
	// Enable
	this->enable( fileNameSection->readBool( "enabled", false ) );
}


PyObject * PyDebugMessageFileLogger::_pyNew( PyTypeObject * pType,
											PyObject * args, PyObject * kwargs )
{
	BW_GUARD;
	MF_ASSERT( pType == &PyDebugMessageFileLogger::s_type_ );

	PyDebugMessageFileLoggerPtr pFileLogger =  
		PyDebugMessageFileLoggerPtr( new PyDebugMessageFileLogger, 
						PyDebugMessageFileLoggerPtr::FROM_NEW_REFERENCE );
	if (pFileLogger->py_config( args, kwargs ) == NULL )
	{
		return NULL;
	}
	return pFileLogger.newRef();
}


PyObject * PyDebugMessageFileLogger::py_config( PyObject * args, PyObject * kwargs )
{
	BW_GUARD;

	BW::string fileLogName = "";
	uint16 severities = DebugMessageFileLogger::SERVERITY_ALL;
	uint8 sources = DebugMessageFileLogger::SOURCE_ALL;
	DebugMessageFileLogger::FileOpenMode openMode = DebugMessageFileLogger::APPEND;
	BW::string category = "";

	char* pArgFileLogName = NULL;
	PyObject * pArgSeveritiesPyObj = NULL;
	char* pArgCategory = NULL;
	PyObject* pArgSourcesPyObj = NULL;
	char* pArgOpenMode = NULL;
	const char * keywords[] = { "fileName", "severities", "category",
								"sources", "openMode", NULL };
	if (!PyArg_ParseTupleAndKeywords( args, kwargs, "s|OsOs:BigWorld.FileLogger",
			const_cast< char ** >( keywords ),
			&pArgFileLogName, &pArgSeveritiesPyObj, &pArgCategory, 
			&pArgSourcesPyObj, &pArgOpenMode ))
	{
		return NULL;
	}

	char buffer[ 4096 ];
	StringBuilder errorMsg( buffer, 4096 );

	// Log file name
	MF_ASSERT(pArgFileLogName != NULL);
	fileLogName = pArgFileLogName;

	// Severities
	if (pArgSeveritiesPyObj && pArgSeveritiesPyObj != Py_None)
	{
		BW::vector<BW::string> serverityArray;
		ScriptTuple tupleSeverities;
		ScriptObject argSeveritiesScriptObj( 
					pArgSeveritiesPyObj, ScriptObject::FROM_BORROWED_REFERENCE );
		// Passed a sequences of severities
		if (ScriptTuple::check( argSeveritiesScriptObj ))
		{
			tupleSeverities = ScriptTuple::create( argSeveritiesScriptObj );
		}
		// Passed one severity string
		else if (ScriptString::check( argSeveritiesScriptObj ))
		{
			tupleSeverities = ScriptTuple::create( 1 );
			tupleSeverities.setItem( 0, ScriptString::create( argSeveritiesScriptObj ) );
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "PyDebugMessageFileLogger::py_config: "
				"expected a tuple of strings for severities." );
			return NULL;
		}
		Py_ssize_t sz = tupleSeverities.size();
		for (Py_ssize_t i = 0; i < sz; i++)
		{
			BW::string severity;
			ScriptObject item = tupleSeverities.getItem( i );
			if (Script::setData( item.get(), severity ) == 0)
			{
				serverityArray.push_back( severity );
			}
		}
		if (serverityArray.size() > 0)
		{
			errorMsg.clear();
			severities = 
				DebugMessageFileLogger::convertSeverities( serverityArray, errorMsg );
			// There is error with input severities
			if (errorMsg.length() > 0)
			{
				PyErr_SetString( PyExc_ValueError, errorMsg.string() );
				return NULL;
			}
		}
	}

	// Category
	if (pArgCategory != NULL)
	{
		category = pArgCategory;
	}

	// Sources
	if (pArgSourcesPyObj && pArgSourcesPyObj != Py_None)
	{
		BW::vector<BW::string> sourceArray;
		ScriptTuple tupleSources;
		ScriptObject argSourcesScriptObj( 
					pArgSourcesPyObj, ScriptObject::FROM_BORROWED_REFERENCE );
		// Passed a sequences of sources.
		if (ScriptTuple::check( argSourcesScriptObj ))
		{
			tupleSources = ScriptTuple::create( argSourcesScriptObj );
		}
		// Passed one source string.
		else if (ScriptString::check( argSourcesScriptObj ))
		{
			tupleSources = ScriptTuple::create( 1 );
			tupleSources.setItem( 0, ScriptString::create( argSourcesScriptObj ) );
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "PyDebugMessageFileLogger::py_config: "
				"expected a tuple of strings for sources." );
			return NULL;
		}

		Py_ssize_t sz = tupleSources.size();
		for (Py_ssize_t i = 0; i < sz; i++)
		{
			BW::string source;
			ScriptObject item = tupleSources.getItem( i );
			if (Script::setData( item.get(), source ) == 0)
			{
				sourceArray.push_back( source );
			}
		}
		if (sourceArray.size() > 0)
		{
			errorMsg.clear();
			sources = DebugMessageFileLogger::convertSources( sourceArray, errorMsg );
			// There is error with input sources
			if (errorMsg.length() > 0)
			{
				PyErr_SetString( PyExc_ValueError, errorMsg.string() );
				return NULL;
			}
		}
	}


	// Open mode
	if (pArgOpenMode != NULL)
	{
		errorMsg.clear();
		openMode = DebugMessageFileLogger::convertOpenMode( pArgOpenMode, errorMsg );
		// There is error with input openMode
		if (errorMsg.length() > 0)
		{
			PyErr_SetString( PyExc_ValueError, errorMsg.string() );
			return NULL;
		}
	}

	// Set config
	this->config( fileLogName, severities, category, sources, openMode, false );

	Py_RETURN_NONE;
}


ConfigCreatedFileLoggers::ConfigCreatedFileLoggers():
	size_( 0 )
{
	BW_GUARD;
}


/**
 *	This method tries to add a fileLogger into the list
 *
 *  @param fileLogger	The fileLogger to be added.
 *	@return				True if success, false if the limit MAX_FILE_LOGGERS is
 *							already reached.
 */
bool ConfigCreatedFileLoggers::addFileLogger( PyDebugMessageFileLoggerPtr fileLogger )
{
	BW_GUARD;
	if (size_ < MAX_FILE_LOGGERS)
	{
		fileLoggers_[ size_++ ] = fileLogger;
		return true;
	}
	return false;
}


PyDebugMessageFileLoggerPtr ConfigCreatedFileLoggers::operator []( size_t i )
{
	BW_GUARD;
	if (i < this->len())
	{
		return fileLoggers_[ i ];
	}
	return PyDebugMessageFileLoggerPtr( NULL );
}

ConfigCreatedFileLoggers  configCreatedFileLoggers_;


/*~ function BigWorld.defaultLoggers
 *	@components{ client, tools }
 *
 *	This function return a list of fileLoggers created
 *	by the config xml file.
 *
 *  @return The config created fileLoggers.
*/
static ScriptList defaultLoggers()
{
	BW_GUARD;
	ScriptList pList = ScriptList::create( configCreatedFileLoggers_.len() );

	size_t iValid = 0;
	for (size_t i = 0; i < configCreatedFileLoggers_.len(); i++)
	{
		PyDebugMessageFileLoggerPtr pFileLogger = configCreatedFileLoggers_[i];
		if (pFileLogger != NULL)// This should not happen though
		{
			pList.setItem( iValid++, pFileLogger );
		}
	}
	return pList;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, defaultLoggers, END, BigWorld )

BW_END_NAMESPACE


