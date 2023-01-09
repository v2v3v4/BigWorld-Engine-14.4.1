#include "script/first_include.hpp"

#include "server_app.hpp"
#include "updatable.hpp"
#include "updatables.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

namespace
{
// Initialised with ScriptFrequentTasks::init. This is the object that the
// Updatable instances are added to.
Updatables * g_pUpdatables = NULL;
}


namespace ScriptFrequentTasks
{

void init( Updatables * pUpdatables )
{
	g_pUpdatables = pUpdatables;
}

} // namespace ScriptFrequentTasks


/**
 *	This class is used to register frequent script tasks with the application.
 */
class ScriptFrequentTask : public Updatable
{
public:
	ScriptFrequentTask( ScriptObject pCallback ) :
		pCallback_( pCallback )
	{
	}

	virtual void update()
	{
		PyObject * pResult = Script::ask( pCallback_.newRef(),
												PyTuple_New( 0 ) );

		if (pResult)
		{
			if (PyObject_IsTrue( pResult ))
			{
				g_pUpdatables->remove( this );
				delete this;
			}

			Py_DECREF( pResult );
		}
		else
		{
			ERROR_MSG( "ScriptFrequentTask::update: Script call failed\n" );
			Script::printError();
		}
	}

private:
	ScriptObject pCallback_;
};

namespace
{

// -----------------------------------------------------------------------------
// Section: Fini
// -----------------------------------------------------------------------------

#if 0
// TODO: It would be nice to clean up these objects on shutdown.

class FrequentTaskFini : public Script::FiniTimeJob
{
public:
	virtual void fini()
	{
		// TODO: Destroy all tasks
	}
};

FrequentTaskFini g_frequentTaskFini;

#endif


/**

 *	This method is exposed to scripting. It is used to register a frequent task.
 */
bool addFrequentTask( PyObjectPtr pCallback, int level )
{
	if (!PyCallable_Check( pCallback.get() ))
	{
		PyErr_SetString( PyExc_TypeError, "Argument is not callable" );

		return false;
	}

	if (!g_pUpdatables)
	{
		PyErr_SetString( PyExc_SystemError,
				"ScriptFrequentTask not initialised correctly" );
		return false;
	}

	ScriptFrequentTask * pTask = new ScriptFrequentTask( 
		ScriptObject( pCallback ) );

	if (!g_pUpdatables->add( pTask, level ))
	{
		ERROR_MSG( "addFrequentTask: Failed to registerForUpdate\n" );

		PyErr_SetString( PyExc_ValueError, "Failed to register for update" );
		delete pTask;


		return false;
	}

	return true;
}
/*~ function BigWorld.addFrequentTask
 *	@components{ base, cell, db }
 *
 *	The function addFrequentTask registers a callback function to be
 *	called periodically (at game tick frequency). If the callback ever
 *	returns True, it is deregistered and will not be called again.
 *
 *	@param callback Specifies the callable to be used. This is a function that
 *	takes no parameters.
 *	@param level Optional argument for the level to run the task at. This higher
 *	the level, the later the task is executed. Currently the only valid levels
 *	are 0 and 1.
 */
PY_AUTO_MODULE_FUNCTION_WITH_DOC( RETOK, addFrequentTask,
		ARG( PyObjectPtr, OPTARG( int, 0, END ) ), BigWorld,
"The function addFrequentTask registers a callback function to be \n"
"called periodically (at game tick frequency). If the callback ever \n"
"returns True, it is deregistered and will not be called again." )

} // anonymous namespace

BW_END_NAMESPACE

// script_frequent_tasks.cpp
