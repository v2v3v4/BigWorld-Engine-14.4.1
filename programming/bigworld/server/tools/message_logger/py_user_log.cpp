#include <Python.h>

#include "py_user_log.hpp"

#include "log_string_interpolator.hpp"
#include "py_query_result.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
PyObject * PyUserLog_fetch( PyObject * self, PyObject * args,
		PyObject * kwargs );
PyObject * PyUserLog_getComponents( PyObject * self, PyObject * args );
PyObject * PyUserLog_getEntry( PyObject * self, PyObject * args );
PyObject * PyUserLog_getSegments( PyObject * self, PyObject * args );


/**
 *	Methods for the PyBWLog type
 */
static PyMethodDef PyUserLog_methods[] =
{
	{ "fetch", (PyCFunction)PyUserLog_fetch, METH_KEYWORDS,
			"Performs a query on the log" },
	{ "getComponents", (PyCFunction)PyUserLog_getComponents, METH_VARARGS,
			"Returns the list of component names" },
	{ "getEntry", (PyCFunction)PyUserLog_getEntry, METH_VARARGS,
			"Returns a specific log entry" },
	{ "getSegments", (PyCFunction)PyUserLog_getSegments, METH_VARARGS,
			"Returns a list of entries corresponding to this log's segments" },
	{ NULL, NULL, 0, NULL }
};


/**
 *	Type object for PyUserLog
 */
PyTypeObject PyUserLog::s_type_ =
{
	PyObject_HEAD_INIT( &PyType_Type )
	0,										/* ob_size */
	const_cast< char * >( "PyUserLog" ),	/* tp_name */
	sizeof( PyUserLog ),					/* tp_basicsize */
	0,										/* tp_itemsize */
	PyUserLog::_tp_dealloc,					/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_compare */
	PyUserLog::_tp_repr,					/* tp_repr */
	0,										/* tp_as_number */
	0,										/* tp_as_sequence */
	0,										/* tp_as_mapping */
	0,										/* tp_hash */
	0,										/* tp_call */
	0,										/* tp_str */
	PyUserLog::_tp_getattro,				/* tp_getattro */
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
	PyUserLog_methods,						/* tp_methods */
	0,										/* tp_members */
	0,										/* tp_getset */
	0,										/* tp_base */
	0,										/* tp_dict */
	0,										/* tp_descr_get */
	0,										/* tp_descr_set */
	0,										/* tp_dictoffset */
	0,										/* tp_init */
	0,										/* tp_alloc */
	0,										/* tp_new */
	0,										/* tp_free */
	0,										/* tp_is_gc */
	0,										/* tp_bases */
	0,										/* tp_mro */
	0,										/* tp_cache */
	0,										/* tp_subclasses */
	0,										/* tp_weaklist */
	0,										/* tp_del */
#if ( PY_MAJOR_VERSION > 2 || ( PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION >=6 ) )
	0,										/* tp_version_tag */
#endif
};



//=====================================
// UserComponentPopulator
//=====================================

class UserComponentPopulator : public UserComponentVisitor
{
public:
	UserComponentPopulator();
	~UserComponentPopulator();

	bool onComponent( const LoggingComponent &component );

	PyObject * getList();
private:
	void cleanupOnError();

	// Note: we don't want to delete pList in a destructor as it will be
	// used to return with the result set. The only case it will be destroyed
	// is if an error occurs during population. This is handled in
	// cleanupOnError
	PyObject *pList_;
};


UserComponentPopulator::UserComponentPopulator() :
	pList_( PyList_New( 0 ) )
{ }


UserComponentPopulator::~UserComponentPopulator()
{
	Py_XDECREF( pList_ );
}


PyObject * UserComponentPopulator::getList()
{
	Py_XINCREF( pList_ );
	return pList_;
}


void UserComponentPopulator::cleanupOnError()
{
	Py_DECREF( pList_ );
	pList_ = NULL;
}


bool UserComponentPopulator::onComponent( const LoggingComponent & component )
{
	if (pList_ == NULL)
	{
		return false;
	}

	PyObjectPtr pyEntry( Py_BuildValue( "sii(si)",
			component.getComponentName().c_str(),
			component.getPID(),
			component.getAppInstanceID(),
			component.getFirstEntry().getSuffix(),
			component.getFirstEntry().getIndex() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pyEntry == NULL)
	{
		this->cleanupOnError();
		return false;
	}

	if (PyList_Append( pList_, pyEntry.get() ) == -1)
	{
		this->cleanupOnError();
		return false;
	}

	return true;
}



//=====================================
// UserSegmentPopulator
//=====================================


class UserSegmentPopulator : public UserSegmentVisitor
{
public:
	UserSegmentPopulator();
	~UserSegmentPopulator();

	bool onSegment( const UserSegmentReader *pSegment );

	PyObject *getList();
private:
	void cleanupOnError();

	// Note: we don't want to delete pList in a destructor as it will be
	// used to return with the result set. The only case it will be destroyed
	// is if an error occurs during population. This is handled in
	// cleanupOnError
	PyObject *pList_;
};


UserSegmentPopulator::UserSegmentPopulator() :
	pList_( PyList_New( 0 ) )
{ }


UserSegmentPopulator::~UserSegmentPopulator()
{
	Py_XINCREF( pList_ );
}


PyObject * UserSegmentPopulator::getList()
{
	Py_XINCREF( pList_ );
	return pList_;
}


void UserSegmentPopulator::cleanupOnError()
{
	Py_DECREF( pList_ );
	pList_ = NULL;
}


bool UserSegmentPopulator::onSegment( const UserSegmentReader *pSegment )
{
	if (pList_ == NULL)
	{
		return false;
	}

	PyObjectPtr pyEntry( Py_BuildValue( "sddiii",
			pSegment->getSuffix().c_str(),
			pSegment->getStartLogTime().asDouble(),
			pSegment->getEndLogTime().asDouble(),
			pSegment->getNumEntries(),
			pSegment->getEntriesLength(),
			pSegment->getArgsLength() ),
		PyObjectPtr::STEAL_REFERENCE );

	if (pyEntry == NULL)
	{
		this->cleanupOnError();
		return false;
	}

	if (PyList_Append( pList_, pyEntry.get() ) == -1)
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
 *	This method returns an attribute.
 */
PyObject * PyUserLog::pyGetAttribute( const char * attr )
{
	if (strcmp( attr, "uid" ) == 0)
	{
		return this->pyGet_uid();
	}

	if (strcmp( attr, "username" ) == 0)
	{
		return this->pyGet_username();
	}

	PyObject * pName = PyString_InternFromString( attr );
	PyObject * pResult = PyObject_GenericGetAttr( this, pName );
	Py_DECREF( pName );

	return pResult;
}


/**
 *	This method invokes the user log's fetch method.
 */
PyObject * PyUserLog_fetch( PyObject * self, PyObject * args,
		PyObject * kwargs )
{
	return static_cast< PyUserLog * >( self )->py_fetch( args, kwargs );
}


/**
 *	This method returns the result of a query on the user log.
 */
PyObject * PyUserLog::py_fetch( PyObject * args, PyObject * kwargs )
{
	if (pBWLog_ == NULL)
	{
		PyErr_Format( PyExc_RuntimeError, "UserLog missing reference to a "
			"valid BWLog instance." );
		return NULL;
	}

	QueryParamsPtr pParams = pBWLog_->getQueryParamsFromPyArgs( args, kwargs );
	if (pParams == NULL)
	{
		// Sets PyErr in QueryParams::init
		return NULL;
	}

	// Make sure the QueryParams know where the query is coming from.
	pParams->setUID( this->getUID() );

	return pBWLog_->fetch( this, pParams );
}


/**
 *	This method invokes the user log's getComponents method.
 */
PyObject * PyUserLog_getComponents( PyObject * self, PyObject * args )
{
	return static_cast< PyUserLog * >( self )->py_getComponents( args );
}


/**
 * Returns a list of tuples corresponding to the components in this user's log.
 * Each tuple takes the format (name, pid, appid, firstEntry). The
 * 'firstEntry' member is a 2-tuple of (suffix, index).
 */
PyObject * PyUserLog::py_getComponents( PyObject * args )
{
	UserComponentPopulator components;

	pUserLogReader_->getUserComponents( components );

	return components.getList();
}


/**
 *	This method invokes the user log's getEntry method.
 */
PyObject * PyUserLog_getEntry( PyObject * self, PyObject * args )
{
	return static_cast< PyUserLog * >( self )->py_getEntry( args );
}


// TODO: this method assumes the user has knowledge of the suffix / index
//       structure of the logs. this knowledge should be abstracted away from
//       the user.
/**
 * Returns a PyQueryResult object corresponding to the provided tuple of
 * (suffix, index).
 */
PyObject * PyUserLog::py_getEntry( PyObject * args )
{
	const char *suffix;
	int index;

	if (!PyArg_ParseTuple( args, "(si)", &suffix, &index ))
	{
		return NULL;
	}

	LogEntryAddress entryAddr( suffix, index );
	LogEntry entry;
	UserSegmentReader *pSegment = NULL;

	// We might not be able to get an entry because the segment it lives in may
	// have been rolled.
	if (!pUserLogReader_->getEntryAndSegment( entryAddr, entry, pSegment,
			false ))
	{
		Py_RETURN_NONE;
	}

	// Get the fmt string to go with it
	const LogStringInterpolator *pHandler =
		pBWLog_->getHandlerForLogEntry( entry );

	if (pHandler == NULL)
	{
		PyErr_Format( PyExc_LookupError,
			"PyUserLog::py_getEntry: Unknown string offset: %u",
			entry.stringOffset() );
		return NULL;
	}

	// Get the Component to go with it
	const LoggingComponent *pComponent =
				pUserLogReader_->getComponentByID( entry.userComponentID() );

	if (pComponent == NULL)
	{
		PyErr_Format( PyExc_LookupError,
			"PyUserLog::py_getEntry: Unknown component id: %d",
			entry.userComponentID() );
		return NULL;
	}

	// Interpolate the message given the entry details
	BW::string message;
	if (!pSegment->interpolateMessage( entry, pHandler, message ))
	{
		PyErr_Format( PyExc_LookupError, "PyUserLog::py_getEntry: "
			"Failed to resolve log message." );
		return NULL;
	}

	BW::string metadata;
	if (!pSegment->metadata( entry, metadata ))
	{
		PyErr_Format( PyExc_LookupError, "PyUserLog::py_getEntry: "
			"Failed to retrieve meta data." );
		return NULL;
	}

	return new PyQueryResult( entry, pBWLog_, pUserLogReader_,
								pComponent, message, metadata );
}


/**
 *	This method invokes the user log's getSegments method.
 */
PyObject * PyUserLog_getSegments( PyObject * self, PyObject * args )
{
	return static_cast< PyUserLog * >( self )->py_getSegments( args );
}


/**
 * Returns a list of tuples corresponding to the segments in this UserLog.
 * Each tuple takes the format (suffix, starttime, endtime, nEntries, size
 * of entries file, size of args file).
 */
PyObject * PyUserLog::py_getSegments( PyObject * args )
{
	UserSegmentPopulator segments;

	pUserLogReader_->visitAllSegmentsWith( segments );

	return segments.getList();
}


/**
 *	This method returns the user name of the user log.
 */
PyObject * PyUserLog::pyGet_username()
{
	BW::string username = this->getUsername();

	return PyString_FromStringAndSize(
			const_cast< char * >( username.data() ), username.size() );
}


/**
 *	This method invokes the user log's getUID method.
 */
PyObject * PyUserLog::pyGet_uid()
{
	return PyInt_FromLong( this->getUID() );
}


/**
 *	This method implements the Python dealloc method
 */
void PyUserLog::_tp_dealloc( PyObject * pObj )
{
	delete static_cast< PyUserLog * >( pObj );
}

/**
 *	This method implements the Python repr method.
 */
PyObject * PyUserLog::_tp_repr( PyObject * pObj )
{
	PyUserLog * pThis = static_cast< PyUserLog * >(pObj);

	char	str[512];
	bw_snprintf( str, sizeof(str), "PyUserLog at %p", pThis );

	return PyString_InternFromString( str );
}


/**
 *	This method implements the Python getattribute method.
 */
PyObject * PyUserLog::_tp_getattro( PyObject * pObj, PyObject * name )
{
	return static_cast< PyUserLog * >( pObj )->pyGetAttribute(
			PyString_AS_STRING( name ) );
}



//=====================================
// Non-Python API
//=====================================


/**
 *	Constructor
 */
PyUserLog::PyUserLog( UserLogReaderPtr pUserLogReader, PyBWLogPtr pBWLog ) :
	pUserLogReader_( pUserLogReader ),
	pBWLog_( pBWLog )
{
	PyTypeObject * pType = &PyUserLog::s_type_;

	if (PyType_Ready( pType ) < 0)
	{
		ERROR_MSG( "PyUserLog: Type %s is not ready\n", pType->tp_name );
	}

	PyObject_Init( this, pType );
}


/**
 * This method allows other Python modules to get a handle on the
 * UserLogReader this module is using.
 *
 * It is used primarily by BWLogModule to pass the UserLogReader into
 * QueryModule.
 */
UserLogReaderPtr PyUserLog::getUserLog() const
{
	return pUserLogReader_;
}


/**
 *	Returns the log's UID
 */
uint16 PyUserLog::getUID() const
{
	return pUserLogReader_->getUID();
}


/**
 *	Returns the username for the log.
 */
BW::string PyUserLog::getUsername() const
{
	return pUserLogReader_->getUsername();
}

BW_END_NAMESPACE

// py_user_log.cpp
