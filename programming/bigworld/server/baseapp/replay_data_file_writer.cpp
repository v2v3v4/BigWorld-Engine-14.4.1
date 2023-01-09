#include "replay_data_file_writer.hpp"

#include "connection/replay_data.hpp"
#include "connection/replay_header.hpp"
#include "connection/replay_metadata.hpp"

#include "cstdmf/memory_stream.hpp"

#include "network/compression_stream.hpp"

#include "server/server_app_config.hpp"


#include <sys/stat.h>
#include <sys/types.h>

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: ReplayDataFileWriter
// ----------------------------------------------------------------------------

ReplayDataFileWriter::Writers ReplayDataFileWriter::s_activeWriters_;


/**
 *	Constructor for new replay data files.
 *
 *	@param pWriter 			The background file writer instance.
 *	@param pChecksumScheme	The checksum scheme used for signing.
 *	@param compressionType 	The compression level to use.
 *	@param digest 			The defs digest.
 *	@param numTicksToSign	The number of ticks to sign together.
 *	@param metaData			The meta-data to write.
 *	@param nonce 			The nonce to use when writing the header. This
 *							should only be used for unit tests, normally this
 *							should be empty so a suitable nonce can be
 *							generated.
 */
ReplayDataFileWriter::ReplayDataFileWriter( IBackgroundFileWriter * pWriter,
			ChecksumSchemePtr pChecksumScheme,
			BWCompressionType compressionType,
			const MD5::Digest & digest,
			uint numTicksToSign,
			const ReplayMetaData & metaData,
			const BW::string & nonce ) :
		compressionType_( compressionType ),
		currentTick_( 0 ),
		lastTickPendingWrite_( 0 ),
		lastTickWritten_( 0 ),
		lastChunkPosition_( 0 ),
		lastChunkLength_( 0 ),
		lastSignature_(),
		numTicksToSign_( numTicksToSign ),
		pChecksumScheme_( pChecksumScheme ),
		bufferedTickData_(),
		numTicksWritten_( 0 ),
		path_( pWriter->path() ),
		isFinalising_( false ),
		pWriter_( pWriter ),
		listeners_()
{
	this->commonInit();

	MemoryOStream fileStream;

	ReplayHeader header( pChecksumScheme_, digest,
		ServerAppConfig::updateHertz(), nonce );

	lastSignature_.reset();
	header.addToStream( fileStream, &lastSignature_ );

	// Prime the checksum scheme with the header's signature for writing out
	// the meta-data chunk.
	const int signatureLength = static_cast<int>(
		header.reportedSignatureLength() );

	pChecksumScheme->reset();
	pChecksumScheme->readBlob( lastSignature_.retrieve( signatureLength ), 
		signatureLength );

	lastSignature_.reset();
	metaData.addToStream( fileStream, &lastSignature_ );

	lastChunkPosition_ = fileStream.size();

	pWriter_->queueWrite( fileStream );
}


/**
 *	Constructor for recovering from existing partial replay files.
 *
 *	@param pWriter				The background file writer instance.
 *	@param pChecksumScheme		The checksum scheme for signing.
 *	@param recoveryData 		The recovery data.
 */
ReplayDataFileWriter::ReplayDataFileWriter( IBackgroundFileWriter * pWriter,
			ChecksumSchemePtr pChecksumScheme,
			const RecordingRecoveryData & recoveryData ) :
		compressionType_( recoveryData.compressionType() ),
		currentTick_( recoveryData.lastTickWritten() + 1 ),
		lastTickPendingWrite_( recoveryData.lastTickWritten() ),
		lastTickWritten_( recoveryData.lastTickWritten() ),
		lastChunkPosition_( recoveryData.nextTickPosition() ),
		lastChunkLength_( 0 ),
		lastSignature_( recoveryData.lastSignature().size() ),
		numTicksToSign_( recoveryData.numTicksToSign() ),
		pChecksumScheme_( pChecksumScheme ),
		bufferedTickData_(),
		numTicksWritten_( recoveryData.numTicks() ),
		path_( pWriter->path() ),
		isFinalising_( false ),
		pWriter_( pWriter ),
		listeners_()
{
	this->commonInit();

	const BW::string & lastSignatureString = recoveryData.lastSignature();

	lastSignature_.addBlob( lastSignatureString.data(),
		lastSignatureString.size() );

	TRACE_MSG( "ReplayDataFileWriter::ReplayDataFileWriter: "
			"Recovering from gameTime = %u, filePosition = %lu: %s\n",
		lastTickWritten_ + 1, recoveryData.nextTickPosition(),
		path_.c_str() );

	pWriter_->queueSeek( recoveryData.nextTickPosition(), SEEK_SET );
}


/**
 *	This method performs initialisation common to new files and recovered
 *	files.
 */
void ReplayDataFileWriter::commonInit()
{
	MF_ASSERT( pChecksumScheme_->isGood() );

	s_activeWriters_.insert( Writers::value_type( path_, this ) );

	pWriter_->setListener( this );

	TRACE_MSG( "ReplayDataFileWriter::commonInit: Signature length: %zu\n",
		pChecksumScheme_->streamSize() );
}


/**
 *	Destructor.
 */
ReplayDataFileWriter::~ReplayDataFileWriter()
{
	if (pWriter_)
	{
		pWriter_->setListener( NULL );
	}

	this->notifyListenersOfDestruction();

	if (!bufferedTickData_.empty())
	{
		NOTICE_MSG( "ReplayDataFileWriter::~ReplayDataFileWriter: "
				"Still have %zd ticks to write out.\n",
			bufferedTickData_.size() );
	}
}


/**
 *	This method adds the tick data from a cell for a particular tick.
 *
 *	@param tick 		The game tick.
 *	@param numCells		The number of cell tick data to expect for this tick,
 *						including this one.
 *	@param blob			The tick data.
 */
void ReplayDataFileWriter::addTickData( GameTime tick, uint numCells,
		BinaryIStream & blob )
{
	if (isFinalising_)
	{
		NOTICE_MSG( "ReplayDataFileWriter::addTickData: "
				"Already finalising for %s\n",
			path_.c_str() );
		blob.finish();
		return;
	}

	if (tick < currentTick_)
	{
		// This can happen if a new cell enters a space, and reports a numCells
		// count with one extra (itself) and is the last tick data to be
		// received.

		// TODO: Cater for this case.
		if (blob.remainingLength() > 0)
		{
			ERROR_MSG( "ReplayDataFileWriter::addTickData: "
					"Dropped %d bytes from new cell for tick #%u already "
					"written out, now at #%u (for recording \"%s\").\n",
				blob.remainingLength(),
				tick,
				currentTick_,
				path_.c_str());

			blob.finish();
		}

		return;
	}

	if ((currentTick_ == 0) && (bufferedTickData_.empty()))
	{
		if (blob.remainingLength() == 0) 
		{
			ERROR_MSG( "ReplayDataFileWriter::addTickData: "
					"Ignored the first tick with empty data "
					"tick #%u (for recording \"%s\").\n",
				tick,
				path_.c_str());

			return;
		}

		currentTick_ = tick;

		// Pretend we have written previous ticks.
		lastTickWritten_ = lastTickPendingWrite_ = (tick - 1);
	}

	TickData::iterator iTickData = bufferedTickData_.find( tick );
	if (iTickData == bufferedTickData_.end())
	{
		iTickData = bufferedTickData_.insert(
				TickData::value_type( tick, TickReplayData() ) ).first;
		iTickData->second.init( tick, numCells );
	}

	TickReplayData & tickReplayData = iTickData->second;

	if (numCells > tickReplayData.numExpectedCells())
	{
		tickReplayData.numExpectedCells( numCells );
	}
	// else if (numCells < tickReplayData.numExpectedCells())
	// {
		// We should never be decrementing our expected number of cells. In the
		// event a cell is removed from a space, once all cells are in
		// agreement over the number of cells in the space, a complete set of
		// tick data will be produced, and incomplete sets from earlier ticks
		// will be deemed complete and written out.
	// }

	tickReplayData.transferFromCell( blob );

	if (tickReplayData.numCellsLeft() == 0)
	{
		this->finaliseCompletedTickData( tickReplayData );
	}
}


/**
 *	This method finalises any complete tick data from the current tick onwards,
 *	and potentially writes a new chunk of signed ticks to disk if there have
 *	accumulated enough ticks to sign.
 *
 *	@param completedTickReplayData 	The latest completed tick data. This tick
 *									and all earlier received tick data will be
 *									written out, even if the earlier tick data
 *									is not complete.
 */
void ReplayDataFileWriter::finaliseCompletedTickData(
		TickReplayData & completedTickReplayData )
{
	MF_ASSERT( completedTickReplayData.numCellsLeft() == 0);

	while (currentTick_ <= completedTickReplayData.tick())
	{
		TickData::iterator iTickData = bufferedTickData_.find( currentTick_ );
		if (iTickData != bufferedTickData_.end())
		{
			TickReplayData & tickReplayData = iTickData->second;

			if (tickReplayData.numCellsLeft() > 0)
			{
				INFO_MSG( "ReplayDataFileWriter::finaliseCompletedTickData: "
						"Writing out incomplete tick %u "
						"before completed tick %u (num cells left = %u)\n",
					currentTick_,
					completedTickReplayData.tick(),
					tickReplayData.numCellsLeft() );
			}

			tickReplayData.finalise( compressionType_ );
		}

		++currentTick_;
	}

	this->signAndWriteCompletedTickData();
}


/**
 *	This method signs and writes a chunk of completed tick data chunks.
 *
 *	@param isLastChunk	Whether this is the last chunk for the file, and should
 *						be signed even if there are not enough ticks to meet
 *						the usual numTicksToSign_ threshold.
 */
void ReplayDataFileWriter::signAndWriteCompletedTickData(
		bool isLastChunk /* = false */ )
{
	if (!currentTick_)
	{
		// No data has been added yet.
		return;
	}

	// Current tick will have been incremented to the next tick we are
	// expecting but have not received yet.

	MF_ASSERT( currentTick_ > lastTickPendingWrite_ );
	GameTime nextTickToWrite = (lastTickPendingWrite_ + 1);
	while ((nextTickToWrite < currentTick_) &&
			(isLastChunk ||
				((currentTick_ - nextTickToWrite) >= numTicksToSign_)))
	{
		MemoryOStream fileData;

		MF_ASSERT( size_t( lastSignature_.size() ) ==
			pChecksumScheme_->streamSize() );
		pChecksumScheme_->reset();
		pChecksumScheme_->readBlob( lastSignature_.retrieve( 0 ),
			lastSignature_.size() );

		// Figure out how big the chunk will be for the chunk header.
		const uint numTicksToActuallySign =
			std::min( numTicksToSign_, (currentTick_ - nextTickToWrite) );

		ReplayData::SignedChunkLength chunkLength = 0;
		const TickData::iterator iFirstTickToWrite =
			bufferedTickData_.find( nextTickToWrite );

		TickData::iterator iTickData = iFirstTickToWrite;

		// First pass to calculate the chunk length.

		GameTime chunkTime = nextTickToWrite;

		while (chunkTime < (nextTickToWrite + numTicksToActuallySign))
		{
			MF_ASSERT( chunkTime <= iTickData->first );

			if (chunkTime < iTickData->first)
			{
				// Account for lengths of ticks with missing data. Ticks may
				// have no data if all CellApps in a space die suddenly, and
				// the CellAppMgr waits until the CellApp timeout period
				// expires to assign a new cell (default of 3 seconds).

				// Since they're all empty, they're all identical length.
				chunkLength += (iTickData->first - chunkTime) *
					TickReplayData::tickHeaderSize();

				DEBUG_MSG(
					"ReplayDataFileWriter::signAndWriteCompletedTickData: "
						"Assuming empty data for ticks in range [%u, %u]\n",
					chunkTime, iTickData->first );

				chunkTime = iTickData->first;
			}

			chunkLength += iTickData->second.size();

			++iTickData;
			++chunkTime;
		}

		// Second pass to actually collect and write out the chunk.

		// Write out the tick data and sign it with the checksum scheme.
		chunkTime = nextTickToWrite;

		{
			ChecksumOStream stream( fileData, pChecksumScheme_,
				/* shouldReset */ false );

			stream << chunkLength;

			iTickData = iFirstTickToWrite;

			while (chunkTime < (nextTickToWrite + numTicksToActuallySign))
			{
				MF_ASSERT( chunkTime <= iTickData->first );

				while (chunkTime < iTickData->first)
				{
					// OK, now we actually add the empty ticks that we counted
					// above. 
					TickReplayData::streamTickHeader( stream, chunkTime );
					++chunkTime;
				}

				stream.transfer( iTickData->second.data(),
					iTickData->second.size() );

				++iTickData;
				++chunkTime;
			}

			lastTickPendingWrite_ = (chunkTime - 1);

			lastSignature_.reset();
			stream.finalise( &lastSignature_ );
		}

		pWriter_->queueWrite( fileData,
			/* userData */ int(lastTickPendingWrite_) );

		nextTickToWrite = (lastTickPendingWrite_ + 1);
	}
}


/**
 *	This method sets the listener callback object.
 *
 *	Listeners should have longer lifetimes than the writer they are listening
 *	to; that is, they should not be destructed before any writers they listen
 *	to are destructed, otherwise this will cause dangling pointers that will
 *	cause issues when writers are destructed.
 *
 *	@param pListener 	The listener object to add.
 */
void ReplayDataFileWriter::addListener(
		ReplayDataFileWriterListener * pListener )
{
	listeners_.push_back( pListener );
}


/*
 *	Override from BackgroundFileWriterListener.
 */
void ReplayDataFileWriter::onBackgroundFileWritingError(
		IBackgroundFileWriter & writer )
{
	s_activeWriters_.erase( path_ );
	this->notifyListenersOfError();
}


/*
 *	Override from BackgroundFileWriterListener.
 */
void ReplayDataFileWriter::onBackgroundFileWritingComplete(
		IBackgroundFileWriter & writer, long filePosition, int userArg )
{
	if (userArg != 0)
	{
		const uint newLastTickWritten = GameTime( userArg );
		const uint numNewTicksWritten = newLastTickWritten - lastTickWritten_;

		numTicksWritten_ += numNewTicksWritten;

		lastChunkPosition_ = filePosition;
		lastChunkLength_ = sizeof( ReplayData::SignedChunkLength );

		TickData::iterator iWrittenTickData =
			bufferedTickData_.find( lastTickWritten_ + 1 );

		MF_ASSERT( iWrittenTickData != bufferedTickData_.end() );
		GameTime lastTickDeleted = 0;

		while ((iWrittenTickData != bufferedTickData_.end()) &&
				(iWrittenTickData->first <= newLastTickWritten))
		{
			TickData::iterator toDelete = iWrittenTickData;
			lastChunkLength_ += iWrittenTickData->second.size();
			++iWrittenTickData;
			lastTickDeleted = toDelete->first;
			bufferedTickData_.erase( toDelete );
		}

		MF_ASSERT( lastTickDeleted == newLastTickWritten );

		lastTickWritten_ = newLastTickWritten;
		lastChunkLength_ += pChecksumScheme_->streamSize();
  	}

	// If we are finalising, we need to figure out when we have written all
	// the completed ticks. Either our buffer is empty, or the earliest tick
	// for which we have some data is in the future and incomplete.
	const bool haveMoreTickDataToWrite = (!bufferedTickData_.empty() &&
		(bufferedTickData_.begin()->first < currentTick_));

	if (isFinalising_ && !haveMoreTickDataToWrite)
	{
		this->closeWriter();
	}

	this->notifyListenersOfCompletion();
}


/**
 *	This method closes the writer.
 *
 *	@param shouldFinalise 	If false, the file will be closed for writing
 *							without being set read-only or its numTicks field
 *							updated. If true, this is equivalent to calling
 *							finalise().
 */
void ReplayDataFileWriter::close( bool shouldFinalise )
{
	if (shouldFinalise)
	{
		this->finalise();
	}
	else
	{
		this->closeWriter();
	}
}


/**
 *	This method starts finalising the replay data file.
 */
void ReplayDataFileWriter::finalise()
{
	isFinalising_ = true;

	this->signAndWriteCompletedTickData( /* isLastChunk */ true );

	const size_t fieldOffset = ReplayHeader::numTicksFieldOffset(
		pChecksumScheme_ );

	const GameTime numTicksWillBeWritten = numTicksWritten_ +
		(lastTickPendingWrite_ - lastTickWritten_);

	pWriter_->queueSeek( fieldOffset, SEEK_SET );
	pWriter_->queueWriteBlob(
		reinterpret_cast< const char *>( &numTicksWillBeWritten ),
		sizeof(GameTime) );
	pWriter_->queueChmod( S_IRUSR | S_IRGRP | S_IROTH );
}


/**
 *	This method releases the writer and removes this writer from the active
 *	writing set.
 */
void ReplayDataFileWriter::closeWriter()
{
	isFinalising_ = true;

	pWriter_->setListener( NULL );

	if (!pWriter_->hasError())
	{
		// Hold onto the writer if there was an error.
		pWriter_ = NULL;
	}

	// Allow writers for this path again.
	s_activeWriters_.erase( path_ );

	TRACE_MSG( "ReplayDataFileWriter::closeWriter(): "
			"Closed writing for \"%s\"\n",
		path_.c_str() );
}


/**
 *	This method returns true if the underlying file writer is in error.
 *
 *	@return true if an error has occurred, false otherwise.
 */
bool ReplayDataFileWriter::hasError() const
{
	return pWriter_->hasError();
}


/**
 *	This method returns the error string from the file writer.
 *
 *	@return an error string.
 */
BW::string ReplayDataFileWriter::errorString() const
{
	if (!pWriter_.get() || !pWriter_->hasError())
	{
		return BW::string();
	}

	return pWriter_->errorString();
}


/**
 *	This method returns whether a replay data file writer already exists for
 *	the given path.
 *
 *	@param path 	The path to check
 *	@return 		true if there is existing writer with that path, false
 *					otherwise.
 */
// static
bool ReplayDataFileWriter::existsForPath( const BW::string & path )
{
	return (s_activeWriters_.count( path ) > 0);
}


/**
 *	This method closes active writer instances that are not already finalising.
 *
 *	@param shouldFinalise 	Whether the writers that are not already finalising
 *							should be finalised or closed immediately.
 *	@param pListener 		This listener is added to each of the writer
 *							instances that are finalising (even those that were
 *							already finalising) to receive notification when
 *							they are each finalised.
 */
//static
void ReplayDataFileWriter::closeAll(
		ReplayDataFileWriterListener * pListener,
		bool shouldFinalise /* true */ )
{
	Writers::iterator iWriter = s_activeWriters_.begin();

	while (iWriter != s_activeWriters_.end())
	{
		ReplayDataFileWriter & writer = *(iWriter->second);

		++iWriter;

		INFO_MSG( "ReplayDataFileWriter::closeAll: %s \"%s\"\n",
			writer.isFinalising() ? "Awaiting finalisation for" : "Finalising",
			writer.path().c_str() );

		if (pListener)
		{
			writer.addListener( pListener );
		}

		if (!writer.isFinalising())
		{
			writer.close( shouldFinalise );
		}
	}
}


/**
 *	This method returns whether there are unfinalised writers.
 *
 *	@return true if all active writers have been finalised, otherwise false.
 */
// static
bool ReplayDataFileWriter::haveAllClosed()
{
	return s_activeWriters_.empty();
}


/**
 *	This method notifies registered listeners of an error condition on this
 *	writer.
 */
void ReplayDataFileWriter::notifyListenersOfError()
{
	Listeners listenersCopy( listeners_ );
	Listeners::iterator iListener = listenersCopy.begin();

	while (iListener != listenersCopy.end())
	{
		(*iListener)->onReplayDataFileWritingError( *this );
		++iListener;
	}
}


/**
 *	This method notifies registered listeners of writing completion on this
 *	writer.
 */
void ReplayDataFileWriter::notifyListenersOfCompletion()
{
	Listeners listenersCopy( listeners_ );
	Listeners::iterator iListener = listenersCopy.begin();

	while (iListener != listenersCopy.end())
	{
		(*iListener)->onReplayDataFileWritingComplete( *this );
		++iListener;
	}
}


/**
 *	This method notifies registered listeners of this writer's destruction.
 */
void ReplayDataFileWriter::notifyListenersOfDestruction()
{
	Listeners listenersCopy( listeners_ );
	Listeners::iterator iListener = listenersCopy.begin();

	while (iListener != listenersCopy.end())
	{
		(*iListener)->onReplayDataFileWriterDestroyed( *this );
		++iListener;
	}
}


/**
 *	This method returns the recovery data for this writer.
 */
const RecordingRecoveryData ReplayDataFileWriter::recoveryData() const
{
	MemoryOStream & lastSignatureNonConst =
		const_cast< MemoryOStream & >( lastSignature_ );

	BW::string lastSignatureString(
		(char *)lastSignatureNonConst.retrieve( 0 ),
		lastSignatureNonConst.remainingLength() );

	return RecordingRecoveryData( compressionType_, numTicksWritten_,
		lastTickWritten_,
		/* nextChunkPosition */ lastChunkPosition_ + lastChunkLength_,
		lastSignatureString, numTicksToSign_ );
}


// ----------------------------------------------------------------------------
// Section: TickReplayData
// ----------------------------------------------------------------------------


/**
 *	Constructor.
 */
TickReplayData::TickReplayData() :
		tick_( 0 ),
		numExpectedCells_( 0 ),
		numCellsLeft_( 0 ),
		pBlob_( NULL )
{}


/**
 *	This method initialises this object.
 *
 *	@param tick 		The game time.
 *	@param numCells 	The number of cells for the tick.
 */
void TickReplayData::init( GameTime tick, uint numCells )
{
	tick_ = tick;
	numExpectedCells_ = numCellsLeft_ = numCells;

	if (pBlob_)
	{
		delete pBlob_;
	}

	pBlob_ = new MemoryOStream();

}


/**
 *	Destructor.
 */
TickReplayData::~TickReplayData()
{
	bw_safe_delete( pBlob_ );
}


/**
 *	This method transfers the blob data for this tick originating from a cell.
 *
 *	@param data 	The blob data.
 */
void TickReplayData::transferFromCell( BinaryIStream & data )
{
	if (numCellsLeft_ == 0)
	{
		NOTICE_MSG( "TickReplayData::transferFromCell: "
				"Already have complete set of the cell data\n" );
		data.finish();
		return;
	}

	pBlob_->transfer( data, data.remainingLength() );
	--numCellsLeft_;
}


/**
 *	This method finalises the tick replay data before being written out.
 *
 *	@param compressionType 	The compression level to use.
 */
void TickReplayData::finalise( BWCompressionType compressionType )
{
	MemoryOStream * pUncompressed = pBlob_;

	pBlob_ = new MemoryOStream();

	this->streamTickHeader( *pBlob_, tick_ );

	{
		CompressionOStream compressionStream( *pBlob_,
			compressionType );

		BinaryOStream & stream = compressionStream;
		stream.transfer( *pUncompressed, pUncompressed->remainingLength() );
	}

	bw_safe_delete( pUncompressed );

	// Finalise the tick data length header field.
	ReplayData::TickDataLength * pLength =
		(ReplayData::TickDataLength *)((char *) pBlob_->retrieve( 0 ) +
			sizeof(GameTime));
	*pLength = pBlob_->remainingLength() -
		sizeof(GameTime) - sizeof(ReplayData::TickDataLength);
}


/**
 *	This method adjusts the number of expected cells based on tick data sent
 *	from CellApps.
 *
 *	@param newValue 	The new number of expected cells in the recorded space.
 */
void TickReplayData::numExpectedCells( uint newValue )
{
	MF_ASSERT( newValue > numExpectedCells_ );

	int delta = (newValue - numExpectedCells_);
	numCellsLeft_ += delta;

	if (delta > 1)
	{
		ERROR_MSG( "TickReplayData::numExpectedCells: "
				"%d cells were added, expected at most 1\n",
			delta );
	}

	numExpectedCells_ = newValue;
}


/**
 *	This method returns the size of a tick data with no data.
 */
/* static */
int TickReplayData::tickHeaderSize()
{
	return sizeof(GameTime) + sizeof(ReplayData::TickDataLength);
}


/**
 *	This method streams on the tick header for the given game time to the given
 *	stream. It reserves the length field by setting it to 0.
 *
 *	@param stream 	The output stream.
 *	@param tick		The game time.
 */
/* static */
void TickReplayData::streamTickHeader( BinaryOStream & stream, GameTime tick )
{
	stream << tick << ReplayData::TickDataLength( 0 );
}


BW_END_NAMESPACE


// replay_data_file_writer.cpp
