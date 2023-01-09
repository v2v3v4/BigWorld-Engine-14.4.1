#ifndef PYUSER_LOG_HPP
#define PYUSER_LOG_HPP

#include "pyscript/pyobject_pointer.hpp"

BW_BEGIN_NAMESPACE

class PyUserLog;
typedef SmartPointer< PyUserLog > PyUserLogPtr;

BW_END_NAMESPACE


#include "Python.h"

#include "py_bwlog.hpp"
#include "user_log_reader.hpp"


BW_BEGIN_NAMESPACE


class PyUserLog : public PyObject
{
public:
	/* Python API */
	PyObject* pyGetAttribute( const char *attr );

	PyObject * py_fetch( PyObject * args, PyObject * kwargs );
	PyObject * py_getComponents( PyObject * args );
	PyObject * py_getEntry( PyObject * args );
	PyObject * py_getSegments( PyObject * args );

	PyObject * pyGet_uid();
	PyObject * pyGet_username();

	/* Non-Python API */
	PyUserLog( UserLogReaderPtr pUserLogReader, PyBWLogPtr pBWLog );

	UserLogReaderPtr getUserLog() const;
	uint16 getUID() const;

	void incRef() const { Py_INCREF( (PyObject *)this ); }
	void decRef() const { Py_DECREF( (PyObject *)this ); }
	int refCount() const { return ((PyObject *)this)->ob_refcnt; }

	static void _tp_dealloc( PyObject * pObj );
	static PyObject * _tp_repr( PyObject * pObj );
	static PyObject * _tp_getattro( PyObject * pObj, PyObject * name );

	static PyTypeObject s_type_;

	BW::string getUsername() const;

private:
	// As we're inheriting from PyObjectPlus we should never delete
	// the object, and rely on Py_DECREF to destroy the object
	//~PyUserLog();

	UserLogReaderPtr pUserLogReader_;

	PyBWLogPtr pBWLog_;
};

BW_END_NAMESPACE

#endif // PYUSER_LOG_HPP
