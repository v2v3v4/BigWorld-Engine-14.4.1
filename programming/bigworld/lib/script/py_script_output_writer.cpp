#include "pch.hpp"

#include "py_script_output_writer.hpp"

#include "script_object.hpp"
#include "script_output_hook.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/log_msg.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/personality.hpp"


DECLARE_DEBUG_COMPONENT2( "Script", 0 )


namespace BW
{

typedef SmartPointer< ScriptOutputWriter > ScriptOutputWriterPtr;

// -----------------------------------------------------------------------------
// Section: PyOutputStream
// -----------------------------------------------------------------------------
/*~ class NoModule.PyOutputStream
 *	@components{ all }
 *  This class is used by BigWorld to implement a Python object that redirects
 *  Python's sys.stdout or sys.stderr to the BigWorld logging system.
 */
/**
 *	This class implements an object suitable to use as sys.stdout or sys.stderr
 *	in that it implements the write method and softspace attribute
 */
class PyOutputStream : public PyObjectPlus
{
	Py_Header( PyOutputStream, PyObjectPlus );

public:
	PyOutputStream( ScriptOutputWriterPtr pOwner, bool isStderr,
		PyTypeObject * pType = &PyOutputStream::s_type_ );

	ScriptOutputWriterPtr pOwner() const { return pOwner_; }

	PY_METHOD_DECLARE( py_write )

	PY_RW_ATTRIBUTE_DECLARE( softspace_, softspace )

private:
	ScriptOutputWriterPtr	pOwner_;
	bool					isStderr_;
	bool					softspace_;
};

typedef ScriptObjectPtr< PyOutputStream > PyOutputStreamPtr;


PY_TYPEOBJECT( PyOutputStream )

/*~ function PyOutputStream.write
 *	@components{ all }
 *
 *  Write a string to this stream. The Python IO system calls this.
 *  @param string The string to write.
 *  @return None
 */
PY_BEGIN_METHODS( PyOutputStream )
	PY_METHOD( write )
PY_END_METHODS()

/*~ attribute PyOutputStream.softspace
 *	@components{ all }
 *
 *  Attribute used by Python's 'print' statement
 *  to track its current soft-space state.
 *  @type Read-Write Boolean
 */
PY_BEGIN_ATTRIBUTES( PyOutputStream )
	PY_ATTRIBUTE( softspace )
PY_END_ATTRIBUTES()


/**
 *	Constructor
 */
PyOutputStream::PyOutputStream( ScriptOutputWriterPtr pOwner, bool isStderr,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pOwner_( pOwner ),
	isStderr_( isStderr ),
	softspace_( false )
{
	BW_GUARD;
}


/**
 *	This method implements the Python write method. It is used to redirect the
 *	write calls to this object's pOwner_.
 */
PyObject * PyOutputStream::py_write( PyObject * args )
{
	BW_GUARD;

	char * msg = NULL;
	Py_ssize_t msglen = 0;

	if (!PyArg_ParseTuple( args, "s#", &msg, &msglen ))
	{
		ERROR_MSG( "PyOutputStream::py_write: Bad args\n" );
		return NULL;
	}

	// Handle embedded NULL (i.e. not UTF-8)
	if (memchr( msg, '\0', msglen ))
	{
		ScriptObject repr = ScriptObject( PyObject_Repr(
				PyTuple_GET_ITEM( args, 0 ) ),
			ScriptObject::FROM_NEW_REFERENCE );

		pOwner_->handleWrite( BW::string( PyString_AsString( repr.get() ),
			PyString_GET_SIZE( repr.get() ) ), isStderr_ );

		Py_RETURN_NONE;
	}

	pOwner_->handleWrite( BW::string( msg, msglen ), isStderr_ );
	Py_RETURN_NONE;

}


// -----------------------------------------------------------------------------
// Section: PythonOutputWriter
// -----------------------------------------------------------------------------
// Note: we don't actually lock the GIL, we're assuming our locking is correct.
// See PythonOutputWriter::addHook and PythonOutputWriter::delHook for details


/**
 *	Constructor
 *
 *	On construction this class overrides the stdout and stderr members of the
 *	sys module with PyOutputStreams that redirect to this instance.
 */
ScriptOutputWriter::ScriptOutputWriter() :
	ReferenceCount(),
	stdoutBuffer_(),
	stderrBuffer_(),
	hooks_(),
	pCurrentHook_( NULL ),
	shouldDeleteCurrentHook_( false )
{
	BW_GUARD;

	MF_ASSERT( Py_IsInitialized() );

	ScriptObject stdoutReplacement = ScriptObject( new PyOutputStream( this,
		/* isStderr */ false ), ScriptObject::FROM_NEW_REFERENCE );
	ScriptObject stderrReplacement = ScriptObject( new PyOutputStream( this,
		/* isStderr */ true ), ScriptObject::FROM_NEW_REFERENCE );

	MF_VERIFY( PySys_SetObject( "stdout", stdoutReplacement.get() ) == 0 );
	MF_VERIFY( PySys_SetObject( "stderr", stderrReplacement.get() ) == 0 );
}


/**
 *	Destructor
 */
ScriptOutputWriter::~ScriptOutputWriter()
{
	BW_GUARD;

	// We can't assert this, the usual case of PythonOutputWriter destruction is
	// clearing out the sys module in Py_Finalize, which happens after the
	// Py_IsInitialized() flag is cleared.
	//MF_ASSERT( Py_IsInitialized() );

	// This will crash unless Py_IsInitialized() or we are in that part
	// of Py_Finalize() between clearing Py_IsInitialized(), and
	// calling _PyGILState_Fini().
	//PyGILState_STATE gilState = PyGILState_Ensure();

	// pCurrentHook_ is only set by calls to PythonOutputWriter::handleWrite
	// from PyOutputStream, which holds a PythonOutputWriterPtr.
	MF_ASSERT( pCurrentHook_ == NULL );

	for (Hooks::iterator iHook = hooks_.begin(); iHook != hooks_.end(); ++iHook)
	{
		(*iHook)->onOutputWriterDestroyed( this );
	}

	hooks_.clear();

	//PyGILState_Release( gilState );
}


/**
 *	This method adds the given ScriptOutputHook to our collection of hooks.
 *
 *	This method must not be called without the GIL held, which in practice
 *	means calling only on the main thread or during Python script execution
 *	on a background thread.
 *
 *	A hook added twice will receive messages twice.
 *
 *	@param pHook	The ScriptOutputHook to add to our collection
 */
void ScriptOutputWriter::addHook( ScriptOutputHook * pHook )
{
	BW_GUARD;

	MF_ASSERT( Py_IsInitialized() );

	//PyGILState_STATE gilState = PyGILState_Ensure();

	hooks_.push_back( pHook );

	//PyGILState_Release( gilState );
}


/**
 *	This method removes the given ScriptOutputHook from our collection of hooks
 *
 *	This method must not be called without the GIL held, which in practice
 *	means calling only on the main thread or during Python script execution
 *	on a background thread.
 *
 *	A hook added twice must be deleted twice to stop receiving messages.
 *
 *	@param pHook	The ScriptOutputHook to remove from our collection
 */
void ScriptOutputWriter::delHook( ScriptOutputHook * pHook )
{
	BW_GUARD;

	MF_ASSERT( Py_IsInitialized() );

	//PyGILState_STATE gilState = PyGILState_Ensure();

	if (pHook == pCurrentHook_)
	{
		// This happens if our hook deletes itself.
		// Don't invalidate the iterator we're currently calling.
		shouldDeleteCurrentHook_ = true;
	}
	else
	{
		Hooks::iterator iHook =
			std::find( hooks_.begin(), hooks_.end(), pHook );
		if (iHook != hooks_.end())
		{
			hooks_.erase( iHook );
		}
	}

	//PyGILState_Release( gilState );
}


/**
 *	This static method finds the PythonOutputWriter that currently is receiving
 *	stdio output. sys.stdout is preferred if somehow sys.stdout and sys.stderr
 *	have different PythonOutputWriter instances attached.
 *
 *	@return	A PythonOutputWriter receiving sys.stdout if present, otherwise a
 *			PythonOutputWriter receiving sys.stderr if present, otherwise NULL.
 */
/* static */ ScriptOutputWriter * ScriptOutputWriter::getCurrentStdio()
{
	BW_GUARD;

	MF_ASSERT( Py_IsInitialized() );

	/* PySys_GetObject returns borrowed references */
	ScriptObject currentStdout = ScriptObject( PySys_GetObject( "stdout" ),
		ScriptObject::FROM_BORROWED_REFERENCE );
	PyOutputStreamPtr pStdout = PyOutputStreamPtr::create( currentStdout );
	if (pStdout)
	{
		return pStdout->pOwner().get();
	}

	ScriptObject currentStderr = ScriptObject( PySys_GetObject( "stderr" ),
		ScriptObject::FROM_BORROWED_REFERENCE );
	PyOutputStreamPtr pStderr = PyOutputStreamPtr::create( currentStderr );
	if (pStderr)
	{
		return pStderr->pOwner().get();
	}

	return NULL;
}


/**
 *	This method assembles a buffer from the input, and once it has a terminating
 *	newline, flushes it to the BigWorld logs and calls all the hooks.
 *
 *	This method is for the private use of PyOutputStream.
 *
 *	@param msg	The input given to the calling PyOutputStream
 *	@param isStderr	Whether the input was given on stderr or equivalent.
 */
void ScriptOutputWriter::handleWrite( const BW::string & msg, bool isStderr )
{
	BW_GUARD;

	BW::string & buffer = isStderr ? stderrBuffer_ : stdoutBuffer_;

	buffer += msg;

	if (!buffer.empty() && buffer[ buffer.size() - 1 ] == '\n')
	{
		// This is done so that the hack to prefix the time in cell and the base
		// applications works (needs a \n in the format string).
		buffer.resize( buffer.size() - 1 );

		BW::string::size_type lineStart = 0;

		do
		{
			BW::string::size_type lineEnd = buffer.find( '\n', lineStart );

			BW::string line( buffer.data() + lineStart,
				(lineEnd != BW::string::npos) ? 
					(lineEnd - lineStart) : (buffer.size() - lineStart) );

			LogMsg( isStderr ? MESSAGE_PRIORITY_ERROR : MESSAGE_PRIORITY_INFO ).
				source( MESSAGE_SOURCE_SCRIPT ).
				write( "%s\n", line.c_str() );

			lineStart = (lineEnd != BW::string::npos) ? 
				(lineEnd + 1) : BW::string::npos;
		}
		while (lineStart != BW::string::npos);

		buffer = "";
	}

	//PyGILState_STATE gilState = PyGILState_Ensure();

	// We _must_ have had the GIL to be fed data.
	//MF_ASSERT( gilState == PyGILState_LOCKED );

	MF_ASSERT( pCurrentHook_ == NULL );
	MF_ASSERT( shouldDeleteCurrentHook_ == false );

	Hooks::iterator iHook = hooks_.begin();

	while (iHook != hooks_.end())
	{
		pCurrentHook_ = *iHook;
		(*iHook)->onScriptOutput( msg, isStderr );

		if (shouldDeleteCurrentHook_)
		{
			iHook = hooks_.erase( iHook );
			shouldDeleteCurrentHook_ = false;
		}
		else
		{
			++iHook;
		}
	}

	pCurrentHook_ = NULL;

	MF_ASSERT( shouldDeleteCurrentHook_ == false );

	//PyGILState_Release( gilState );
}


} // namespace BW

// py_script_output_writer.cpp
