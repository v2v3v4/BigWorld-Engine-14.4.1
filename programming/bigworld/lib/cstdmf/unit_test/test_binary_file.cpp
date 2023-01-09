#include "pch.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/static_array.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_vector.hpp"



BW_BEGIN_NAMESPACE


namespace
{
	///The test file sizes in bytes used for read tests.
	const int TEST_READ_SIZES[] = {1, 2, 3, 102400};
	///The test read operation sizes in bytes used for read tests.
	const int TEST_READ_OP_SIZES[] = {1, 2, 3, 1048576};
	///The test file sizes in bytes used for write tests.
	const int TEST_WRITE_SIZES[] = {0, 1, 2, 3, 102400};
	///The test write operation sizes in bytes used for read tests.
	const int TEST_WRITE_OP_SIZES[] = {1, 2, 3, 1048576};

	/**
	 *	This functor tests the initial state of a BinaryFile with the
	 *	given size, FILE pointer, and data pointer.
	 */
	class TestBinaryFileInitState
	{
	public:
		TestBinaryFileInitState(
			BinaryFile & testFile,
			size_t testFileSize,
			const FILE * cFile = static_cast<FILE *>(0),
			const void * data = static_cast<FILE *>(0) )
			:
			testFile_(testFile)
			, testFileSize_(testFileSize)
			, cFile_(cFile)
			, data_(data)
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			CHECK_EQUAL( cFile_, testFile_.file() );
			CHECK_EQUAL( data_, testFile_.data() );
			CHECK_EQUAL( false, testFile_.error() );
			if (data_ == NULL)
			{
				CHECK_EQUAL( 0, testFile_.dataSize() );
				CHECK_EQUAL( testFileSize_, testFile_.bytesAvailable() );
			}
			else if (cFile_ != BinaryFile::FILE_INTERNAL_BUFFER)
			{
				CHECK_EQUAL( static_cast<ptrdiff_t>(testFileSize_),
					testFile_.dataSize() );
				CHECK_EQUAL( testFileSize_, testFile_.bytesAvailable() );
			}
			CHECK_EQUAL( 0u, testFile_.cursorPosition() );
		}

	private:
		BinaryFile & testFile_;
		const size_t testFileSize_;
		const FILE * cFile_;
		const void * data_;
	};


	/**
	 *	This functor uses @a testFile.read() to get the contents,
	 *	expecting them to equal @a expectedContents.
	 */
	class TestRead
	{
	public:
		TestRead(
			BinaryFile & testFile,
			const BW::vector<uint8> & expectedContents,
			int operationSize )
			:
			testFile_(testFile)
			, expectedContents_(expectedContents)
			, operationSize_(operationSize)
		{
			BW_GUARD;

			MF_ASSERT( !expectedContents.empty() );
			MF_ASSERT( testFile_.cursorPosition() == 0 );
			MF_ASSERT( operationSize_ > 0 );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			BW::vector<uint8> testBuffer( expectedContents_.size() );
			size_t bytesRemainingToRead = testBuffer.size();

			while (bytesRemainingToRead > 0)
			{
				const size_t readOpSize = std::min(
					static_cast<size_t>(operationSize_),
					bytesRemainingToRead );

				MF_ASSERT( testBuffer.size() >= bytesRemainingToRead );
				MF_ASSERT( bytesRemainingToRead > 0 );
				const size_t testBufferOffset =
					testBuffer.size() - bytesRemainingToRead;
				RETURN_ON_FAIL_CHECK( readOpSize ==
					testFile_.read(
						& testBuffer[testBufferOffset], readOpSize ) );

				bytesRemainingToRead -= readOpSize;
			}

			CHECK( !testFile_.error() );
			CHECK( testBuffer == expectedContents_ );

			MF_ASSERT( !testBuffer.empty() );
			CHECK_EQUAL( 0u, testFile_.read(& testBuffer[0], 1) );
			CHECK( !testFile_.error() );//Expect EOF.
			CHECK_EQUAL( 0u, testFile_.read(& testBuffer[0], 1) );
			CHECK( !testFile_.error() );//Still expect no error now.
		}

	private:
		BinaryFile & testFile_;
		const BW::vector<uint8> expectedContents_;
		int operationSize_;
	};


	/**
	 *	This functor uses @a testFile.write() to write @a testContents
	 *	into the file, then checks that the contents equal @a testContents.
	 */
	class TestWrite
	{
	public:
		TestWrite(
			BinaryFile & testFile,
			const BW::vector<uint8> & testContents,
			int operationSize )
			:
			testFile_(testFile)
			, testContents_(testContents)
			, operationSize_(operationSize)
		{
			BW_GUARD;

			MF_ASSERT( testFile_.cursorPosition() == 0 );
			MF_ASSERT( operationSize_ > 0 );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			size_t bytesRemainingToWrite = testContents_.size();
			size_t bytesWrittenSoFar = 0;

			while ( bytesRemainingToWrite > 0 )
			{
				const size_t writeOpSize = std::min(
					static_cast<size_t>(operationSize_),
					bytesRemainingToWrite);

				MF_ASSERT( testContents_.size() >= bytesRemainingToWrite );
				MF_ASSERT( bytesRemainingToWrite > 0 );
				const size_t testContentsOffset =
					testContents_.size() - bytesRemainingToWrite;
				testFile_.write(
					& testContents_[testContentsOffset], writeOpSize );

				bytesRemainingToWrite -= writeOpSize;
				bytesWrittenSoFar += writeOpSize;

				MF_ASSERT( (bytesRemainingToWrite + bytesWrittenSoFar) ==
					testContents_.size() );

				RETURN_ON_FAIL_CHECK( bytesWrittenSoFar ==
					testFile_.cursorPosition() );
			}

			CHECK( !testFile_.error() );
			CHECK_EQUAL( testContents_.size(), testFile_.cursorPosition() );
			if (testFile_.data() != NULL)
			{
				CHECK( testFile_.dataSize() >=
					static_cast<ptrdiff_t>(testContents_.size()) );
			}

			BW::vector<uint8> testBuffer( testContents_.size() );
			if ( (testFile_.file() == NULL)
				||
				(testFile_.file() == BinaryFile::FILE_INTERNAL_BUFFER))
			{
				//Using an internal buffer.
				CHECK( testFile_.data() != NULL );

				MF_ASSERT(
					static_cast<std::ptrdiff_t>(testContents_.size()) <=
					testFile_.dataSize() );

				if (!testContents_.empty())
				{
					CHECK_EQUAL(
						memcmp(& testContents_[0], testFile_.data(),
						testContents_.size()),
						0 );
				}
			}
			else
			{
				FILE * rawFileAccess = testFile_.file();
				rewind( rawFileAccess );
				CHECK_EQUAL(
					testBuffer.size(),
					fread(
						testBuffer.empty() ? NULL :
							reinterpret_cast<void *>(& testBuffer[0]),
						1, testBuffer.size(), rawFileAccess));
				CHECK( feof(rawFileAccess) == 0 );
				CHECK( testBuffer == testContents_ );

				testBuffer.resize( 1 );
				CHECK_EQUAL(
					0u,
					fread(reinterpret_cast<void *>(& testBuffer[0]),
						1, testBuffer.size(), rawFileAccess));
				CHECK( feof(rawFileAccess) != 0 );
			}
		}

	private:
		BinaryFile & testFile_;
		const BW::vector<uint8> testContents_;
		int operationSize_;
	};


	uint8 randomUint8()
	{
		BW_GUARD;

		return (std::rand() % std::numeric_limits<uint8>::max());
	}


	/**
	 *	@return A vector with random contents that are always the same for
	 *		a given size and @a extraSeed value.
	 */
	BW::vector<uint8> getRandomVector( size_t size, uint32 extraSeed = 0 )
	{
		BW_GUARD;

		BW::vector<uint8> result( size );
		MF_ASSERT( result.size() == size );
		//Seed here so guaranteed reproducible.
		std::srand( static_cast<uint>(size + extraSeed) );
		std::generate(
			result.begin(), result.end(), &randomUint8 );
		return result;
	}


	/**
	 *	Creates a temp file (to be deleted automatically on program exit
	 *	or when closed) that contains the contents @a fileContents.
	 *	@return The temp file (after rewind called to position at the
	 *		beginning), or NULL on error.
	 */
	FILE * createTempFile( const BW::vector<uint8> & fileContents )
	{
		BW_GUARD;

		FILE * tempFileC = tmpfile();
		if (tempFileC == NULL)
		{
			return NULL;
		}

		if ( !fileContents.empty() )
		{
			if (fwrite( &fileContents[0], 1, fileContents.size(), tempFileC ) !=
				fileContents.size())
			{
				fclose(tempFileC);
				return NULL;
			}
		}

		rewind( tempFileC );
		return tempFileC;
	}


	/**
	 *	This functor creates a temporary file.
	 *	It will write testDataSize random bytes into it.
	 *	It then checks that the bytes are read correctly using BinaryFile.
	 */
	class TestFileRead
	{
	public:
		TestFileRead( int operationSize, int testDataSize )
			:
			operationSize_(operationSize)
			, testDataSize_(testDataSize)
		{
			BW_GUARD;

			MF_ASSERT( operationSize_ > 0 );
			MF_ASSERT( testDataSize > 0 );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			MF_ASSERT( testDataSize_ > 0 );

			const BW::vector<uint8> fileContents =
				getRandomVector( testDataSize_ );

			FILE * tempFile = createTempFile( fileContents );
			ASSERT_WITH_MESSAGE(
				tempFile != NULL,
				"Can't continue without the temp file. " );

			BinaryFile file( tempFile );

			TestBinaryFileInitState testInitStateFunctor(
				file, fileContents.size(), tempFile );
			TEST_SUB_FUNCTOR( testInitStateFunctor );

			TestRead testReadFunctor( file, fileContents, operationSize_ );

			//We do two test read passes, to check that rewind functions
			//correctly.
			for (uint32 readPass = 0; readPass < 2; ++readPass)
			{
				TEST_SUB_FUNCTOR( testReadFunctor );

				file.rewind();
				CHECK( !file.error() );//Never error after rewind.
				CHECK_EQUAL( 0u, file.cursorPosition() );
			}
		}

	private:
		///The size of file operations in bytes.
		int operationSize_;
		///The file size used during the tests.
		int testDataSize_;
	};


	/**
	 *	This functor creates a temporary file.
	 *	It will write testDataSize random bytes into it using BinaryFile.
	 *	It then checks that the bytes were written correctly.
	 */
	class TestFileWrite
	{
	public:
		TestFileWrite( int operationSize, int testDataSize )
			:
			operationSize_(operationSize)
			, testDataSize_(testDataSize)
		{
			BW_GUARD;

			MF_ASSERT( operationSize_ > 0 );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			const BW::vector<uint8> fileContents =
				getRandomVector( testDataSize_ );

			FILE * tempFile = createTempFile( BW::vector<uint8>() );
			ASSERT_WITH_MESSAGE(
				tempFile != NULL,
				"Can't continue without the temp file. " );

			BinaryFile file( tempFile );

			TestBinaryFileInitState testInitStateFunctor(
				file, 0, tempFile );
			TEST_SUB_FUNCTOR( testInitStateFunctor );

			//We do two test write passes, to check that rewind functions
			//correctly.
			for (uint32 writePass = 0; writePass < 2; ++writePass)
			{
				const BW::vector<uint8> fileContents =
					getRandomVector( testDataSize_, writePass );
				TestWrite testWriteFunctor(
					file, fileContents, operationSize_ );
				TEST_SUB_FUNCTOR( testWriteFunctor );

				file.rewind();
				CHECK( !file.error() );//Never error after rewind.
				CHECK_EQUAL( 0u, file.cursorPosition() );
			}
		}

	private:
		///The size of file operations in bytes.
		int operationSize_;
		///The file size used during the tests.
		int testDataSize_;
	};


	/**
	 *	This functor creates a memory buffer, then reads from it
	 *	using BinaryFile.
	 */
	class TestMemoryRead
	{
	public:
		/**
		 *	The modes that the buffer can be associated with the BinaryFile
		 *	for testing.
		 */
		enum Mode
		{
			///Directly use a provided buffer.
			M_DIRECT,
			///Copy data from a provided buffer.
			M_COPY,
			///Reserve the buffer internally.
			M_RESERVE
		};

		TestMemoryRead( int operationSize, int testDataSize, Mode mode )
			:
			operationSize_(operationSize)
			, testDataSize_(testDataSize)
			, mode_(mode)
		{
			BW_GUARD;

			MF_ASSERT( operationSize_ > 0 );
			MF_ASSERT( testDataSize > 0 );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			MF_ASSERT(testDataSize_ > 0);

			BW::vector<uint8> fileContents =
				getRandomVector( testDataSize_ );

			BinaryFile file( NULL );
			const FILE * expectedFilePtr = NULL;
			const void * expectedDataPtr = NULL;
			switch(mode_)
			{
			default:
				MF_ASSERT(false && "Unexpected mode. ");
				FAIL( "Unexpected mode. " );
				return;

			case M_DIRECT:
				file.wrap(
					reinterpret_cast<void *>(& fileContents[0]),
						static_cast<int>(fileContents.size()), false );
				expectedFilePtr = NULL;
				expectedDataPtr = & fileContents[0];
				break;

			case M_COPY:
				file.wrap(
					reinterpret_cast<void *>(& fileContents[0]),
						static_cast<int>(fileContents.size()), true );
				expectedFilePtr = BinaryFile::FILE_INTERNAL_BUFFER;
				expectedDataPtr = file.data();
				break;

			case M_RESERVE:
				void * internalBuffer = file.reserve( (int)fileContents.size() );
				memcpy(
					internalBuffer, & fileContents[0], fileContents.size() );
				expectedFilePtr = BinaryFile::FILE_INTERNAL_BUFFER;
				expectedDataPtr = file.data();
				break;
			}
			TestBinaryFileInitState testInitStateFunctor(
				file, fileContents.size(), expectedFilePtr, expectedDataPtr );
			TEST_SUB_FUNCTOR( testInitStateFunctor );

			TestRead testReadFunctor( file, fileContents, operationSize_ );
			TEST_SUB_FUNCTOR( testReadFunctor );
		}

	private:
		///The size of file operations in bytes.
		int operationSize_;
		///The file size used during the tests.
		int testDataSize_;
		///Buffering mode used for the test.
		Mode mode_;
	};


	/**
	 *	This functor creates a memory buffer, then writes to it
	 *	using BinaryFile.
	 */
	class TestMemoryWrite
	{
	public:
		/**
		 *	The modes that the buffer can be associated with the BinaryFile
		 *	for testing.
		 */
		enum Mode
		{
			///Directly use a provided buffer.
			M_DIRECT,
			///Copy data from a provided buffer.
			M_COPY,
			///Reserve the buffer internally.
			M_RESERVE
		};

		TestMemoryWrite( int operationSize, int testDataSize, Mode mode )
			:
			operationSize_(operationSize)
			, testDataSize_(testDataSize)
			, mode_(mode)
		{
			BW_GUARD;

			MF_ASSERT(operationSize_ > 0);
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			BW::vector<uint8> fileContents =
				getRandomVector( testDataSize_ );
			BW::vector<uint8> userBuffer;

			BinaryFile file( NULL );
			const FILE * expectedFilePtr = NULL;
			const void * expectedDataPtr = NULL;
			switch(mode_)
			{
			default:
				MF_ASSERT( false && "Unexpected mode. " );
				FAIL( "Unexpected mode. " );
				return;

			case M_DIRECT:
				userBuffer.resize( std::max(fileContents.size(), size_t(1) ) );
				file.wrap(
					reinterpret_cast<void *>(& userBuffer[0]),
						static_cast<int>(fileContents.size()), false );
				expectedFilePtr = NULL;
				expectedDataPtr = & userBuffer[0];
				break;

			case M_COPY:
				userBuffer.resize( std::max(fileContents.size(), size_t(1) ) );
				file.wrap(
					reinterpret_cast<void *>( & userBuffer[0]),
						static_cast<int>(fileContents.size()), true );
				expectedFilePtr = BinaryFile::FILE_INTERNAL_BUFFER;
				expectedDataPtr = file.data();
				break;

			case M_RESERVE:
				//We reserve only one byte, to test that the buffer will
				//expand in size as required during the write.
				void * internalBuffer = file.reserve( 1 );
				expectedFilePtr = BinaryFile::FILE_INTERNAL_BUFFER;
				expectedDataPtr = file.data();
				CHECK_EQUAL( internalBuffer, expectedDataPtr );
				break;
			}
			TestBinaryFileInitState testInitStateFunctor(
				file, fileContents.size(), expectedFilePtr, expectedDataPtr );
			TEST_SUB_FUNCTOR( testInitStateFunctor );

			TestWrite testWriteFunctor( file, fileContents, operationSize_ );
			TEST_SUB_FUNCTOR( testWriteFunctor );
		}

	private:
		///The size of file operations in bytes.
		int operationSize_;
		///The file size used during the tests.
		int testDataSize_;
		///Buffering mode used for the test.
		Mode mode_;
	};


	/**
	 *	This functor is used to test a range of file reads using the
	 *	specified read operation size.
	 */
	class TestFileReads
	{
	public:
		explicit TestFileReads( int operationSize )
			:
			operationSize_(operationSize)
		{
			BW_GUARD;

			MF_ASSERT(operationSize_ > 0);
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			for(std::size_t i = 0; i < ARRAY_SIZE(TEST_READ_SIZES); ++i)
			{
				TestFileRead testFunc( operationSize_, TEST_READ_SIZES[i] );
				BW::ostringstream name;
				name << TEST_READ_SIZES[i] << "B file size";
				TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
			}
		}

	private:
		///The number of bytes attempted to read for each read operation.
		int operationSize_;
	};


	/**
	 *	This functor is used to test a range of file writes using the
	 *	specified write operation size.
	 */
	class TestFileWrites
	{
	public:
		explicit TestFileWrites( int operationSize )
			:
			operationSize_(operationSize)
		{
			BW_GUARD;

			MF_ASSERT(operationSize_ > 0);
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			for(std::size_t i = 0; i < ARRAY_SIZE(TEST_WRITE_SIZES); ++i)
			{
				TestFileWrite testFunc( operationSize_, TEST_WRITE_SIZES[i] );
				BW::ostringstream name;
				name << TEST_WRITE_SIZES[i] << "B file size";
				TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
			}
		}

	private:
		///The number of bytes attempted to write for each write operation.
		int operationSize_;
	};


	/**
	 *	This functor is used to test a range of memory reads using the
	 *	specified read operation size.
	 */
	class TestMemoryReads
	{
	public:
		explicit TestMemoryReads( int operationSize )
			:
			operationSize_(operationSize)
		{
			BW_GUARD;

			MF_ASSERT(operationSize_ > 0);
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			for(std::size_t i = 0; i < ARRAY_SIZE(TEST_READ_SIZES); ++i)
			{
				{
					TestMemoryRead testFunc( operationSize_,
						TEST_READ_SIZES[i], TestMemoryRead::M_COPY );
					BW::ostringstream name;
					name << TEST_READ_SIZES[i] << "B file size/copy";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
				{
					TestMemoryRead testFunc( operationSize_,
						TEST_READ_SIZES[i], TestMemoryRead::M_DIRECT );
					BW::ostringstream name;
					name << TEST_READ_SIZES[i] << "B file size/direct";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
				{
					TestMemoryRead testFunc( operationSize_,
						TEST_READ_SIZES[i], TestMemoryRead::M_RESERVE );
					BW::ostringstream name;
					name << TEST_READ_SIZES[i] << "B file size/reserve";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
			}
		}

	private:
		///The number of bytes attempted to read for each read operation.
		int operationSize_;
	};


	/**
	 *	This functor is used to test a range of memory writes using the
	 *	specified write operation size.
	 */
	class TestMemoryWrites
	{
	public:
		explicit TestMemoryWrites( int operationSize )
			:
			operationSize_( operationSize )
		{
			BW_GUARD;

			MF_ASSERT(operationSize_ > 0);
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			for(std::size_t i = 0; i < ARRAY_SIZE(TEST_WRITE_SIZES); ++i)
			{
				{
					TestMemoryWrite testFunc( operationSize_,
						TEST_WRITE_SIZES[i], TestMemoryWrite::M_COPY );
					BW::ostringstream name;
					name << TEST_WRITE_SIZES[i] << "B file size/copy";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
				{
					TestMemoryWrite testFunc( operationSize_,
						TEST_WRITE_SIZES[i], TestMemoryWrite::M_DIRECT );
					BW::ostringstream name;
					name << TEST_WRITE_SIZES[i] << "B file size/direct";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
				{
					TestMemoryWrite testFunc( operationSize_,
						TEST_WRITE_SIZES[i], TestMemoryWrite::M_RESERVE );
					BW::ostringstream name;
					name << TEST_WRITE_SIZES[i] << "B file size/reserve";
					TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
				}
			}
		}

	private:
		///The number of bytes attempted to write for each write operation.
		int operationSize_;
	};
}


TEST( BinaryFile_initDefaultState )
{
	BW_GUARD;

	BinaryFile file( NULL );

	TestBinaryFileInitState testInitStateFunctor( file, 0 );
	TEST_SUB_FUNCTOR( testInitStateFunctor );

	//Check read/write get errors.
	unsigned char testBuffer = 0;

	CHECK_EQUAL( 0u, file.read(& testBuffer, 1) );
	CHECK( file.error() );
	file.error( false );
	CHECK( !file.error() );

	CHECK_EQUAL( 0u, file.write(& testBuffer, 1) );
	CHECK( file.error() );

	file.rewind();
	CHECK( !file.error() );//Never error after rewind.
	CHECK_EQUAL( 0u, file.cursorPosition() );
	CHECK( file.error() );//Error to check position on null file.
}


TEST( BinaryFile_FileRead )
{
	BW_GUARD;

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_READ_OP_SIZES); ++i)
	{
		TestFileReads testFunc( TEST_READ_OP_SIZES[i] );
		BW::ostringstream name;
		name << TEST_READ_OP_SIZES[i] << "B opsize";
		TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
	}
}


TEST( BinaryFile_FileWrite )
{
	BW_GUARD;

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_WRITE_OP_SIZES); ++i)
	{
		TestFileWrites testFunc( TEST_WRITE_OP_SIZES[i] );
		BW::ostringstream name;
		name << TEST_WRITE_OP_SIZES[i] << "B opsize";
		TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
	}
}


TEST( BinaryFile_MemoryRead )
{
	BW_GUARD;

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_READ_OP_SIZES); ++i)
	{
		TestMemoryReads testFunc( TEST_READ_OP_SIZES[i] );
		BW::ostringstream name;
		name << TEST_READ_OP_SIZES[i] << "B opsize";
		TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
	}
}


TEST( BinaryFile_MemoryWrite )
{
	BW_GUARD;

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_WRITE_OP_SIZES); ++i)
	{
		TestMemoryWrites testFunc( TEST_WRITE_OP_SIZES[i] );
		BW::ostringstream name;
		name << TEST_WRITE_OP_SIZES[i] << "B opsize";
		TEST_SUB_FUNCTOR_NAMED( testFunc, name.str() );
	}
}


TEST( BinaryFile_Cache )
{
	BW_GUARD;

	//Test with one data size just for code coverage.
	const size_t TEST_DATA_SIZE = 1505;
	const BW::vector<uint8> testContents = getRandomVector( TEST_DATA_SIZE );
	FILE * tempFile = createTempFile( testContents );
	ASSERT_WITH_MESSAGE(
		tempFile != NULL,
		"Can't continue without the temp file. " );

	BinaryFile file( tempFile );

	CHECK( !file.error() );
	CHECK_EQUAL( static_cast<ptrdiff_t>(testContents.size()), file.cache() );
	RETURN_ON_FAIL_CHECK( file.dataSize() ==
		static_cast<ptrdiff_t>(testContents.size()) );
	CHECK_EQUAL(
		memcmp(file.data(), & testContents[0], testContents.size()),
		0 );
}


TEST( BinaryFile_Map )
{
	BW_GUARD;

	const int TEST_MAP_INSERTIONS[] = {0, 1, 500};

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_MAP_INSERTIONS); ++i)
	{
		BW::map<int, int> testMap;
		std::srand( 0 );//Ensure repeatable.
		for(int insertionsDone = 0;
			insertionsDone < TEST_MAP_INSERTIONS[insertionsDone];
			++insertionsDone)
		{
			testMap[std::rand()] = std::rand();
		}

		//Memory-backed I/O tested above, so we assume we can use it here to
		//check map operations.

		BinaryFile outFile( NULL );
		outFile.reserve( 1 );//Make memory-backed.
		outFile.writeMap( testMap );
		RETURN_ON_FAIL_CHECK( !outFile.error() );

		BinaryFile inFile( NULL );
		void * inBuffer = inFile.reserve( (int)outFile.cursorPosition() );
		memcpy( inBuffer, outFile.data(), outFile.cursorPosition() );

		BW::map<int, int> recoveredMap;
		inFile.readMap( recoveredMap );
		RETURN_ON_FAIL_CHECK( !inFile.error() );

		CHECK( testMap == recoveredMap );
	}
}


TEST( BinaryFile_Vector )
{
	BW_GUARD;

	const int TEST_VECTOR_SIZES[] = {0, 1, 500};

	for(std::size_t i = 0; i < ARRAY_SIZE(TEST_VECTOR_SIZES); ++i)
	{
		const BW::vector<uint8> testVector =
			getRandomVector(TEST_VECTOR_SIZES[i]);

		//Memory-backed I/O tested above, so we assume we can use it here to
		//check map operations.

		BinaryFile outFile( NULL );
		outFile.reserve( 1 );//Make memory-backed.
		outFile.writeSequence( testVector );
		RETURN_ON_FAIL_CHECK( !outFile.error() );

		BinaryFile inFile( NULL );
		void * inBuffer = inFile.reserve( (int)outFile.cursorPosition() );
		memcpy( inBuffer, outFile.data(), outFile.cursorPosition() );

		BW::vector<uint8> recoveredVector;
		inFile.readSequence( recoveredVector );
		RETURN_ON_FAIL_CHECK( !inFile.error() );

		CHECK( testVector == recoveredVector );
	}
}

BW_END_NAMESPACE

// test_binary_file.cpp
