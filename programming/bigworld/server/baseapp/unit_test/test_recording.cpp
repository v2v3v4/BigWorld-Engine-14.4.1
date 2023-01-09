#include "pch.hpp"

#include "../replay_data_file_writer.hpp"

#include "connection/replay_controller.hpp"
#include "connection/replay_data.hpp"
#include "connection/replay_data_file_reader.hpp"
#include "connection/replay_header.hpp"
#include "connection/replay_metadata.hpp"
#include "connection/replay_tick_loader.hpp"

#include "network/compression_stream.hpp"
#include "network/sha_checksum_scheme.hpp"
#include "network/signed_checksum_scheme.hpp"

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/timestamp.hpp"

#include <sys/stat.h>
#include <sys/time.h>

BW_BEGIN_NAMESPACE

namespace  // (anonymous)
{

// This key was generated using a prime256v1: X9.62/SECG curve over a 256 bit
// prime field.
//
// Generated using OpenSSL like so:
//
// openssl ecparam -name prime256v1 -genkey > ec-private.key
// openssl ec -in ec-private.key -pubout > ec-public.key


const BW::string PRIVATE_KEY =
"-----BEGIN EC PRIVATE KEY-----\n\
MIGkAgEBBDDPo2svU5j9+Cdd17e13D90CIHDGUgdDu1AnPkqXW7PxvaNRujhWloU\n\
4vQ4ND5NquCgBwYFK4EEACKhZANiAASM5SVuxBBdyM89W24v5AM834DYwbDNFv0S\n\
NlNI88jJrEH03VfCCRHXdCBqBpHGRbJ0db6vyABPbQFB4QpoGLvCgaYoC/whUf22\n\
XRCxr7Wpg+Rqdx6JNXlb+IUPyf0fnys=\n\
-----END EC PRIVATE KEY-----";


const BW::string PUBLIC_KEY =
"-----BEGIN PUBLIC KEY-----\n\
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEjOUlbsQQXcjPPVtuL+QDPN+A2MGwzRb9\n\
EjZTSPPIyaxB9N1XwgkR13QgagaRxkWydHW+r8gAT20BQeEKaBi7woGmKAv8IVH9\n\
tl0Qsa+1qYPkanceiTV5W/iFD8n9H58r\n\
-----END PUBLIC KEY-----";

const BW::string PUBLIC_KEY_OTHER =
"-----BEGIN PUBLIC KEY-----\n\
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE8Ol7qFGn4FilfY6bRBCkN93+fD2PIMN5\n\
VRuGWibFlDA1UktBPksjSjIvsDg772UxTcvPgDBe6r6rwhlVA0H95/LrVCQB0ctF\n\
YaXqxzDVkfYAQTLrhusJ9Gsl9Bp5FUx0\n\
-----END PUBLIC KEY-----";


const BW::string NONCE = "PICKLES!"; 	// should be 8 bytes (NONCE_SIZE)
										// excluding null terminator.

const uint GAME_UPDATE_FREQUENCY = 10U;
const uint32 NUM_SAMPLE_TICK_DATA_INTS = 20;
const GameTime STARTING_TICK = 100;
const GameTime NUM_TICKS_TO_WRITE = 80;
const GameTime NUM_TICKS_TO_RECOVER_AT = 40;
const GameTime NUM_TICKS_TO_BE_OVERWRITTEN = 15;

const char TEST_TEXT[] = "Hello, World!";
const uint TEST_USER_DATA[] = {
	0xdeadbeef,
	0xcafefeed
};

const BW::string TEST_METADATA_KEY = "Hello";
const BW::string TEST_METADATA_VALUE = "World";


} // end namespace (anonymous)

/**
 *	This class mocks the background file writer interface. All data written is
 *	redirected to a memory buffer.
 */
class MockBackgroundFileWriter : public IBackgroundFileWriter
{
public:
	MockBackgroundFileWriter( const char * existingContents = NULL,
		size_t existingContentsLength = 0U );

	~MockBackgroundFileWriter();

	void processRequestCallbacks();

	/**
	 *	Return the buffer data.
	 */
	const char * bufferData() { return &(buffer_[0]); }

	/**
	 *	Return the buffer size.
	 */
	size_t bufferSize() const { return buffer_.size(); }


	/**
	 *	The mode of the mocked file.
	 */
	mode_t mode() { return mode_; }

	static uint numInstances();

	// Overrides from IBackgroundFileWriter
	virtual const BW::string & path() const;
	virtual void closeFile();
	virtual void queueWrite( BinaryIStream & stream, int userData = 0 );
	virtual void queueSeek( long offset, int whence, int userData = 0 );
	virtual void queueChmod( int mode, int userData = 0 );
	virtual void setListener( BackgroundFileWriterListener * pListener )
	{
		pListener_ = pListener;
	}
	virtual bool hasError() const { return false; }
	virtual BW::string errorString() const { return ""; }

private:
	void queueRequest( int userData );
	char * writePosition();

	BackgroundFileWriterListener * 	pListener_;
	BW::vector< char > 				buffer_;
	size_t 							writeCursor_;
	mode_t 							mode_;

	typedef std::pair< long, int > 	Request;
	typedef BW::vector< Request > 	Requests;
	Requests 						queuedRequests_;

	static uint 					s_numInstances_;

};


uint MockBackgroundFileWriter::s_numInstances_ = 0;


/**
 *	Constructor.
 *
 *	@param existingContents 		If not NULL, this is used to mock an
 *									existing file's contents.
 *	@param existingContentsLength 	The length of the existing contents.
 */
MockBackgroundFileWriter::MockBackgroundFileWriter(
		const char * existingContents,
		size_t existingContentsLength ) :
	IBackgroundFileWriter(),
	buffer_(),
	writeCursor_( 0 ),
	mode_( 0666 ),		// read and write bits set
	queuedRequests_()
{
	buffer_.resize( existingContentsLength );
	memcpy( &(buffer_.front()), existingContents, existingContentsLength );

	++s_numInstances_;
}


/**
 *	Destructor.
 */
MockBackgroundFileWriter::~MockBackgroundFileWriter()
{
	--s_numInstances_;
}


/**
 *	This method returns the number of instances.
 */
uint MockBackgroundFileWriter::numInstances()
{
	return s_numInstances_;
}


/*
 *	Override from IBackgroundFileWriter.
 */
const BW::string & MockBackgroundFileWriter::path() const
{
	static BW::string s_path( "MockBackgroundFileWriter" );
	return s_path;
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::closeFile()
{
	buffer_.clear();
	queuedRequests_.clear();
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::queueWrite( BinaryIStream & stream,
		int userData )
{
	this->queueRequest( userData );

	int streamSize = stream.remainingLength();

	if ((writeCursor_ + streamSize) > buffer_.size())
	{
		buffer_.resize( writeCursor_ + streamSize );
	}

	void * writePos = this->writePosition();
	memcpy( writePos, stream.retrieve( streamSize ), streamSize );
	writeCursor_ += streamSize;
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::queueSeek( long offset, int whence,
		int userData )
{
	this->queueRequest( userData );
	switch (whence)
	{
	case SEEK_SET:
		writeCursor_ = offset;
		break;
	case SEEK_END:
		writeCursor_ = buffer_.size() - offset;
		break;
	case SEEK_CUR:
		writeCursor_ += offset;
		break;
	default:
		if (pListener_)
		{
			pListener_->onBackgroundFileWritingError( *this );
		}
		break;
	}
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::queueChmod( int mode, int userData )
{
	this->queueRequest( userData );
	mode_ = mode;
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::queueRequest( int userData )
{
	long filePosition = writeCursor_;
	queuedRequests_.push_back( Request( filePosition, userData ) );
}


/*
 *	Override from IBackgroundFileWriter.
 */
char * MockBackgroundFileWriter::writePosition()
{
	return const_cast< char * >( &buffer_[writeCursor_] );
}


/*
 *	Override from IBackgroundFileWriter.
 */
void MockBackgroundFileWriter::processRequestCallbacks()
{
	Requests requests;
	requests.swap( queuedRequests_ );

	Requests::const_iterator iRequest = requests.begin();

	while ((pListener_ != NULL) && (iRequest != requests.end()))
	{
		const Request & request = *iRequest;

		pListener_->onBackgroundFileWritingComplete( *this,
			request.first, request.second );

		++iRequest;
	}
}


/**
 *	This class mocks a listener class that stores away the user data values
 *	that are given to the completion callbacks for later query and verification
 *	by unit tests.
 */
class MockBackgroundFileWriterListener : public BackgroundFileWriterListener
{
public:
	/**
	 *	Constructor.
	 */
	MockBackgroundFileWriterListener() :
		hasError_( false ),
		userDataValues_()
	{}

	/**
	 *	Destructor.
	 */
	virtual ~MockBackgroundFileWriterListener()
	{}


	/* Override from BackgroundFileWriterListener. */
	virtual void onBackgroundFileWritingError(
		IBackgroundFileWriter & writer )
	{
		hasError_ = true;
	}

	/* Override from BackgroundFileWriterListener. */
	virtual void onBackgroundFileWritingComplete(
		IBackgroundFileWriter & writer, long filePosition,
		int userData )
	{
		userDataValues_.push_back( UserDataValue( filePosition, userData ) );
	}

	bool hasError() const 	{ return hasError_; }
	void clearError() 		{ hasError_ = false; }

	typedef std::pair< long, int > UserDataValue;
	typedef BW::vector< UserDataValue > UserDataValues;

	const UserDataValues & userDataValues() const { return userDataValues_; }

private:
	bool 			hasError_;
	UserDataValues 	userDataValues_;
};


/**
 *	This class mocks a listener for a ReplayDataFileReader. It collects the
 *	calls for receiving the header and the tick data.
 */
class MockReplayDataFileReaderListener : public IReplayDataFileReaderListener
{
public:

	/**
	 *	Constructor.
	 */
	MockReplayDataFileReaderListener() :
			IReplayDataFileReaderListener(),
			header_( NULL ),
			metaData_(),
			tickData_(),
			errorType_( ERROR_NONE ),
			errorDescription_(),
			isReaderDestroyed_( false )
	{}


	/**
	 *	Destructor.
	 */
	virtual ~MockReplayDataFileReaderListener()
	{
		for (const_iterator iTickData = this->begin();
				iTickData != this->end();
				++iTickData)
		{
			delete iTickData->second;
		}
	}


	/* Override from IReplayDataFileReaderListener. */
	virtual bool onReplayDataFileReaderHeader( ReplayDataFileReader & reader,
		const ReplayHeader & header, BW::string & errorString )
	{
		header_ = header;
		return true;
	}

	/* Override from IReplayDataFileReaderListener. */
	virtual bool onReplayDataFileReaderMetaData( ReplayDataFileReader & reader,
		const ReplayMetaData & metaData, BW::string & errorString )
	{
		metaData_ = metaData;
		return true;
	}

	/* Override from IReplayDataFileReaderListener. */
	virtual bool onReplayDataFileReaderTickData( ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString )
	{
		MemoryOStream * pStream = new MemoryOStream( stream.remainingLength() );
		pStream->transfer( stream, stream.remainingLength() );
		tickData_.push_back( std::make_pair( time, pStream ) );

		return true;
	}


	/* Override from IReplayDataFileReaderListener. */
	virtual void onReplayDataFileReaderError( ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription )
	{
		errorType_ = errorType;
		errorDescription_ = errorDescription;
	}


	/* Override from IReplayDataFileReaderListener. */
	virtual void onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader )
	{
		isReaderDestroyed_ = true;
	}

	bool hasError() const 				{ return errorType_ != ERROR_NONE; }

	const BW::string & errorDescription() const { return errorDescription_; }

	const ReplayHeader & header() const { return header_; }

	typedef BW::vector< std::pair< GameTime, MemoryOStream * > > TickData;
	typedef TickData::const_iterator const_iterator;

	const_iterator begin() const 		{ return tickData_.begin(); }
	const_iterator end() const 			{ return tickData_.end(); }

	bool isReaderDestroyed() const 		{ return isReaderDestroyed_; }

private:
	ReplayHeader 	header_;
	ReplayMetaData 	metaData_;
	TickData 		tickData_;
	ErrorType 		errorType_;
	BW::string 		errorDescription_;
	bool 			isReaderDestroyed_;
};




/**
 *	Fixture for recording class unit tests.
 */
class Fixture
{
public:
	Fixture();


	/**
	 *	Return the file writer.
	 */
	MockBackgroundFileWriter & fileWriter()
	{
		return static_cast< MockBackgroundFileWriter & >( *pFileWriter_ );
	}

	/**
	 *	Reset the file writer to a new instance.
	 *
	 *	@param pNewWriter 	The new writer instance.
	 */
	void setFileWriter( MockBackgroundFileWriter * pNewWriter )
	{
		pFileWriter_ = pNewWriter;
	}

protected:
	void addSampleTickData( ReplayDataFileWriter & writer, GameTime tick );
	bool verifySampleTickData( BinaryIStream & stream, GameTime tick );


	// Unit tests have access to these pre-initialised objects for their tests.
	MockBackgroundFileWriterListener 	listener_;

	ChecksumSchemePtr 					pPrivateScheme_;
	ChecksumSchemePtr 					pPublicScheme_;
	ReplayMetaData						metaData_;
	MD5::Digest 						digest_;

private:
	IBackgroundFileWriterPtr 			pFileWriter_;
};


/**
 *	Constructor.
 */
Fixture::Fixture():
		listener_(),
		pPrivateScheme_( ReplayChecksumScheme::create( PRIVATE_KEY,
			/* isPrivate */ true ) ),
		pPublicScheme_( ReplayChecksumScheme::create( PUBLIC_KEY,
			/* isPrivate */ false ) ),
		metaData_(),
		digest_(),
		pFileWriter_( new MockBackgroundFileWriter() )
{
	this->fileWriter().setListener( &listener_ );

	MD5 md5;
	// As good a source of test data as any...
	md5.append( PRIVATE_KEY.data(), sizeof( PRIVATE_KEY ) );
	md5.getDigest( digest_ );

	metaData_.init( pPrivateScheme_ );

	metaData_.add( TEST_METADATA_KEY, TEST_METADATA_VALUE );
}


/**
 *	This method adds sample tick data to the file writer.
 *
 *	@param writer 	The replay data file writer
 *	@param tick 	The game time of the tick to write.
 */
void Fixture::addSampleTickData( ReplayDataFileWriter & writer, GameTime tick )
{
	MemoryOStream buffer( NUM_SAMPLE_TICK_DATA_INTS * sizeof(uint32) );

	buffer << tick; // This is added to the payload verify that the tick added
					// is the tick we expect.

	for (uint32 i = 0; i < NUM_SAMPLE_TICK_DATA_INTS; ++i)
	{
		buffer << i;
	}

	writer.addTickData( tick, 1, buffer );
}


/**
 *	This method verifies the sample tick data from the given stream.
 *
 *	@param stream 	The stream containing the tick data to verify.
 *	@param tick 	The game time that it should match up to.
 */
bool Fixture::verifySampleTickData( BinaryIStream & stream, GameTime tick )
{
	GameTime streamTick;
	stream >> streamTick;
	if (streamTick != tick)
	{
		return false;
	}

	for (uint32 i = 0; i < NUM_SAMPLE_TICK_DATA_INTS; ++i)
	{
		uint32 j = 0;
		stream >> j;
		if (i != j)
		{
			return false;
		}
	}

	return true;
}



TEST_F( Fixture, MockBackgroundFileWriter_Basic )
{
	// Queue up some writes.
	this->fileWriter().queueWriteBlob( TEST_TEXT, sizeof( TEST_TEXT ),
		int(TEST_USER_DATA[0]) );
	this->fileWriter().processRequestCallbacks();

	CHECK_EQUAL( 1UL, listener_.userDataValues().size() );
	CHECK_EQUAL( 0L, listener_.userDataValues().back().first );
	CHECK_EQUAL( TEST_USER_DATA[0],
		uint(listener_.userDataValues().back().second) );

	MemoryOStream writeStream;
	static const int first = 1;
	static const float second = 2.f;
	static const BW::string third = "three";

	writeStream << first << second << third;
	this->fileWriter().queueWrite( writeStream, int(TEST_USER_DATA[1]) );

	this->fileWriter().processRequestCallbacks();

	// Check the write callbacks were called.

	CHECK_EQUAL( 2UL, listener_.userDataValues().size() );
	CHECK_EQUAL( long( sizeof( TEST_TEXT ) ),
		listener_.userDataValues().back().first );
	CHECK_EQUAL( TEST_USER_DATA[1],
		uint(listener_.userDataValues().back().second) );

	// Check the correct data was written.
	MemoryIStream readStream( this->fileWriter().bufferData(),
		this->fileWriter().bufferSize() );
	const void * pData = readStream.retrieve( sizeof( TEST_TEXT ) );

	CHECK( !readStream.error() );
	CHECK( 0 == memcmp( pData, TEST_TEXT, sizeof( TEST_TEXT ) ) );

	int firstStream;
	float secondStream;
	BW::string thirdStream;
	readStream >> firstStream >> secondStream >> thirdStream;

	CHECK( !readStream.error() );
	CHECK( readStream.remainingLength() == 0 );
	CHECK_EQUAL( first, firstStream );
	CHECK_EQUAL( second, secondStream );
	CHECK_EQUAL( third, thirdStream );
}


TEST_F( Fixture, ReplayChecksumScheme_Basic )
{
	CHECK( pPrivateScheme_.get() != NULL );

	// Stream some test data, and sign it using the replay checksum scheme.

	MemoryOStream baseStream;
	MemoryOStream signature;
	int payloadSize = 0;

	{
		ChecksumOStream stream( baseStream, pPrivateScheme_ );

		for (uint32 i = 0; i < NUM_SAMPLE_TICK_DATA_INTS; ++i)
		{
			stream << i;
		}

		payloadSize = baseStream.size();
		stream.finalise( &signature );
	}

	size_t checksumSize = baseStream.size() - payloadSize;

	CHECK_EQUAL( pPrivateScheme_->streamSize(), checksumSize );
	CHECK_EQUAL( size_t( signature.remainingLength() ), checksumSize );
	signature.finish();

	CHECK_EQUAL( checksumSize + payloadSize, size_t(baseStream.size()) );

	// Use the public-key-only checksum scheme to do verification.
	{
		ChecksumIStream verifyStream( baseStream, pPublicScheme_ );

		for (uint32 i = 0; i < NUM_SAMPLE_TICK_DATA_INTS; ++i)
		{
			uint32 j = 0;
			verifyStream >> j;
			CHECK( !verifyStream.error() );
			CHECK_EQUAL( i, j );
		}

		CHECK( verifyStream.verify() );
		CHECK( !verifyStream.error() );
		CHECK( 0 == baseStream.remainingLength() );
		CHECK( !baseStream.error() );
	}

	baseStream.rewind();

	// Make sure that the wrong key will not verify
	{
		ChecksumSchemePtr pWrongPublicScheme(
			ReplayChecksumScheme::create( PUBLIC_KEY_OTHER,
				/* isPrivate */ false ) );
		CHECK( pWrongPublicScheme );

		ChecksumIStream verifyStream( baseStream, pWrongPublicScheme );
		CHECK( !verifyStream.error() );

		for (uint32 i = 0; i < NUM_SAMPLE_TICK_DATA_INTS; ++i)
		{
			uint32 j = 0;
			verifyStream >> j;
			CHECK( !verifyStream.error() );
			CHECK_EQUAL( i, j );
		}

		CHECK( !verifyStream.verify() );
		CHECK( verifyStream.error() );
		CHECK( 0 == baseStream.remainingLength() );
		CHECK( !baseStream.error() );
	}
}


TEST_F( Fixture, ReplayHeader_Basic )
{
	ReplayHeader replayHeader( pPrivateScheme_, digest_,
		GAME_UPDATE_FREQUENCY, NONCE );

	MemoryOStream headerStream;
	MemoryOStream signatureWritten;

	// Manual inspection
	ChecksumSchemePtr pScheme = ReplayChecksumScheme::create( PRIVATE_KEY,
		/* isPrivate */ true );

	replayHeader.addToStream( headerStream, &signatureWritten );

	ClientServerProtocolVersion version;
	MD5::Digest streamDigest;
	uint8 streamGameUpdateFrequency;
	BW::string streamNonce;
	ReplayData::HeaderSignatureLength signatureLength;
	ReplayData::Timestamp timestamp;

	{
		ChecksumIStream checkSummedStream( headerStream, pPublicScheme_ );
		checkSummedStream >> version >> streamDigest >>
			streamGameUpdateFrequency >> timestamp;

		streamNonce.assign(
			(char *) checkSummedStream.retrieve( ReplayHeader::NONCE_SIZE ),
			ReplayHeader::NONCE_SIZE );

		checkSummedStream >> signatureLength;

		CHECK(
			ClientServerProtocolVersion::currentVersion().supports( version ) );
		CHECK( digest_ == streamDigest );
		CHECK_EQUAL( GAME_UPDATE_FREQUENCY, streamGameUpdateFrequency );
		CHECK_EQUAL( NONCE, streamNonce );
		CHECK_EQUAL( pPrivateScheme_->streamSize(), signatureLength );

		timeval tv;
		gettimeofday( &tv, NULL ); 
		CHECK( (static_cast<ReplayData::Timestamp>( tv.tv_sec ) - 
			timestamp) < 10 );
		CHECK( checkSummedStream.verify() );
	}

	GameTime numTicks;
	headerStream >> numTicks;
	CHECK_EQUAL( 0U, numTicks );

	// De-streaming using ReplayHeader
	headerStream.rewind();

	ReplayHeader streamHeader( pPublicScheme_ );

	MemoryOStream signatureRead;
	BW::string errorString;
	if (!streamHeader.initFromStream( headerStream, &signatureRead,
			&errorString ))
	{
		FAIL( errorString.c_str() );
	}

	CHECK( version.supports( streamHeader.version() ) );
	CHECK( digest_ == streamHeader.digest() );
	CHECK_EQUAL( GAME_UPDATE_FREQUENCY, streamHeader.gameUpdateFrequency() );

	CHECK_EQUAL( pPublicScheme_->streamSize(),
		size_t( signatureRead.remainingLength() ) );
	CHECK_EQUAL( pPrivateScheme_->streamSize(),
		size_t( signatureRead.remainingLength() ) );

	CHECK( 0 == memcmp(
		signatureRead.retrieve( pPublicScheme_->streamSize() ),
		signatureWritten.retrieve( pPrivateScheme_->streamSize() ),
		pPublicScheme_->streamSize() ) );
}


TEST_F( Fixture, ReplayDataFileWriter_Basic )
{

	// Let's write some ticks.
	//{
		ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
			BW_COMPRESSION_ZIP_BEST_SPEED, digest_, 1, metaData_, NONCE );

		for (GameTime t = STARTING_TICK;
				t < (STARTING_TICK + NUM_TICKS_TO_WRITE); ++t)
		{
			this->addSampleTickData( writer, t );
			this->fileWriter().processRequestCallbacks();
		}

		writer.close( /* shouldFinalise*/ true );

		this->fileWriter().processRequestCallbacks();

		CHECK_EQUAL( 0U, (this->fileWriter().mode() &
			(S_IWUSR | S_IWGRP | S_IWOTH)));
	//}

	// Great! Let's analyse the results.
	//{
		MemoryIStream fileStream( this->fileWriter().bufferData(),
			this->fileWriter().bufferSize() );

		ReplayHeader replayHeader( pPublicScheme_ );

		// Each tick's digest also includes the last tick's signature as part
		// of its data (added at the front of the data). Check that using
		// lastSignature here.
		MemoryOStream lastSignature;

		BW::string errorString;
		if (!replayHeader.initFromStream( fileStream, &lastSignature,
				&errorString ) )
		{
			FAIL( (BW::string( "Failed reading replay header: " ) + 
				errorString).c_str() );
		}

		CHECK( errorString.empty() );
		CHECK_EQUAL( NUM_TICKS_TO_WRITE, replayHeader.numTicks() );
		CHECK_EQUAL( pPublicScheme_->streamSize(),
			size_t( lastSignature.remainingLength() ) );

		ReplayMetaData metaData;
		pPublicScheme_->reset();
		pPublicScheme_->readBlob( lastSignature.retrieve( 
				pPublicScheme_->streamSize() ),
			pPublicScheme_->streamSize() );
		metaData.init( pPublicScheme_ );
		lastSignature.reset();

		if (!metaData.readFromStream( fileStream, &lastSignature, 
				&errorString ))
		{
			FAIL( (BW::string( "Failed reading replay meta-data: " ) + 
				errorString).c_str() );
		}

		metaData_.swap( metaData );

		CHECK( metaData_.hasKey( TEST_METADATA_KEY ) );
		CHECK_EQUAL( TEST_METADATA_VALUE, metaData_.get( TEST_METADATA_KEY ) );

		CHECK_EQUAL( pPublicScheme_->streamSize(), 
			size_t( lastSignature.remainingLength() ) );
		CHECK( errorString.empty() );

		GameTime t = STARTING_TICK;
		while (fileStream.remainingLength())
		{
			pPublicScheme_->reset();
			pPublicScheme_->readBlob(
				lastSignature.retrieve( pPublicScheme_->streamSize() ),
				pPublicScheme_->streamSize() );
			lastSignature.reset();
			ChecksumIStream chunkStream( fileStream,
				pPublicScheme_, /* shouldReset = */ false );

			ReplayData::SignedChunkLength chunkLength;
			chunkStream >> chunkLength;

			MemoryIStream rawTickStream(
				chunkStream.retrieve( chunkLength ), chunkLength );

			while (rawTickStream.remainingLength())
			{
				GameTime tickNum;
				ReplayData::TickDataLength tickLength;
				rawTickStream >> tickNum >> tickLength;

				CHECK_EQUAL( t, tickNum );

				CompressionIStream compressedTickStream( rawTickStream );

				CHECK( this->verifySampleTickData( compressedTickStream, t ) );
				++t;
			}

			CHECK( chunkStream.verify( &lastSignature ) );
		}

		CHECK_EQUAL( STARTING_TICK + NUM_TICKS_TO_WRITE, t );

		CHECK( fileStream.remainingLength() == 0 );
	//}
}


TEST( RecordingRecoveryData )
{
	static const GameTime LAST_GAME_TIME = 1234;
	static const off_t FILE_POSITION = 0xcafebeef;
	static const BW::string LAST_SIGNATURE =
		"This is not a real signature";
	static const uint NUM_TICKS_TO_SIGN = 10;

	RecordingRecoveryData recoveryData( BW_COMPRESSION_ZIP_BEST_SPEED,
		NUM_TICKS_TO_RECOVER_AT, LAST_GAME_TIME, FILE_POSITION,
		LAST_SIGNATURE, NUM_TICKS_TO_SIGN );

	BW::string recoveryDataString = recoveryData.toString();

	RecordingRecoveryData recoveryData2;
	CHECK( recoveryData2.initFromString( recoveryDataString ) );

	CHECK_EQUAL( BW_COMPRESSION_ZIP_BEST_SPEED,
		recoveryData2.compressionType() );
	CHECK_EQUAL( NUM_TICKS_TO_RECOVER_AT, recoveryData2.numTicks() );
	CHECK_EQUAL( LAST_GAME_TIME, recoveryData2.lastTickWritten() );
	CHECK_EQUAL( FILE_POSITION, recoveryData2.nextTickPosition() );
	CHECK_EQUAL( LAST_SIGNATURE, recoveryData2.lastSignature() );
	CHECK_EQUAL( NUM_TICKS_TO_SIGN, recoveryData2.numTicksToSign() );

}


TEST_F( Fixture, ReplayWriter_Recovery )
{
	BW::string recoveryDataString;

	{
		ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
			BW_COMPRESSION_ZIP_BEST_SPEED, digest_, 1, metaData_, NONCE );

		for (GameTime t = STARTING_TICK;
				t < STARTING_TICK + NUM_TICKS_TO_RECOVER_AT;
				++t)
		{
			this->addSampleTickData( writer, t );
			this->fileWriter().processRequestCallbacks();
		}

		recoveryDataString = writer.recoveryData().toString();
		CHECK( !recoveryDataString.empty() );

		for (GameTime t = (STARTING_TICK + NUM_TICKS_TO_RECOVER_AT);
				t < (STARTING_TICK + NUM_TICKS_TO_RECOVER_AT +
						NUM_TICKS_TO_BE_OVERWRITTEN);
				++t)
		{
			this->addSampleTickData( writer, t );
			this->fileWriter().processRequestCallbacks();
		}


		writer.close( /* shouldFinalise */ false );

		this->fileWriter().processRequestCallbacks();

		CHECK( 0U != (this->fileWriter().mode() &
			(S_IWUSR | S_IWGRP | S_IWOTH)));
	}

	BW::string fileContents( this->fileWriter().bufferData(),
		this->fileWriter().bufferSize() );
	this->setFileWriter( new MockBackgroundFileWriter( fileContents.data(),
		fileContents.size() ) );

	{
		RecordingRecoveryData recoveryData;
		CHECK( recoveryData.initFromString( recoveryDataString ) );

		ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
			recoveryData );

		for (GameTime t = recoveryData.lastTickWritten() + 1;
				t < (STARTING_TICK + NUM_TICKS_TO_WRITE);
				++t)
		{
			this->addSampleTickData( writer, t );
			this->fileWriter().processRequestCallbacks();
		}

		writer.close( /* shouldFinalise */ true );

		this->fileWriter().processRequestCallbacks();

		CHECK( 0U == (this->fileWriter().mode() &
			(S_IWUSR | S_IWGRP | S_IWOTH)));
	}


	// Verify!

	//{
		MemoryIStream fileStream( this->fileWriter().bufferData(),
			this->fileWriter().bufferSize() );

		ReplayHeader replayHeader( pPublicScheme_ );

		// Each tick's digest also includes the last tick's signature as part
		// of its data (added at the front of the data). Check that using
		// lastSignature here.
		MemoryOStream lastSignature;

		BW::string errorString;
		CHECK( replayHeader.initFromStream( fileStream, &lastSignature,
			&errorString ) );
		CHECK( errorString.empty() );
		CHECK_EQUAL( NUM_TICKS_TO_WRITE, replayHeader.numTicks() );
		CHECK_EQUAL( pPublicScheme_->streamSize(),
			size_t( lastSignature.remainingLength() ) );

		ReplayMetaData metaData;
		pPublicScheme_->reset();
		pPublicScheme_->readBlob( lastSignature.retrieve( 
				pPublicScheme_->streamSize() ),
			pPublicScheme_->streamSize() );
		metaData.init( pPublicScheme_ );
		lastSignature.reset();

		CHECK( metaData.readFromStream( fileStream, &lastSignature, 
			&errorString ) );
		metaData_.swap( metaData );
		CHECK( metaData_.hasKey( TEST_METADATA_KEY ) );
		CHECK_EQUAL( TEST_METADATA_VALUE, metaData_.get( TEST_METADATA_KEY ) );

		CHECK_EQUAL( pPublicScheme_->streamSize(), 
			size_t( lastSignature.remainingLength() ) );
		CHECK( errorString.empty() );

		GameTime t = STARTING_TICK;
		while (fileStream.remainingLength())
		{
			pPublicScheme_->reset();
			pPublicScheme_->readBlob(
				lastSignature.retrieve( pPublicScheme_->streamSize() ),
				pPublicScheme_->streamSize() );
			lastSignature.reset();
			ChecksumIStream chunkStream( fileStream, pPublicScheme_,
				/* shouldReset = */ false );

			ReplayData::SignedChunkLength chunkLength;
			chunkStream >> chunkLength;

			MemoryIStream rawTickStream(
				chunkStream.retrieve( chunkLength ), chunkLength );

			while (rawTickStream.remainingLength())
			{
				GameTime tickNum;
				ReplayData::TickDataLength tickLength;
				rawTickStream >> tickNum >> tickLength;

				CHECK_EQUAL( t, tickNum );

				CompressionIStream compressedTickStream( rawTickStream );

				CHECK( this->verifySampleTickData( compressedTickStream, t ) );
				++t;
			}

			CHECK( chunkStream.verify( &lastSignature ) );
		}

		CHECK_EQUAL( STARTING_TICK + NUM_TICKS_TO_WRITE, t );

		CHECK( fileStream.remainingLength() == 0 );
	//}
}


TEST( MockBackgroundFileWriter_CheckLeak )
{
	// Just check that there are no more instances of MockBackgroundFileWriter
	// lying about.

	CHECK_EQUAL( 0U, MockBackgroundFileWriter::numInstances() );
}


TEST_F( Fixture, ReplayWriter_MultiTickSigning )
{
	// Let's make it co-prime to the number of ticks being written so we get a
	// remainder at the end.
	static const uint NUM_TICKS_TO_SIGN = 7;

	// Let's write some ticks.
	//{
		ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
			BW_COMPRESSION_ZIP_BEST_SPEED, digest_, NUM_TICKS_TO_SIGN,
			metaData_, NONCE );

		for (GameTime t = STARTING_TICK;
				t < (STARTING_TICK + NUM_TICKS_TO_WRITE); ++t)
		{
			this->addSampleTickData( writer, t );
			this->fileWriter().processRequestCallbacks();
		}

		writer.close( /* shouldFinalise*/ true );

		this->fileWriter().processRequestCallbacks();

		CHECK_EQUAL( 0U, (this->fileWriter().mode() &
			(S_IWUSR | S_IWGRP | S_IWOTH)));
	//}

	// Great! Let's analyse the results.
	//{
		MemoryIStream fileStream( this->fileWriter().bufferData(),
			this->fileWriter().bufferSize() );

		ReplayHeader replayHeader( pPublicScheme_ );

		// Each tick's digest also includes the last tick's signature as part
		// of its data (added at the front of the data). Check that using
		// lastSignature here.
		MemoryOStream lastSignature;

		BW::string errorString;
		CHECK( replayHeader.initFromStream( fileStream, &lastSignature,
			&errorString ) );
		CHECK( errorString.empty() );
		CHECK_EQUAL( NUM_TICKS_TO_WRITE, replayHeader.numTicks() );

		CHECK_EQUAL( pPublicScheme_->streamSize(),
			size_t( lastSignature.remainingLength() ) );

		ReplayMetaData metaData;
		pPublicScheme_->reset();
		pPublicScheme_->readBlob( lastSignature.retrieve( 
				pPublicScheme_->streamSize() ),
			pPublicScheme_->streamSize() );
		metaData.init( pPublicScheme_ );
		lastSignature.reset();

		CHECK( metaData.readFromStream( fileStream, &lastSignature, 
			&errorString ) );
		metaData_.swap( metaData );

		CHECK_EQUAL( pPublicScheme_->streamSize(), 
			size_t( lastSignature.remainingLength() ) );
		CHECK( errorString.empty() );

		GameTime t = STARTING_TICK;
		while (fileStream.remainingLength())
		{
			pPublicScheme_->reset();
			pPublicScheme_->readBlob(
				lastSignature.retrieve( pPublicScheme_->streamSize() ),
				pPublicScheme_->streamSize() );
			lastSignature.reset();
			ChecksumIStream chunkStream( fileStream,
				pPublicScheme_, /* shouldReset = */ false );

			ReplayData::SignedChunkLength chunkLength;
			chunkStream >> chunkLength;

			MemoryIStream rawTickStream(
				chunkStream.retrieve( chunkLength ), chunkLength );

			uint numTicksInChunk = 0;
			while (rawTickStream.remainingLength())
			{
				GameTime tickNum;
				ReplayData::TickDataLength tickLength;
				rawTickStream >> tickNum >> tickLength;

				CHECK_EQUAL( t, tickNum );

				CompressionIStream compressedTickStream( rawTickStream );

				CHECK( this->verifySampleTickData( compressedTickStream, t ) );
				++t;

				++numTicksInChunk;
			}

			if (chunkStream.remainingLength())
			{
				CHECK_EQUAL( NUM_TICKS_TO_SIGN, numTicksInChunk );
			}
			else
			{
				CHECK_EQUAL( NUM_TICKS_TO_WRITE % NUM_TICKS_TO_SIGN,
					numTicksInChunk );
			}

			CHECK( chunkStream.verify( &lastSignature ) );
		}

		CHECK_EQUAL( STARTING_TICK + NUM_TICKS_TO_WRITE, t );

		CHECK( fileStream.remainingLength() == 0 );
	//}
}


TEST_F( Fixture, ReplayDataFileReader_BasicReading )
{
	static const uint NUM_TICKS_TO_SIGN = 10;
	ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
		BW_COMPRESSION_ZIP_BEST_SPEED, digest_, NUM_TICKS_TO_SIGN, metaData_,
		NONCE );

	for (GameTime t = STARTING_TICK;
			t < (STARTING_TICK + NUM_TICKS_TO_WRITE); ++t)
	{
		this->addSampleTickData( writer, t );
		this->fileWriter().processRequestCallbacks();
	}

	writer.close( /* shouldFinalise*/ true );

	this->fileWriter().processRequestCallbacks();

	MemoryIStream fileStream( this->fileWriter().bufferData(),
		this->fileWriter().bufferSize() );

	MockReplayDataFileReaderListener listener;
	
	uint numBytesAdded = 0;
	{
		ReplayDataFileReader reader( listener, PUBLIC_KEY );

		static const int ADD_AMOUNT = 20;

		while (fileStream.remainingLength())
		{
			const int addAmount = std::min( ADD_AMOUNT,
				fileStream.remainingLength() );

			MemoryIStream blob( fileStream.retrieve( addAmount ), addAmount );
			if (!reader.addData( blob ))
			{
				ERROR_MSG( "Error while adding data to reader: %s\n",
					listener.errorDescription().c_str() );
				break;
			}

			numBytesAdded += addAmount;
			CHECK_EQUAL( numBytesAdded, reader.numBytesAdded() );
			CHECK( reader.numBytesRead() <= reader.numBytesAdded() );
		}

		if (listener.hasError())
		{
			FAIL( reader.lastError().c_str() );
		}

		CHECK( reader.hasReadHeader() );

		CHECK( reader.hasReadMetaData() );
		CHECK( reader.metaData().hasKey( TEST_METADATA_KEY ) );
		CHECK_EQUAL( TEST_METADATA_VALUE, 
			reader.metaData().get( TEST_METADATA_KEY ) );

		CHECK_EQUAL( NUM_TICKS_TO_WRITE, reader.numTicksRead() );
		CHECK_EQUAL( this->fileWriter().bufferSize(), reader.numBytesRead() );

		CHECK( !listener.isReaderDestroyed() );
	}

	CHECK( digest_ == listener.header().digest() );
	CHECK_EQUAL( NUM_TICKS_TO_WRITE, listener.header().numTicks() );

	CHECK( listener.isReaderDestroyed() );

	MockReplayDataFileReaderListener::const_iterator iTickData =
		listener.begin();
	GameTime nextExpectedTick = STARTING_TICK;

	while (iTickData != listener.end())
	{
		CHECK_EQUAL( nextExpectedTick, iTickData->first );
		this->verifySampleTickData( *(iTickData->second), iTickData->first );
		++nextExpectedTick;
		++iTickData;
	}

	CHECK_EQUAL( STARTING_TICK + NUM_TICKS_TO_WRITE, nextExpectedTick );

}



TEST_F( Fixture, ReplayDataFileReader_NoKey )
{
	static const uint NUM_TICKS_TO_SIGN = 10;
	ReplayDataFileWriter writer( &this->fileWriter(), 
		pPrivateScheme_, BW_COMPRESSION_ZIP_BEST_SPEED, digest_, 
		NUM_TICKS_TO_SIGN, metaData_, NONCE );

	for (GameTime t = STARTING_TICK;
			t < (STARTING_TICK + NUM_TICKS_TO_WRITE); ++t)
	{
		this->addSampleTickData( writer, t );
		this->fileWriter().processRequestCallbacks();
	}

	writer.close( /* shouldFinalise*/ true );

	this->fileWriter().processRequestCallbacks();

	MemoryIStream fileStream( this->fileWriter().bufferData(),
		this->fileWriter().bufferSize() );

	MockReplayDataFileReaderListener listener;

	{
		ReplayDataFileReader reader( listener, "" );

		static const int ADD_AMOUNT = 20;

		while (fileStream.remainingLength())
		{
			const int addAmount = std::min( ADD_AMOUNT,
				fileStream.remainingLength() );

			MemoryIStream blob( fileStream.retrieve( addAmount ), addAmount );
			if (!reader.addData( blob ))
			{
				ERROR_MSG( "Error while adding data to reader: %s\n",
					listener.errorDescription().c_str() );
				break;
			}
		}

		CHECK( !listener.hasError() );
		MF_ASSERT( NUM_TICKS_TO_WRITE == reader.numTicksRead() );

		CHECK( !listener.isReaderDestroyed() );
	}

	CHECK( digest_ == listener.header().digest() );
	CHECK_EQUAL( NUM_TICKS_TO_WRITE, listener.header().numTicks() );

	CHECK( listener.isReaderDestroyed() );

	MockReplayDataFileReaderListener::const_iterator iTickData =
		listener.begin();
	GameTime nextExpectedTick = STARTING_TICK;

	while (iTickData != listener.end())
	{
		CHECK_EQUAL( nextExpectedTick, iTickData->first );
		this->verifySampleTickData( *(iTickData->second), iTickData->first );
		++nextExpectedTick;
		++iTickData;
	}

	CHECK_EQUAL( STARTING_TICK + NUM_TICKS_TO_WRITE, nextExpectedTick );

}


/**
 *	This class mocks a file provider to provide file API to an in-memory
 *	buffer.
 */
class MockFileProvider : public IFileProvider
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param data 	The data to use as the mock file data.
	 */
	MockFileProvider( BinaryIStream & data ) :
			data_( data.remainingLength() ),
			isOpen_( false ),
			didLeakFileHandle_( false )
	{
		data_.transfer( data, data.remainingLength() );
	}

	virtual ~MockFileProvider() {}


	bool isOpen() const 				{ return isOpen_; }
	bool didLeakFileHandle() const 		{ return didLeakFileHandle_; }

	/* Override from IFileProvider. */
	virtual bool FOpen( const char * path, const char * mode )
	{
		if (isOpen_)
		{
			didLeakFileHandle_ = true;
		}

		data_.rewind();
		isOpen_ = true;

		return true;
	}


	/* Override from IFileProvider. */
	virtual void FClose()
	{
		isOpen_ = false;
	}


	/* Override from IFileProvider. */
	virtual size_t FRead( char * buffer, size_t memberSize,
		size_t numMembers )
	{
		size_t numMembersLeft = size_t(data_.remainingLength()) / memberSize;
		if (numMembers > numMembersLeft)
		{
			numMembers = numMembersLeft;
		}

		memcpy( buffer, data_.retrieve( numMembers * memberSize ),
			numMembers * memberSize );
		return numMembers;
	}


	/* Override from IFileProvider. */
	virtual int FError()
	{
		// We never return an error.
		return 0;
	}


	/* Override from IFileProvider. */
	virtual int FEOF() 
	{
		return (data_.remainingLength() > 0) ? 1 : 0;
	}


	/* Override from IFileProvider. */
	virtual void ClearErr()
	{
		// No-op.
	}

private:
	MemoryOStream 	data_;

	bool 			isOpen_;
	bool 			didLeakFileHandle_;
};


/**
 *	This class is used to get callbacks from a ReplayTickLoader.
 */
class MockReplayTickLoaderListener : public IReplayTickLoaderListener
{
public:
	typedef ReplayTickLoader::TickIndex 	TickIndex;
	typedef ReplayTickLoader::RequestType 	RequestType;
	typedef ReplayTickLoader::ErrorType 	ErrorType;

	struct ReplayTickLoaderCallback
	{
		ReplayTickLoaderCallback( RequestType aRequestType,
#if defined( DEBUG_TICK_DATA )
					TickIndex aStartIndex, TickIndex anEndIndex,
#endif  // defined( DEBUG_TICK_DATA )
					ReplayTickData * apStart,
					uint aNumTicks ) :
				requestType( aRequestType ),
#if defined( DEBUG_TICK_DATA )
				startIndex( aStartIndex ),
				endIndex( anEndIndex ),
#endif // defined( DEBUG_TICK_DATA )
				pStart( apStart ),
				numTicks( aNumTicks )
		{}

		RequestType requestType;
#if defined( DEBUG_TICK_DATA )
		TickIndex 	startIndex;
		TickIndex	endIndex;
#endif // defined( DEBUG_TICK_DATA )
		ReplayTickData * pStart;
		uint		numTicks;
	};

	typedef BW::vector< ReplayTickLoaderCallback > Callbacks;

	/**
	 *	Constructor.
	 *
	 *	@param pChecksumScheme 	The checksum scheme used.
	 */
	MockReplayTickLoaderListener( ChecksumSchemePtr pChecksumScheme ) :
			IReplayTickLoaderListener(),
			haveReadHeader_( false ),
			header_( pChecksumScheme ),
			callbacks_(),
			errorType_( ReplayTickLoader::ERROR_NONE ),
			errorString_()
	{}


	/** Destructor. */
	virtual ~MockReplayTickLoaderListener()
	{
		this->clear();
	}

	bool haveReadHeader() const 			{ return haveReadHeader_; }
	const ReplayHeader & header() const 	{ return header_; }

	Callbacks::const_iterator begin() const	{ return callbacks_.begin(); }
	const ReplayTickLoaderCallback & front() const
	{
		return callbacks_.front();
	}

	Callbacks::const_iterator end() const	{ return callbacks_.end(); }

	const ReplayTickLoaderCallback & back() const { return callbacks_.back (); }

	size_t size()							{ return callbacks_.size(); }
	uint totalTicks() const;
	void clear();

	ErrorType errorType() const 			{ return errorType_; }
	const BW::string & errorString() const 	{ return errorString_; }


	/* Override from IReplayTickLoaderListener. */
	virtual void onReplayTickLoaderReadHeader( const ReplayHeader & header,
			GameTime firstGameTime )
	{
		haveReadHeader_ = true;
		header_ = header;
	}


	/* Override from IReplayTickLoaderListener. */
	virtual void onReplayTickLoaderPrependTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks )
	{
		callbacks_.push_back( ReplayTickLoaderCallback(
			ReplayTickLoader::PREPEND,
#if defined( DEBUG_TICK_DATA )
			pStart->tick(), pEnd->tick(),
#endif // defined( DEBUG_TICK_DATA )
			pStart, totalTicks ) );
	}


	/* Override from IReplayTickLoaderListener. */
	virtual void onReplayTickLoaderAppendTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks )
	{
		callbacks_.push_back( ReplayTickLoaderCallback(
			ReplayTickLoader::APPEND,
#if defined( DEBUG_TICK_DATA )
			pStart->tick(), pEnd->tick(),
#endif // defined( DEBUG_TICK_DATA )
			pStart, totalTicks ) );
	}


	/* Override from IReplayTickLoaderListener. */
	virtual void onReplayTickLoaderError( ReplayTickLoader::ErrorType errorType,
		const BW::string & errorString )
	{
		errorType_ = errorType;
		errorString_ = errorString;
	}

private:
	void deleteTickData( ReplayTickData * pStart );

	bool 				haveReadHeader_;
	ReplayHeader 		header_;

	Callbacks			callbacks_;

	ErrorType 			errorType_;
	BW::string 			errorString_;
};


/**
 *	This method clears the collected callbacks.
 */
void MockReplayTickLoaderListener::clear()
{
	for (Callbacks::const_iterator iTickData = this->begin();
			iTickData != this->end();
			++iTickData)
	{
		this->deleteTickData( iTickData->pStart );
	}

	callbacks_.clear();
}


/**
 *	This method returns the total number of ticks collected.
 */
uint MockReplayTickLoaderListener::totalTicks() const
{
	Callbacks::const_iterator iCallback = callbacks_.begin();
	uint totalTicks = 0;

	while (iCallback != callbacks_.end())
	{
		totalTicks += iCallback->numTicks;
		++iCallback;
	}

	return totalTicks;
}


/**
 *	This method deletes data in the given tick data linked list.
 */
void MockReplayTickLoaderListener::deleteTickData( ReplayTickData * pNode )
{
	while (pNode)
	{
		ReplayTickData * pDelete = pNode;
		pNode->data().finish();
		pNode = pNode->pNext();
		bw_safe_delete( pDelete );
	}
}


TEST_F( Fixture, ReplayTickLoader_Basic )
{
	static const uint NUM_TICKS_TO_SIGN = 10;
	ReplayDataFileWriter writer( &this->fileWriter(), pPrivateScheme_,
		BW_COMPRESSION_ZIP_BEST_SPEED, digest_, NUM_TICKS_TO_SIGN, metaData_,
		NONCE );

	for (GameTime t = STARTING_TICK;
			t < (STARTING_TICK + NUM_TICKS_TO_WRITE); ++t)
	{
		this->addSampleTickData( writer, t );
		this->fileWriter().processRequestCallbacks();
	}

	writer.close( /* shouldFinalise*/ true );

	this->fileWriter().processRequestCallbacks();

	MemoryIStream fileStream( this->fileWriter().bufferData(),
		this->fileWriter().bufferSize() );

	TaskManager taskManager;
	taskManager.startThreads( 1 );

	MockFileProvider * pProvider = new MockFileProvider( fileStream );
	IFileProviderPtr pFileProvider( pProvider );

	MockReplayTickLoaderListener listener( pPublicScheme_ );

	ReplayTickLoaderPtr pTickLoader( new ReplayTickLoader( taskManager,
		&listener, "dummy file path", PUBLIC_KEY, pFileProvider ) );

	pTickLoader->addHeaderRequest();

	BW::TimeStamp startTimeStamp( BW::timestamp() );

	while ((startTimeStamp.ageInSeconds() < 1) &&
			!listener.haveReadHeader())
	{
		taskManager.tick();
	}

	if (!listener.haveReadHeader())
	{
		FAIL( "Failed to get header" );
	}

	CHECK( listener.haveReadHeader()) ;
	CHECK_EQUAL( NUM_TICKS_TO_WRITE, listener.header().numTicks() );

	static const int READ_TICK_INDEX_OFFSET = 10;
	static const uint NUM_TICKS_TO_READ = NUM_TICKS_TO_WRITE / 2;

	pTickLoader->addRequest( ReplayTickLoader::APPEND, READ_TICK_INDEX_OFFSET,
		READ_TICK_INDEX_OFFSET + NUM_TICKS_TO_READ );

	while ((startTimeStamp.ageInSeconds() < 1) &&
			(listener.size() == 0))
	{
		taskManager.tick();
	}

	CHECK_EQUAL( 1U, listener.size() );

	CHECK_EQUAL( ReplayTickLoader::APPEND, listener.back().requestType );
#if defined( DEBUG_TICK_DATA )
	CHECK_EQUAL( READ_TICK_INDEX_OFFSET, listener.back().startIndex );
	CHECK_EQUAL( int(READ_TICK_INDEX_OFFSET + NUM_TICKS_TO_READ),
		listener.back().endIndex );
#endif // defined( DEBUG_TICK_DATA )
	CHECK_EQUAL( NUM_TICKS_TO_READ, listener.back().numTicks );


	ReplayTickData * pStart = listener.back().pStart;
	GameTime expectedTime = STARTING_TICK + READ_TICK_INDEX_OFFSET;
	while (pStart)
	{
		CompressionIStream compressedTickStream( pStart->data() );
		CHECK( this->verifySampleTickData( compressedTickStream,
			expectedTime ) );
		++expectedTime;
		pStart = pStart->pNext();
	}
	CHECK_EQUAL( STARTING_TICK + READ_TICK_INDEX_OFFSET + NUM_TICKS_TO_READ,
		expectedTime );

	pTickLoader = NULL;
	taskManager.stopAll();

	CHECK( !pProvider->isOpen() );
	CHECK( !pProvider->didLeakFileHandle() );
}

BW_END_NAMESPACE

// test_recording.cpp
