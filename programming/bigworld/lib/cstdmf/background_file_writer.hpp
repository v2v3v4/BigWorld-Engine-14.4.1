#ifndef BACKGROUND_FILE_WRITER_HPP
#define BACKGROUND_FILE_WRITER_HPP


#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/memory_stream.hpp"

#include <sys/types.h>

#include <queue>


BW_BEGIN_NAMESPACE

class IBackgroundFileWriter;
typedef SmartPointer< IBackgroundFileWriter > IBackgroundFileWriterPtr;

class BackgroundFileWriter;
typedef SmartPointer< BackgroundFileWriter > BackgroundFileWriterPtr;

class BackgroundFileWriterTask;
typedef SmartPointer< BackgroundFileWriterTask > BackgroundFileWriterTaskPtr;

class WriteRequest;
class WriteDataWriteRequest;


/**
 *	Callback interface for BackgroundFileWriter.
 */
class BackgroundFileWriterListener
{
public:

	/**
	 *	Destructor.
	 */
	virtual ~BackgroundFileWriterListener() {}

	
	/**
	 *	This callback is called when a file writing error was encountered. 
	 *
	 *	The error is stored and can be accessed via the errorString().
	 *
	 *	@param writer 	The background file writer.
	 */
	virtual void onBackgroundFileWritingError( 
		IBackgroundFileWriter & writer ) = 0;


	/**
	 *	This callback is called when a file writing request is completed.
	 *
	 *	@param writer 			The background file writer.
	 *	@param filePosition 	The file position when the request was started.
	 *	@param userData 		The user data passed when the request was queued.
	 */
	virtual void onBackgroundFileWritingComplete( 
		IBackgroundFileWriter & writer, long filePosition, 
		int userData ) = 0;
};


/**
 *	The interface for background file writers.
 */
class IBackgroundFileWriter : public ReferenceCount
{
public:
	/** Destructor. */
	virtual ~IBackgroundFileWriter() {}

	/**
	 *	This method returns the path of the file being written.
	 */
	virtual const BW::string & path() const = 0;


	/**
	 *	This method closes the currently open file.
	 */
	virtual void closeFile() = 0;


	/**
	 *	Add data to the write queue. This should only be called from the main
	 *	thread.
	 *
	 *	@param stream 	The data to write.
	 *	@param userData	The user-defined argument to associate with this
	 *					request.
	 */
	virtual void queueWrite( BinaryIStream & stream, int userData = 0 ) = 0;


	/**
	 *	Add data to the write queue. This should only be called from the main
	 *	thread.
	 *
	 *	@param data 	The data to write.
	 *	@param size 	The length of the data to write.
	 *	@param userData	The user-defined argument to associate with this
	 *					request.
	 */
	void queueWriteBlob( const void * data, int size, int userData = 0 );


	/**
	 *	This method queues a seek operation.
	 *
	 *	@param offset 	The offset argument to fseek().
	 *	@param whence 	The whence argument to fseek().
	 *	@param userData	The user-defined argument to associate with this
	 *					request.
	 */
	virtual void queueSeek( long offset, int whence, int userData = 0 ) = 0;


	/**
	 *	This method queues a chmod() operation.
	 *
	 *	@param mode 	The mode argument to fchmod().
	 *	@param userData	The user-defined argument to associate with this
	 *					request.
	 */
	virtual void queueChmod( int mode, int userData = 0 ) = 0;


	/**
	 *	Set the listener to be used for this writer. This listener will be
	 *	called for each request successfully completed, as well as when an
	 *	error occurs.
	 *
	 *	@param pListener 	The listener object.
	 */
	virtual void setListener( BackgroundFileWriterListener * pListener ) = 0;


	/** 
	 *	This method returns true if an error has occurred. 
	 */
	virtual bool hasError() const = 0;


	/**
	 *	This method returns an error string of the last file error.
	 *
	 *	@return an error string.
	 */
	virtual BW::string errorString() const = 0;

protected:

	/**
	 *	Constructor.
	 */
	IBackgroundFileWriter() : 
		ReferenceCount()
	{}

};


class BackgroundFilePath;
typedef SmartPointer< BackgroundFilePath > BackgroundFilePathPtr;

/**
 *	This class is used to provide paths to BackgroundFileWriter, which may
 *	require some operations to resolve to an absolute path name to be done in
 *	the background thread.
 *
 *	(We can't call BWResource directory because doing so would add a circular
 *	dependency between libstdmf and libresmgr).
 */
class BackgroundFilePath : public SafeReferenceCount
{
public:
	/**
	 *	This method creates a BackgroundFilePath object from a file system path
	 *	(that does not require resolution).
	 *
	 *	@param path 	The file system path.
	 */
	static BackgroundFilePathPtr createFileSystemPath( const BW::string & path )
	{
		return new BackgroundFilePath( path );
	}


	/** Destructor. */
	virtual ~BackgroundFilePath() {}


	/**
	 *	This method is called in the background task to resolve the path.
	 *
	 *	Subclasses should override this method to modify the path_ member.
	 *
	 *	@return 	True on success, false otherwise.
	 */
	virtual bool resolve() 			{ return true; }


	/**
	 *	This method returns the path name. If resolve() has been called, this
	 *	will reflect the post-resolution path.
	 *
	 *	Note that this is not thread-safe with respect to the call to resolve()
	 *	done in the background thread. Accesses to this method should be
	 *	avoided (or protected if necessary) while we a call to resolve() has
	 *	been scheduled.
	 */
	const BW::string & path() const { return path_; }

protected:
	/**
	 *	Constructor.
	 */
	BackgroundFilePath( const BW::string & path ) :
			SafeReferenceCount(),
			path_( path )
	{}

	BW::string path_;
};


/**
 *	This class is used to write to a file in a background task. 
 *
 *	Asynchronous requests are queued to be run in a background worker thread,
 *	and a callback is triggered on the listener object for each request
 *	handled.
 */
class BackgroundFileWriter : public IBackgroundFileWriter
{
	friend class BackgroundFileWriterTask;
public:
	typedef BW::vector< WriteRequest * > WriteRequests;

	BackgroundFileWriter( TaskManager & taskManager );

	void initWithFileSystemPath( const BW::string & path,
		bool shouldOverwrite = false,
		bool shouldTruncate = false );

	void initWithPath( BackgroundFilePathPtr pPath,
		bool shouldOverwrite = false,
		bool shouldTruncate = false );

	virtual ~BackgroundFileWriter();


	// Overrides from IBackgroundFileWriter.
	virtual void closeFile();
	virtual const BW::string & path() const { return unresolvedPath_; }
	virtual void queueWrite( BinaryIStream & stream, int userData = 0 );
	virtual void queueSeek( long offset, int whence, int userData = 0 );
	virtual void queueChmod( int mode, int userData = 0 );

	/*
	 *	Override from IBackgroundFileWriter.
	 */
	virtual void setListener( BackgroundFileWriterListener * pListener )
	{
		pListener_ = pListener;
	}

	/*
	 *	Override from IBackgroundFileWriter.
	 */
	virtual bool hasError() const { return hasError_; }

	virtual BW::string errorString() const;

private:
	void startBgTask();
	bool onBgTaskComplete( BackgroundFileWriterTaskPtr pBackgroundTask, 
		bool hasError, int errorCode );
	void onWriteCompleted( WriteRequest & request );
	void queueRequest( WriteRequest * pRequest );


	TaskManager & 					taskManager_;

	BW::string 						unresolvedPath_;
	BackgroundFilePathPtr 			pPath_;

	MemoryOStream *					pBufferedWriteData_;

	WriteRequests					writeRequests_;

	BackgroundFileWriterListener * 	pListener_;
	bool 							hasError_;
	int								errno_;

	BackgroundFileWriterTaskPtr		pBackgroundTask_;
};


BW_END_NAMESPACE


#endif // BACKGROUND_FILE_WRITER_HPP
