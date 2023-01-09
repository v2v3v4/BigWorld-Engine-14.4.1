#include <Python.h>

#include "py_query_result.hpp"

#include "log_entry.hpp"
#include "logging_component.hpp"
#include "query_result.hpp"
#include "user_log.hpp"


BW_BEGIN_NAMESPACE

PyObject * PyQueryResult_format( PyObject * self, PyObject * args );
PyObject * PyQueryResult_metadata( PyObject * self, PyObject * args );
PyObject * PyQueryResult_getTime( PyObject * self, PyObject * args );


/**
 *	Methods for the PyQueryResult type
 */
static PyMethodDef PyQueryResult_methods[] =
{
	{ "format", (PyCFunction)PyQueryResult_format, METH_VARARGS,
			"Docs for format" },
	{ "metadata", (PyCFunction)PyQueryResult_metadata, METH_VARARGS,
			"Return the meta-data associated with the log message (if any)" },
	{ "getTime", (PyCFunction)PyQueryResult_getTime, METH_VARARGS,
			"Return the time stamp of the log entry" },
	{ NULL, NULL, 0, NULL }
};


/**
 *	Type object for PyQueryResult
 */
PyTypeObject PyQueryResult::s_type_ =
{
	PyObject_HEAD_INIT( &PyType_Type )
	0,										/* ob_size */
	const_cast< char * >( "PyQueryResult" ),	/* tp_name */
	sizeof( PyQueryResult ),				/* tp_basicsize */
	0,										/* tp_itemsize */
	PyQueryResult::_tp_dealloc,				/* tp_dealloc */
	0,										/* tp_print */
	0,										/* tp_getattr */
	0,										/* tp_setattr */
	0,										/* tp_compare */
	PyQueryResult::_tp_repr,				/* tp_repr */
	0,										/* tp_as_number */
	0,										/* tp_as_sequence */
	0,										/* tp_as_mapping */
	0,										/* tp_hash */
	0,										/* tp_call */
	0,										/* tp_str */
	PyQueryResult::_tp_getattro,			/* tp_getattro */
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
	PyQueryResult_methods,					/* tp_methods */
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
// Python API
//=====================================



/**
 *	This method returns an attribute object.
 */
PyObject * PyQueryResult::pyGetAttribute( const char * attr )
{
	if (strcmp( attr, "time" ) == 0)
	{
		return this->pyGet_time();
	}

	if (strcmp( attr, "host" ) == 0)
	{
		return this->pyGet_host();
	}

	if (strcmp( attr, "pid" ) == 0)
	{
		return this->pyGet_pid();
	}

	if (strcmp( attr, "appid" ) == 0)
	{
		return this->pyGet_appid();
	}

	if (strcmp( attr, "username" ) == 0)
	{
		return this->pyGet_username();
	}

	if (strcmp( attr, "component" ) == 0)
	{
		return this->pyGet_component();
	}

	if (strcmp( attr, "severity" ) == 0)
	{
		return this->pyGet_severity();
	}

	if (strcmp( attr, "message" ) == 0)
	{
		return this->pyGet_message();
	}

	if (strcmp( attr, "stringOffset" ) == 0)
	{
		return this->pyGet_stringOffset();
	}

	PyObject * pName = PyString_InternFromString( attr );
	PyObject * pResult = PyObject_GenericGetAttr( this, pName );
	Py_DECREF( pName );

	return pResult;
}


/**
 *	This method sets the value of an attribute object.
 */
int PyQueryResult::pySetAttribute( const char * attr, PyObject * value )
{
	PyObject * pName = PyString_InternFromString( attr );
	int result = PyObject_GenericSetAttr( this, pName, value );
	Py_DECREF( pName );

	return result;
}


/**
 *	This method invokes the query result's format method.
 */
PyObject * PyQueryResult_format( PyObject * self, PyObject * args )
{
	return static_cast< PyQueryResult * >( self )->py_format( args );
}


/**
 *	This method formats the log result according to the supplied display flags.
 */
PyObject * PyQueryResult::py_format( PyObject * args )
{
	if (pQueryResult_ == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Query result has not been properly "
			"initialised." );
		return NULL;
	}

	unsigned flags = SHOW_ALL;
	if (!PyArg_ParseTuple( args, "|I", &flags ))
	{
		return NULL;
	}

	int len;

	const char *line = pQueryResult_->format( flags, &len );
 	return PyString_FromStringAndSize( line, len );
}


/**
 *	This method invokes the query result's format method.
 */
PyObject * PyQueryResult_metadata( PyObject * self, PyObject * args )
{
	return static_cast< PyQueryResult * >( self )->py_metadata();
}


/**
 *
 */
PyObject * PyQueryResult::py_metadata()
{
	const BW::string & metadata = pQueryResult_->metadata();

	return PyString_FromStringAndSize( metadata.c_str(), metadata.length() );
}


/**
 *	This method invokes the query result's Get_Time method.
 */
PyObject * PyQueryResult_getTime( PyObject * self, PyObject * args )
{
	return static_cast< PyQueryResult * >( self )->pyGet_time();
}


/**
 *	This method returns the time attribute.
 */
PyObject * PyQueryResult::pyGet_time()
{
	return PyFloat_FromDouble( this->getTime() );
}


/**
 *	This method implements the Python repr method.
 */
PyObject * PyQueryResult::_tp_repr( PyObject * pObj )
{
	PyQueryResult * pThis = static_cast< PyQueryResult * >( pObj );
	char str[ 512 ];
	bw_snprintf( str, sizeof( str ), "PyQueryResult at %p", pThis );

	return PyString_InternFromString( str );
}


/**
 *	This method implements the Python getattribute method.
 */
PyObject * PyQueryResult::_tp_getattro( PyObject * pObj, PyObject * name )
{
	return static_cast< PyQueryResult * >( pObj )->pyGetAttribute(
			PyString_AS_STRING( name ) );
}



//=====================================
// Non-Python API
//=====================================


/**
 *	Constructors
 */
PyQueryResult::PyQueryResult() :
	pQueryResult_( new QueryResult() )
{
	PyTypeObject * pType = &PyQueryResult::s_type_;

	if (PyType_Ready( pType ) < 0)
	{
		ERROR_MSG( "PyQueryResult: Type %s is not ready\n", pType->tp_name );
	}

	PyObject_Init( this, pType );
}


PyQueryResult::PyQueryResult( const LogEntry &entry, PyBWLogPtr pBWLog,
	UserLogReaderPtr pUserLog, const LoggingComponent *pComponent,
	const BW::string & message, const BW::string & metadata ) :

	pQueryResult_( new QueryResult( entry, pBWLog->getBWLogReader(),
					pUserLog.getObject(), pComponent, message, metadata ) )
{
	PyTypeObject * pType = &PyQueryResult::s_type_;

	if (PyType_Ready( pType ) < 0)
	{
		ERROR_MSG( "PyQueryResult: Type %s is not ready\n", pType->tp_name );
	}

	PyObject_Init( this, pType );
}


/**
 *	Destructor
 */
PyQueryResult::~PyQueryResult()
{
	if (pQueryResult_)
	{
		delete pQueryResult_;
	}
}


/**
 *	This method returns the message attribute as a Python object.
 */
PyObject * PyQueryResult::pyGet_message()
{
	const BW::string message = this->getMessage();

	return PyString_FromStringAndSize(
			const_cast< char * >( message.data() ), message.size() );
}


/**
 *	This method returns the query result's message.
 */
BW::string PyQueryResult::getMessage() const
{
	return pQueryResult_->getMessage();
}


/**
 *	This method returns the stringOffset attribute.
 */
PyObject * PyQueryResult::pyGet_stringOffset()
{
	return PyInt_FromLong( this->getStringOffset() );
}


/**
 *	This method returns the query result's string offset.
 */
MessageLogger::FormatStringOffsetId PyQueryResult::getStringOffset() const
{
	return pQueryResult_->getStringOffset();
}


/**
 *	This method returns the query result's time.
 */
double PyQueryResult::getTime() const
{
	return pQueryResult_->getTime();
}


/**
 *	This method returns the host attribute.
 */
PyObject * PyQueryResult::pyGet_host()
{
	return PyString_InternFromString( const_cast< char * >( this->getHost() ) );
}


/**
 *	This method returns the host of the query result.
 */
const char * PyQueryResult::getHost() const
{
	return pQueryResult_->getHost();
}


/**
 *	This method returns the pid attribute.
 */
PyObject * PyQueryResult::pyGet_pid()
{
	return PyInt_FromLong( this->getPID() );
}


/**
 *	This method returns the PID of the query result.
 */
int PyQueryResult::getPID() const
{
	return pQueryResult_->getPID();
}


/**
 *	This method returns the appID attribute.
 */
PyObject * PyQueryResult::pyGet_appid()
{
	return PyInt_FromLong( this->getAppInstanceID() );
}


/**
 *	This method returns the app instance ID of the query result.
 */
int PyQueryResult::getAppInstanceID() const
{
	return pQueryResult_->getAppInstanceID();
}


/**
 *	This method returns the username attribute.
 */
PyObject * PyQueryResult::pyGet_username()
{
	return PyString_InternFromString(
			const_cast< char * >( this->getUsername() ) );
}


/**
 *	This method returns the username of the query result.
 */
const char * PyQueryResult::getUsername() const
{
	return pQueryResult_->getUsername();
}


/**
 *	This method returns the component attribute.
 */
PyObject * PyQueryResult::pyGet_component()
{
	return PyString_InternFromString( const_cast< char * >(
				this->getComponent() ) );
}


/**
 *	This method returns the component of the query result.
 */
const char * PyQueryResult::getComponent() const
{
	return pQueryResult_->getComponent();
}


/**
 *	This method returns the severity attribute.
 */
PyObject * PyQueryResult::pyGet_severity()
{
	return PyInt_FromLong( this->getSeverity() );
}


/**
 *	This method returns the severity of the query result.
 */
int PyQueryResult::getSeverity() const
{
	return pQueryResult_->getSeverity();
}

BW_END_NAMESPACE

// py_query_result.cpp
