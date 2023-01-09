#include "pch.hpp"

#include "test_harness.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/file_system.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/filename_case_checker.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE

// File/directory helper functions for unit tests
namespace {

	void ensureFileNonExistant( const BW::StringRef& fileName, MultiFileSystemPtr fileSystem )
	{
		FILE * f = fileSystem->posixFileOpen( fileName, "r" );
		if (f)
		{
			fclose( f );
			fileSystem->eraseFileOrDirectory( fileName );
		}
	}

	bool removeDirectory( const BW::StringRef& fileName, MultiFileSystemPtr fileSystem )
	{
		return fileSystem->eraseFileOrDirectory( fileName );
	}

	bool touchDirectory( const BW::StringRef& dirName, MultiFileSystemPtr fileSystem )
	{
		return fileSystem->makeDirectory( dirName );
	}

	bool touchFile( const BW::StringRef& fileName, MultiFileSystemPtr fileSystem )
	{
		FILE * f = fileSystem->posixFileOpen( fileName, "w" );

		bool ret = ( f != NULL );	// file should exist
		if (f)
		{
			fclose( f );
		}

		return ret;
	}

}	// anonymous namespace


const BW::string TEMP_DIR_NAME("/tmp/");

TEST_F( ResMgrUnitTestHarness, ResMgrTest_IFileSystem )
{
	CHECK( this->isOK() );

	const BW::string NEW_DIR( "bigworld_file_system_test" );
	const BW::string NEW_FILE( "test_file" );
	
	const BW::string TEMP_DIR = BWResource::instance().getDefaultPath() + TEMP_DIR_NAME;
	FileSystemPtr pFS = NativeFileSystem::create( TEMP_DIR );

	// clean up base dir
	// We do it this way because file system does not have a way to delete its
	// base directory/basepath. We cannot pass an empty string to eraseFileOrDirectory.
	removeDirectory( TEMP_DIR_NAME, BWResource::instance().fileSystem() );

	// create base dir.
	CHECK( pFS->makeDirectory( "" ) );

	// Test makeDirectory and getFileType
	CHECK( pFS->makeDirectory( NEW_DIR ) );
	CHECK( !pFS->makeDirectory( NEW_DIR ) );
	CHECK( pFS->getFileType( NEW_DIR ) == IFileSystem::FT_DIRECTORY );

	// Test writeFile and getFileType
	const BW::string relativeFilePath = NEW_DIR + '/' + NEW_FILE;
	const char * testFileData = ". . . ignorance gives one a large range of "
			"probabilities. -- Daniel Deronda by George Eliot";
	size_t testFileDataLen = strlen( testFileData );
	BinaryPtr	pTestFileDataBlock( 
			new BinaryBlock( testFileData, testFileDataLen, "std" ) );
	CHECK( pFS->writeFile( relativeFilePath, pTestFileDataBlock, true ) );
	CHECK( pFS->getFileType( relativeFilePath ) == IFileSystem::FT_FILE );
	IFileSystem::FileInfo fi;
	CHECK( pFS->getFileType( relativeFilePath, &fi ) == IFileSystem::FT_FILE );
	CHECK( fi.size == testFileDataLen && fi.created > 0 && 
			fi.modified > 0 && fi.accessed > 0 );

	// Test getAbsolutePath
	const BW::string absoluteFilePath = TEMP_DIR + relativeFilePath;
	const BW::string absoluteDirPath = TEMP_DIR + NEW_DIR;
	CHECK( pFS->getAbsolutePath( relativeFilePath ) == absoluteFilePath );
	CHECK( pFS->getAbsolutePath( NEW_DIR ) == absoluteDirPath );

	// Test getAbsoluteFileType
	const BW::string absoluteNonExistPath = TEMP_DIR + "i.dont.really.exist";
	CHECK( NativeFileSystem::getAbsoluteFileType( absoluteFilePath ) == 
		IFileSystem::FT_FILE );
	CHECK( NativeFileSystem::getAbsoluteFileType( absoluteDirPath ) == 
		IFileSystem::FT_DIRECTORY );
	CHECK( NativeFileSystem::getAbsoluteFileType( absoluteNonExistPath ) == 
		IFileSystem::FT_NOT_FOUND );
	CHECK( NativeFileSystem::getAbsoluteFileType( absoluteFilePath, &fi ) == 
		IFileSystem::FT_FILE );
	CHECK( fi.size == testFileDataLen && fi.created > 0 && 
			fi.modified > 0 && fi.accessed > 0 );

	// Test eraseFileOrDirectory and getFileType
	CHECK( pFS->eraseFileOrDirectory( relativeFilePath ) );
	CHECK( !pFS->eraseFileOrDirectory( relativeFilePath ) );
	CHECK( pFS->getFileType( relativeFilePath ) == IFileSystem::FT_NOT_FOUND );
	CHECK( pFS->eraseFileOrDirectory( NEW_DIR ) );
	CHECK( !pFS->eraseFileOrDirectory( NEW_DIR ) );
	CHECK( pFS->getFileType( NEW_DIR ) == IFileSystem::FT_NOT_FOUND );
	
	// clean up base dir
	removeDirectory( TEMP_DIR_NAME, BWResource::instance().fileSystem() );
}

TEST_F( ResMgrUnitTestHarness, ResMgr_TestMultiFileSystem )
{
	CHECK( this->isOK() );

	const BW::string FIRST_PATH = "ichy/";
	const BW::string SECOND_PATH = "knee/";

	// Set-up MultiFileSystem
	const BW::string TEMP_DIR = BWResource::instance().getDefaultPath() + TEMP_DIR_NAME;
	FileSystemPtr pTempFS = NativeFileSystem::create( TEMP_DIR );

	// setup base dir
	removeDirectory( TEMP_DIR_NAME, BWResource::instance().fileSystem() );
	CHECK( pTempFS->makeDirectory( "" ) );

	CHECK( pTempFS->makeDirectory( FIRST_PATH ) );
	CHECK( pTempFS->makeDirectory( SECOND_PATH ) );

	FileSystemPtr pFS1 = NativeFileSystem::create( TEMP_DIR + FIRST_PATH );
	FileSystemPtr pFS2 = NativeFileSystem::create( TEMP_DIR + SECOND_PATH );

	MultiFileSystem multiFS;

	multiFS.addBaseFileSystem( pFS1 );
	multiFS.addBaseFileSystem( pFS2 );

	// Test resolveToAbsolutePath
	const char * testFileData = "Any sufficiently advanced technology is "
			"indistinguishable from magic. -- Arthur C. Clarke";
	size_t testFileDataLen = strlen( testFileData );
	BinaryPtr	pTestFileDataBlock( 
			new BinaryBlock( testFileData, testFileDataLen, "std" ) );
	BW::string secondFullPath = "blah";
	CHECK( pFS2->writeFile( secondFullPath, pTestFileDataBlock, true ) );
	CHECK( multiFS.resolveToAbsolutePath( secondFullPath ) == 
			IFileSystem::FT_FILE );
	CHECK( secondFullPath == TEMP_DIR + SECOND_PATH + "blah" );
	BW::string firstFullPath = "blah";
	CHECK( pFS1->makeDirectory( firstFullPath ) );
	CHECK( multiFS.resolveToAbsolutePath( firstFullPath ) == 
			IFileSystem::FT_DIRECTORY );
	CHECK( firstFullPath == TEMP_DIR + FIRST_PATH + "blah" );
	BW::string nonExistPath = "i.dont.exist";
	CHECK( multiFS.resolveToAbsolutePath( nonExistPath ) == 
			IFileSystem::FT_NOT_FOUND );
	CHECK( nonExistPath == "i.dont.exist" );

	// Clean-up
	CHECK( pFS2->eraseFileOrDirectory( "blah" ) );
	CHECK( pFS1->eraseFileOrDirectory( "blah" ) );

	CHECK( pTempFS->eraseFileOrDirectory( FIRST_PATH ) );
	CHECK( pTempFS->eraseFileOrDirectory( SECOND_PATH ) );

	// clean up base dir
	removeDirectory( TEMP_DIR_NAME, BWResource::instance().fileSystem() );
}



#ifdef  _WIN32	// Windows only tests


// Test file name case-sensitive checking
TEST_F( ResMgrUnitTestHarness, WinFileSystem_test_filenameCaseCheckCorrectness )
{
	CHECK( this->isOK() );
	BW::string testName(this->m_name);

	MultiFileSystemPtr fileSystem = BWResource::instance().fileSystem();	
	touchDirectory( testName, fileSystem );

	BW::FilenameCaseChecker caseChecker1;
	BW::FilenameCaseChecker caseChecker2;
	BW::string fileName = testName + "\\_file1.txt";

	// incorrect case - all upper 
	BW::string upperFileName( fileName );
	std::transform( std::begin(upperFileName), std::end(upperFileName),
		std::begin(upperFileName), toupper );

	MF_ASSERT( upperFileName.compare( fileName ) != 0 );	// make sure it is different

	// absolute path versions
	BW::string absFileName(fileSystem->getAbsolutePath( fileName ));
	BW::string upperAbsFileName(fileSystem->getAbsolutePath( upperFileName ));

	// ensure file doesn't exist
	ensureFileNonExistant( fileName, fileSystem );
	ensureFileNonExistant( upperFileName, fileSystem );
	
	// test behaviour when file doesn't exist
	bool missingFileResult1 = caseChecker1.check( absFileName, false  );
	CHECK_EQUAL( missingFileResult1, true );

	// test non-exist behaviour on secondary checker. 
	// Note that this is uppercase value is cached as the correct one.
	bool missingFileResult2 = caseChecker2.check( upperAbsFileName, false  );
	CHECK_EQUAL( missingFileResult2, true );

	// make file - (not the upper case name)
	CHECK( touchFile( fileName, fileSystem ) );
	
	// check correct case
	bool lowerCaseResult1 = caseChecker1.check( absFileName, false );
	CHECK_EQUAL( lowerCaseResult1, true );

	// check incorrect case.
	bool lowerCaseResult2 = caseChecker1.check( upperAbsFileName, false );
	CHECK_EQUAL( lowerCaseResult2, false );

	// check upper case with uppercase version cached as correct (since didn't exist)
	// even though the file on disk is lower case. So we expect it to return true.
	//
	// Any path that doesn't exist gets added to the cache and considered
	// as case-correct. capitalized variants can keep being added until
	// a case-insensitive version of the path actually exists on disk.
	//
	// This test is not exactly expected/designed behaviour but is documenting
	// the observed behaviour so that developers are alerted when they change it.
	// If this is intended to be changed, then this check can be removed.
	bool upperCaseResult1 = caseChecker2.check( upperAbsFileName, false );
	CHECK_EQUAL( upperCaseResult1, true );

	// check lower case on upper-case cache.
	bool upperCaseResult2 = caseChecker2.check( absFileName, false );
	CHECK_EQUAL( upperCaseResult2, true );

	// clean up
	ensureFileNonExistant( fileName, fileSystem );
	ensureFileNonExistant( upperFileName, fileSystem );
	removeDirectory( this->m_name, fileSystem );
}


#endif	// _WIN32

BW_END_NAMESPACE

// test_file_system.cpp

