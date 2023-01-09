#include "Python.h"
#include "frameobject.h"
#include "errno.h"
#include "string.h"

#include "cstdmf/debug_message_source.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/debug.hpp"


#if ENABLE_PROFILER

BW_BEGIN_NAMESPACE

namespace
{
// Forward declarations
static PyObject * _bw_profile___onThreadStart( PyObject *self, PyObject *args );
static PyObject * _bw_profile___onThreadEnd( PyObject *self );
static PyObject * _bw_profile_enable( PyObject *self, PyObject *args );
static PyObject * _bw_profile_isEnabled( PyObject *self, PyObject *args );
static PyObject * _bw_profile_tick( PyObject *self );
static PyObject * _bw_profile_startJsonDump( PyObject *self );
static PyObject * _bw_profile_setJsonDumpCount( PyObject *self, PyObject *args );
static PyObject * _bw_profile_setJsonDumpDir( PyObject *self, PyObject *args );
static PyObject * _bw_profile_getJsonDumpState( PyObject *self );
static PyObject * _bw_profile_getJsonDumpStateValue( PyObject *self, PyObject *args );
static PyObject * _bw_profile_getJsonOutputFilePath( PyObject *self, PyObject *args );


static PyMethodDef _bw_profile_methods[] =
{
	{ "__onThreadStart", (PyCFunction)_bw_profile___onThreadStart, METH_VARARGS,
			"Thread start hook; begins profiling a thread" },
	{ "__onThreadEnd", (PyCFunction)_bw_profile___onThreadEnd, METH_NOARGS,
			"Thread end hook; finishes profiling a thread" },
	{ "enable", (PyCFunction)_bw_profile_enable, METH_VARARGS,
			"Enable or disable performance profiling" },
	{ "isEnabled", (PyCFunction)_bw_profile_isEnabled, METH_VARARGS,
			"Returns whether performance profiling currently running" },
	{ "tick", (PyCFunction)_bw_profile_tick, METH_NOARGS,
			"Profiler Tick event" },
	{ "startJsonDump", (PyCFunction)_bw_profile_startJsonDump, METH_NOARGS,
			"Create a json dump file" },
	{ "setJsonDumpCount", (PyCFunction)_bw_profile_setJsonDumpCount,
			METH_VARARGS, "Set the Json Dump Count " },
	{ "setJsonDumpDir", (PyCFunction)_bw_profile_setJsonDumpDir,
			METH_VARARGS, "Set the Json Dump Dir " },
	{ "getJsonDumpState", (PyCFunction)_bw_profile_getJsonDumpState,
			METH_NOARGS, "Get the Json Dump State" },
	{ "getJsonDumpStateValue", (PyCFunction)_bw_profile_getJsonDumpStateValue,
			METH_VARARGS, "Get the equivalent dump state enum value of the "
				"argument string" },
	{ "getJsonOutputFilePath", (PyCFunction)_bw_profile_getJsonOutputFilePath,
			METH_VARARGS, "Get the latest json output file path " },
	{ NULL, NULL, 0, NULL }
};


int profileFunc( PyObject * /*pObject*/, PyFrameObject * pFrame, 
	int action, PyObject * pArg )
{
	switch (action)
	{
	case PyTrace_CALL:
		g_profiler.addEntry( PyString_AsString( pFrame->f_code->co_name ),
			Profiler::EVENT_START, 0, Profiler::CATEGORY_PYTHON );
		break;
	case PyTrace_RETURN:
		g_profiler.addEntry( PyString_AsString( pFrame->f_code->co_name ),
			Profiler::EVENT_END, 0, Profiler::CATEGORY_PYTHON );
		break;
	case PyTrace_C_CALL:
		if (pArg && PyCFunction_Check( pArg ))
		{
			const char * functionName =
				((PyCFunctionObject*)pArg)->m_ml->ml_name;
			g_profiler.addEntry( functionName, Profiler::EVENT_START, 0,
				Profiler::CATEGORY_PYTHON );
		}
		break;
	case PyTrace_C_RETURN:
	case PyTrace_C_EXCEPTION:
		if (pArg && PyCFunction_Check( pArg ))
		{
			const char * functionName =
				((PyCFunctionObject*)pArg)->m_ml->ml_name;
			g_profiler.addEntry( functionName, Profiler::EVENT_END, 0,
				Profiler::CATEGORY_PYTHON );
		}
		break;
	}

	return 0;
}


static PyObject * _bw_profile___onThreadStart( PyObject *self, PyObject *args )
{
	char *threadName;

	if (!PyArg_ParseTuple( args, "s", &threadName ))
	{
		return NULL;
	}

	g_profiler.addThisThread( threadName );
	PyEval_SetProfile( &profileFunc, NULL );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile___onThreadEnd( PyObject *self )
{
	g_profiler.removeThread( OurThreadID() );
	PyEval_SetProfile( NULL, NULL );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_enable( PyObject *self, PyObject *args )
{
	int i;

	if (!PyArg_ParseTuple( args, "i", &i ))
	{
		return NULL;
	}

	g_profiler.enable( (i == 0) ? false : true );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_isEnabled( PyObject *self, PyObject *args )
{
	int status = g_profiler.watcherEnabled() ? 1 : 0;

	return Py_BuildValue( "i", status );
}


static PyObject * _bw_profile_tick( PyObject *self )
{
	g_profiler.tick();

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_startJsonDump( PyObject *self )
{
	g_profiler.watcherSetDumpProfile( true );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_setJsonDumpCount( PyObject *self, PyObject *args )
{
	int count;

	if (!PyArg_ParseTuple( args, "i", &count ))
	{
		return NULL;
	}

	g_profiler.setJsonDumpCount( count );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_setJsonDumpDir( PyObject *self, PyObject *args )
{
	const char *dir = NULL;

	if (!PyArg_ParseTuple( args, "s", &dir ))
	{
		return NULL;
	}

	if (access( dir, W_OK ) < 0)
	{
		BW::string errMessage = (char *)( "Unable to write to JSON dump directory: " );
		errMessage += dir;
		errMessage += ": ";
		errMessage += strerror( errno );
		errMessage += ".";

		ERROR_MSG( "_bw_profile_getJsonDumpStateValue: %s\n",
			errMessage.c_str() );

		// Raise an exception in python
		BW::string exceptionMessage = (char *)( "setJsonDumpDir: " );
		exceptionMessage += errMessage;

		PyErr_SetString( PyExc_IOError, exceptionMessage.c_str() );

		return NULL;
	}

	g_profiler.setJsonDumpDir( BW::string( dir ) );

	Py_RETURN_NONE;
}


static PyObject * _bw_profile_getJsonDumpState( PyObject *self )
{
	int dumpState =
		g_profiler.getJsonDumpState();

	return Py_BuildValue( "i", dumpState );
}


/*
 * This function avoids the duplicate hardcoding of values representing the
 * possible dump states. It ensures the following scenarios are handled
 * correctly:
 *
 * - If the enum order or values are changed in profiler.hpp then the python
 *   module will be able to adapt to the values since they are not hardcoded.
 * - Any unhandled changes to profiler.hpp will result in a compile error here
 * - Any unhandled changes to this function will result in an exception in the
 *   bw_profile python module immediately upon initialisation.
 *
 * @returns an integer representation of the requested dump state
 */
static PyObject * _bw_profile_getJsonDumpStateValue( PyObject *self, PyObject *args )
{
	const char *strState = NULL;
	int dumpStateValue;

	if (!PyArg_ParseTuple( args, "s", &strState ))
	{
		return NULL;
	}

	if (strcmp( strState, "JSON_DUMPING_INACTIVE" ) == 0)
	{
		dumpStateValue = Profiler::JSON_DUMPING_INACTIVE;
	}
	else if (strcmp( strState, "JSON_DUMPING_ACTIVE" ) == 0)
	{
		dumpStateValue = Profiler::JSON_DUMPING_ACTIVE;
	}
	else if (strcmp( strState, "JSON_DUMPING_COMPLETED" ) == 0)
	{
		dumpStateValue = Profiler::JSON_DUMPING_COMPLETED;
	}
	else if (strcmp( strState, "JSON_DUMPING_FAILED" ) == 0)
	{
		dumpStateValue = Profiler::JSON_DUMPING_FAILED;
	}
	else
	{
		ERROR_MSG( "_bw_profile_getJsonDumpStateValue: "
			"Unable to get unknown json dump state value %s", strState );

		// Raise an exception in python
		BW::string exceptionMessage = (char *)("getJsonDumpStateValue: "
			"Invalid dump state value ");
		exceptionMessage += strState;
		PyErr_SetString( PyExc_RuntimeError, exceptionMessage.c_str() );

		return NULL;
	}

	return Py_BuildValue( "i", (int) dumpStateValue );
}


static PyObject * _bw_profile_getJsonOutputFilePath( PyObject *self, PyObject *args )
{
	BW::string jsonOutputFilePath =
		g_profiler.getJsonOutputFilePath();

	return Py_BuildValue( "s", jsonOutputFilePath.c_str() );
}


}; // anonymous namespace


PyMODINIT_FUNC
init_bw_profile()
{
	// While we are not in main, our init method should do for marking as main
	BW_SYSTEMSTAGE_MAIN();

	// TODO: make this configurable
	const int MAX_PROFILER_MEMORY_BYTES = 128 * 1024 * 1024;
	g_profiler.init( MAX_PROFILER_MEMORY_BYTES );

	const char *MODULE_NAME = "_bw_profile";
	PyObject *pModuleObj = Py_InitModule3( (char *)MODULE_NAME, _bw_profile_methods,
									"BigWorld bw_profile module");

	if (pModuleObj == NULL)
	{
		return;
	}

	PyEval_SetProfile( &profileFunc, NULL );
}

BW_END_NAMESPACE

#endif // ENABLE_PROFILER

// bw_profile.cpp
