#include "pch.hpp"

#include "replay_controller.hpp"
#include "replay_data.hpp"
#include "replay_header.hpp"
#include "replay_tick_loader.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_util.hpp"

#include <stdio.h>


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

/**
 *	This class implements the IFileProvider to provide real file operations.
 */
class RealFileProvider : public IFileProvider
{
public:
	/**
	 *	Constructor.
	 */
	RealFileProvider() :
		IFileProvider(),
		fp_( NULL )
	{}

	/**
	 *	Destructor.
	 */
	virtual ~RealFileProvider()
	{
		this->FClose();
	}

	/* Override from IFileProvider. */
	virtual bool FOpen( const char * path, const char * mode )
	{
		if (fp_)
		{
			fclose( fp_ );
		}

		fp_ = bw_fopen( path, mode );

		return (fp_ != NULL);
	}


	/* Override from IFileProvider. */
	virtual void FClose()
	{
		if (fp_)
		{
			fclose( fp_ );
			fp_ = NULL;
		}
	}


	/* Override from IFileProvider. */
	virtual size_t FRead( char * buffer, size_t memberSize, size_t numMembers )
	{
		return fread( buffer, memberSize, numMembers, fp_ );
	}


	/* Override from IFileProvider. */
	virtual int FEOF()
	{
		return feof( fp_ );
	}


	/* Override from IFileProvider. */
	virtual int FError()
	{
		return ferror( fp_ );
	}


	/* Override from IFileProvider. */
	virtual void ClearErr()
	{
		return clearerr( fp_ );
	}


private:
	FILE * fp_;
};


// -----------------------------------------------------------------------------
// Section: ReplayTickLoaderTask
// -----------------------------------------------------------------------------

/**
 *	This class is for loading ticks in the background from the temporary file
 *	created for storing tick data, or an already downloaded replay file.
 */
class ReplayTickLoaderTask : public BackgroundTask,
		public IReplayDataFileReaderListener
{
public:
	typedef ReplayTickLoader::TickIndex TickIndex;

	ReplayTickLoaderTask( ReplayTickLoaderPtr pTickLoader,
		IFileProviderPtr pFileProvider,
		ReplayTickLoader::RequestType type,
		const BW::string & verifyingKey, size_t verifyFromPosition,
		TickIndex start, TickIndex end );

	/** Destructor. */
	virtual ~ReplayTickLoaderTask() { }

	// Overrides from BackgroundTask
	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );

	// Overrides from IReplayDataFileReaderListener
	virtual bool onReplayDataFileReaderHeader(
		ReplayDataFileReader & reader,
		const ReplayHeader & header,
		BW::string & errorString );

	virtual bool onReplayDataFileReaderMetaData( ReplayDataFileReader & reader,
		const ReplayMetaData & metaData,
		BW::string & errorString );

	virtual bool onReplayDataFileReaderTickData( ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString );

	virtual void onReplayDataFileReaderError( ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription );

	/* Override from IReplayDataFileReaderListener. */
	virtual void onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader )
	{}


private:
	void fetchTicks();

	ReplayTickLoaderPtr 	pTickLoader_;
	IFileProviderPtr		pFileProvider_;

	ReplayTickLoader::RequestType
							requestType_;

	ReplayDataFileReader 	reader_;
	bool 					hasReadHeader_;
	bool					hasCompleted_;
	ReplayHeader 			header_;
	GameTime				startTime_;
	TickIndex				start_;
	TickIndex				end_;

	ReplayTickData * 		pStart_;
	ReplayTickData * 		pEnd_;

	uint 					numTicksRetrieved_;

	ReplayTickLoader::ErrorType
							errorType_;
	BW::string 				errorString_;
};

#ifdef _MSC_VER
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // _MSC_VER

/**
 *	Constructor
 *
 *	@param pTickLoader 	The tick loader.
 *	@param type 		The type of the request.
 *	@param verifyingKey The key to use for verifying signatures, in PEM format.
 *	@param verifyFromPosition
 						The file position to start verifying signatures from.
 *	@param headerSize 	The size of the header.
 *	@param start 		The start tick of the request (inclusive).
 *	@param end			The end tick of the request (exclusive).
 */
ReplayTickLoaderTask::ReplayTickLoaderTask(
			ReplayTickLoaderPtr pTickLoader,
			IFileProviderPtr pFileProvider,
			ReplayTickLoader::RequestType type,
			const BW::string & verifyingKey,
			size_t verifyFromPosition,
			TickIndex start,
			TickIndex end ) :
	BackgroundTask( "ReplayTickLoaderTask" ),
	pTickLoader_( pTickLoader ),
	pFileProvider_( pFileProvider ),
	requestType_( type ),
	reader_( *this, verifyingKey, /* shouldDecompress */ false,
		verifyFromPosition ),
	hasReadHeader_( false ),
	hasCompleted_( false ),
	header_( NULL ),
	startTime_( 0 ),
	start_( start ),
	end_( end ),
	pStart_( NULL ),
	pEnd_( NULL ),
	numTicksRetrieved_( 0U ),
	errorType_( ReplayTickLoader::ERROR_NONE ),
	errorString_()
{
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

/**
 *	This method fetches tick data from the local file into a standalone linked
 *	list.
 */
void ReplayTickLoaderTask::fetchTicks()
{
	BackgroundTaskPtr pThis( this );

	pStart_ = NULL;
	pEnd_ = NULL;
	numTicksRetrieved_ = 0U;
	errorType_ = ReplayTickLoader::ERROR_NONE;
	errorString_.clear();

	if (!pFileProvider_->FOpen( pTickLoader_->replayFilePath().c_str(), "rb" ))
	{
		errorType_ = ReplayTickLoader::ERROR_FILE_MISSING;
		errorString_ = "Failed to open replay file";
		return;
	}

	static const size_t FILE_READ_SIZE = 4096U;
	char readBuffer[FILE_READ_SIZE];

	// Read the header and any ticks requested.
	while (!hasCompleted_)
	{
		size_t numRead = pFileProvider_->FRead( readBuffer, 1, FILE_READ_SIZE );

		if (numRead < FILE_READ_SIZE)
		{
			// Got short-count, either end-of-file or an read error.

			if (pFileProvider_->FError())
			{
				errorType_ = ReplayTickLoader::ERROR_FILE_CORRUPTED;
				errorString_ = strerror( errno );
			}
			else
			{
				MF_ASSERT( pFileProvider_->FEOF() );
			}

			// No more data or error.
			hasCompleted_ = true;
		}

		if (!reader_.addData( readBuffer, numRead ))
		{
			errorType_ = ReplayTickLoader::ERROR_FILE_CORRUPTED;
			errorString_ = reader_.lastError();
			hasCompleted_ = true;
		}
	}

	pFileProvider_->FClose();
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool ReplayTickLoaderTask::onReplayDataFileReaderHeader(
		ReplayDataFileReader & reader,
		const ReplayHeader & header,
		BW::string & errorString )
{
	hasReadHeader_ = true;
	header_ = header;

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */ 
bool ReplayTickLoaderTask::onReplayDataFileReaderMetaData(
		ReplayDataFileReader & reader,
		const ReplayMetaData & metaData,
		BW::string & errorString )
{
	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool ReplayTickLoaderTask::onReplayDataFileReaderTickData(
		ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & tickData,
		BW::string & errorString )
{
	BW_GUARD;

	MF_ASSERT( isCompressed );
	int tickIndex = time - reader.firstGameTime();
	
	if (startTime_ == 0)
	{
		startTime_ = time;
	}

	if (tickIndex < start_)
	{
		tickData.finish();
		return true;
	}
	else if (tickIndex >= end_)
	{
		// We're done, signal the reader to stop reading.
		tickData.finish();
		hasCompleted_ = true;
		return true;
	}

	ReplayTickData * pItem = new ReplayTickData( tickData );

#if defined( DEBUG_TICK_DATA )
	pItem->tick( tickIndex );
#endif // defined( DEBUG_TICK_DATA )

	if (pStart_ == NULL)
	{
		pStart_ = pItem;
	}
	else
	{
		pEnd_->pNext( pItem );
	}
	pEnd_ = pItem;
	++numTicksRetrieved_;

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
void ReplayTickLoaderTask::onReplayDataFileReaderError(
		ReplayDataFileReader & reader,
		ErrorType errorType,
		const BW::string & errorString )
{
	errorString_ = errorString;

	// Map the reader error to our error space.

	switch (errorType)
	{
		case ERROR_KEY_ERROR:
			errorType_ = ReplayTickLoader::ERROR_KEY_ERROR;
			break;

		case ERROR_SIGNATURE_MISMATCH:
			errorType_ = ReplayTickLoader::ERROR_FILE_SIGNATURE_MISMATCH;
			break;

		case ERROR_FILE_CORRUPTED:
			errorType_ = ReplayTickLoader::ERROR_FILE_CORRUPTED;
			break;

		case ERROR_NONE:
		default:
			errorType_ = ReplayTickLoader::ERROR_NONE;
			break;
	}
}


/*
 *	Override from BackgroundTask.
 */
void ReplayTickLoaderTask::doBackgroundTask( TaskManager & mgr )
{
	BW_GUARD;

	MF_ASSERT( requestType_ != ReplayTickLoader::NO_REQUEST );

	this->fetchTicks();

	mgr.addMainThreadTask( this );
}


/*
 *	Override from BackgroundTask.
 */
void ReplayTickLoaderTask::doMainThreadTask( TaskManager & mgr )
{
	if ((errorType_ != ReplayTickLoader::ERROR_NONE) || !errorString_.empty())
	{
		pTickLoader_->onReplayTickLoaderTaskError( errorType_, errorString_ );
		return;
	}

	pTickLoader_->onReplayTickLoaderTaskFinished( requestType_, startTime_,
		start_, pStart_, end_, pEnd_, numTicksRetrieved_, reader_ );
}


} // end anonymous namespace


// -----------------------------------------------------------------------------
// Section: ReplayTickLoader
// -----------------------------------------------------------------------------
/**
 *	Constructor
 *
 *	@param taskManager 			The background task manager.
 *	@param listener 			The listener object to callback on.
 *	@param replayFilePath 		The path to the replay file path.
 *	@param verifyingKey 		The key to use for verifying signatures.
 *	@param pFileProvider 		The file provider to use. If NULL, the default
 *								real file provider will be used. This is used
 *								for injecting mock object for file operations.
 */
ReplayTickLoader::ReplayTickLoader (
			TaskManager & taskManager,
			IReplayTickLoaderListener * pListener,
			const BW::string & replayFilePath,
			const BW::string & verifyingKey,
			IFileProviderPtr pFileProvider /* = NULL */ ) :
		SafeReferenceCount(),
		taskManager_( taskManager ),
		pListener_( pListener ),
		pFileProvider_( pFileProvider.get() ? 
			pFileProvider : new RealFileProvider() ),
		replayFilePath_( replayFilePath ),
		verifyingKey_( verifyingKey ),
		verifiedToPosition_( 0 ),
		requestType_( NO_REQUEST ),
		requestStart_( 0 ),
		requestEnd_( 0 ),
		isRegisteredForBackground_( false )
{
}


/**
 * This method will be called when the IReplayTickLoaderListener is no 
 * longer valid.
 */
void ReplayTickLoader::onListenerDestroyed(
		IReplayTickLoaderListener * pListener )
{
	MF_ASSERT( pListener == pListener_ );
	pListener_ = NULL;
}


/**
 *	This method adds a request for the header to the queue.
 */
void ReplayTickLoader::addHeaderRequest()
{
	this->addRequest( READ_HEADER, 0, 1 );
}


/**
 *	This method adds a request to the queue, this will overwrite any previous
 *	requests which have not completed.
 *
 *	@param type		The type of request, either ReplayTickRequest::PREPEND or
 *					ReplayTickRequest::APPEND.
 *	@param start	The start game time to retrieve from (inclusive).
 *	@param end		The end game time to retrieve to (exclusive).
 */
void ReplayTickLoader::addRequest( RequestType type, TickIndex start,
		TickIndex end )
{
	requestType_ = type;
	requestStart_ = start;
	requestEnd_ = end;

	this->scheduleNewTask();
}


/**
 *	This method is called by the task when the file reading has failed.
 *
 *	@param errorType 	The error type.
 *	@param errorString 	The error description.
 */
void ReplayTickLoader::onReplayTickLoaderTaskError( ErrorType errorType,
		const BW::string & errorString )
{
	isRegisteredForBackground_ = false;
	requestType_ = NO_REQUEST;
	requestStart_ = requestEnd_ = 0;
	if (pListener_)
	{
		pListener_->onReplayTickLoaderError( errorType, errorString );
	}
}


/*
 *	This method handles a completed ReplayTickRequest. It is given a linked 
 *	list of tick data read from the file; the elements are now owned by
 *	this object and must be managed accordingly.
 *
 *	@param type 	The request type.
 *	@param startTime
					The game time of the start tick.
 *	@param start	The start tick index.
 *	@param pStart	The first node of the retrieved linked list.
 *	@param end		The end tick index retrieved to (inclusive).
 *	@param pEnd		The last node of the linked list.
 *	@param numTicksRetrieved
 *					The number of ticks retrieved (the size of the list).
 *	@param reader 	The file reader instance used.
 */
void ReplayTickLoader::onReplayTickLoaderTaskFinished( RequestType type,
		GameTime startTime,
		TickIndex start, ReplayTickData * pStart,
		TickIndex end, ReplayTickData * pEnd,
		uint numTicksRetrieved,
		const ReplayDataFileReader & reader )
{
	isRegisteredForBackground_ = false;

	verifiedToPosition_ = reader.verifiedToPosition();

	if ((type != requestType_) ||
			(start != requestStart_) ||
			(end != requestEnd_))
	{
		// Request has changed, re-register background task
		while (pStart != NULL)
		{
			ReplayTickData * pNext = pStart;
			pStart = pNext->pNext();
			delete pNext;
		}

		this->scheduleNewTask();
		return;
	}

	switch (requestType_)
	{
	case ReplayTickLoader::PREPEND:
		if (pListener_)
		{
			pListener_->onReplayTickLoaderPrependTicks( pStart, pEnd,
				numTicksRetrieved );
		}
		break;

	case ReplayTickLoader::APPEND:
		if (pListener_)
		{
			pListener_->onReplayTickLoaderAppendTicks( pStart, pEnd,
				numTicksRetrieved );
		}
		break;

	case ReplayTickLoader::READ_HEADER:
		if (reader.hasReadHeader() && (pStart != NULL))
		{
			// We read the first tick to find the first game time.
			// We can discard it now.
			MF_ASSERT( pStart->pNext() == NULL );
			delete pStart;

			if (pListener_)
			{
				pListener_->onReplayTickLoaderReadHeader( reader.header(),
					startTime );
			}
		}
		break;

	case ReplayTickLoader::NO_REQUEST:
		break;
	}

	requestType_ = ReplayTickLoader::NO_REQUEST;
	requestStart_ = 0;
	requestEnd_ = 0;
}


/**
 *	This method schedules a new background task with the current request
 *	parameters.
 */
void ReplayTickLoader::scheduleNewTask()
{
	if (isRegisteredForBackground_)
	{
		return;
	}

	taskManager_.addBackgroundTask(
		new ReplayTickLoaderTask( this, pFileProvider_, requestType_,
			verifyingKey_, verifiedToPosition_,
			requestStart_, requestEnd_ ) );

	isRegisteredForBackground_ = true;
}


BW_END_NAMESPACE

// replay_tick_loader.cpp
