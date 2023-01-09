#include "test_compiler.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/discovery/conversion_rules.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

TestCompiler::TestCompiler( bool recursive, 
							int numThreads, 
							const StringRef & intermediatePath, 
							const StringRef & outputPath )
	: AssetCompiler()
	, taskCount_( 0 )
	, taskFailedCount_( 0 )
	, hasCyclicError_( false )
	, hasDependencyError_( false )
	, hasConversionError_( false )
{
	setRecursive(recursive);
	setForceRebuild(false);
	setThreadCount(numThreads);
	setIntermediatePath(intermediatePath);
	setOutputPath(outputPath);

	testingDirectory_ = BWResource::resolveFilename( "tools/asset_pipeline/resources/testfiles/" ) + "testing/";

	initCompiler();
}

TestCompiler::~TestCompiler()
{
	finiCompiler();
}

bool TestCompiler::shouldIterateDirectory( const BW::StringRef & directory )
{
	// Don't iterate into intermediate path
	if (!intermediatePath_.empty())
	{
		BW::string intermediatePath = testingDirectory_ + intermediatePath_;
		if (directory.substr( 0, intermediatePath.length() ) == intermediatePath)
		{
			return false;
		}
	}

	// Don't iterate into output path
	if (!outputPath_.empty())
	{
		BW::string outputPath = testingDirectory_ + outputPath_;
		if (directory.substr( 0, outputPath.length() ) == outputPath)
		{
			return false;
		}
	}

	// Don't iterate into svn folders
	StringRef svn = "/.svn";
	if (directory.find( svn ) != StringRef::npos)
	{
		return false;
	}

	return true;
}

void TestCompiler::onTaskStarted( ConversionTask & conversionTask )
{
	InterlockedIncrement( &taskCount_ );

	AssetCompiler::onTaskStarted( conversionTask );
}

void TestCompiler::onTaskCompleted( ConversionTask & conversionTask )
{
	if (conversionTask.status_ == ConversionTask::FAILED)
	{
		InterlockedIncrement( &taskFailedCount_ );
	}

	AssetCompiler::onTaskCompleted( conversionTask );
}


/*
 *	Override from DebugMessageCallback.
 */
bool TestCompiler::handleMessage( DebugMessagePriority messagePriority, 
								  const char * pCategory,
								  DebugMessageSource messageSource,
								  const LogMetaData & metaData,
								  const char * pFormat, 
								  va_list argPtr )
{
	char msg[2048];
	bw_vsnprintf( msg, 2048, pFormat, argPtr );

	if (strstr( msg, "Cyclic dependency" ))
	{
		hasCyclicError_ = true;
	}
	else if (strstr( msg, "DEPENDENCY ERROR!" ))
	{
		hasDependencyError_ = true;
	}
	else if (strstr( msg, "CONVERSION ERROR!" ))
	{
		hasConversionError_ = true;
	}

	return AssetCompiler::handleMessage( messagePriority,
										 pCategory,
										 messageSource,
										 metaData,
										 pFormat,
										 argPtr );
}

bool TestCompiler::resolveRelativePath( BW::string & path ) const
{
	if (BWResource::pathIsRelative( path ))
	{
		// Already relative
		return true;
	}

	if (!intermediatePath_.empty())
	{
		BW::string intermediatePath = testingDirectory_ + intermediatePath_;
		if (strncmp( path.c_str(), intermediatePath.c_str(), intermediatePath.length() ) == 0)
		{
			// The path provided is an absolute intermediate path
			// Transform it to an absolute source path and fall through
			size_t begin = intermediatePath.length();
			size_t end = path.length();
			path = testingDirectory_ + path.substr( begin, end - begin );
			return true;
		}
	}

	if (!outputPath_.empty())
	{
		BW::string outputPath = testingDirectory_ + outputPath_;
		if (strncmp( path.c_str(), outputPath.c_str(), outputPath.length() ) == 0)
		{
			// The path provided is an absolute output path
			// Transform it to an absolute source path and fall through
			size_t begin = outputPath.length();
			size_t end = path.length();
			path = testingDirectory_ + path.substr( begin, end - begin );
			return true;
		}
	}

	// The path provided is an absolute source path
	MF_ASSERT( path.length() < MAX_PATH );
	char relativePath[MAX_PATH];
	bw_str_copy( relativePath, MAX_PATH, path );
	if (!BWResource::resolveToRelativePathT( relativePath, MAX_PATH ))
	{
		return false;
	}
	path = relativePath;
	return true;
}

bool TestCompiler::resolveSourcePath( BW::string & path ) const
{
	if (BWResource::pathIsRelative( path ))
	{
		// The path provided is a relative path
		path = BWResource::resolveFilename( path );
		return true;
	}

	if (!intermediatePath_.empty())
	{
		BW::string intermediatePath = testingDirectory_ + intermediatePath_;
		if (strncmp( path.c_str(), intermediatePath.c_str(), intermediatePath.length() ) == 0)
		{
			// The path provided is an absolute intermediate path
			// Transform it to an absolute source path and fall through
			size_t begin = intermediatePath.length();
			size_t end = path.length();
			path = testingDirectory_ + path.substr( begin, end - begin );
			return true;
		}
	}

	if (!outputPath_.empty())
	{
		BW::string outputPath = testingDirectory_ + outputPath_;
		if (strncmp( path.c_str(), outputPath.c_str(), outputPath.length() ) == 0)
		{
			// The path provided is an absolute output path
			size_t begin = outputPath.length();
			size_t end = path.length();
			path = testingDirectory_ + path.substr( begin, end - begin );
			return true;
		}
	}

	// The path provided is already an absolute source path
	return true;
}

bool TestCompiler::resolveIntermediatePath( BW::string & path ) const
{
	if (intermediatePath_.empty())
	{
		if (!BWResource::pathIsRelative( path ))
		{
			// The path provided is already an absolute output path
			return true;
		}

		// The path provided is a relative path
		path = BWResource::resolveFilename( path );
		return true;
	}
	else
	{
		BW::string intermediatePath = testingDirectory_ + intermediatePath_;
		if (strncmp( path.c_str(), intermediatePath.c_str(), intermediatePath.length() ) == 0)
		{
			// The path provided is already an absolute output path
			return true;
		}

		if (BWResource::pathIsRelative( path ))
		{
			// The path provided is a relative path.
			BW::string relativeTestingDirectory = testingDirectory_;
			resolveRelativePath( relativeTestingDirectory );
			MF_ASSERT( strncmp( path.c_str(), relativeTestingDirectory.c_str(), relativeTestingDirectory.length() ) == 0);
			size_t begin = relativeTestingDirectory.length();
			size_t end = path.length();
			path = intermediatePath + path.substr( begin, end - begin );
			return true;
		}

		if (!outputPath_.empty())
		{
			BW::string outputPath = testingDirectory_ + outputPath_;
			if (strncmp( path.c_str(), outputPath.c_str(), outputPath.length() ) == 0)
			{
				// The path provided is an absolute output path
				size_t begin = outputPath.length();
				size_t end = path.length();
				path = intermediatePath + path.substr( begin, end - begin );
				return true;
			}
		}

		// The path provided is an absolute source path
		MF_ASSERT( strncmp( path.c_str(), testingDirectory_.c_str(), testingDirectory_.length() ) == 0);
		size_t begin = testingDirectory_.length();
		size_t end = path.length();
		path = intermediatePath + path.substr( begin, end - begin );
		return true;
	}
}

bool TestCompiler::resolveOutputPath( BW::string & path ) const
{
	if (outputPath_.empty())
	{
		if (!BWResource::pathIsRelative( path ))
		{
			// The path provided is already an absolute output path
			return true;
		}

		// The path provided is a relative path
		path = BWResource::resolveFilename( path );
		return true;
	}
	else
	{
		BW::string outputPath = testingDirectory_ + outputPath_;
		if (strncmp( path.c_str(), outputPath.c_str(), outputPath.length() ) == 0)
		{
			// The path provided is already an absolute output path
			return true;
		}

		if (BWResource::pathIsRelative( path ))
		{
			// The path provided is a relative path.
			BW::string relativeTestingDirectory = testingDirectory_;
			resolveRelativePath( relativeTestingDirectory );
			MF_ASSERT( strncmp( path.c_str(), relativeTestingDirectory.c_str(), relativeTestingDirectory.length() ) == 0);
			size_t begin = relativeTestingDirectory.length();
			size_t end = path.length();
			path = outputPath + path.substr( begin, end - begin );
			return true;
		}

		if (!intermediatePath_.empty())
		{
			BW::string intermediatePath = testingDirectory_ + intermediatePath_;
			if (strncmp( path.c_str(), intermediatePath.c_str(), intermediatePath.length() ) == 0)
			{
				// The path provided is an absolute intermediate path
				size_t begin = intermediatePath.length();
				size_t end = path.length();
				path = outputPath + path.substr( begin, end - begin );
				return true;
			}
		}

		// The path provided is an absolute source path
		MF_ASSERT( strncmp( path.c_str(), testingDirectory_.c_str(), testingDirectory_.length() ) == 0);
		size_t begin = testingDirectory_.length();
		size_t end = path.length();
		path = outputPath + path.substr( begin, end - begin );
		return true;
	}
}

bool TestCompiler::testConversionRule( const StringRef & sourceFile, 
									   const StringRef & outputFile, 
									   const StringRef & converterName, 
									   const StringRef & converterParams )
{
	bool res = false;
	if (setupTest())
	{
		BW::string sourcePath = testingDirectory_ + sourceFile;
		BW::string outputPath = testingDirectory_ + outputFile;
		resolveRelativePath( outputPath );

		// Search through all the available rules and see if any can match our output file
		ConversionRules::const_iterator it;
		for ( it = conversionRules_.cbegin(); it != conversionRules_.cend(); ++it )
		{
			BW::string _sourcePath;
			if ( (*it)->getSourceFile( outputPath, _sourcePath ))
			{
				// Verify that the source file returned is the one we expect
				res = ( _sourcePath == sourcePath );
				break;
			}
		}

		if (res)
		{
			// Get the task for the source file calling findTasks first in case we 
			// are testing a root conversion rule. GetTask only returns existing tasks
			// or new tasks for non root rules.
			taskFinder_.findTasks( sourcePath );
			ConversionTask & task = taskFinder_.getTask( sourcePath );
			ConverterMap::iterator it = converterMap_.find( task.converterId_ );
			// Verify the returned task uses the expected converter and converter params
			res = (it != converterMap_.end() &&
				   task.converterParams_ == converterParams &&
				   it->second->name_ == converterName);
		}
	}
	res &= cleanupTest();
	return res;
}

bool TestCompiler::testBuildFile( const StringRef & file )
{
	bool res = false;
	if (setupTest())
	{
		ConversionTask & task = taskFinder_.getTask( testingDirectory_ + file );
		queueTask( task );
		taskProcessor_.processTasks();
		res = verifyTest();
	}
	res &= cleanupTest();
	return res;
}

bool TestCompiler::testBuildDirectory( const StringRef & directory )
{
	bool res = false;
	if (setupTest())
	{
		taskFinder_.findTasks( testingDirectory_ + directory );
		taskProcessor_.processTasks();
		res = verifyTest();
	}
	res &= cleanupTest();
	return res;
}

bool TestCompiler::setSourceFileDirectory( const StringRef & sourceFileDirectory )
{
	if (!BWResource::isDir( sourceFileDirectory ))
	{
		ERROR_MSG( "Source file directory %s is not a valid directory.\n", 
			sourceFileDirectory.to_string().c_str() );
		return false;
	}
	sourceFileDirectory_ = BWResource::resolveFilename( sourceFileDirectory );
	return true;
}

bool TestCompiler::setOutputFileDirectory( const StringRef & outputFileDirectory )
{
	if (!BWResource::isDir( outputFileDirectory ))
	{
		ERROR_MSG( "Output file directory %s is not a valid directory.\n", 
			outputFileDirectory.to_string().c_str() );
		return false;
	}
	outputFileDirectory_ = BWResource::resolveFilename( outputFileDirectory );
	return true;
}

bool TestCompiler::addSourceFile( const StringRef & sourceFile )
{
	if (sourceFileDirectory_.empty())
	{
		ERROR_MSG( "Source file directory invalid. Cannot add source file.\n" );
		return false;
	}
	if (!BWResource::isFile( sourceFileDirectory_ + sourceFile ))
	{
		ERROR_MSG( "Source file %s is not a valid file.\n", 
			sourceFile.to_string().c_str() );
		return false;
	}
	sourceFiles_.push_back( sourceFile.to_string() );
	return true;
}

bool TestCompiler::addOutputFile( const StringRef & outputFile )
{
	if (outputFileDirectory_.empty())
	{
		ERROR_MSG( "Output file directory invalid. Cannot add output file.\n" );
		return false;
	}
	if (!BWResource::isFile( outputFileDirectory_ + outputFile ))
	{
		ERROR_MSG( "Source file %s is not a valid file.\n", 
			outputFile.to_string().c_str() );
		return false;
	}
	outputFiles_.push_back( outputFile.to_string() );
	return true;
}

namespace {

/**
 * This helper function is a work-around for copying a file's data and
 * ignoring metadata such as read-only flags.
 * It reads all data into memory first, so avoid for large files.
 */
bool copyFileData( FileSystemPtr fileSystem, 
				  const BW::StringRef & sourcePath,  
				  const BW::StringRef & destPath )
{
	BinaryPtr data = fileSystem->readFile( sourcePath );
	if ( data )
	{
		return fileSystem->writeFile( destPath, data, true );
	}
	return false;
}

}	// anonymous namespace



bool TestCompiler::setupTest()
{
	if (!cleanupTest())
	{
		return false;
	}

	// copy over testing files into temporary/testing directory
	for ( BW::vector<BW::string>::const_iterator it = 
		sourceFiles_.cbegin(); it != sourceFiles_.cend(); ++it )
	{
		BW::string sourceFile = sourceFileDirectory_ + *it;
		BW::string testFile = testingDirectory_ + *it;
		BWResource::ensurePathExists( testFile );

		bool copied = false;
		FileSystemPtr fileSystem = BWResource::instance().fileSystem();
		IFileSystem::FileType fileType = fileSystem->getFileTypeEx( sourceFile );

		if (fileType == IFileSystem::FT_DIRECTORY)
		{
			copied = fileSystem->copyFileOrDirectory( sourceFile, testFile );
		}
		else if (fileType != IFileSystem::FT_NOT_FOUND)
		{
			// copy file data while ignoring metadata such as read only flags.
			copied = copyFileData( fileSystem, sourceFile, testFile );
		}
		if (!copied)
		{
			ERROR_MSG( "Could not copy source file %s to testing location %s\n", 
				sourceFile.c_str(), testFile.c_str() );
			return false;
		}
	}

	BWResource::instance().flushModificationMonitor();

	taskCount_ = 0;
	taskFailedCount_ = 0;
	hasCyclicError_ = false;
	hasDependencyError_ = false;
	hasConversionError_ = false;

	return true;
}

bool TestCompiler::verifyTest()
{
	for ( BW::vector<BW::string>::const_iterator it = 
		outputFiles_.cbegin(); it != outputFiles_.cend(); ++it )
	{
		BW::string outputFile = outputFileDirectory_ + *it;
		BW::string testFile = testingDirectory_ + *it;
		
		BinaryPtr outputFilePtr = BWResource::instance().fileSystem()->readFile( outputFile );
		if (!outputFilePtr.hasObject())
		{
			ERROR_MSG( "Could not find output file %s\n", 
				outputFile.c_str() );
			return false;
		}

		BinaryPtr testFilePtr = BWResource::instance().fileSystem()->readFile( testFile );
		if (!testFilePtr.hasObject())
		{
			ERROR_MSG( "Could not find built output file %s\n", 
				testFile.c_str() );
			return false;
		}

		uint64 outputFileHash = Hash64::compute( outputFilePtr->data(), outputFilePtr->len() );
		uint64 testFileHash = Hash64::compute( testFilePtr->data(), testFilePtr->len() );
		if (outputFileHash != testFileHash)
		{
			ERROR_MSG( "Built output file %s does not match provided output file %s\n", 
				testFile.c_str(), outputFile.c_str() );
			return false;
		}
	}

	return true;
}

bool TestCompiler::cleanupTest()
{
	if (!BWResource::instance().fileExists( testingDirectory_ ))
	{
		return true;
	}

	if (!BWResource::instance().fileSystem()->eraseFileOrDirectory( testingDirectory_ ))
	{
		ERROR_MSG( "Could not remove testing directory %s", 
			testingDirectory_.c_str() );
		return false;
	}

	BWResource::instance().flushModificationMonitor();

	return true;
}

BW_END_NAMESPACE
