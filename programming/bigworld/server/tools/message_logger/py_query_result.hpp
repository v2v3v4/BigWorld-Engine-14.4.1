#ifndef PY_QUERY_RESULT_HPP
#define PY_QUERY_RESULT_HPP

#include "Python.h"

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class PyQueryResult;
typedef SmartPointer< PyQueryResult > PyQueryResultPtr;

BW_END_NAMESPACE


#include "py_bwlog.hpp"
#include "query_result.hpp"

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 * This is the Python object returned from a Query.
 */
class PyQueryResult : public PyObject
{
public:
	PyQueryResult();
	PyQueryResult( const LogEntry &entry, PyBWLogPtr pBWLog,
		UserLogReaderPtr pUserLog, const LoggingComponent *component,
		const BW::string & message, const BW::string & metadata );

	PyObject * pyGet_time();
	PyObject * pyGet_host();
	PyObject * pyGet_pid();
	PyObject * pyGet_appid();
	PyObject * pyGet_username();
	PyObject * pyGet_component();
	PyObject * pyGet_severity();
	PyObject * pyGet_message();
	PyObject * pyGet_stringOffset();

	PyObject * py_format( PyObject * pArgs );
	PyObject * py_metadata();

	void incRef() const { Py_INCREF( (PyObject *)this ); }
	void decRef() const { Py_DECREF( (PyObject *)this ); }

	int refCount() const { return ((PyObject *)this)->ob_refcnt; }

	static PyObject * _tp_repr( PyObject * pObj );
	static PyObject * _tp_getattro( PyObject * pObj, PyObject * name );

	static void _tp_dealloc( PyObject * pObj )
	{
		delete static_cast< PyQueryResult * >( pObj );
	}

	static PyTypeObject s_type_;

private:

	// Let the reference counting take care of deletion
	~PyQueryResult();

	// Python API
	PyObject * pyGetAttribute( const char *attr );
	int pySetAttribute( const char * attr, PyObject * value );

	// Properties

	QueryResult *pQueryResult_;

	// Accessors for the Python attributes
	BW::string getMessage() const;
	MessageLogger::FormatStringOffsetId getStringOffset() const;
	double getTime() const;
	const char * getHost() const;
	int getPID() const;
	MessageLogger::AppInstanceID getAppInstanceID() const;
	const char * getUsername() const;
	const char * getComponent() const;
	int getSeverity() const;
};

BW_END_NAMESPACE

#endif // PY_QUERY_RESULT_HPP
