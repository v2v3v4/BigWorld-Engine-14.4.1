#include "pch.hpp"

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/string_utils.hpp"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__unix__)
#include <unistd.h>
#endif

#include <cstdio>


BW_BEGIN_NAMESPACE


namespace TestBackgroundFileWriter
{


/**
 *	This class is used as a fixture for the BackgroundFileWriter unit tests.
 *
 *	It starts worker threads, and provides a hook for terminating a test.
 */
class Fixture
{
public:

	/**
	 *	Constructor.
	 */
	Fixture() : 
			isFinished_( false )
	{
		unlink( this->tempFilePath() );
	}

	/**
	 *	Destructor.
	 */
	~Fixture()
	{
		unlink( this->tempFilePath() );
	}

	
	/**
	 *	This method stops the current test.
	 */
	void stopRunning()
	{
		isFinished_ = true;
	}


	/**
	 *	This method starts the worker threads and starts the test.
	 */
	void run()
	{
		isFinished_ = false;

		BgTaskManager::instance().startThreads( 2 );

		while (!isFinished_)
		{
			BgTaskManager::instance().tick();
		}

		BgTaskManager::instance().stopAll();
	}

	/**
	 *	This method returns a file path suitable to be used for creating a 
	 *	temporary file.
	 */
	static char * tempFilePath()
	{
		if (!s_tempFilePath_[0])
		{
			// This is static so we only call it once per unit test program
			// invocation - individual tests should clean up.

#if defined( _WIN32 )
			BW::wstring tempFilePathWString = getTempFilePathName();
			if (!bw_wtoutf8( tempFilePathWString, s_tempFilePath_, 
								sizeof( s_tempFilePath_ ) ))
			{
				ERROR_MSG( "Could not convert temporary file path wide-string to UTF-8\n" );
				return NULL;
			}
#elif defined( __unix__ )
			BW::string tempFilePathString = getTempFilePathName();
			strncpy( s_tempFilePath_, tempFilePathString.c_str(), 
						sizeof( s_tempFilePath_ ) );
			s_tempFilePath_[ sizeof( s_tempFilePath_ ) - 1 ] = 0;
			if (!s_tempFilePath_[0])
			{
				ERROR_MSG( "Could not create temporary file.\n" );
				return NULL;
			}
#endif // defined( _WIN32 )

		}

		return s_tempFilePath_;
	}

private:
	bool isFinished_;

	static char s_tempFilePath_[1024];
};

char Fixture::s_tempFilePath_[1024] = "";


class TestFileWriterListener : public BackgroundFileWriterListener
{
public:
	TestFileWriterListener( Fixture & fixture ) :
			fixture_( fixture ),
			hasError_( false )
	{}


	virtual ~TestFileWriterListener() 
	{
		
	}

	bool hasError() const { return hasError_; }
	void setError() 
	{ 
		hasError_ = true; 
		fixture_.stopRunning(); 
	}

	Fixture & fixture() { return fixture_; }

	// Overrides from BackgroundFileWriterListener
	virtual void onBackgroundFileWritingError( 
		IBackgroundFileWriter & writer )
	{
		ERROR_MSG( "onBackgroundFileWritingError called: %s\n", 
			writer.errorString().c_str() );
		this->setError();
	}

private:
	Fixture & fixture_;
	bool hasError_;
};


/**
 *	This class writes out increasing unsigned 32-bit integers to a file.
 */
class IncrementingFileWriter : public TestFileWriterListener
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param fixture	The test fixture.
	 *	@param pWriter	The background file writer instance.
	 *	@param max		The maximum number of integers to write.
	 */
	IncrementingFileWriter( Fixture & fixture, 
				BackgroundFileWriterPtr pWriter, uint32 max = 100 ) :
			TestFileWriterListener( fixture ),
			pWriter_( pWriter ),
			nextExpected_( 1 ),
			max_( max )
	{
		pWriter_->setListener( this );
	}


	/** Destructor. */
	virtual ~IncrementingFileWriter()  {}

	/**
	 *	This method queues the writes necessary to write out the integers.
	 */
	void queueWrites()
	{
		for (uint32 i = 1; i <= max_; ++i)
		{
			pWriter_->queueWriteBlob( (const char *)&i, sizeof( i ), i );
		}
	}

	// Overrides from BackgroundFileWriterListener
	virtual void onBackgroundFileWritingComplete( 
			IBackgroundFileWriter & writer, long filePosition, int userArg )
	{
		if (nextExpected_ != uint32( userArg ))
		{
			ERROR_MSG( "onBackgroundFileWritingComplete: "
					"next expected = %u, got %u\n",
				nextExpected_, userArg );
			this->setError();
			return;
		}

		++nextExpected_;

		if (uint32( userArg ) == max_)
		{
			this->fixture().stopRunning();
		}
	}

	/**
	 *	This method verifies that the file at the given path has the expected number of integers.
	 */
	static bool verify( const char * path, uint32 expectedTotal = 100 )
	{
		bool isOK = true;
		FILE * f = fopen( path, "rb" );

		if (!f)
		{
			ERROR_MSG( "IncrementingFileWriter::verify: "
					"Couldn't open file: %s\n", 
				strerror( errno ) );
			return false;
		}

		uint32 nextRead = 0;
		uint32 expected = 1;

		bool isDone = false;

		while (!isDone)
		{
			size_t numRead = fread( &nextRead, sizeof( uint32 ), 1, f );
			if (numRead == 1)
			{
				if (nextRead != expected)
				{
					ERROR_MSG( "IncrementingFileWriter::verify: "
							"Read %u, expecting %u\n", 
						nextRead, expected );
					isOK = false;
					isDone = true;
				}
				++expected;
			} 
			else 
			{
				isDone = true;
			}
		}

		
		if (isOK && (nextRead != expectedTotal))
		{
			ERROR_MSG( "IncrementingFileWriter::verify: "
					"Expecting to end on %u, got %u\n", 
				expectedTotal, nextRead );
			isOK = false;
		}

		fclose( f );

		return isOK;
	}


private:
	BackgroundFileWriterPtr pWriter_;
	uint32 nextExpected_;
	uint32 max_;
};

TEST_F( Fixture, TestNoPreExistingFile )
{
	CHECK( this->tempFilePath() != NULL );

	BackgroundFileWriterPtr pWriter = new BackgroundFileWriter( 
		BgTaskManager::instance() );
	
	pWriter->initWithFileSystemPath( this->tempFilePath() );

	IncrementingFileWriter ifWriter( *this, pWriter );

	ifWriter.queueWrites();

	this->run();

	CHECK( !pWriter->hasError() );
	CHECK( IncrementingFileWriter::verify( this->tempFilePath() ) );
}


TEST_F( Fixture, TestPreExistingFile )
{
	CHECK( this->tempFilePath() != NULL );

	FILE * f = fopen( this->tempFilePath(), "wb" );

	const char TEST_DATA[] = "Some data";
	CHECK_EQUAL( fwrite( TEST_DATA, sizeof( TEST_DATA ), 1, f ), size_t( 1 ) );
	CHECK_EQUAL( fclose( f ), 0 );


	BackgroundFileWriterPtr pWriter = new BackgroundFileWriter( 
		BgTaskManager::instance() );
	
	pWriter->initWithFileSystemPath( this->tempFilePath(), 
		/*shouldOverwrite: */ true );

	IncrementingFileWriter ifWriter( *this, pWriter );

	ifWriter.queueWrites();

	this->run();

	CHECK( !pWriter->hasError() );
	CHECK( IncrementingFileWriter::verify( this->tempFilePath() ) );

}


/**
 *	This listener stops running the test after receiving a set number of
 *	callbacks.
 */
class FixedTotalListener: 
		public TestFileWriterListener 
{
public:
	FixedTotalListener( Fixture & fixture, uint expectedNumCallbacks ) :
			TestFileWriterListener( fixture ),
			expectedNumCallbacks_( expectedNumCallbacks ),
			numCallbacks_( 0 )
	{}

	virtual ~FixedTotalListener()
	{}

	// Overrides from BackgroundFileWriterListener
	virtual void onBackgroundFileWritingComplete( 
			IBackgroundFileWriter & writer, long filePosition, int userArg )
	{
		++numCallbacks_;

		if (numCallbacks_ == expectedNumCallbacks_)
		{
			this->fixture().stopRunning();
		}
		else if (numCallbacks_ > expectedNumCallbacks_)
		{
			ERROR_MSG( "FixedTotalListener::onBackgroundFileWritingComplete: "
					"Got unexpected callback\n" );
			this->setError();
		}
	}

private:
	uint expectedNumCallbacks_;
	uint numCallbacks_;
};

TEST_F( Fixture, TestInterleavedSeekWriting )
{
	CHECK( this->tempFilePath() != NULL );

	BackgroundFileWriterPtr pWriter = new BackgroundFileWriter( 
		BgTaskManager::instance() );
		
	pWriter->initWithFileSystemPath( this->tempFilePath() );

	FixedTotalListener fixedTotalListener( *this, 200 );
	pWriter->setListener( &fixedTotalListener );

	for (uint32 i = 1; i <= 100; ++i)
	{
		uint32 out = ((i % 2) != 0) ? i : uint32( -1 );
		
		pWriter->queueWriteBlob( (const char *)&out, sizeof( i ) );
	}

	for (uint32 i = 1; i <= 50; ++i)
	{
		pWriter->queueSeek( (i * 2 - 1) * sizeof( uint32 ), SEEK_SET );
		uint32 out = i * 2;
		pWriter->queueWriteBlob( (const char *)&out, sizeof( i ) );
	}

	this->run();

	CHECK( !pWriter->hasError() );
	CHECK( IncrementingFileWriter::verify( this->tempFilePath() ) );
}


} // end namespace TestBackgroundFileWriter


BW_END_NAMESPACE

// test_background_file_writer.cpp
