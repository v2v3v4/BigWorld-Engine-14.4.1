#include "Python.h"

#include "py_bwlog.hpp"

#include "constants.hpp"
#include "mlutil.hpp"
#include "query_params.hpp"
#include "py_query.hpp"
#include "py_user_log.hpp"

#include "network/logger_message_forwarder.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/log_msg.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
PyObject * PyBWLog_fetch( PyObject * self, PyObject * args,
		PyObject * kwargs );
PyObject * PyBWLog_getCategoryNames( PyObject * self, PyObject * args );
PyObject * PyBWLog_getComponentNames( PyObject * self, PyObject * args );
PyObject * PyBWLog_getFormatStrings( PyObject * self, PyObject * args );
PyObject * PyBWLog_getHostnames( PyObject * self, PyObject * args );
PyObject * PyBWLog_getUsers( PyObject * self, PyObject * args );
PyObject * PyBWLog_getUserLog( PyObject * self, PyObject * args );
PyObject * PyBWLog_refreshLogFiles( PyObject * self, PyObject * args );


/**
 *	Methods for the PyBWLog type
 */
static PyMethodDef PyBWLog_methods[] =
{
	{ "fetch", (PyCFunction)PyBWLog_fetch, METH_KEYWORDS | METH_VARARGS,
			"Performs a query on the user log" },
	{ "getCategoryNames", (PyCFunction)PyBWLog_getCategoryNames, METH_VARARGS,
			"Returns the category names" },
	{ "getComponentNames", (PyCFunction)PyBWLog_getComponentNames, METH_VARARGS,
			"Returns the list of component names" },
	{ "getFormatStrings", (PyCFunction)PyBWLog_getFormatStrings, METH_VARARGS,
			"Returns the list of format strings" },
	{ "getHostnames", (PyCFunction)PyBWLog_getHostnames, METH_VARARGS,
			"Returns the dict of hostnames" },
	{ "getUsers", (PyCFunction)PyBWLog_getUsers, METH_VARARGS,
			"Returns the dict of users" },
	{ "getUserLog", (PyCFunction)PyBWLog_getUserLog, METH_VARARGS,
			"Returns the user log" },
	{ "refreshLogFiles", (PyCFunction)PyBWLog_refreshLogFiles, METH_NOARGS,
			"Refreshes log file mappings" },
	{ NULL, NULL, 0, NULL }
};


/**
 *	Type object for PyBWLog
 */
PyTypeObject PyBWLog::s_type_ =
{
	PyObject_HEAD_INIT( &PyType_Type )
	0,										/* ob_size */
	const_cast< char * >( "PyBWLog" ),		/* tp_name */
	sizeof( PyBWLog ),						/* tp_basicsize */
	0,										/* tp_itemsize */
	PyBWLog::_tp_dealloc,					/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_compare */
	PyBWLog::_tp_repr,						/* tp_repr */
	0,										/* tp_as_number */
	0,										/* tp_as_sequence */
	0,										/* tp_as_mapping */
	0,										/* tp_hash */
	0,										/* tp_call */
	0,										/* tp_str */
	PyBWLog::_tp_getattro,					/* tp_getattro */
	0,										/* tp_setattro */
	0,										/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,						/* tp_flags */
	0,										/* tp_doc */
	0,										/* tp_traverse */
	0,										/* tp_clear */
	0,										/* tp_richcompare */
	0,										/* tp_weaklistoffset */
	0,										/* tp_iter */
	0,										/* tp_iternext */
	PyBWLog_methods,						/* tp_methods */
	0,										/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	0,										/* tp_init */
	0,										/* tp_alloc */
	(newfunc)PyBWLog::_pyNew,				/* tp_new */
	0,										/* tp_free */
	0,										/* tp_is_gc */
	0,										/* tp_bases */
	0,										/* tp_mro */
	0,										/* tp_cache */
	0,										/* tp_subclasses */
	0,										/* tp_weaklist */
	0,										/* tp_del */
#if PY_MAJOR_VERSION > 2 || ( PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION >=6 )
	0,										/* tp_version_tag */
#endif
};



//=====================================
// HostnamePopulator
//=====================================


class HostnamePopulator : public HostnameVisitor
{
public:
	HostnamePopulator();
	~HostnamePopulator();

	bool onHost( MessageLogger::IPAddress ipAddress,
		const BW::string & hostname );

	PyObject *getDictionary();
private:
	void cleanupOnError();

	// Note: we don't want to delete pDict in a destructor as it will be
	// used to return with the result set. The only case it will be destroyed
	// is if an error occurs during population. This is handled in
	// cleanupOnError
	PyObject *pDict_;
};


HostnamePopulator::HostnamePopulator() :
	pDict_( PyDict_New() )
{ }


HostnamePopulator::~HostnamePopulator()
{
	Py_XDECREF( pDict_ );
}


PyObject * HostnamePopulator::getDictionary()
{
	Py_XINCREF( pDict_ );
	return pDict_;
}


void HostnamePopulator::cleanupOnError()
{
	Py_DECREF( pDict_ );
	pDict_ = NULL;
}


bool HostnamePopulator::onHost( MessageLogger::IPAddress ipAddress,
	const BW::string & hostname )
{
	if (pDict_ == NULL)
	{
		return false;
	}

	char *hostAddr = inet_ntoa( (in_addr&)ipAddress );
	PyObjectPtr pyHostname( PyString_InternFromString( hostname.c_str() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pyHostname == NULL)
	{
		this->cleanupOnError();
		return false;
	}

	if (PyDict_SetItemString( pDict_, hostAddr, pyHostname.get() ) == -1)
	{
		this->cleanupOnError();
		return false;
	}

	return true;
}




//=====================================
// LogCategoriesPopulator
//=====================================


class LogCategoriesPopulator : public CategoriesVisitor
{
public:
	LogCategoriesPopulator();
	~LogCategoriesPopulator();

	bool onCategory( const BW::string & categoryName );

	PyObject *getList();
private:
	void cleanupOnError();

	// Note: we don't want to delete pDict in a destructor as it will be
	// used to return with the result set. The only case it will be destroyed
	// is if an error occurs during population. This is handled in
	// cleanupOnError
	PyObject *pList_;
};


LogCategoriesPopulator::LogCategoriesPopulator() :
	pList_( PyList_New( 0 ) )
{ }


LogCategoriesPopulator::~LogCategoriesPopulator()
{
	Py_XDECREF( pList_ );
}


PyObject * LogCategoriesPopulator::getList()
{
	Py_XINCREF( pList_ );
	return pList_;
}


void LogCategoriesPopulator::cleanupOnError()
{
	Py_DECREF( pList_ );
	pList_ = NULL;
}


bool LogCategoriesPopulator::onCategory( const BW::string & categoryName )
{
	if (pList_ == NULL)
	{
		return false;
	}

	PyObjectPtr pyString( PyString_InternFromString( categoryName.c_str() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pyString == NULL)
	{
		this->cleanupOnError();
		return false;
	}

	if (PyList_Append( pList_, pyString.get() ) == -1 )
	{
		this->cleanupOnError();
		return false;
	}

	return true;
}








//=====================================
// LogComponentsPopulator
//=====================================


class LogComponentsPopulator : public LogComponentsVisitor
{
public:
	LogComponentsPopulator();
	~LogComponentsPopulator();

	bool onComponent( const BW::string &componentName );

	PyObject *getList();
private:
	void cleanupOnError();

	// Note: we don't want to delete pDict in a destructor as it will be
	// used to return with the result set. The only case it will be destroyed
	// is if an error occurs during population. This is handled in
	// cleanupOnError
	PyObject *pList_;
};


LogComponentsPopulator::LogComponentsPopulator() :
	pList_( PyList_New( 0 ) )
{ }


LogComponentsPopulator::~LogComponentsPopulator()
{
	Py_XDECREF( pList_ );
}


PyObject * LogComponentsPopulator::getList()
{
	Py_XINCREF( pList_ );
	return pList_;
}


void LogComponentsPopulator::cleanupOnError()
{
	Py_DECREF( pList_ );
	pList_ = NULL;
}


bool LogComponentsPopulator::onComponent( const BW::string &componentName )
{
	if (pList_ == NULL)
	{
		return false;
	}

	PyObjectPtr pyString( PyString_InternFromString( componentName.c_str() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pyString == NULL)
	{
		this->cleanupOnError();
		return false;
	}

	if (PyList_Append( pList_, pyString.get() ) == -1 )
	{
		this->cleanupOnError();
		return false;
	}

	return true;
}




//=====================================
// Python API
//=====================================


/**
 *	This method is the implementation of the object's _tp_new method.
 */
PyObject * PyBWLog::pyNew( PyObject * args )
{
	// Enable syslog output of error messages from a Python script
	LogMsg::shouldWriteToSyslog( true );

	const char *pLogDir = NULL;
	// We don't support writing via Python
	if (!PyArg_ParseTuple( args, "s", &pLogDir ))
	{
		// No need for a PyErr_Format here.. will be set by ParseTuple
		return NULL;
	}

	// Create the log object
	PyBWLogPtr pLog( new PyBWLog(), PyBWLogPtr::STEAL_REFERENCE );

	bool status = true;
	status = pLog->init( pLogDir );

	if (!status)
	{
		PyErr_Format( PyExc_IOError,
			"Log init failed for log dir '%s'.",
			(pLogDir != NULL) ? pLogDir : pLog->getLogDirectory() );
		return NULL;
	}

	// Increment the reference count so it will be owned by the interpreter
	// when we leave this scope
	Py_INCREF( pLog.getObject() );
	return pLog.getObject();
}


/**
 *	This method returns an attribute object.
 */
PyObject * PyBWLog::pyGetAttribute( const char * attr )
{
	if (strcmp( attr, "logDirectory" ) == 0)
	{
		return this->pyGet_logDirectory();
	}

	PyObject * pName = PyString_InternFromString( attr );
	PyObject * pResult = PyObject_GenericGetAttr( this, pName );
	Py_DECREF( pName );

	return pResult;
}


/**
 *	This method calls the log's fetch method.
 */
PyObject * PyBWLog_fetch( PyObject * self, PyObject * args,
		PyObject * kwargs )
{
	return static_cast< PyBWLog * >( self )->py_fetch( args, kwargs );
}


/**
 *	This method returns the result of a query on the user log.
 */
PyObject * PyBWLog::py_fetch( PyObject * args, PyObject * kwargs )
{
	QueryParamsPtr pParams = this->getQueryParamsFromPyArgs( args, kwargs );

	if (pParams == NULL)
	{
		// PyErr set in getQueryParamsFromPyArgs via init()
		return NULL;
	}

	uint16 uid = pParams->getUID();
	PyUserLogPtr pUserLog = this->getUserLog( uid );

	if (pUserLog == NULL)
	{
		PyErr_Format( PyExc_RuntimeError,
			"PyBWLog::py_fetch: No user log for UID %d\n", uid );
		return NULL;
	}

	return this->fetch( pUserLog, pParams );
}


/**
 *	This method calls the log's getCategoryNames method.
 */
PyObject * PyBWLog_getCategoryNames( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getCategoryNames( args );
}


/**
 *	This method returns None, in place of the list of category names.
 */
PyObject * PyBWLog::py_getCategoryNames( PyObject * args )
{
	LogCategoriesPopulator categories;

	pLogReader_->getCategoryNames( categories );

	return categories.getList();
}


/**
 *	This method calls the log's getComponentNames method.
 */
PyObject * PyBWLog_getComponentNames( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getComponentNames( args );
}


/**
 *	This method returns the list of component names.
 */
PyObject * PyBWLog::py_getComponentNames( PyObject * args )
{
	LogComponentsPopulator components;

	pLogReader_->getComponentNames( components );

	return components.getList();
}


/**
 *	This method calls the log's getFormatStrings method.
 */
PyObject * PyBWLog_getFormatStrings( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getFormatStrings( args );
}


/**
 *	This method returns the list of format strings.
 */
PyObject * PyBWLog::py_getFormatStrings( PyObject * args )
{
	FormatStringList formatStrings = pLogReader_->getFormatStrings();

	PyObject *pList = PyList_New( formatStrings.size() );
	int i = 0;

	FormatStringList::iterator it = formatStrings.begin();
	while (it != formatStrings.end())
	{
		PyObject *pyFormatString = PyString_InternFromString( it->c_str() );
		PyList_SET_ITEM( pList, i, pyFormatString );
		++it;
		++i;
	}

	PyList_Sort( pList );

	return pList;
}


/**
 *	This method calls the log's getHostNames method.
 */
PyObject * PyBWLog_getHostnames( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getHostnames( args );
}


/**
 *	This method returns the dict of hostnames.
 */
PyObject * PyBWLog::py_getHostnames( PyObject * args )
{
	HostnamePopulator hosts;

	pLogReader_->getHostnames( hosts );

	return hosts.getDictionary();
}


/**
 *	This method calls the log's getUsers method.
 */
PyObject * PyBWLog_getUsers( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getUsers( args );
}


/**
 *	This method returns the dict of users.
 */
PyObject * PyBWLog::py_getUsers( PyObject * args )
{
	PyObject *pDict = PyDict_New();

	const UsernamesMap &usernames = pLogReader_->getUsernames();

	UsernamesMap::const_iterator it = usernames.begin();
	while (it != usernames.end())
	{
		PyObject *pyUid = PyInt_FromLong( it->first );
		const char *username = it->second.c_str();

		PyDict_SetItemString( pDict, username, pyUid );
		++it;
	}

	return pDict;
}


/**
 *	This method calls the log's getUserLog method.
 */
PyObject * PyBWLog_getUserLog( PyObject * self, PyObject * args )
{
	return static_cast< PyBWLog * >( self )->py_getUserLog( args );
}


/**
 *	This method returns the user log
 */
PyObject * PyBWLog::py_getUserLog( PyObject * args )
{
	int uid;
	if (!PyArg_ParseTuple( args, "i", &uid ))
	{
		// NB: not setting PyErr_Format here, PyArg_ will set something
		//     appropriate for us.
		return NULL;
	}

	PyUserLogPtr pyUserLog = this->getUserLog( uid );
	if (pyUserLog == NULL)
	{
		PyErr_Format( PyExc_KeyError,
			"No entries for UID %d in this log", uid );
		return NULL;
	}

	PyObject *ret = pyUserLog.getObject();
	Py_INCREF( ret );
	return ret;
}


/**
 *	This method implements the Python dealloc method
 */
void PyBWLog::_tp_dealloc( PyObject * pObj )
{
	delete static_cast< PyBWLog * >( pObj );
}

/**
 *	This method implements the Python repr method
 */
PyObject * PyBWLog::_tp_repr( PyObject * pObj )
{
	PyBWLog * pThis = static_cast< PyBWLog * >(pObj);
	char	str[512];
	bw_snprintf( str, sizeof(str), "PyBWLog at %p", pThis );

	return PyString_InternFromString( str );
}


/**
 *	This method implements the Python getattribute method
 */
PyObject * PyBWLog::_tp_getattro( PyObject * pObj, PyObject * name )
{
	return static_cast< PyBWLog * >( pObj )->pyGetAttribute(
			(((PyStringObject *)(name))->ob_sval) );
}


PyObject * PyBWLog_refreshLogFiles( PyObject * self, PyObject * args )
{
	return Py_BuildValue( "i", 
		static_cast< PyBWLog * >( self )->refreshFileMaps() );
}


//=====================================
// Non-Python API
//=====================================

/**
 *	Constructor
 */
PyBWLog::PyBWLog() :
	pLogReader_( NULL )
{
	PyTypeObject * pType = &PyBWLog::s_type_;

	if (PyType_Ready( pType ) < 0)
	{
		ERROR_MSG( "PyBWLog: Type %s is not ready\n", pType->tp_name );
	}

	PyObject_Init( this, pType );
}


/**
 *	Destructor
 */
PyBWLog::~PyBWLog()
{
	if (pLogReader_ != NULL)
	{
		delete pLogReader_;
	}
}


/**
 *	This method creates the log reader
 */
bool PyBWLog::init( const char *logDir )
{
	if (pLogReader_ != NULL)
	{
		ERROR_MSG( "PyBWLog::init: Log reader already exists, unable to "
				"re-initialise.\n" );
		return false;
	}

	pLogReader_ = new LogReaderMLDB();
	return pLogReader_->init( logDir );
}


/**
 *	This method converts an arg list into a set of query parameters
 */
QueryParamsPtr PyBWLog::getQueryParamsFromPyArgs( PyObject * args,
	PyObject * kwargs )
{
	QueryParamsPtr pParams( new QueryParams(), QueryParamsPtr::NEW_REFERENCE );

	if (!pParams->init( args, kwargs, pLogReader_ ) || !pParams->isGood())
	{
		return NULL;
	}

	return pParams;
}


/**
 *	This method queries the given user log with the given parameters
 */
PyObject * PyBWLog::fetch( PyUserLogPtr pUserLog, QueryParamsPtr pParams )
{
	if (pUserLog->getUID() != pParams->getUID())
	{
		PyErr_Format( PyExc_RuntimeError, "UserLog UID %d does not match "
			"QueryParams UID %d\n", pUserLog->getUID(), pParams->getUID() );
		return NULL;
	}

	return new PyQuery( this, pParams, pUserLog->getUserLog() );
}


/**
 *	This method converts the log reader's directory to a Python string.
 */
PyObject * PyBWLog::pyGet_logDirectory()
{
	return PyString_InternFromString( this->getLogDirectory() );
}


/**
 *	This method returns the log reader's directory
 */
const char *PyBWLog::getLogDirectory() const
{
	const char *pLogDir = NULL;

	if (pLogReader_ != NULL)
	{
		pLogDir = pLogReader_->getLogDirectory();
	}

	return pLogDir;
}


/**
 *	This method converts an IP address into its associated host
 */
BW::string PyBWLog::getHostByAddr( MessageLogger::IPAddress ipAddress ) const
{
	return pLogReader_->getHostByAddr( ipAddress );
}


/**
 * Allows access to our contained LogReaderMLDB object.
 *
 * It should only be used to temporarily gain access to the reader to resolve
 * log state and then discard. DO NOT STORE THIS POINTER!
 *
 * This is a borrowed reference.
 */
LogReaderMLDB * PyBWLog::getBWLogReader()
{
	return pLogReader_;
}


/**
 * This method provides a simple pass through to the wrapped LogReaderMLDB.
 *
 * @returns true on successful file map refresh, false on error.
 */
bool PyBWLog::refreshFileMaps()
{
	return pLogReader_->refreshFileMaps();
}


/**
 *	This method returns the log entry handler
 */
const LogStringInterpolator *PyBWLog::getHandlerForLogEntry(
	const LogEntry &entry )
{
	return pLogReader_->getHandlerForLogEntry( entry );
}


/**
*	This method creates a user log
 */
PyUserLogPtr PyBWLog::getUserLog( uint16 uid )
{
	UserLogReaderPtr pUserLogReader = pLogReader_->getUserLog( uid );
	if (pUserLogReader == NULL)
	{
		return NULL;
	}

	PyUserLogPtr pUserLog( new PyUserLog( pUserLogReader, this ),
										PyUserLogPtr::STEAL_REFERENCE );

	return pUserLog;
}

BW_END_NAMESPACE

// pybwlog.cpp
