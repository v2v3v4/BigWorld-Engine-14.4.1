#include "pch.hpp"

#include "replay_data_file_reader.hpp"


#include "replay_checksum_scheme.hpp"
#include "replay_data.hpp"
#include "replay_header.hpp"
#include "replay_metadata.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/checksum_stream.hpp"

#include "network/compression_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class implements the ReplayDataFileReader class.
 */
class ReplayDataFileReader::Impl
{
public:
	Impl( ReplayDataFileReader & parent,
		IReplayDataFileReaderListener & listener,
		const BW::string & verifyingKey,
		bool shouldDecompress,
		size_t verifyFromPosition );
	~Impl();

	bool addData( BinaryIStream & data );
	void reset();
	void clearBuffer();


	/**
	 *	This method returns whether an error has occurred.
	 */
	bool hasError() const 				{ return !lastError_.empty(); }

	/**
	 *	This method returns the description of the last error that occurred.
	 */
	const BW::string & lastError() const { return lastError_; }


	/**
	 *	This method clears the last error, allowing further calls to addData().
	 */
	void clearError() { lastError_.clear(); }

	/**
	 *	This returns whether the header has been read yet. This includes
	 *	headers that have been deemed to be invalid.
	 */
	bool hasReadHeader() const			{ return hasReadHeader_; }

	/**
	 *	This returns the header read from the replay data file.
	 */
	const ReplayHeader & header() const { return header_; }

	/** This method returns whether meta-data has been read. */
	bool hasReadMetaData() const 		{ return hasReadMetaData_; }

	/** This method returns the meta-data read. */
	const ReplayMetaData & metaData() const { return metaData_; }

	/**
	 *	This returns the first game time read from the replay file, or 0 if
	 *	none has been read yet.
	 */
	GameTime firstGameTime() const		{ return firstGameTime_; }

	/**
	 *	This returns the number of game ticks read from the file so far.
	 */
	uint numTicksRead() const			{ return numTicksRead_; }

	/**
	 *	This returns the number of bytes read from the file so far.
	 */
	size_t numBytesRead() const 		{ return numBytesRead_; }

	/**
	 *	This method returns the number of signed chunks read. The header counts
	 *	as a chunk.
	 */
	uint numChunksRead() const 			{ return numChunksRead_; }

	/**
	 *	This method returns the number of bytes added (but not necessarily read).
	 */
	size_t numBytesAdded() const 		{ return numBytesAdded_; }

	/**
	 *	This method returns the position before which the data is not required
	 *	to be verified.
	 */
	size_t verifiedToPosition() const	{ return verifiedToPosition_; }

	/**
	 *	This returns the number of signed chunks that have had their signature
	 *	verified. The header counts as a chunk.
	 */
	uint numChunksVerified() const 		{ return numChunksVerified_; }

	/**
	 *	This returns the file offset of the numTicks field in the replay file.
	 */
	size_t numTicksFieldOffset() const
	{
		return ReplayHeader::numTicksFieldOffset( pChecksumScheme_ );
	}


	class Memento;
	friend class Memento;

private:

	bool readHeader();
	bool readMetaData();
	bool readNextChunkLength();
	bool readNextChunk();
	bool readTickData( BinaryIStream & stream );
	void primeChecksumScheme();

	void handleReadError( const BW::string & errorString );

	ReplayDataFileReader & 				parent_;
	IReplayDataFileReaderListener & 	listener_;
	bool 								shouldDecompress_;
	ChecksumSchemePtr 					pChecksumScheme_;

	BW::string 							lastError_;
	bool								hasReadHeader_;
	ReplayHeader						header_;

	bool 								hasReadMetaData_;
	ReplayMetaData						metaData_;

	MemoryOStream 						lastSignature_;
	size_t 								nextChunkLength_;

	MemoryOStream						buffer_;

	GameTime							firstGameTime_;
	GameTime 							lastGameTime_;
	uint 								numTicksRead_;

	size_t 								numBytesRead_;
	uint								numChunksRead_;
	size_t 								numBytesAdded_;
	size_t								verifiedToPosition_;
	uint								numChunksVerified_;
};

#ifdef _MSC_VER
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // _MSC_VER

/**
 *	This class is used to save away state so that a reader can revert back to
 *	its state before adding possibly bad data.
 */
class ReplayDataFileReader::Impl::Memento
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param reader 	The reader to save state for.
	 */
	Memento( ReplayDataFileReader::Impl & reader ) :
			reader_( reader ),
			isActive_( true ),
			hasReadHeader_( reader.hasReadHeader_ ),
			header_( reader.header_ ),
			hasReadMetaData_( reader.hasReadMetaData_ ),
			metaData_( reader.metaData_ ),
			lastSignature_( reader.lastSignature_.remainingLength() ),
			nextChunkLength_( reader.nextChunkLength_ ),
			buffer_( reader.buffer_.remainingLength() ),
			firstGameTime_( reader.firstGameTime_ ),
			lastGameTime_( reader.lastGameTime_ ),
			numTicksRead_( reader.numTicksRead_ ),
			numBytesRead_( reader.numBytesRead_ ),
			numChunksRead_( reader.numChunksRead_ ),
			numBytesAdded_( reader.numBytesAdded_ ),
			verifiedToPosition_( reader.verifiedToPosition_ ),
			numChunksVerified_( reader.numChunksVerified_ )
	{
		lastSignature_.addBlob( reader.lastSignature_.retrieve( 0 ),
			reader.lastSignature_.remainingLength() );

		buffer_.addBlob( reader.buffer_.retrieve( 0 ),
			reader.buffer_.remainingLength() );

	}


	/**
	 *	Destructor.
	 */
	~Memento()
	{
		this->apply();
	}
	

	/**
	 *	This method applies the saved state in this object onto the given
	 *	reader instance.
	 */
	void apply()
	{
		if (!isActive_)
		{
			return;
		}

		reader_.hasReadHeader_ 		 	= hasReadHeader_;
		reader_.header_ 			 	= header_;
		reader_.hasReadMetaData_ 		= hasReadMetaData_;
		reader_.metaData_ 				= metaData_;

		reader_.lastSignature_.reset();
		reader_.lastSignature_.transfer( lastSignature_,
			lastSignature_.remainingLength() );

		reader_.nextChunkLength_ 		= nextChunkLength_;

		reader_.buffer_.reset();
		reader_.buffer_.transfer( buffer_, buffer_.remainingLength() );

		reader_.firstGameTime_ 			= firstGameTime_;
		reader_.lastGameTime_ 			= lastGameTime_;
		reader_.numTicksRead_ 			= numTicksRead_;
		reader_.numBytesRead_ 			= numBytesRead_;
		reader_.numChunksRead_ 			= numChunksRead_;
		reader_.numBytesAdded_ 			= numBytesAdded_;
		reader_.verifiedToPosition_ 	= verifiedToPosition_;
		reader_.numChunksVerified_ 		= numChunksVerified_;
	}


	/**
	 *	This method returns whether this memento is active, and will be applied
	 *	on its reader instance on destruction.
	 */
	bool isActive() const 			{ return isActive_; }

	/**
	 *	This method sets the active state for this memento instance.
	 */
	void isActive( bool value ) 	{ isActive_ = value; }

private:
	// Member data.
 	ReplayDataFileReader::Impl & 		reader_;
	bool								isActive_;

	bool								hasReadHeader_;
	ReplayHeader						header_;

	bool 								hasReadMetaData_;
	ReplayMetaData						metaData_;

	MemoryOStream 						lastSignature_;
	size_t 								nextChunkLength_;

	MemoryOStream						buffer_;

	GameTime							firstGameTime_;
	GameTime 							lastGameTime_;
	uint 								numTicksRead_;

	size_t 								numBytesRead_;
	uint								numChunksRead_;
	size_t 								numBytesAdded_;
	size_t								verifiedToPosition_;
	uint								numChunksVerified_;
};


/**
 *	Constructor.
 *
 *	@param listener 			The listener to receive callbacks.
 *	@param verifyingKey 		The public key to use for verifying signatures,
 *								in PEM format.
 *	@param shouldDecompress 	Whether the tick data should be decompressed.
 *	@param verifyFromPosition 	This sets the position from which signatures
 *								will be verified. Data before this position
 *								will not have their signatures verified.
 */
ReplayDataFileReader::ReplayDataFileReader(
			IReplayDataFileReaderListener & listener,
			const BW::string & verifyingKey,
			bool shouldDecompress /* = true */,
			size_t verifyFromPosition /* = 0U */ ) :
		pImpl_( new Impl( *this, listener, verifyingKey, shouldDecompress,
			verifyFromPosition ) )
{}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

/**
 *	Constructor.
 *
 *	@param parent 				The ReplayDataFileReader this object
 *								implements.
 *	@param listener 			The listener to receive callbacks.
 *	@param verifyingKey 		The public key to use for verifying signatures,
 *								in PEM format. Can be empty, in which case no
 *								verification happens.
 *	@param shouldDecompress 	Whether the tick data should be decompressed.
 *	@param verifyFromPosition 	This sets the position from which signatures
 *								will be verified. Data before this position
 *								will not have their signatures verified.
 */
ReplayDataFileReader::Impl::Impl( ReplayDataFileReader & parent,
			IReplayDataFileReaderListener & listener,
			const BW::string & verifyingKey,
			bool shouldDecompress,
			size_t verifyFromPosition ) :
		parent_( parent ),
		listener_( listener ),
		shouldDecompress_( shouldDecompress ),
		pChecksumScheme_( verifyingKey.empty() ? NULL :
			ReplayChecksumScheme::create( verifyingKey,
				/* isPrivate */ false  ) ),
		lastError_(),
		hasReadHeader_( false ),
		header_( pChecksumScheme_ ),
		hasReadMetaData_( false ),
		metaData_(),
		lastSignature_(),
		nextChunkLength_( 0U ),
		buffer_(),
		firstGameTime_( 0U ),
		lastGameTime_( 0U ),
		numTicksRead_( 0U ),
		numBytesRead_( 0U ),
		numChunksRead_( 0U ),
		numBytesAdded_( 0U ),
		verifiedToPosition_( verifyFromPosition ),
		numChunksVerified_( 0U )
{
	buffer_.canRewind( false );
}


/**
 *	This method resets this object so that it can read from the beginning of
 *	the file again.
 */
void ReplayDataFileReader::reset()
{
	BW_GUARD;
	pImpl_->reset();
}


/**
 *	This method implements the ReplayDataFileReader::reset() method.
 */
void ReplayDataFileReader::Impl::reset()
{
	if (pChecksumScheme_)
	{
		pChecksumScheme_->reset();
	}

	lastError_.clear();
	hasReadHeader_ 				= false;
	header_ 					= ReplayHeader( pChecksumScheme_ );
	hasReadMetaData_ 			= false;
	metaData_					= ReplayMetaData();
	lastSignature_.reset();
	nextChunkLength_ 			= 0U;
	buffer_.reset();
	firstGameTime_				= 0U;
	lastGameTime_				= 0U;
	numTicksRead_				= 0U;
	numBytesRead_				= 0U;
	numChunksRead_				= 0U;
	numBytesAdded_				= 0U;
	numChunksVerified_			= 0U;

	// verifiedToPosition is not reset here.
}


/**
 *	Destructor.
 */
ReplayDataFileReader::~ReplayDataFileReader()
{
	bw_safe_delete( pImpl_ );
}


/**
 *	Destructor.
 */
ReplayDataFileReader::Impl::~Impl()
{
	BW_GUARD;

	listener_.onReplayDataFileReaderDestroyed( parent_ );
}


/**
 *	This method adds data to be read into the file header and game tick data.
 *
 *	@param data 	The data stream. The remaining length of the stream will be
 *					added.
 *
 *	@return 		False if an error occurred, true otherwise.
 */
bool ReplayDataFileReader::addData( BinaryIStream & data )
{
	BW_GUARD;
	return pImpl_->addData( data );
}


/**
 *	This method adds data to be read into the file header and game tick data.
 *
 *	@param data 	The data.
 *	@param dataSize	The size of the data.
 *
 *	@return 		False if an error occurred, true otherwise.
 */
bool ReplayDataFileReader::addData( const void * data, size_t dataSize )
{
	BW_GUARD;
	MemoryIStream stream( data, static_cast<int>(dataSize) );
	return this->addData( stream );
}


/**
 *	This method implements the ReplayDataFileReader::addData().
 */
bool ReplayDataFileReader::Impl::addData( BinaryIStream & data )
{
	if (!lastError_.empty())
	{
		return false;
	}

	if (pChecksumScheme_ && !pChecksumScheme_->isGood())
	{
		listener_.onReplayDataFileReaderError( parent_,
			IReplayDataFileReaderListener::ERROR_KEY_ERROR,
			pChecksumScheme_->errorString() );
		lastError_ = "Verifying key error: " + 
			pChecksumScheme_->errorString();
		return false;
	}

	// If memento goes out of scope without deactivation, we reset our state to
	// this point.
	Memento memento( *this );

	numBytesAdded_ += data.remainingLength();
	buffer_.transfer( data, data.remainingLength() );

	if (!hasReadHeader_)
	{
		if (!this->readHeader())
		{
			if (lastError_.empty())
			{
				lastError_ = "Header is invalid";
			}
			return false;
		}
	}

	if (hasReadHeader_ && !hasReadMetaData_)
	{
		if (!this->readMetaData())
		{
			if (lastError_.empty())
			{
				lastError_ = "Meta-data is invalid";
			}
			return false;
		}
	}

	if (hasReadHeader_ && hasReadMetaData_)
	{
		while (this->readNextChunkLength())
		{
			if (!this->readNextChunk())
			{
				if (lastError_.empty())
				{
					lastError_ = "Chunk data is invalid";
				}
				return false;
			}
		}

		if (!lastError_.empty())
		{
			return false;
		}
	}

	memento.isActive( false );
	return true;
}


/**
 *	This method clears any buffered data, resetting the state of the reader to
 *	the location of the last fully parsed position.
 *
 *	This is useful for scrubbing buffered data that is shown to be bad from
 *	ReplayDataFileReader::addData().
 *
 *	After calling this method, the numBytesAdded for this reader is updated
 *	to the end of the last complete chunk of data parsed, that is, it is
 *	set back to value of numBytesRead.
 */
void ReplayDataFileReader::clearBuffer()
{
	pImpl_->clearBuffer();
}


/**
 *	This method implements ReplayDataFileReader::clearBuffer().
 */
void ReplayDataFileReader::Impl::clearBuffer()
{
	numBytesAdded_ -= buffer_.remainingLength();
	MF_ASSERT( numBytesAdded_ == numBytesRead_ );
	buffer_.reset();
}


/**
 *	This method attempts to read the header from the data in the buffer.
 *
 *	@param headerSize 	The expected size of the header.
 *
 *	@return 			false if an error occurred, true otherwise, including
 *						if a header was not read because of insufficient data.
 */
bool ReplayDataFileReader::Impl::readHeader()
{
	BW_GUARD;

	const void * bufferStart = buffer_.retrieve( 0 );
	const size_t bufferLength = 
		static_cast< size_t >( buffer_.remainingLength() );

	if ((nextChunkLength_ && (bufferLength < nextChunkLength_)) ||
		(!nextChunkLength_ && 
			(!ReplayHeader::checkSufficientLength( 
				bufferStart, bufferLength, nextChunkLength_ ))))
	{
		// Insufficient data.
		return true;
	}

	const size_t headerSize = nextChunkLength_;
	nextChunkLength_ = 0;

	MemoryIStream headerStream( 
		buffer_.retrieve( static_cast<int>(headerSize) ),
			static_cast<int>(headerSize) );

	const bool shouldVerify = (pChecksumScheme_.exists()) &&
		(headerSize >= verifiedToPosition_);

	// Use the signature from the header as the last signature for the first
	// tick.
	lastSignature_.reset();
	if (pChecksumScheme_.exists())
	{
		pChecksumScheme_->reset();
	}

	BW::string errorString;
	if (!header_.initFromStream( headerStream, &lastSignature_,
			&errorString, shouldVerify ))
	{
		errorString = "Failed to read header: " + errorString;
		this->handleReadError( errorString );
		return false;
	}

	hasReadHeader_ = true;
	numBytesRead_ += headerSize;
	++numChunksRead_;

	if (shouldVerify)
	{
		++numChunksVerified_;
		verifiedToPosition_ = headerSize;
	}

	MF_ASSERT( headerStream.remainingLength() == 0 );

	if (!listener_.onReplayDataFileReaderHeader( parent_, header_, 
			errorString ))
	{
		lastError_ = errorString;
		return false;
	}

	return true;
}


/**
 *	This method reads the meta-data chunk.
 *
 *	@return 				True if successful, otherwise false.
 */
bool ReplayDataFileReader::Impl::readMetaData()
{
	const void * bufferStart = buffer_.retrieve( 0 );
	const size_t bufferLength = 
		static_cast< size_t >( buffer_.remainingLength() );

	if ((nextChunkLength_ && (bufferLength < nextChunkLength_)) ||
		(!nextChunkLength_ && 
			(!ReplayMetaData::checkSufficientLength( 
				bufferStart, bufferLength, header_.reportedSignatureLength(),
				nextChunkLength_ ))))
	{
		// Insufficient data.
		return true;
	}


	const size_t metaDataLength = nextChunkLength_;
	nextChunkLength_ = 0;

	MemoryIStream metaDataStream( 
		buffer_.retrieve( static_cast<int>(metaDataLength) ),
			static_cast<int>(metaDataLength) );

	const size_t endOffset = (numBytesRead_ + metaDataLength);
	const bool shouldVerify = (pChecksumScheme_.exists() &&
		 (endOffset >= verifiedToPosition_));

	ChecksumSchemePtr pChecksumScheme;
	if (shouldVerify)
	{
		this->primeChecksumScheme();
	}

	ReplayMetaData metaData;
	if (shouldVerify)
	{
		metaData.init( pChecksumScheme_ );
	}
	else
	{
		MF_ASSERT( hasReadHeader_ );
		metaData.init( header_.reportedSignatureLength() );
	}

	lastSignature_.reset();
	BW::string errorString;
	if (!metaData.readFromStream( metaDataStream, &lastSignature_,
			&errorString ))
	{
		errorString = "Failed to read meta-data: " + errorString;
		this->handleReadError( errorString );
		return false;
	}

	metaData_.swap( metaData );
	hasReadMetaData_ = true;

	numBytesRead_ += metaDataLength;
	++numChunksRead_;

	if (shouldVerify)
	{
		++numChunksVerified_;
		verifiedToPosition_ = endOffset;
	}

	if (!listener_.onReplayDataFileReaderMetaData( parent_, metaData_,
			errorString ))
	{
		lastError_ = errorString;
		return false;
	}

	return true;
}


/**
 *	This method reads the next data chunk length from the added data.
 *
 *	@return		True if the next chunk has been completely read into the
 *				buffer, false otherwise.
 */
bool ReplayDataFileReader::Impl::readNextChunkLength()
{
	if (nextChunkLength_ == 0)
	{
		ReplayData::SignedChunkLength chunkLength = 0;

		if (size_t(buffer_.remainingLength()) < sizeof(chunkLength))
		{
			return false;
		}

		MemoryIStream chunkLengthStream( buffer_.retrieve( 0 ),
			sizeof(chunkLength));
		chunkLengthStream >> chunkLength;

		if (chunkLengthStream.error())
		{
			lastError_ = "Corrupted chunk length";
			return false;
		}

		if (!chunkLength)
		{
			lastError_ = "Got zero chunk length";
			return false;
		}

		nextChunkLength_ = static_cast<size_t>( chunkLength );
	}

	return (static_cast<size_t>( buffer_.remainingLength() ) >= 
		(sizeof(ReplayData::SignedChunkLength) + 
			nextChunkLength_ + 
			header_.reportedSignatureLength() ));
}


/**
 *	This method reads the next chunk payload out of the buffer.
 *
 *	@return false if an error occurred, true otherwise.
 */
bool ReplayDataFileReader::Impl::readNextChunk()
{
	BW_GUARD;

	MF_ASSERT( nextChunkLength_ > 0 );

	// Use the last signature to seed the state of the checksum scheme.
	const bool shouldVerify = pChecksumScheme_.exists() &&
		((numBytesRead_ + 
				sizeof(ReplayData::SignedChunkLength) + 
				nextChunkLength_) >=
			verifiedToPosition_);

	ChecksumSchemePtr pChecksumScheme = NULL;
	if (shouldVerify)
	{
		pChecksumScheme = pChecksumScheme_;
		this->primeChecksumScheme();
	}

	ChecksumIStream checksumStream( buffer_, pChecksumScheme,
		/* shouldReset */ false );

	// Already read chunk length in readNextChunkLength(), just stream it off
	// to add to checksumming.
	checksumStream.retrieve( sizeof(ReplayData::SignedChunkLength) );

	MemoryIStream payload( checksumStream.retrieve( 
			static_cast<int>( nextChunkLength_ ) ), 
		static_cast<int>( nextChunkLength_ ) );

	numBytesRead_ += sizeof(ReplayData::SignedChunkLength);
	numBytesRead_ += nextChunkLength_;

	lastSignature_.reset();
	if (!checksumStream.verify( &lastSignature_ ))
	{
		listener_.onReplayDataFileReaderError( parent_,
			IReplayDataFileReaderListener::ERROR_SIGNATURE_MISMATCH,
			checksumStream.errorString() );

		lastError_ = "Error reading chunk: " + checksumStream.errorString();
		return false;
	}

	if (!shouldVerify)
	{
		lastSignature_.transfer( buffer_, header_.reportedSignatureLength() );
	}
	else
	{
		verifiedToPosition_ = numBytesRead_;
		++numChunksVerified_;
	}

	numBytesRead_ += header_.reportedSignatureLength();
	++numChunksRead_;
	nextChunkLength_ = 0;

	// OK we've read and verified the tick data if we needed to, it can be
	// processed now.
	bool ret = this->readTickData( payload );
	payload.finish();

	if (!ret)
	{
		if (lastError_.empty())
		{
			lastError_ = "Invalid tick data";
		}
		return false;
	}
	return true;
}


/**
 *	This method reads and processes the tick data present in one chunk.
 *
 *	@param payload 	The chunk payload to read.
 */
bool ReplayDataFileReader::Impl::readTickData( BinaryIStream & payload )
{
	BW_GUARD;

	if (payload.error())
	{
		payload.finish();
		
		listener_.onReplayDataFileReaderError( parent_,
			IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
			"Tick data stream has errors" );
		lastError_ = "File corrupted: Tick data stream has errors";
		return false;
	}
	
	while (payload.remainingLength() > 0)
	{
		GameTime gameTime;
		ReplayData::TickDataLength dataLength;

		payload >> gameTime >> dataLength;

		if (payload.error() || (dataLength > size_t(payload.remainingLength())))
		{
			payload.finish();
			listener_.onReplayDataFileReaderError( parent_,
				IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
				"Could not read game time data header" );
			lastError_ = "File corrupted: Could not read game time data header";
			return false;
		}

		if ((lastGameTime_ != 0) && (gameTime != (lastGameTime_ + 1)))
		{
			payload.finish();
			listener_.onReplayDataFileReaderError( parent_,
				IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
				"Game time mismatch" );
			lastError_ = "File corrupted: Game time mismatch";
			return false;
		}

		if ((header_.numTicks() != 0) && (numTicksRead_ == header_.numTicks()))
		{
			payload.finish();
			listener_.onReplayDataFileReaderError( parent_,
				IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
				"Extraneous tick data" );

			lastError_ = "File corrupted: Extraneous tick data";
			return false;
		}

		if (!firstGameTime_)
		{
			firstGameTime_ = gameTime;
		}

		lastGameTime_ = gameTime;
		++numTicksRead_;

		MemoryIStream tickData( payload.retrieve( dataLength ),
			dataLength );

		if (payload.error())
		{
			payload.finish();
			
			listener_.onReplayDataFileReaderError( parent_,
				IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
				"Could not read tick data" );
			lastError_ = "File corrupted: Could not read tick data";
			return false;
		}

		bool success = false;
		BW::string errorString;

		if (shouldDecompress_)
		{
			CompressionIStream compressionStream( tickData );
			BinaryIStream & decompressedStream = compressionStream;
			success = listener_.onReplayDataFileReaderTickData( parent_,
					gameTime, /* isCompressed */ !shouldDecompress_, 
					decompressedStream, errorString );
		}
		else
		{
			success = listener_.onReplayDataFileReaderTickData( parent_,
					gameTime, /* isCompressed */ !shouldDecompress_, 
					tickData, errorString );
		}

		tickData.finish();
		
		if (!success)
		{
			lastError_ = errorString;
			return false;
		}
	}

	return true;
}


/**
 *	This method handles a read error and informs the listener.
 *
 *	If the checksum scheme reports an error, this is reported back to the
 *	listener. If checksum scheme does not report an error, then a general
 *	corruption error is raised with the listener.
 */
void ReplayDataFileReader::Impl::handleReadError(
		const BW::string & errorString )
{
	if (pChecksumScheme_.exists() && !pChecksumScheme_->isGood())
	{
		listener_.onReplayDataFileReaderError( parent_,
			IReplayDataFileReaderListener::ERROR_SIGNATURE_MISMATCH,
			errorString );
	}
	else
	{
		listener_.onReplayDataFileReaderError( parent_,
			IReplayDataFileReaderListener::ERROR_FILE_CORRUPTED,
			errorString );
	}

	lastError_ = errorString;
}


/**
 *	This method primes the checksum stream for the next chunk.
 */
void ReplayDataFileReader::Impl::primeChecksumScheme()
{
	MF_ASSERT( pChecksumScheme_.exists() );

	pChecksumScheme_->reset();

	const size_t signatureLength = pChecksumScheme_->streamSize();
	MF_ASSERT( static_cast<size_t>( lastSignature_.remainingLength() ) == 
		signatureLength );

	pChecksumScheme_->readBlob( lastSignature_.retrieve( 
			static_cast<int>(signatureLength) ),
		signatureLength );
}


/**
 *	This method returns whether an error has occurred while reading replay
 *	data.
 */
bool ReplayDataFileReader::hasError() const
{
	return pImpl_->hasError();
}


/**
 *	This method returns a description of the last error that occurred.
 */
const BW::string & ReplayDataFileReader::lastError() const
{
	return pImpl_->lastError();
}


/**
 *	This method clears the last error, allowing for further calls to addData().
 */
void ReplayDataFileReader::clearError()
{
	pImpl_->clearError();
}


/**
 *	This method returns whether a header has been read from the replay data
 *	file.
 */
bool ReplayDataFileReader::hasReadHeader() const
{
	return pImpl_->hasReadHeader();
}


/**
 *	This method returns the header read from the file.
 */
const ReplayHeader & ReplayDataFileReader::header() const
{
	return pImpl_->header();
}


/**
 *	This method returns whether meta-data has been read.
 */
bool ReplayDataFileReader::hasReadMetaData() const
{
	return pImpl_->hasReadMetaData();
}


/**
 *	This method returns the read meta-data.
 */
const ReplayMetaData & ReplayDataFileReader::metaData() const
{
	return pImpl_->metaData();
}


/**
 *	This method returns the first game time read from the file. If no tick data
 *	has been read, this will return 0.
 */
GameTime ReplayDataFileReader::firstGameTime() const
{
	return pImpl_->firstGameTime();
}


/**
 *	This method returns the number of ticks read.
 */
uint ReplayDataFileReader::numTicksRead() const
{
	return pImpl_->numTicksRead();
}


/**
 *	This method returns the number of bytes read and processed.
 */
size_t ReplayDataFileReader::numBytesRead() const
{
	return pImpl_->numBytesRead();
}


/**
 *	This method returns the number of signed chunks read. The header counts for
 *	one chunk.
 */
uint ReplayDataFileReader::numChunksRead() const
{
	return pImpl_->numChunksRead();
}


/**
 *	This method returns the number of bytes added for processing.
 */
size_t ReplayDataFileReader::numBytesAdded() const
{
	return pImpl_->numBytesAdded();
}


/**
 *	This method returns the byte position that signatures have been verified
 *	to.
 */
size_t ReplayDataFileReader::verifiedToPosition() const
{
	return pImpl_->verifiedToPosition();
}


/**
 *	This method returns the number of signed chunks that have had their
 *	signature verified. The header counts for one chunk.
 */
uint ReplayDataFileReader::numChunksVerified() const
{
	return pImpl_->numChunksVerified();
}


/**
 *	This method returns the offset of the numTicks field in the file header.
 */
size_t ReplayDataFileReader::numTicksFieldOffset() const
{
	return pImpl_->numTicksFieldOffset();
}


/**
 *	This method gives a human-readable string description of the given error
 *	code.
 *
 *	@param errorType	One of the values of the
 *						IReplayDataFileReaderListener::ErrorType enum.
 */
const char * IReplayDataFileReaderListener::errorTypeToString(
		int errorType )
{
	switch (errorType)
	{
	case ERROR_NONE:
		return "No error";
	case ERROR_KEY_ERROR:
		return "Key error";
	case ERROR_SIGNATURE_MISMATCH:
		return "Signature mismatch";
	case ERROR_FILE_CORRUPTED:
		return "File corrupted";
	default:
		return "Unknown error";
	}
}


BW_END_NAMESPACE

// replay_data_file_reader.cpp
