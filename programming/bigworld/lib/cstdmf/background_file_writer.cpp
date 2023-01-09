#include "pch.hpp"

#include "background_file_writer.hpp"
#include "cstdmf/bw_util.hpp"

#include <errno.h>
#include <sys/stat.h>

BW_BEGIN_NAMESPACE


/**
 *	This is the superclass for all background file write requests.
 */
class WriteRequest
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param userData 	The user data to be returned back when the request
	 *						is handled.
	 */
	WriteRequest( int userData ) :
			filePosition_( -1 ),
			userData_( userData )
	{}


	/**
	 *	Destructor.
	 */
	virtual ~WriteRequest()
	{}


	/**
	 *	This method returns the file position before the file operation, or -1
	 *	if the request hasn't been handled yet.
	 */
	long filePosition() const  				{ return filePosition_; }


	/**
	 *	This method sets the file position. This is called just before the
	 *	request is handled.
	 *
	 *	@param filePosition 	The file position to set for this request.
	 */
	void filePosition( long filePosition ) 	{ filePosition_ = filePosition; }


	/**
	 *	Accessor for the user data.
	 */
	int userData() const 					{ return userData_; }


	/**
	 *	This method is called from the background thread to implement the file
	 *	operation.
	 *
	 *	@param writer 	The associated BackgroundFileWriter instance.
	 */
	virtual bool handleRequest( BackgroundFileWriterTask & writer ) = 0;

private:
	long 	filePosition_;
	int 	userData_;
};


// -----------------------------------------------------------------------------
// Section: BackgroundFileWriterTask
// -----------------------------------------------------------------------------


/**
 *	This class is used to write to a file in a background task.
 */
class BackgroundFileWriterTask : public BackgroundTask
{
public:
	BackgroundFileWriterTask( BackgroundFilePathPtr pPath,
			bool shouldOverwrite,
			bool shouldTruncate );

	virtual ~BackgroundFileWriterTask();

	void closeFile();

	void setTaskData( BackgroundFileWriterPtr pBackgroundFileWriter,
		BackgroundFileWriter::WriteRequests & writeRequests,
		MemoryOStream * pBufferedWriteData );

	// These methods should not be called from the main thread
	bool doBackgroundWrite( size_t length );
	bool doBackgroundSeek( long offset, int whence );
	bool doBackgroundChmod( int mode );

private:
	// Overrides from BackgroundTask
	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );

	bool openFile();

	// Member data
	BackgroundFileWriterPtr			pBackgroundFileWriter_;
	BackgroundFilePathPtr 			pPath_;
	FILE *							fp_;
	MemoryOStream *					pBufferedWriteData_;
	BackgroundFileWriter::WriteRequests			
									writeRequests_;
	bool							hasError_;
	int								errno_;
	bool							shouldOverwrite_;
	bool							shouldTruncate_;
};


/**
 *	Constructor.
 *
 *	@param path 			The path of the file to open.
 *	@param shouldOverwrite 	If true, any pre-existing file will be overwritten.
 *							Otherwise, an error will be raised.
 *	@param shouldTruncate	Whether the file will be truncated to zero-length,
 *							or whether any existing file contents will be
 *							preserved.
 */
BackgroundFileWriterTask::BackgroundFileWriterTask( BackgroundFilePathPtr pPath,
			bool shouldOverwrite /* = false */, 
			bool shouldTruncate /* = false */ ) :
		BackgroundTask( "BackgroundFileWriter" ),
		pBackgroundFileWriter_( NULL ),
		pPath_( pPath ),
		fp_( NULL ),
		pBufferedWriteData_( NULL ),
		writeRequests_(),
		hasError_( false ),
		errno_( 0 ),
		shouldOverwrite_( shouldOverwrite ),
		shouldTruncate_( shouldTruncate )
{
}


/**
 *	Destructor.
 */
BackgroundFileWriterTask::~BackgroundFileWriterTask()
{
	if (!hasError_ && 
		pBufferedWriteData_ && pBufferedWriteData_->remainingLength())
	{
		ERROR_MSG( "BackgroundFileWriterTask::~BackgroundFileWriterTask: "
				"Still have %d bytes of buffered write data for \"%s\"\n",
			pBufferedWriteData_->remainingLength(),
			pPath_->path().c_str() );
	}

	this->closeFile();

	BackgroundFileWriter::WriteRequests::iterator iRequest =
		writeRequests_.begin();
	while (iRequest != writeRequests_.end())
	{
		delete *iRequest;
		++iRequest;
	}

	bw_safe_delete( pBufferedWriteData_ );
}


/**
 *	This method closes the currently open file.
 */
void BackgroundFileWriterTask::closeFile()
{
	if (fp_ != NULL)
	{
		fclose( fp_ );
		fp_ = NULL;
	}
}


/**
 *	This method sets the task data, writeRequests will be swapped with an
 *	empty writeRequests vector.
 *
 *	@param pBackgroundFileWriter	The background file writer
 *	@param writeRequests			The write requests
 *	@param pBufferedWriteData		The write data
 */
void BackgroundFileWriterTask::setTaskData( 
		BackgroundFileWriterPtr pBackgroundFileWriter,
		BackgroundFileWriter::WriteRequests & writeRequests,
		MemoryOStream * pBufferedWriteData )
{
	MF_ASSERT( writeRequests_.empty() );
	MF_ASSERT( pBackgroundFileWriter );
	// Only keep a reference to background file writer, while
	// we are processing the background task.
	pBackgroundFileWriter_ = pBackgroundFileWriter;
	writeRequests_.swap( writeRequests );
	pBufferedWriteData_ = pBufferedWriteData;
}


/**
 *	This method implements the file writing operation to be performed in the
 *	background.
 *
 *	@param request 	The length of data to write.
 */
bool BackgroundFileWriterTask::doBackgroundWrite( size_t length )
{
	const char * data = reinterpret_cast< const char * >(
		pBufferedWriteData_->retrieve( static_cast<int>(length) ) );

	size_t numBytesWritten = 0;

	while (numBytesWritten < length)
	{
		size_t writeResult = fwrite( data + numBytesWritten, 1,
				length - numBytesWritten, fp_ );

		if (writeResult == 0)
		{
			errno_ = errno;
			return false;
		}

		numBytesWritten += writeResult;
	}

	return true;
}


/**
 *	This method implements the seek operation to be called in the background
 *	thread.
 *
 *	@param offset 	The offset argument to fseek().
 *	@param whence 	The whence argument to fseek().
 */
bool BackgroundFileWriterTask::doBackgroundSeek( long offset, int whence )
{
	if (0 != fseek( fp_, offset, whence ))
	{
		errno_ = errno;
		return false;
	}

	return true;
}


/**
 *	This method implements the chmod() operation to be called in the background
 *	thread.
 *
 *	@param mode 	The mode to set.
 */
bool BackgroundFileWriterTask::doBackgroundChmod( int mode )
{
#if defined( MF_SERVER )
	if (0 != fchmod( bw_fileno( fp_ ), mode ))
	{
		errno_ = errno;
		return false;
	}
#else // !defined( MF_SERVER )
	WARNING_MSG( "BackgroundFileWriterTask::doBackgroundChmod: "
			"Not supported (path = \"%s\")\n",
		pPath_->path().c_str() );
#endif // defined( MF_SERVER )

	return true;
}


/**
 *	This method opens the file for writing.
 *
 *	@return 		true if successful, false if not and errno is set.
 */
bool BackgroundFileWriterTask::openFile()
{
	if (!pPath_->resolve())
	{
		return false;
	}

	struct stat outputFileStat;

	bool fileExists = (0 == stat( pPath_->path().c_str(), &outputFileStat ));

	if (!fileExists && (errno != ENOENT))
	{
		ERROR_MSG( "BackgroundFileWriterTask::openFile: "
				"Failed to stat \"%s\"\n",
			pPath_->path().c_str() );
		return false;
	}

	if (fileExists && !shouldOverwrite_)
	{
		ERROR_MSG( "BackgroundFileWriterTask::openFile: "
				"Failed to write \"%s\", file already exists.\n",
			pPath_->path().c_str() );
		errno = EEXIST;
		return false;
	}

#if defined( MF_SERVER )
	mode_t mode = outputFileStat.st_mode;

	if (fileExists && 
			((mode & S_IWUSR) == 0) && 
			(-1 == chmod( pPath_->path().c_str(), mode | S_IWUSR )))
	{
		ERROR_MSG( "BackgroundFileWriterTask::openFile: "
				"Cannot change mode of existing file %s: %s\n", 
			pPath_->path().c_str(), strerror( errno ) );

		return false;
	}
#endif // defined( MF_SERVER )

	const char * fileOpenMode = (shouldTruncate_ || !fileExists) ?
		"w+b" : "r+b";

	if (NULL == (fp_ = bw_fopen( pPath_->path().c_str(), fileOpenMode )))
	{
		ERROR_MSG( "BackgroundFileWriterTask::openFile: "
				"Failed to open file for writing (%s): %s\n",
			fileOpenMode, pPath_->path().c_str() );

		return false;
	}

	return true;
}


/*
 *	Override from BackgroundTask.
 */
void BackgroundFileWriterTask::doBackgroundTask( TaskManager & mgr )
{
	MF_ASSERT( pBackgroundFileWriter_ );
	MF_ASSERT( !writeRequests_.empty() && (pBufferedWriteData_ != NULL) );

	if ((fp_ == NULL) && !this->openFile())
	{
		hasError_ = true;
		errno_ = errno;
		mgr.addMainThreadTask( this );
		return;
	}

	BackgroundFileWriter::WriteRequests::iterator ipRequest =
		writeRequests_.begin();

	while (!hasError_ && 
			(ipRequest != writeRequests_.end()))
	{
		WriteRequest * pRequest = *ipRequest;

		pRequest->filePosition( ftell( fp_ ) );

		if (!pRequest->handleRequest( *this ))
		{
			hasError_ = true;
		}

		++ipRequest;
	}

	// Flush out write buffers.
	if (!hasError_ && (0 != fflush( fp_ )))
	{
		errno_ = errno;
		hasError_ = true;
	}

	// Make sure it actually hits disk.
	if (!hasError_ && (0 != bw_fsync( bw_fileno( fp_ ) )))
	{
		errno_ = errno;
		hasError_ = true;
	}

	mgr.addMainThreadTask( this );
}


/*
 *	Override from BackgroundTask.
 */
void BackgroundFileWriterTask::doMainThreadTask( TaskManager & mgr )
{
	MF_ASSERT( pBackgroundFileWriter_ );

	BackgroundFileWriter::WriteRequests::iterator ipRequest =
		writeRequests_.begin();
	while (ipRequest != writeRequests_.end())
	{
		if (!hasError_)
		{
			pBackgroundFileWriter_->onWriteCompleted( **ipRequest );
		}

		delete *ipRequest;
		++ipRequest;
	}

	writeRequests_.clear();

	bw_safe_delete( pBufferedWriteData_ );

	// Only keep a reference to background file writer, while
	// we are processing the background task.
	if (pBackgroundFileWriter_->onBgTaskComplete( this, hasError_, errno_ ))
	{
		pBackgroundFileWriter_ = NULL;
	}
}


// -----------------------------------------------------------------------------
// Section: Write requests
// -----------------------------------------------------------------------------


/**
 *	This class implements a file write data request.
 */
class WriteDataWriteRequest : public WriteRequest
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param length 		The number of bytes to write from the write data
	 *						buffer.
	 *	@param userData 	The user data to associate with this request.
	 */
	WriteDataWriteRequest( size_t length, int userData ) :
			WriteRequest( userData ),
			length_( length )
	{}

	/**
	 *	Destructor.
	 */
	virtual ~WriteDataWriteRequest() {}

	/**
	 *	The length to write out from the write data buffer.
	 */
	size_t length() const { return length_; }


	// Override from WriteRequest.
	virtual bool handleRequest( BackgroundFileWriterTask & writer );

private:
	size_t length_;
};


/*
 *	Override from WriteRequest.
 */
bool WriteDataWriteRequest::handleRequest( BackgroundFileWriterTask & writer )
{
	return writer.doBackgroundWrite( length_ );
}


/**
 *	This class implements a a background file seek request.
 */
class SeekWriteRequest : public WriteRequest
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param offset 		The offset argument to fseek().
	 *	@param whence 		The whence argument to fseek().
	 *	@param userData 	The user data to associate with this request.
	 */
	SeekWriteRequest( long offset, int whence, int userData ) :
			WriteRequest( userData ),
			offset_( offset ),
			whence_( whence )
	{}

	virtual ~SeekWriteRequest() {}


	virtual bool handleRequest( BackgroundFileWriterTask & writer );

private:
	long 	offset_;
	int 	whence_;

};


/*
 *	Override from WriteRequest.
 */
bool SeekWriteRequest::handleRequest( BackgroundFileWriterTask & writer )
{
	return writer.doBackgroundSeek( offset_, whence_ );
}


/**
 *	This request handles a chmod() request.
 */
class ChmodRequest : public WriteRequest
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param mode 		The file mode to set.
	 *	@param userData 	The user data to associate with this request.
	 */
	ChmodRequest( int mode, int userData ) :
			WriteRequest( userData ),
			mode_( mode )
	{}

	/**	Destructor. */
	virtual ~ChmodRequest() {}

	// Override from WriteRequest.
	virtual bool handleRequest( BackgroundFileWriterTask & writer );

private:
	int mode_;
};


/*
 *	Override from WriteRequest.
 */
bool ChmodRequest::handleRequest( BackgroundFileWriterTask & writer )
{
	return writer.doBackgroundChmod( mode_ );
}


// -----------------------------------------------------------------------------
// Section: BackgroundFileWriter
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 *
 *	@param manager 			The BackgroundFileWriterManager instance.
 */
BackgroundFileWriter::BackgroundFileWriter( TaskManager & taskManager ) :
		IBackgroundFileWriter(),
		taskManager_( taskManager ),
		unresolvedPath_(),
		pPath_( NULL ),
		pBufferedWriteData_( new MemoryOStream() ),
		writeRequests_(),
		pListener_( NULL ),
		hasError_( false ),
		errno_( 0 ),
		pBackgroundTask_( NULL )
{
	pBufferedWriteData_->canRewind( false );
}


/**
 *	This method initialises this writer with a file-system path.
 *
 *	@param path 			The file system path for writing.
 *	@param shouldOverwrite	If true, any pre-existing file at the path will be
 *							overwritten, otherwise, an error will be raised via
 *							onBackgroundFileWritingError().
 *	@param shouldTruncate	Whether the file will be truncated to zero-length,
 *							or whether any existing file contents will be
 *							preserved.
 */
void BackgroundFileWriter::initWithFileSystemPath(
		const BW::string & fileSystemPath,
		bool shouldOverwrite /* = false */,
		bool shouldTruncate /* = false */ )
{
	this->initWithPath( BackgroundFilePath::createFileSystemPath(
		fileSystemPath ), shouldOverwrite, shouldTruncate );
}


/**
 *	This method initialises this writer with an abstract file-system path that
 *	may need resolving in the background task.
 *
 *	@param path 			The path object.
 *	@param shouldOverwrite	If true, any pre-existing file at the path will be
 *							overwritten, otherwise, an error will be raised via
 *							onBackgroundFileWritingError().
 *	@param shouldTruncate	Whether the file will be truncated to zero-length,
 *							or whether any existing file contents will be
 *							preserved.
 */
void BackgroundFileWriter::initWithPath( BackgroundFilePathPtr pPath,
		bool shouldOverwrite /* = false */,
		bool shouldTruncate /* = false */ )
{
	pPath_ = pPath;
	unresolvedPath_ = pPath_->path();
	pBackgroundTask_ = new BackgroundFileWriterTask( pPath_,
		shouldOverwrite, shouldTruncate );
}


/**
 *	Destructor.
 */
BackgroundFileWriter::~BackgroundFileWriter()
{
	if (!hasError_ && pBufferedWriteData_->remainingLength())
	{
		ERROR_MSG( "BackgroundFileWriter::~BackgroundFileWriter: "
				"Still have %d bytes of buffered write data for \"%s\"\n",
			pBufferedWriteData_->remainingLength(),
			unresolvedPath_.c_str() );
	}

	this->closeFile();

	WriteRequests::iterator iter = writeRequests_.begin();
	while (iter != writeRequests_.end())
	{
		delete *iter;
		++iter;
	}

	bw_safe_delete( pBufferedWriteData_ );
}


/*
 *	Override from IBackgroundFileWriter.
 */
void BackgroundFileWriter::closeFile()
{
	if (pBackgroundTask_)
	{
		pBackgroundTask_->closeFile();
	}
}


/*
 *	Override from IBackgroundFileWriter.
 */
void BackgroundFileWriter::queueWrite( BinaryIStream & stream, int userData )
{
	if (hasError_)
	{
		stream.finish();
		return;
	}

	int length = stream.remainingLength();
	pBufferedWriteData_->transfer( stream, length );

	WriteDataWriteRequest * pRequest = 
		new WriteDataWriteRequest( length, userData );

	this->queueRequest( pRequest );
}


/*
 *	Override from IBackgroundFileWriter.
 */
void IBackgroundFileWriter::queueWriteBlob( const void * data, int size, 
		int userData )
{
	MemoryIStream dataStream( reinterpret_cast< const char * >( data ), size );
	this->queueWrite( dataStream, userData );
}


/*
 *	Override from IBackgroundFileWriter.
 */
void BackgroundFileWriter::queueSeek( long offset, int whence, int userData )
{
	if (hasError_)
	{
		return;
	}

	SeekWriteRequest * pRequest = new SeekWriteRequest( offset, whence,
		userData );

	this->queueRequest( pRequest );
}


/*
 *	Override from IBackgroundFileWriter.
 */
void BackgroundFileWriter::queueChmod( int mode, int userData )
{
	if (hasError_)
	{
		return;
	}

	ChmodRequest * pRequest = new ChmodRequest( mode, userData );

	this->queueRequest( pRequest );
}


/**
 *	This method queues up a write request.
 *
 *	@param pRequest 	The request to queue.
 */
void BackgroundFileWriter::queueRequest( WriteRequest * pRequest )
{
	writeRequests_.push_back( pRequest );

	if (pBackgroundTask_)
	{
		this->startBgTask();
	}
}


/**
 *	This method will start the background task.
 */
void BackgroundFileWriter::startBgTask()
{
	MF_ASSERT( pBackgroundTask_ );

	pBackgroundTask_->setTaskData( this, writeRequests_,
		pBufferedWriteData_ );

	MF_ASSERT( writeRequests_.empty() );

	taskManager_.addBackgroundTask( pBackgroundTask_ );

	pBufferedWriteData_ = new MemoryOStream();
	pBufferedWriteData_->canRewind( false );

	pBackgroundTask_ = NULL;
}


/**
 *	This method handles the background task completing.
 *
 *	@param pBackgroundTask 	The background task.
 *	@param hasError 		Whether an error occurred.
 *	@param errorCode 		If an error occurred, the error code.
 *
 *	@return 				true if no more requests are to be processed, false
 *							otherwise.
 */
bool BackgroundFileWriter::onBgTaskComplete( 
		BackgroundFileWriterTaskPtr pBackgroundTask, bool hasError, 
		int errorCode )
{
	MF_ASSERT( pBackgroundTask && !pBackgroundTask_.exists() );
	if (hasError)
	{
		ERROR_MSG( "BackgroundFileWriter::onBgTaskComplete: "
				"Got failure: %s\n", strerror( errorCode ) );

		hasError_ = hasError;
		errno_ = errorCode;

		if (pListener_)
		{
			pListener_->onBackgroundFileWritingError( *this );
		}

		return true;
	}

	pBackgroundTask_ = pBackgroundTask;

	const bool hasFinishedAllRequests = writeRequests_.empty();

	if (!hasFinishedAllRequests)
	{
		this->startBgTask();
	}

	return hasFinishedAllRequests;
}


/*
 *	Override from IBackgroundFileWriter.
 */
BW::string BackgroundFileWriter::errorString() const
{
	return strerror( errno_ );
}


/**
 *	This method calls the completion callback for the given write request.
 *
 *	@param request 	The completed request.
 */
void BackgroundFileWriter::onWriteCompleted( WriteRequest & request )
{
	if (pListener_)
	{
		pListener_->onBackgroundFileWritingComplete( *this, 
			request.filePosition(), request.userData() );
	}
}


BW_END_NAMESPACE


// background_file_writer.cpp
