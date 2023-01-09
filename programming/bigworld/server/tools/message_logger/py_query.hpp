#ifndef PY_QUERY_HPP
#define PY_QUERY_HPP

#include "Python.h"

#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_pointer.hpp"


BW_BEGIN_NAMESPACE

class PyQuery;
typedef SmartPointer< PyQuery > PyQueryPtr;

BW_END_NAMESPACE


#include "log_string_interpolator.hpp"
#include "py_bwlog.hpp"
#include "py_query_result.hpp"
#include "query_range.hpp"
#include "user_log_reader.hpp"


BW_BEGIN_NAMESPACE

/**
 * Generator-style object for iterating over query results.
 */
class PyQuery : public PyObject
{
public:
	//PyQuery( BWLog *pLog, QueryParams *pParams, UserLog *pUserLog );
	PyQuery( PyBWLogPtr pBWLog, QueryParamsPtr pParams,
		UserLogReaderPtr pUserLog );

	/* Python API */
	PyObject * pyGetAttribute( const char *attr );

	PyObject * py_get( PyObject * args );
	PyObject * py_inReverse( PyObject * args );
	PyObject * py_getProgress( PyObject * args );
	PyObject * py_resume( PyObject * args );
	PyObject * py_tell( PyObject * args );
	PyObject * py_seek( PyObject * args );
	PyObject * py_step( PyObject * args );
	PyObject * py_setTimeout( PyObject * args );

	void incRef() const { Py_INCREF( (PyObject *)this ); }
	void decRef() const { Py_DECREF( (PyObject *)this ); }

	int refCount() const { return ((PyObject *)this)->ob_refcnt; }

	static PyObject * _tp_repr( PyObject * pObj );
	static PyObject * _tp_getattro( PyObject * pObj, PyObject * name );

	static void _tp_dealloc( PyObject * pObj )
	{
		delete static_cast< PyQuery * >( pObj );
	}

	static PyTypeObject s_type_;

	/* Non-Python API */
	PyObject * next();

private:
	// This object should only be deleted by Py_DECREF
	//~PyQuery();

	PyQueryResult * getResultForEntry( const LogEntry &entry, bool filter );

	PyObject * getContextLines();

	PyObject * updateContextLines( PyQueryResult *pResult, 
		int numLinesOfContext );

	bool interpolate( const LogStringInterpolator *handler,
		QueryRangePtr pRange, BW::string &dest );

protected:
	QueryParamsPtr pParams_;
	QueryRangePtr pRange_;

	PyBWLogPtr pBWLog_;

	UserLogReaderPtr pUserLogReader_;

	PyQueryResultPtr pContextResult_;

	QueryRange::iterator contextPoint_;
	QueryRange::iterator contextCurr_;
	QueryRange::iterator mark_;

	bool separatorReturned_;

	PyObjectPtr pCallback_;

	float timeout_;
	int timeoutGranularity_;
};

BW_END_NAMESPACE

#endif // PY_QUERY_HPP
