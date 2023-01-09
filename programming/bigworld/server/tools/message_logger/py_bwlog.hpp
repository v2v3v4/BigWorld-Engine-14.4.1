#ifndef PY_BWLOG_HPP
#define PY_BWLOG_HPP

#include "Python.h"

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class PyBWLog;
typedef SmartPointer< PyBWLog > PyBWLogPtr;

BW_END_NAMESPACE

#include "py_user_log.hpp"
#include "mldb/log_reader.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class provides the Python wrapper for the LogReaderMLDB class.
 */
class PyBWLog : public PyObject
{
public:

	/* Python API */
	PyObject * pyGetAttribute( const char *attr );

	PyObject * py_fetch( PyObject * args, PyObject * kwargs );
	PyObject * py_getCategoryNames( PyObject * args );
	PyObject * py_getComponentNames( PyObject * args );
	PyObject * py_getFormatStrings( PyObject * args );
	PyObject * py_getHostnames( PyObject * args );
	PyObject * py_getUsers( PyObject * args );
	PyObject * py_getUserLog( PyObject * args );

	PyObject * pyGet_logDirectory();

	static PyObject * pyNew( PyObject * pArgs );
	static PyObject * _pyNew( PyObject *, PyObject * args, PyObject * )
	{
		return PyBWLog::pyNew( args );
	}

	void incRef() const { Py_INCREF( (PyObject *)this ); }
	void decRef() const { Py_DECREF( (PyObject *)this ); }
	int refCount() const { return ((PyObject *)this)->ob_refcnt; }

	static void _tp_dealloc( PyObject * pObj );
	static PyObject * _tp_repr( PyObject * pObj );
	static PyObject * _tp_getattro( PyObject * pObj, PyObject * name );

	static PyTypeObject s_type_;

	/* Non-Python API */
	PyBWLog();

	bool init( const char *logDir );

	QueryParamsPtr getQueryParamsFromPyArgs( PyObject *args, PyObject *kwargs );
	PyObject *fetch( PyUserLogPtr pUserLog, QueryParamsPtr pParams );

	const char * getLogDirectory() const;
	BW::string getHostByAddr( MessageLogger::IPAddress ipAddress ) const;

	LogReaderMLDB *getBWLogReader();

	const LogStringInterpolator *getHandlerForLogEntry( const LogEntry &entry );

	bool refreshFileMaps();

private:
	// Deriving from PyObjectPlus, use Py_DECREF rather than delete
	~PyBWLog();

	PyUserLogPtr getUserLog( uint16 uid );

	LogReaderMLDB *pLogReader_;
};

BW_END_NAMESPACE

#endif // PY_BWLOG_HPP
