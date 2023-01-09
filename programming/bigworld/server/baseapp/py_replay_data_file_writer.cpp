#include "script/first_include.hpp"

#include "py_replay_data_file_writer.hpp"


BW_BEGIN_NAMESPACE


// ----------------------------------------------------------------------------
// Section: PyReplayDataFileWriterListener
// ----------------------------------------------------------------------------


/**
 *	This class is used as an adaptor for a Python callback to be used as the
 *	listener for a ReplayDataFileWriter.
 */
class PyReplayDataFileWriterListener: public ReplayDataFileWriterListener
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param pyWriter 	The PyReplayDataFileWriter wrapper around the
	 *						writer we are listening to.
	 */
	PyReplayDataFileWriterListener( PyReplayDataFileWriter & pyWriter ) :
			pPyWriter_( &pyWriter ),
			pWriterToFinalise_( NULL ),
			pCallback_( NULL )
	{
	}


	/** Destructor. */
	virtual ~PyReplayDataFileWriterListener()
	{
	}


	/**
	 *	This method sets the callback object for our wrapped writer.
	 *
	 *	@param pCallback 	The callback object to set.
	 */
	void setCallback( PyObject * pCallback )
	{
		pCallback_ = pCallback;
	}


	/**
	 *	This method is called when the Python writer wrapper is destroyed
	 *	before the underlying writer has been finalised.
	 *
	 *	The ownership of the given writer is passed to this object, and will be
	 *	deleted when finalisation is complete. The writer will be finalised, if
	 *	not already finalising.
	 *
	 *	@param pWriter 	The writer to wait for finalisation, that is now owned
	 *					by this object.
	 */
	void pyWriterDestroyedBeforeFinalising( ReplayDataFileWriter * pWriter )
	{
		pPyWriter_ = NULL;

		pWriterToFinalise_ = pWriter;
		if (!pWriterToFinalise_->isFinalising())
		{
			pWriterToFinalise_->finalise();
		}
	}


	/*
	 *	Override from ReplayDataFileWriterListener.
	 */
	virtual void onReplayDataFileWriterDestroyed(
			ReplayDataFileWriter & writer )
	{
		delete this;
	}


	/*
	 *	Override from ReplayDataFileWriterListener.
	 */
	virtual void onReplayDataFileWritingError(
			ReplayDataFileWriter & writer )
	{
		if (pCallback_.get())
		{
			PyObject_CallFunction( pCallback_.get(), "OO",
				pPyWriter_ ? pPyWriter_ : Py_None,
				Py_False );
		}

		if (pWriterToFinalise_)
		{
			MF_ASSERT( &writer == pWriterToFinalise_ );
			delete pWriterToFinalise_; // No member access after this point
		}
	}


	/*
	 *	Override from ReplayDataFileWriterListener.
	 */
	virtual void onReplayDataFileWritingComplete(
			ReplayDataFileWriter & writer )
	{
		if (pCallback_.get())
		{
			PyObject_CallFunction( pCallback_.get(), "OO",
				pPyWriter_ ? pPyWriter_ : Py_None,
				Py_True );
		}

		if (writer.isClosed() && pWriterToFinalise_)
		{
			MF_ASSERT( &writer == pWriterToFinalise_ );
			delete pWriterToFinalise_; // No member access after this point
		}
	}


private:
	PyReplayDataFileWriter * pPyWriter_;
	ReplayDataFileWriter * pWriterToFinalise_;
	PyObjectPtr pCallback_;
};


// -----------------------------------------------------------------------------
// Section: PyReplayDataFileWriter
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyReplayDataFileWriter )

PY_BEGIN_METHODS( PyReplayDataFileWriter )

	/*~ function PyReplayDataFileWriter addTickData
	 *	@components{ base }
	 *
	 *	This method queues the given tick data to be written out to the output
	 *	file.
	 *
	 *	@param gameTime 	The game time of the tick.
	 *	@param numCells 	The number of cells in this tick for the recorded
	 *						space.
	 *	@param blob			The tick data blob.
	 */
	PY_METHOD( addTickData )


	/*~ function PyReplayDataFileWriter setListener
	 *	@components{ base }
	 *
	 *	This method sets the listener callback for this writer to receive file
	 *	writing events.
	 *
	 *	@param listener The listener callable object. It is called with:
	 *	@{
	 *	def callback( writer, success )
	 *	@}
	 *
	 *	If an error occurs while writing, the callback is called with the
	 *	writer instance and the success parameter set to False.
	 *
	 *	When writing queued to the file has been completed, the callback is
	 *	called with the success parameter set to True.
	 */
	PY_METHOD( setListener )


	/*~ function PyReplayDataFileWriter close
	 *	@components{ base }
	 *
	 *	This method closes the replay data file. No further tick data can be
	 *	added after calling this method. Any tick data previously queued will
	 *	still be written out.
	 *
	 *	@param shouldFinalise 	If True (the default), the file will be set to
	 *							read-only after finalisation, and the number of
	 *							ticks in the header will be updated. Any
	 *							listeners will receive a final completion
	 *							notification.
	 *							If False, the writer is closed immediately, and
	 *							no completion notification is sent. This is
	 *							useful when allowing other processes to
	 *							recover.
	 */
	PY_METHOD( close )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyReplayDataFileWriter )

	/*~ attribute PyReplayDataFileWriter resourcePath
	 *	@components{ base }
	 *	This attribute returns the initial resource path passed to
	 *	BigWorld.createReplayDataFileWriter().
	 */
	PY_ATTRIBUTE( resourcePath )

	/*~ attribute PyReplayDataFileWriter compressionType
	 *	@components{ base }
	 *	This attribute returns the compression type.
	 */
	PY_ATTRIBUTE( compressionType )

	/*~ attribute PyReplayDataFileWriter path
	 *	@components{ base }
	 *	This attribute contains the resolved absolute path to the file being
	 *	written to.
	 */
	PY_ATTRIBUTE( path )


	/*~ attribute PyReplayDataFileWriter isClosed
	 *	@components{ base }
	 *	This attribute returns whether this writer has been closed.
	 */
	PY_ATTRIBUTE( isClosed )


	/*~ attribute PyReplayDataFileWriter isFinalising
	 *	@components{ base }
	 *	This attribute returns whether this writer is in the process of
	 *	finalising, or is already finalised.
	 */
	PY_ATTRIBUTE( isFinalising )


	/*~ attribute PyReplayDataFileWriter errorString
	 *	@components{ base }
	 *	This attribute contains a string describing the last file error, or
	 *	None if no such error has occurred.
	 */
	PY_ATTRIBUTE( errorString )


	/*~ attribute PyReplayDataFileWriter numTicksWritten
	 *	@components{ base }
	 *	This attribute reports the number of ticks written out to the file.
	 */
	PY_ATTRIBUTE( numTicksWritten )

	/*~ attribute PyReplayDataFileWriter lastTickWritten
	 *	@components{ base }
	 *	This attribute returns the last tick confirmed to be written out to the
	 *	file.
	 */
	PY_ATTRIBUTE( lastTickWritten )

	/*~ attribute PyReplayDataFileWriter lastTickPendingWrite
	 *	@components{ base }
	 *	This attribute returns the last tick that is pending to be written out 
	 *	to disk.
	 */
	PY_ATTRIBUTE( lastTickPendingWrite )

	/*~ attribute PyReplayDataFileWriter lastChunkPosition
	 *	@components{ base }
	 *	This attribute reports the game time of the last tick written out to
	 *	the file.
	 */
	PY_ATTRIBUTE( lastChunkPosition )

	/*~ attribute PyReplayDataFileWriter lastChunkLength
	 *	@components{ base }
	 *	This attribute reports the length of the last tick written out to the
	 *	file.
	 */
	PY_ATTRIBUTE( lastChunkLength )

	/*~ attribute PyReplayDataFileWriter numTicksToSign 
	 *	@components{ base }
	 *	This attribute contains the number of consecutive ticks that will be
	 *	signed in each signed chunk.
	 */
	PY_ATTRIBUTE( numTicksToSign )

	/*~ attribute PyReplayDataFileWriter recoveryData
	 *	@components{ base }
	 *	This attribute contains the serialised recovery data which can be used
	 *	to continue writing in another PyReplayDataFileWriter instance.
	 */
	PY_ATTRIBUTE( recoveryData )

PY_END_ATTRIBUTES()


/**
 *	Constructor for new files.
 *
 *	@param resourcePath 	The resource path used to open the writer.
 *	@param pWriter			A background file writer.
 *	@param compressionType 	The compression level to use.
 *	@param digest 			The digest.
 *	@param pType 			The Python type object to use.
 */
PyReplayDataFileWriter::PyReplayDataFileWriter( const BW::string & resourcePath,
			BackgroundFileWriter * pWriter,
			ChecksumSchemePtr pChecksumScheme,
			uint numTicksToSign,
			BWCompressionType compressionType,
			const MD5::Digest & digest,
			const ReplayMetaData & metaData,
			PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		resourcePath_( resourcePath ),
		pWriter_( new ReplayDataFileWriter( pWriter, pChecksumScheme,
			compressionType, digest, numTicksToSign, metaData ) ),
		pListener_( new PyReplayDataFileWriterListener( *this ) )
{
	pWriter_->addListener( pListener_ );
}


/**
 *	Constructor for recovered files.
 *
 *	@param resourcePath 	The resource path used to open the writer.
 *	@param pWriter			A background file writer.
 *	@param recoveryData 	The recovery state.
 *	@param pType 			The Python type object to use.
 */
PyReplayDataFileWriter::PyReplayDataFileWriter( const BW::string & resourcePath,
			BackgroundFileWriter * pWriter,
			ChecksumSchemePtr pChecksumScheme,
			const RecordingRecoveryData & recoveryData,
			PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		resourcePath_( resourcePath ),
		pWriter_( new ReplayDataFileWriter( pWriter, pChecksumScheme,
			recoveryData ) ),
		pListener_( new PyReplayDataFileWriterListener( *this ) )
{
	pWriter_->addListener( pListener_ );
}


/**
 *	Destructor.
 */
PyReplayDataFileWriter::~PyReplayDataFileWriter()
{
	if (pWriter_->isClosed())
	{
		pListener_ = NULL; // deleted by itself
		bw_safe_delete( pWriter_ );
	}
	else
	{
		pListener_->pyWriterDestroyedBeforeFinalising( pWriter_ );
	}
}


/**
 *	This method implements the addTickData() Python method.
 */
PyObject * PyReplayDataFileWriter::py_addTickData( PyObject * args )
{
	GameTime gameTime;
	size_t numCells;
	const char * blob;
	Py_ssize_t blobLength;

	if (!PyArg_ParseTuple( args, "ILs#",
			&gameTime, &numCells, &blob, &blobLength ))
	{
		return NULL;
	}

	if (pWriter_->hasError())
	{
		PyErr_Format( PyExc_ValueError, "writer has error: %s",
			pWriter_->errorString().c_str() );
		return NULL;
	}

	if (this->isFinalising())
	{
		PyErr_SetString( PyExc_ValueError, "writer is being finalised" );
		return NULL;
	}

	MemoryIStream blobData( blob, blobLength );

	pWriter_->addTickData( gameTime, numCells, blobData );

	Py_RETURN_NONE;
}


/**
 *	This method implements the Python setListener() method.
 */
PyObject * PyReplayDataFileWriter::py_setListener( PyObject * args )
{
	PyObject * callbackObject = NULL;

	if (!PyArg_ParseTuple( args, "O", &callbackObject))
	{
		return NULL;
	}

	if (callbackObject == Py_None)
	{
		callbackObject = NULL;
	}
	else if (!PyCallable_Check( callbackObject ))
	{
		PyErr_SetString( PyExc_TypeError, "callback must be callable" );
		return NULL;
	}

	pListener_->setCallback( callbackObject );

	Py_RETURN_NONE;
}


/**
 *	This method implements the Python close() method.
 */
PyObject * PyReplayDataFileWriter::py_close( PyObject * args )
{
	uint8 shouldFinalise = 1;

	if (!PyArg_ParseTuple( args, "|H", &shouldFinalise ))
	{
		return NULL;
	}

	if (this->isFinalising() || this->isClosed())
	{
		PyErr_SetString( PyExc_ValueError,
			(this->isClosed() ?
				"Writer is already closed" :
				"Writer is already being finalised") );
		return NULL;
	}

	pWriter_->close( shouldFinalise );

	Py_RETURN_NONE;
}


/**
 *	This method implements the accesssor for the path attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_path()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->path() );
}


/**
 *	This method implements the accessor for the errorString attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_errorString()
{
	if (!pWriter_ || !pWriter_->hasError())
	{
		Py_RETURN_NONE;
	}

	BW::string errorString = pWriter_->errorString();
	if (!errorString.size())
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->errorString() );
}


/**
 *	This method implements the accessor for the numTicksWritten attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_numTicksWritten()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->numTicksWritten() );
}


/**
 *	This method implements the accessor for the lastTickWritten attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_lastTickPendingWrite()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->lastTickPendingWrite() );
}


/**
 *	This method implements the accessor for the lastTickWritten attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_lastTickWritten()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->lastTickWritten() );
}


/**
 *	This method implements the accessor for the lastChunkPosition attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_lastChunkPosition()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->lastChunkPosition() );
}


/**
 *	This method implements the accessor for the lastChunkLength attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_lastChunkLength()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->lastChunkLength() );
}


/**
 *	This method implements the accessor for the recoveryData attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_recoveryData()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->recoveryData().toString() );
}


/**
 *	This method implements the accessor for the numTicksToSign attribute.
 */
PyObject * PyReplayDataFileWriter::pyGet_numTicksToSign()
{
	if (!pWriter_)
	{
		Py_RETURN_NONE;
	}

	return Script::getReadOnlyData( pWriter_->numTicksToSign() );
}


BW_END_NAMESPACE


// py_replay_data_file_writer.cpp
