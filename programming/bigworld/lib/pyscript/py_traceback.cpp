#include "pyobject_plus.hpp"
#include "script.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"

#include "network/event_dispatcher.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "python/Include/frameobject.h"
#include "python/Include/traceback.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

BW_BEGIN_NAMESPACE

namespace Script
{


/**
 *	A class for outputting a Python exception traceback, to Python sys.stderr,
 *	with the ability to become asynchronous if the source files blocks on
 *	reading. 
 */
class TraceBack : public Mercury::InputNotificationHandler, 
				  public ReferenceCount
{
private:
	// Size of the file read buffer. Half of this amount in bytes is read from
	// a source file at a time.
	static const size_t BUFFLEN = 8192;

	// The maximum length of time to be busy processing tracebacks before we go
	// yield and wait for the asynchronous file read callback.
	static const uint32 MAX_TRACEBACK_READING_MS = 50;

public:
	TraceBack( PyObjectPtr pExcType, PyObjectPtr pExcValue, 
			PyObjectPtr pExcTraceback );
	virtual ~TraceBack();

	static void setEventDispatcher( 
			Mercury::EventDispatcher * pEventDispatcher );

	void output();

private:
	PyTracebackObject * traceback() const;

	bool isAtDesiredLineNum() const;

	void waitForReadEvent();
	void closeFile();

	virtual int handleInputNotification( int fd );

	void startFrame();
	int processInput( int fd );
	void outputFrame();
	void nextFrame();

private:
	PyObjectPtr pExcType_;		// Exception type.
	PyObjectPtr pExcValue_;		// Exception value.
	PyObjectPtr pExcTraceback_;	// The next frame's exception traceback object.

	char 		lineBuf_[BUFFLEN]; 	// Buffer for file read.
	char * 		lineBufUpTo_;	// Cursor for iterating through lineBuf_.

	const char * line_;			// Marks the start of each distinct source code
								// line.
	int 		lineNum_; 		// Line number corresponding to line_.

	PyObjectPtr pFilename_; 	// Current stack frame source file name.
	int 		fd_;			// File descriptor to read from.
	bool		isRegistered_;	// Whether registered for file events.

	// Event dispatcher to register file descriptors for reading.
	static Mercury::EventDispatcher * s_pEventDispatcher_;
};

Mercury::EventDispatcher * TraceBack::s_pEventDispatcher_ = NULL;


// -----------------------------------------------------------------------------
// Section: Implementation
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param pExcType 		The exception type object.
 *	@param pExcValue 		The exception value object.
 *	@param pExcTraceback 	The exception traceback object.
 */
TraceBack::TraceBack( PyObjectPtr pExcType, PyObjectPtr pExcValue, 
		PyObjectPtr pExcTraceback ) :
	pExcType_( pExcType ),
	pExcValue_( pExcValue ),
	pExcTraceback_( pExcTraceback ),
	lineBufUpTo_( NULL ),
	line_(),
	lineNum_(),
	pFilename_( NULL ),
	fd_( -1 ),
	isRegistered_( false )
{
}


/**
 *	Destructor.
 */
TraceBack::~TraceBack()
{
	MF_ASSERT( !isRegistered_ );
	MF_ASSERT( fd_ == -1 );
}


/**
 *	Set the event dispatcher to use for registering file descriptors.
 *
 *	@param pEventDispatcher 	The event dispatcher.
 */
void TraceBack::setEventDispatcher(
		Mercury::EventDispatcher * pEventDispatcher )
{
	s_pEventDispatcher_ = pEventDispatcher;
}


/**
 *	Output the exception traceback text.
 */
void TraceBack::output()
{
	// Write out the traceback message header
	PySys_WriteStderr( "Traceback (most recent call last):\n" );

	// Make sure we're not freed while waiting for IO, we decref when we
	// write out the exception value in nextFrame().
	this->incRef();
	this->startFrame();
}


/**
 *	Wait until handleInputNotification is called. This makes sure that this is
 *	registered for file events.
 */
void TraceBack::waitForReadEvent()
{
	if (!isRegistered_)
	{
		s_pEventDispatcher_->registerFileDescriptor( fd_, this, "PyTraceback" );
		isRegistered_ = true;
	}
}


/**
 *	Convenience accessor for getting the current traceback object.
 */
PyTracebackObject * TraceBack::traceback() const
{ 
	return reinterpret_cast< PyTracebackObject * >( pExcTraceback_.get() ); 
}


/**
 *	Returns whether the current line is the one wanted.
 */
bool TraceBack::isAtDesiredLineNum() const
{
	return (lineNum_ == this->traceback()->tb_lineno);
}


/**
 *	Set up to read another frame from the backtrace.
 */
void TraceBack::startFrame()
{
	lineNum_ = 1;
	lineBuf_[0] = '\0';
	lineBufUpTo_ = lineBuf_;
	line_ = this->isAtDesiredLineNum() ? lineBufUpTo_ : NULL;

	pFilename_ = reinterpret_cast< PyObject * >( 
		this->traceback()->tb_frame->f_code->co_filename );

	BW::string absFilePath = 
		BWResource::instance().fileSystem()->getAbsolutePath( 
			PyString_AS_STRING( pFilename_.get() ) );

	MF_ASSERT( fd_ == -1 );
	fd_ = open( absFilePath.c_str(), O_RDONLY | O_NONBLOCK );

	if (fd_ == -1)
	{
		// File couldn't be opened, assume it is missing then
		this->outputFrame();
	}
	else
	{
		this->processInput( fd_ );
	}
}


/**
 *	Notification handler for file read.
 *
 *	@param fd	The file descriptor to read from.
 */
int TraceBack::handleInputNotification( int fd )
{
	return this->processInput( fd );
}


/**
 *	Process read input on the file descriptor.
 *
 *	@param fd 	the file descriptor that can be read
 */
int TraceBack::processInput( int fd )
{
	MF_ASSERT( fd == fd_ );
	bool isDone = false;

	uint64 started = timestamp();

	while (!isDone)
	{
		uint64 elapsedMillis = (timestamp() - started) * 1000 /
			stampsPerSecond();

		if (elapsedMillis > MAX_TRACEBACK_READING_MS)
		{
			// We've taken too long, yield to rest of the system.
			this->waitForReadEvent();
			return 0;
		}

		size_t toRead = BUFFLEN/2;

		if (line_ == NULL)
		{
			lineBufUpTo_ = lineBuf_;
		}
		else
		{
			toRead = BUFFLEN - ptrdiff_t( lineBufUpTo_ - lineBuf_ );

			if (toRead == 0)
			{
				lineBuf_[ BUFFLEN - 1 ] = '\0';
			}
		}

		// Half fill the buffer
		ssize_t numBytesRead = read( fd_, lineBufUpTo_, toRead );

		if (numBytesRead == -1)
		{
			if (errno == EAGAIN)
			{
				// IO isn't ready right now, register the file descriptor
				// for read notifications. 
				this->waitForReadEvent();
				return 0;
			}

			// File read error, just forget about finding the source line.
			ERROR_MSG( "TraceBack::processInput: "
					"error reading \"%s\" (%s), "
					"exception raised on line %d\n",
				PyString_AS_STRING( pFilename_.get() ), 
				strerror( errno ),
				this->traceback()->tb_lineno );

			line_ = "";
			isDone = true;
		}
		else if (numBytesRead == 0)
		{
			if (line_ == NULL)
			{
				// premature EOF, print blank line
				line_ = "";
			}
			isDone = true;
		}

		const char * lineBufEnd = lineBufUpTo_ + size_t( numBytesRead );

		// Scan for new lines in what we have read in the buffer
		while (!isDone && (lineBufUpTo_ < lineBufEnd))
		{
			// We have got the end of a line.
			if (*lineBufUpTo_ == '\n')
			{
				if (line_)
				{
					// We have found the end of the line we're after.
					*lineBufUpTo_ = '\0';
					isDone = true;

					// Skip whitespace at the front of the next line
					while (*line_ == ' ' || *line_ == '\t' || 
							*line_ == '\f')
					{
						++line_;
					}
				}
				else
				{
					++lineNum_;

					// Is this the start of the one we're after?
					if (this->isAtDesiredLineNum())
					{
						line_ = lineBufUpTo_ + 1;
					}
				}
			}

			++lineBufUpTo_;
		}
	}

	this->closeFile();

	this->outputFrame();

	return 0;
}


/**
 *	This method closes the file associated with this object.
 */
void TraceBack::closeFile()
{
	// Make sure we're not registered with the event dispatcher.
	if (isRegistered_)
	{
		s_pEventDispatcher_->deregisterFileDescriptor( fd_ );
		isRegistered_ = false;
	}

	close( fd_ );
	fd_ = -1;
}


/**
 *	Write out the next stack frame's worth of traceback.
 */
void TraceBack::outputFrame()
{
	// Write out a traceback frame's info into a buffer. The buffer is
	// initially sized to this value, but will grow if it has to. This
	// should be enough for most source lines.
	size_t tbOutputBufLen = 256;
	char * tbOutputBuf = new char[tbOutputBufLen];
	bool writeOK = false;
	do
	{

		// Write out the frame info.
		size_t bufWritten = bw_snprintf( tbOutputBuf, tbOutputBufLen, 
			"  File \"%.500s\", line %d, in %.500s\n",
			PyString_AS_STRING( pFilename_.get() ), 
			this->traceback()->tb_lineno, 
			PyString_AS_STRING( 
				this->traceback()->tb_frame->f_code->co_name ) );

		// Write out the source line if we have one.
		if (line_ != NULL && line_[0] && bufWritten < tbOutputBufLen)
		{
			bufWritten += bw_snprintf( tbOutputBuf + bufWritten, 
					tbOutputBufLen - bufWritten,
				"    %s\n", line_ );
		}

		if (bufWritten < tbOutputBufLen)
		{
			writeOK = true;
		}
		else
		{
			// Allocate a larger buffer and try again.
			tbOutputBufLen *= 2;

			delete [] tbOutputBuf;
			tbOutputBuf = new char[tbOutputBufLen];
		}
	} while (!writeOK);


	PySys_WriteStderr( "%s", tbOutputBuf );
	delete [] tbOutputBuf;

	this->nextFrame();
}


/**
 *	Start reading the next frame.
 */
void TraceBack::nextFrame()
{
	// Traverse the stack
	pExcTraceback_ = reinterpret_cast< PyObject * >( 
		this->traceback()->tb_next );

	// Is there another frame?
	if (!pExcTraceback_)
	{
		// We're finished here, print out the actual error
		PyErr_Display( pExcType_.get(), pExcValue_.get(), NULL );

		// And free ourself from where we incref'd in output().
		this->decRef();
		return;
	}

	// It's not over yet, start the cycle anew
	this->startFrame();
}


// -----------------------------------------------------------------------------
// Section: Python functions
// -----------------------------------------------------------------------------

extern "C"
{

/* function printTraceBack
 *	@components{ all }
 *	py_printTraceBack emulates the internal Python stack trace but without any
 *	risk of blocking the main thread due to reading source file lines for
 *	output.
 * 
 *	@param excType			The exception type object.
 *	@param excValue			The exception value.
 *	@param excTraceback 	The traceback object associated with when the
 *							exception raised.
 */
static PyObject * py_printTraceBack( PyObject * self, PyObject * args )
{
	PyObject * pExcType;
	PyObject * pExcValue;
	PyObject * pExcTraceback;

	if (PyArg_ParseTuple( args, "OOO", &pExcType, &pExcValue, &pExcTraceback ))
	{
		// If pyExcTraceback  is not valid, assume no traceback is required
		// and call back into the default handler
		if (!PyTraceBack_Check( pExcTraceback )) 
		{
			PyErr_Display( pExcType, pExcValue, pExcTraceback );
		}
		else
		{
			// Create a new context for writing the trace
			SmartPointer< TraceBack > pTraceBack = 
				new TraceBack( pExcType, pExcValue, pExcTraceback );

			pTraceBack->output();
		}
	}
	else
	{
		ERROR_MSG( "py_printTraceBack(): Could not parse args\n" );
		PyErr_Clear();
	}
	Py_RETURN_NONE;
}

} // extern "C"


// -----------------------------------------------------------------------------
// Section: Initialisation
// -----------------------------------------------------------------------------

/**
 *	Initialise our own traceback mechanism which uses non-blocking IO when
 *	reading source files. The event dispatcher is required for asynchronous
 *	file reads.
 *
 *	Note that it is assumed that the event dispatcher be kept around until the
 *	Python interpreter has been finalised.
 *
 *	@param pEventDispatcher 	An event dispatcher to register file
 *								descriptors with.
 */
void initExceptionHook( Mercury::EventDispatcher * pEventDispatcher )
{
	static PyMethodDef printTraceBackDef =
	{
		const_cast< char * >( "printTraceBack" ), 	// ml_name
		(PyCFunction)py_printTraceBack,				// ml_meth
		METH_VARARGS | METH_STATIC, 				// ml_flags
		const_cast< char * >( "" ) 					// ml_doc
	};

	TraceBack::setEventDispatcher( pEventDispatcher );
	PySys_SetObject( "excepthook", 
					 PyCFunction_New( &printTraceBackDef, NULL ) );
}


} // namespace Script

BW_END_NAMESPACE

// py_traceback.cpp
