#include "pch.hpp"

#include "client_server_protocol_version.hpp"
#include "replay_checksum_scheme.hpp"
#include "replay_controller.hpp"
#include "replay_data.hpp"
#include "replay_header.hpp"
#include "replay_tick_loader.hpp"
#include "server_connection.hpp"

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/guard.hpp"

#include "network/basictypes.hpp"
#include "network/compression_stream.hpp"
#include "network/misc.hpp"

#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif


BW_BEGIN_NAMESPACE



namespace // (anonymous)
{
/*
 *	How tick loading works:
 *
 *	When ticks are receieved from the server they are first written to the disk,
 *	then if the number of ticks currently stored in memory is less than
 *	g_maxTicksInMemory once it reaches this level, do not add any more to the
 *	memory until after the number of loaded ticks left in the memory is below
 *	g_minTicksInMemory.
 *
 *	Calculations based on Highlands with 300+ entities
 *
 *	g_minTicksInMemory = 300 = 30 sec @ 10hz
 *					with 300 entities = ~2.5MB
 *
 *	g_maxTicksInMemory = 3000 = 5 min @ 10hz
 *					with 300 entities = ~25MB
 */
GameTime g_minTicksInMemory = 300;
GameTime g_maxTicksInMemory = 3000;

GameTime g_idealTicksLeft = 20;

#if ENABLE_WATCHERS
class WatcherIniter
{
public:
	WatcherIniter()
	{
		MF_WATCH( "Replay/Minimum ticks in memory",
				g_minTicksInMemory );
		MF_WATCH( "Replay/Maximum ticks in memory",
				g_maxTicksInMemory );
	}
};

WatcherIniter g_watcherIniter;

#endif // ENABLE_WATCHERS

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: EntityVolatileData
// -----------------------------------------------------------------------------


/**
 *	This method caches the volatile data read from the replay data, and can be
 *	used to re-apply the volatile data for the benefit of filters expecting
 *	regular volatile updates.
 */
class EntityVolatileData
{
public:

	/**
	 *	Default constructor.
	 */
	EntityVolatileData() :
		entityID_( 0 ),
		vehicleID_( 0 ),
		position_(),
		direction_(),
		timeLastUpdated_( 0 )
	{}


	/**
	 *	Constructor.
	 *
	 *	@param entityID 	The entity ID.
	 *	@param vehicleID 	The vehicle ID.
	 *	@param position 	The position.
	 *	@param direction 	The direction.
	 *	@param now			The game time the volatile data was read from.
	 */
	EntityVolatileData( EntityID entityID, EntityID vehicleID,
				const Position3D & position, const Direction3D & direction,
				GameTime now ) :
			entityID_( entityID ),
			vehicleID_( vehicleID ),
			position_( position ),
			direction_( direction ),
			timeLastUpdated_( now )
	{}


	/**
	 *	This method applies the volatile update to the given handler, if the
	 *	time since the last application of this update is more than the given
	 *	threshold.
	 *
	 *	@param handler 			The replay controller handler.
	 *	@param spaceID 			The space ID.
	 *	@param now 				The current game time.
	 *	@param injectionPeriod	The maximum period for injected volatile
	 *							updates.
	 */
	void injectIfNecessary( ReplayControllerHandler & handler, SpaceID spaceID,
			GameTime now, GameTime injectionPeriod )
	{
		if (timeLastUpdated_ > now)
		{
			return;
		}

		if (( now - timeLastUpdated_) > injectionPeriod)
		{
			this->inject( handler, spaceID );
			timeLastUpdated_ = now;
		}
	}

	void inject( ReplayControllerHandler & handler, SpaceID spaceID )
	{
		handler.onEntityMoveWithError( entityID_, spaceID,
				vehicleID_, position_, Vector3::zero(),
				direction_.yaw, direction_.pitch, direction_.roll, true );
	}

private:
	EntityID		entityID_;
	EntityID		vehicleID_;
	Position3D		position_;
	Direction3D		direction_;
	GameTime		timeLastUpdated_;
};


/**
 *	This class stores a cache of volatile data read from the replay data, and
 *	re-applies them periodically to the replay controller handler if they
 *	haven't changed.
 */
class EntityVolatileCache
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param injectionPeriod	The maximum period for injected volatile
	 *							updates.
	 */
	EntityVolatileCache( GameTime injectionPeriod ) :
			collection_(),
			injectionPeriod_( injectionPeriod )
	{}


	/**
	 *	This method injects volatile updates if necessary.
	 *
	 *	@param handler 	The replay controller handler.
	 *	@param spaceID 	The space ID.
	 *	@param now 		The current game time.
	 */
	void processForTick( ReplayControllerHandler & handler, SpaceID spaceID,
			GameTime now, bool forceProcess = false )
	{
		Collection::iterator iData = collection_.begin();
		while (iData != collection_.end())
		{
			if (forceProcess)
			{
				iData->second.inject( handler, spaceID );
			}
			else
			{
				iData->second.injectIfNecessary( handler, spaceID, now,
					injectionPeriod_ );
			}
			++iData;
		}
	}


	/**
	 *	This method sets the volatile data for a particular entity.
	 *
	 *	@param entityID 	The entity ID.
	 *	@param vehicleID 	The entity's vehicle ID, or 0 if not on vehicle.
	 *	@param position 	The entity's position.
	 *	@param direction 	The entity's direction.
	 *	@param now 			The game time of the update.
	 */
	void setVolatile( EntityID entityID, EntityID vehicleID,
			const Position3D & position, const Direction3D & direction,
			GameTime now )
	{
		collection_[entityID] = EntityVolatileData( entityID, vehicleID,
			position, direction, now );
	}


	/**
	 *	This method removes an entity's cached volatile data.
	 *
	 *	@param entityID 	The ID of the entity which will have its cached
	 *						volatile data removed.
	 */
	void clearVolatile( EntityID entityID )
	{
		collection_.erase( entityID );
	}


private:
	typedef BW::map< EntityID, EntityVolatileData > Collection;

	Collection	collection_;
	GameTime	injectionPeriod_;
};


// -----------------------------------------------------------------------------
// Section: FileWriterWaiter
// -----------------------------------------------------------------------------
/**
 *	This class is used to keep the ReplayController active while the
 *	background file writer is processing, and close it before destructing the
 *	replay controller.
 */
class FileWritingWaiter : private BackgroundFileWriterListener
{
public:
	static void waitFor( BackgroundFileWriter & writer,
			ReplayControllerPtr pReplayController )
	{
		writer.setListener( new FileWritingWaiter( &writer,
			pReplayController ) );
		writer.queueSeek( 0, SEEK_END, -1 );
	}

private:


	/* Override from BackgroundFileWriterListener */
	virtual void onBackgroundFileWritingError(
			IBackgroundFileWriter & writer )
	{
		delete this;
	}


	/* Override from BackgroundFileWriterListener */
	virtual void onBackgroundFileWritingComplete(
			IBackgroundFileWriter & writer, long filePosition,
			int userData )
	{
		if (userData == -1)
		{
			delete this;
		}
	}


	/**
	 *	Constructor.
	 *
	 *	@param pWriter 				The file writer to wait for.
	 *	@param pReplayController 	The replay controller to keep alive.
	 */
	FileWritingWaiter( BackgroundFileWriterPtr pWriter,
				ReplayControllerPtr pReplayController ) :
			pWriter_( pWriter ),
			pReplayController_( pReplayController )
	{}


	BackgroundFileWriterPtr 	pWriter_;
	ReplayControllerPtr 		pReplayController_;
};


// -----------------------------------------------------------------------------
// Section: ReplayController
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param taskManager 			The task manager to use for file operations.
 *	@param spaceID 				The space ID.
 *	@param entityDefConstants 	The def constants.
 *	@param verifyingKey 		The EC public key used to verify signatures in
 *								PEM string form.
 *	@param handler 				The handler instance.
 *	@param replayFilePath 		The buffer file path. This is used to buffer
 *								the replay data on disk.
 *	@param replayFileType		The type of file specified for
 *								replayFileWritePath (valid values in
 *								ReplayFileType enum).
 *	@param volatileInjectionPeriod
 *								The maximum period for injected volatile
 *								updates.
 */
ReplayController::ReplayController( TaskManager & taskManager,
		SpaceID spaceID,
		const EntityDefConstants & entityDefConstants,
		const BW::string & verifyingKey,
		ReplayControllerHandler & handler,
		const BW::string & replayFilePath,
		ReplayFileType replayFileType,
		GameTime volatileInjectionPeriod ) :
	ReferenceCount(),
	IReplayDataFileReaderListener(),
	IReplayTickLoaderListener(),
	entityDefConstants_( entityDefConstants ),
	handler_( handler ),
	errorMessage_(),
	pBufferFileWriter_( NULL ),
	taskManager_( taskManager ),
	pTickCounter_( NULL ),
	pTickLoader_( NULL ),
	haveReadHeader_( false ),
	pNextRead_( NULL ),
	pLastRead_( NULL ),
	spaceID_( spaceID ),
	gameUpdateFrequency_( 0 ),
	timeSinceLastTick_( 0.0 ),
	firstGameTime_( 0 ),
	currentTick_( 0 ),
	seekToTick_( -1 ),
	numTicksLoaded_( 0 ),
	numTicksReported_( 0 ),
	speedScale_( 1.0f ),
	isPlayingStream_( false ),
	isStreamLoaded_( false ),
	isReceiving_( replayFileType != FILE_ALREADY_DOWNLOADED ),
	shouldDelayVolatileInjection_( false ),
	isLive_( false ),
	isDestroyed_( false ),
	replayFileType_( replayFileType ),
	replayFilePath_( replayFilePath ),
	pVolatileCache_( new EntityVolatileCache( volatileInjectionPeriod ) )
{
	pTickLoader_ = new ReplayTickLoader( taskManager,
		this, replayFilePath_, verifyingKey );

	if (replayFileType != FILE_ALREADY_DOWNLOADED)
	{
		// The tick counter minimally parses the data from a downloading replay
		// file to determine how many ticks it holds. It does no verification,
		// but needs the verifying key to know how big signatures will be.
		// Verification will be carried out by the tick loader.
		pTickCounter_ = new ReplayDataFileReader( *this,
			verifyingKey,
			/* shouldDecompress */ false,
			// Don't verify anything, just count ticks.
			/* verifyFromPosition */ size_t(-1) );
	}
}


/**
 *	Destructor.
 */
ReplayController::~ReplayController()
{
	if (!isDestroyed_)
	{
		WARNING_MSG( "ReplayController::~ReplayController: "
			"isDestroyed_ = false\n" );
	}

	while (pNextRead_ != NULL)
	{
		ReplayTickData * pNext = pNextRead_;
		pNextRead_ = pNext->pNext();
		delete pNext;
	}

	if (replayFileType_ == FILE_REMOVE)
	{
		bw_unlink( replayFilePath_.c_str() );
	}

	if (pTickCounter_)
	{
		bw_safe_delete( pTickCounter_ );
	}
}


/**
 *	This method destroys the replay controller data.
 */
void ReplayController::destroy()
{
	if (isDestroyed_)
	{
		return;
	}

	isDestroyed_ = true;
	pTickLoader_->onListenerDestroyed( this );
	pTickLoader_ = NULL;

	if (pBufferFileWriter_)
	{
		if ((replayFileType_ != FILE_ALREADY_DOWNLOADED) &&
				haveReadHeader_ &&
				(pTickCounter_->header().numTicks() != pTickCounter_->numTicksRead()))
		{
			// We have only downloaded the partial replay, or it started off
			// as a live replay. We need to write out the number of complete ticks that
			// we have been able to read so far.
			pBufferFileWriter_->queueSeek( 
				static_cast<long>(pTickCounter_->numTicksFieldOffset()),
				SEEK_SET );
			GameTime numTicks = pTickCounter_->numTicksRead();
			pBufferFileWriter_->queueWriteBlob( &numTicks, sizeof( numTicks ) );
		}

		FileWritingWaiter::waitFor( *pBufferFileWriter_, this );
		pBufferFileWriter_ = NULL;
	}

	handler_.onReplayControllerDestroyed( *this );
}


/**
 *	 This method checks if the replay controller is currently seeking
 *
 *	@return true If currently seeking to a tick, false otherwise
 */
bool ReplayController::isSeekingToTick() const
{
	return (seekToTick_ != -1);
}


/**
 *	This method pauses playback
 */
void ReplayController::pausePlayback()
{
	isPlayingStream_ = false;
	seekToTick_ = -1;
	timeSinceLastTick_ = 0.0;
}


/**
 *	This method resumes playback
 */
void ReplayController::resumePlayback()
{
	isPlayingStream_ = true;
	seekToTick_ = -1;
	timeSinceLastTick_ = 0.0;
}


/**
 *	This method advances or rewinds the current tick time.
 *
 *	When advancing the tick time, events are played from each tick to the
 *	specified tick time.
 *
 *	When setting a tick time before the current tick time, playback is reset
 *	to the start of the replay data, and events are played forward to the
 *	specified tick time.
 *
 *	Rewinding depends on the path to a recording buffer file being valid, given
 *	at construction.
 *
 *	@param newTick 	The tick time to advance/rewind the current tick to.
 *
 *	@return 		true on success, false on error.
 */
bool ReplayController::setCurrentTick( GameTime newTick )
{
	if (isDestroyed_)
	{
		return false;
	}

	if (newTick < currentTick_)
	{
		pTickLoader_->addRequest( ReplayTickLoader::PREPEND, 0,
				std::min( currentTick_, g_maxTicksInMemory ) );
	}

	seekToTick_ = newTick;
	shouldDelayVolatileInjection_ = true;

	return true;
}

/*
 *	Override from BackgroundFileWriterListener.
 */
void ReplayController::onBackgroundFileWritingError(
		IBackgroundFileWriter & writer )
{
	ERROR_MSG( "ReplayController::onBackgroundFileWritingError: %s\n",
		writer.errorString().c_str() );
	errorMessage_ = "Failed to write to \"" + writer.path() + 
			"\": " + writer.errorString();

	this->handleError( errorMessage_ );
}


/*
 *	Override from BackgroundFileWriterListener.
 */
void ReplayController::onBackgroundFileWritingComplete(
		IBackgroundFileWriter & writer, long filePosition,
		int userData )
{
}

/**
 *	This method prepends the given tick range to the current list of loaded
 *	ticks.
 *
 *	@param pStart 			The start of the tick data.
 *	@param pEnd 			The end of the tick data.
 *	@param totalTicks 		The total number of ticks between pStart and pEnd.
 *
 */
void ReplayController::onReplayTickLoaderPrependTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, unsigned int totalTicks )
{
	if (totalTicks == 0)
	{
		seekToTick_ = -1;

		WARNING_MSG( "ReplayController::onReplayTickLoaderPrependTicks: "
			"Failed to prepend ticks, no ticks returned.\n" );
		return;
	}

	if (totalTicks != currentTick_)
	{
		while (pNextRead_ != NULL)
		{
			ReplayTickData * pNext = pNextRead_;
			pNextRead_ = pNextRead_->pNext();
			delete pNext;
		}
		numTicksLoaded_ = 0;
		pLastRead_ = NULL;
	}

	MF_ASSERT( pStart );
	MF_ASSERT( pEnd );

	numTicksLoaded_ += totalTicks;

#if defined( DEBUG_TICK_DATA )
	if (pNextRead_ != NULL)
	{
		MF_ASSERT( pNextRead_->tick() - 1 == pEnd->tick() );
	}
#endif // defined( DEBUG_TICK_DATA )

	pEnd->pNext( pNextRead_ );
	pNextRead_ = pStart;

	if (pLastRead_ == NULL)
	{
		pLastRead_ = pEnd;
	}

	this->resetEntities();

	currentTick_ = 0;
}


/**
 *	This method appends the given tick range to the current list of loaded
 *	ticks.
 *
 *	@param pStart 			The start of the tick data.
 *	@param pEnd 			The end of the tick data.
 *	@param totalTicks 		The total number of ticks between pStart and pEnd.
 *
 */
void ReplayController::onReplayTickLoaderAppendTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, unsigned int totalTicks )
{
	if (totalTicks == 0)
	{
		seekToTick_ = -1;
		DEBUG_MSG( "ReplayController::onReplayTickLoaderAppendTicks: "
			"Tick data writer is slower than tick data "
			"reader due to slow network or slow disk, no ticks returned.\n" );
		return;
	}

	MF_ASSERT( pStart );
	MF_ASSERT( pEnd );

#if defined( DEBUG_TICK_DATA )
	if (pLastRead_ != NULL)
	{
		MF_ASSERT( pLastRead_->tick() + 1 == pStart->tick() );
	}
#endif // defined( DEBUG_TICK_DATA )

	numTicksLoaded_ += totalTicks;

	if (pLastRead_ == NULL)
	{
		MF_ASSERT( pNextRead_ == NULL );
		pNextRead_ = pStart;
		pLastRead_ = pEnd;
	}
	else
	{
		pLastRead_->pNext( pStart );
		pLastRead_ = pEnd;
	}
}


/**
 *	This method ticks the playback.
 *
 *	@param timeDelta 	The amount of real time elapsed since the last call to
 *						tick().
 */
void ReplayController::tick( double timeDelta )
{
	if (isDestroyed_)
	{
		return;
	}

	if (shouldDelayVolatileInjection_)
	{
		shouldDelayVolatileInjection_ = false;
		pVolatileCache_->processForTick( handler_, spaceID_,
				currentTick_, /* forceProcess: */ true );
	}

	if (!haveReadHeader_ &&
			(pTickLoader_->nextQueued() == ReplayTickLoader::NO_REQUEST))
	{
		pTickLoader_->addHeaderRequest();
		return;
	}

	if (!isPlayingStream_ && !this->isSeekingToTick())
	{
		return;
	}

	if (pTickLoader_->nextQueued() == ReplayTickLoader::PREPEND)
	{
		return;
	}

	int ticksLeft = 0;

	if (this->isSeekingToTick())
	{
		ticksLeft = std::min( numTicksLoaded_, seekToTick_ - currentTick_ );
		timeSinceLastTick_ = 0.0;
	}
	else if (currentTick_ == 0)
	{
		// Make sure we only load the first tick at the start.
		ticksLeft = 1;
		timeSinceLastTick_ = 0.0;
	}
	else
	{
		timeSinceLastTick_ += timeDelta;
		ticksLeft = int( timeSinceLastTick_ * gameUpdateFrequency_ );

		if (ticksLeft <= 0)
		{
			return;
		}

		timeSinceLastTick_ -= double( ticksLeft ) / gameUpdateFrequency_;
	}

	GameTime highestLoadedTick = numTicksLoaded_ + currentTick_;
	if ((numTicksLoaded_ < g_minTicksInMemory) &&
		(this->numTicksTotal() > highestLoadedTick) &&
		(pTickLoader_->nextQueued() == ReplayTickLoader::NO_REQUEST))
	{
		GameTime ticksToLoad = (g_maxTicksInMemory - numTicksLoaded_ +
			ticksLeft);

		pTickLoader_->addRequest( ReplayTickLoader::APPEND, highestLoadedTick,
			highestLoadedTick + ticksToLoad );
	}

	while ((this->isSeekingToTick() || isPlayingStream_) &&
			(ticksLeft > 0) && (pNextRead_ != NULL))
	{
		ReplayTickData * pNextTickData = pNextRead_;
		pNextRead_ = pNextTickData->pNext();

		if (pLastRead_ == pNextTickData)
		{
			pLastRead_ = NULL;
		}

		if ( this->isSeekingToTick() )
		{
			handler_.onReplayControllerIncreaseTotalTime(
				*this,
				1.0 / gameUpdateFrequency_ );
		}

		pNextTickData->data().rewind();
		if (!this->processTick( pNextTickData->data() ))
		{
			handler_.onReplayControllerGotCorruptedData();
			return;
		}

		handler_.onReplayControllerPostTick( *this, currentTick_ );
		++currentTick_;
		--ticksLeft;

		delete pNextTickData;
		--numTicksLoaded_;
	}

	if (currentTick_ == (GameTime)seekToTick_)
	{
		seekToTick_ = -1;
	}

	if (isReceiving_ &&
			(speedScale_ > 0.f) && (speedScale_ < 1.f) &&
			((this->numTicksTotal() - currentTick_) <= g_idealTicksLeft))
	{
		speedScale_ = 1.f;
	}

	if ((pNextRead_ == NULL) && isPlayingStream_ && !isReceiving_ &&
		(this->numTicksTotal() <= currentTick_))
	{
		this->handleFinal();
	}
}


/**
 *	This method loads the header and calculates the number of ticks from within
 *	the replay file. When replayFileType_ is FILE_ALREADY_DOWNLOADED
 */
void ReplayController::onReplayTickLoaderReadHeader(
		const ReplayHeader & header, GameTime firstGameTime )
{
	INFO_MSG( "ReplayController::onReplayTickLoaderReadHeader: "
			"Header version = %s, digest = %s, updateHertz = %uHz, "
			"numTicks = %d, firstGameTime = %u\n",
		header.version().c_str(),
		header.digest().quote().c_str(),
		header.gameUpdateFrequency(),
		header.numTicks(),
		firstGameTime );

	// Ensure that we do not cease to exist due to handler_ callback.
	ReplayControllerPtr pThisHolder( this );

	if (!ClientServerProtocolVersion::currentVersion().supports(
			header.version() ))
	{
		handler_.onReplayControllerGotBadVersion( *this, header.version() );
		errorMessage_ = "Client-server protocol version mismatch";
		this->destroy();
		return;
	}

	if (!handler_.onReplayControllerReadHeader( *this, header ))
	{
		errorMessage_ = "Header rejected by application";
		this->destroy();
		return;
	}

	isPlayingStream_ = true;
	isStreamLoaded_ = true;
	haveReadHeader_ = true;

	gameUpdateFrequency_ = header.gameUpdateFrequency();
	firstGameTime_ = firstGameTime;
	isLive_ = (header.numTicks() == 0);
	numTicksReported_ = header.numTicks();

	if ((header.numTicks() == 0) &&
			(replayFileType_ == FILE_ALREADY_DOWNLOADED))
	{
		WARNING_MSG( "ReplayController::onReplayTickLoaderReadHeader: "
			"Replay file does not have a reported ticks within its header\n" );
	}
}


/**
 *	This method handles errors from the replay tick loader.
 *
 *	@param errorType 	The error type.
 *	@param errorString 	A description of the error that occurred.
 */
void ReplayController::onReplayTickLoaderError(
		ReplayTickLoader::ErrorType errorType,
		const BW::string & errorString )
{
	if ((errorType == ReplayTickLoader::ERROR_FILE_MISSING) &&
			(replayFileType_ != FILE_ALREADY_DOWNLOADED) &&
			!haveReadHeader_)
	{
		// This can happen if we are still waiting on the initial data.
		return;
	}

	ERROR_MSG( "ReplayController::onReplayTickLoaderError: %s\n",
		errorString.c_str() );
	errorMessage_ = "Invalid/corrupt replay data: " + errorString;

	this->handleError( errorMessage_ );
}


/**
 *	This method receives replay data to be played back. This data blob is
 *	typically the result of a URL request for a replay data file, but is not
 *	limited to such, for example, it can also be data read from a file.
 *
 *	@param data 	The replay data file data to add.
 *
 *	@return			True on success, false if there the received data is invalid.
 */
bool ReplayController::addReplayData( BinaryIStream & data )
{
	BW_GUARD;

	MF_ASSERT( replayFileType_ != FILE_ALREADY_DOWNLOADED );

	if (!pTickCounter_->addData( data.retrieve( 0 ), data.remainingLength() ))
	{
		ERROR_MSG( "ReplayController::addReplayData: Could not parse replay data: %s\n",
			pTickCounter_->lastError().c_str() );

		errorMessage_ = BW::string( "Invalid or corrupt replay data: " ) +
			pTickCounter_->lastError();
		this->destroy();
		return false;
	}

	if (!isDestroyed_ && (pBufferFileWriter_ == NULL))
	{
		// Create on-demand so that we only create files if there is data to be
		// written.

		pBufferFileWriter_ = new BackgroundFileWriter( taskManager_ );

		// Add a listener to capture all possible errors. It will be replaced
		// by the FileWritingWaiter instance in ReplayController::destroy().
		pBufferFileWriter_->setListener( this );

		pBufferFileWriter_->initWithFileSystemPath( replayFilePath_,
				/* shouldOverwrite */ true,
				/* shouldTruncate */ true );
	}

	if (pBufferFileWriter_)
	{
		pBufferFileWriter_->queueWrite( data );
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool ReplayController::onReplayDataFileReaderMetaData( 
		ReplayDataFileReader & reader, const ReplayMetaData & metaData,
		BW::string & errorString )
{
	// Ensure that we do not cease to exist due to handler_ callback.
	ReplayControllerPtr pThisHolder( this );

	if (!handler_.onReplayControllerReadMetaData( *this, metaData ))
	{
		errorString = "User callback rejected meta-data";
		return false;
	}

	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
bool ReplayController::onReplayDataFileReaderTickData(
		ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString )
{
	stream.finish();
	return true;
}


/*
 *	Override from IReplayDataFileReaderListener.
 */
void ReplayController::onReplayDataFileReaderError(
		ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription )
{
	MF_ASSERT( &reader == pTickCounter_ );
	ERROR_MSG( "ReplayController::onReplayDataFileReaderError: %s %s\n",
		this->errorTypeToString( errorType ), errorDescription.c_str() );

	errorMessage_ = BW::string( "Invalid/corrupt replay data: " ) + 
			BW::string( this->errorTypeToString( errorType ) ) + 
			errorDescription;

	this->handleError( errorDescription );
}


/**
 *	This method signals that there is no more replay data to be received.
 */
void ReplayController::replayDataFinalise()
{
	isReceiving_ = false;
}


/**
 *	This method parses and enacts the given tick data.
 *
 *	@param data 	The tick data stream to process.
 *
 *	@return			True on success, false on error, and the replay controller is
 *					destroyed.
 */
bool ReplayController::processTick( BinaryIStream & data )
{
	if (data.remainingLength() == 0)
	{
		// Empty ticks are expected if CellApps die in a space all at one time.
		return true;
	}

	CompressionIStream compressionStream( data );

	BinaryIStream & tickData = compressionStream;
	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::processTick: "
			"Replay data corrupted: failed to decompress tick data\n" );
		data.finish();
		errorMessage_ = "Invalid/corrupt replay data: "
			"Failed to decompress tick data";
		this->destroy();
		return false;
	}

	bool isSpaceDataLoaded = false;
	
	while (!tickData.error() &&
			!isDestroyed_ &&
			(size_t( tickData.remainingLength() ) >
				sizeof(ReplayData::BlockType)))
	{
		ReplayData::BlockType blockType;
		tickData >> blockType;

		if (tickData.error())
		{
			ERROR_MSG( "ReplayController::processTick: "
				"Failed to get block type\n" );
			tickData.finish();
			errorMessage_ = "Invalid/corrupt replay data: "
				"Could not read block type";
			this->destroy();
			return false;
		}

		switch (blockType)
		{
		case ReplayData::BLOCK_TYPE_ENTITY_CREATE:
			this->handleEntityCreate( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_VOLATILE:
			this->handleEntityVolatile( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_VOLATILE_ON_GROUND:
			this->handleEntityVolatileOnGround( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_METHOD:
			this->handleEntityMethod( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_PROPERTY_CHANGE:
			this->handleEntityPropertyChange( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_PLAYER_STATE_CHANGE:
			this->handleEntityPlayerStateChange( tickData );
			break;
		case ReplayData::BLOCK_TYPE_PLAYER_AOI_CHANGE:
			this->handleEntityAoIChange( tickData );
			break;
		case ReplayData::BLOCK_TYPE_ENTITY_DELETE:
			this->handleEntityDelete( tickData );
			break;
		case ReplayData::BLOCK_TYPE_SPACE_DATA:
			this->handleSpaceData( tickData );
			isSpaceDataLoaded = true;
			break;
		case ReplayData::BLOCK_TYPE_FINAL:
			this->handleFinal();
			// Return as this is final
			return true;
		default:
			ERROR_MSG( "ReplayController::processTick: "
					"Invalid block type %d\n",
				(int)blockType );
			errorMessage_ = "Invalid/corrupt replay data: "
				"Could not read valid block type";
			this->destroy();
			return false;
		}
	}

	if ((currentTick_ == 0) && !isSpaceDataLoaded)
	{
		ERROR_MSG( "ReplayController::processTick: "
				"Invalid tick data for tick 0, no space data\n" );
		errorMessage_ = "Invalid/corrupt replay data: "
			"Could not read space data for tick 0";
		this->destroy();
		return false;
	}
	
	pVolatileCache_->processForTick( handler_, spaceID_, currentTick_ );
	return true;
}


/**
 *	This method handles an entity creation.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityCreate( BinaryIStream & tickData )
{
	EntityID vehicleID = NULL_ENTITY_ID;
	tickData >> vehicleID;

	{
		CompressionIStream stream( tickData );

		EntityID id;
		stream >> id;

		EntityTypeID type;
		stream >> type;

		Position3D pos( 0.f, 0.f, 0.f );
		Direction3D dir;

		stream >> pos >> dir.yaw >> dir.pitch >> dir.roll;

		handler_.onEntityCreate( id, type,
			spaceID_, vehicleID, pos, dir.yaw, dir.pitch, dir.roll,
			stream );
	}
}


/**
 *	This method handles an entity volatile update.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityVolatile( BinaryIStream & tickData )
{
	EntityID entityID;
	Position3D position;
	Direction3D direction;
	EntityID vehicle;
	tickData >> entityID >> position >> direction >> vehicle;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityVolatile: "
			"Failed to get data.\n" );
		return;
	}

	this->moveEntity( entityID, spaceID_, vehicle, position, Vector3::zero(),
		direction );

}


/**
 *	This method handles an entity volatile update for on-ground entities.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityVolatileOnGround( BinaryIStream & tickData )
{
	EntityID entityID;
	float x = 0.f;
	float z = 0.f;
	Direction3D direction;
	EntityID vehicle;
	tickData >> entityID >> x >> z >> direction >> vehicle;

	Position3D position( x, ServerConnection::NO_POSITION, z );

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityVolatileOnGround: "
			"Failed to get data.\n" );
		return;
	}

	this->moveEntity( entityID, spaceID_, vehicle, position, Vector3::zero(),
		direction );
}


/**
 *	This method moves an entity to a specified position.
 *	If we are seeking, we need to keep the last position as a buffer,
 *	So that we can move the entity immediately after seeking.
 *
 *	@param id 				The entity id.
 *	@param spaceID 			The space ID.
 *	@param vehicleID		The vehicle ID.
 *	@param pos 				The new destination of the entity.
 *	@param posError			The back up position if we cannot move the entity
*	@param direction 		The direction of the entity.
 */
void ReplayController::moveEntity( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, const Vector3 & posError,
		const Direction3D & direction )
{
	bool isVolatile = !this->isSeekingToTick();

	if (!this->isSeekingToTick() || ((seekToTick_ - currentTick_) > 1))
	{
		handler_.onEntityMoveWithError( id, spaceID, vehicleID, pos, posError,
			direction.yaw, direction.pitch, direction.roll, isVolatile );
	}

	pVolatileCache_->setVolatile( id, vehicleID, pos, direction, currentTick_ );
}


/**
 *	This method handles an entity method.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityMethod( BinaryIStream & tickData )
{
	EntityID entityID;
	Mercury::MessageID messageID;
	int32 streamSize;

	tickData >> entityID >> messageID >> streamSize;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityMethod: "
			"Failed to get data\n" );
		return;
	}

	MemoryIStream data( tickData.retrieve( streamSize ), streamSize );

	int exposedMethodID =
		ClientInterface::Range::entityMethodRange.exposedIDFromMsgID(
			messageID, data,
			entityDefConstants_.maxExposedClientMethodCount() );

	handler_.onEntityMethod( entityID, exposedMethodID, data );
}


/**
 *	This method handles a property change.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityPropertyChange( BinaryIStream & tickData )
{
	EntityID entityID;
	uint8 isSlice;
	int32 streamSize;

	tickData >> entityID >> isSlice >> streamSize;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityPropertyChange: "
			"Failed to get data\n" );
		return;
	}

	MemoryIStream data( tickData.retrieve( streamSize ), streamSize );
	handler_.onNestedEntityProperty( entityID, data,
		isSlice == 1 );
}


/**
 *	This method handles an entity transitioning between being a player and
 *	being a non-player.
 *
 *	@param tickData 	The tick data.
 */
void ReplayController::handleEntityPlayerStateChange( BinaryIStream & tickData )
{
	EntityID playerID = 0;
	uint8 hasBecomePlayer = 0;

	tickData >> playerID >> hasBecomePlayer;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityPlayerStateChange: "
				"Error while streaming player AoI data\n" );
		return;
	}

	handler_.onReplayControllerEntityPlayerStateChange( *this, playerID,
		/* hasBecomePlayer */ (hasBecomePlayer != 0) );

	if (hasBecomePlayer)
	{
		uint32 numInAoI = 0;

		tickData >> numInAoI;

		EntityID entityID = 0;
		while ((numInAoI-- > 0) && !tickData.error())
		{
			tickData >> entityID;
			handler_.onReplayControllerAoIChange( *this, playerID,
				entityID, /* hasEnteredAoI */ true );
		}

		if (tickData.error())
		{
			ERROR_MSG( "ReplayController::handleEntityPlayerStateChange: "
					"Error while streaming AoI entities for %d\n",
				playerID );
			return;
		}
	}
}


/**
 *	This method handles a player's AoI change.
 *
 *	@param tickData 	The tick data.
 */
void ReplayController::handleEntityAoIChange( BinaryIStream & tickData )
{
	EntityID playerID;
	EntityID entityID;
	uint8 hasEnteredAoI;

	tickData >> playerID >> entityID >> hasEnteredAoI;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityAoIChange: "
				"Failed to get data\n" );
		return;
	}

	handler_.onReplayControllerAoIChange( *this, playerID, entityID,
		(hasEnteredAoI != 0) );
}


/**
 *	This method handles an entity deletion.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleEntityDelete( BinaryIStream & tickData )
{
	EntityID entityID;

	tickData >> entityID;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleEntityDelete: "
			"Failed to get data\n" );
		return;
	}

	handler_.onEntityLeave( entityID, CacheStamps() );
	pVolatileCache_->clearVolatile( entityID );
}


/**
 *	This method handles a space data change.
 *
 *	@param tickData 	The tick data stream.
 */
void ReplayController::handleSpaceData( BinaryIStream & tickData )
{
	SpaceEntryID spaceEntryID;
	uint16 key;
	BW::string value;

	tickData >> spaceEntryID >> key >> value;

	if (tickData.error())
	{
		ERROR_MSG( "ReplayController::handleSpaceData: "
			"Failed to get data\n" );
		return;
	}

	handler_.spaceData( spaceID_, spaceEntryID, key, value );
}


/**
 *	This method handles a replay error.
 */
void ReplayController::handleError( const BW::string & error )
{
	handler_.onReplayControllerError( *this, error );
}


/**
 *	This method handles the 'finish' replay message, indicating end of replay.
 */
void ReplayController::handleFinal()
{
	handler_.onReplayControllerFinish( *this );
}


/**
 *	This method calls through to the server message handler to reset its
 *	entities.
 */
void ReplayController::resetEntities()
{
	handler_.onEntitiesReset( /* keepPlayerOnBase */ false );
}


/**
 *	This method returns the number of ticks known to be contained in the replay
 *	file.
 */
GameTime ReplayController::numTicksTotal() const
{
	if (replayFileType_ == ReplayController::FILE_ALREADY_DOWNLOADED)
	{
		return numTicksReported_;
	}
	else
	{
		return pTickCounter_->numTicksRead();
	}
}


/**
 *	This method returns the server time of the current tick of the
 *	recording.
 */
GameTime ReplayController::getCurrentTickTime() const
{
	if (!haveReadHeader_)
	{
		// We don't have the first game time yet.
		return 0;
	}

	return (firstGameTime_ + currentTick_);
}


/**
 *	This method returns the server time of the current tick of the recording,
 *	in seconds.
 */
double ReplayController::getCurrentTickTimeInSeconds() const
{
	if (!haveReadHeader_)
	{
		return -1.f;
	}

	return double( this->getCurrentTickTime() ) / gameUpdateFrequency_;
}



BW_END_NAMESPACE


// replay_controller.cpp
