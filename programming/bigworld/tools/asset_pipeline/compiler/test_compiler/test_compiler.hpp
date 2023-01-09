#ifndef TEST_COMPILER_HPP
#define TEST_COMPILER_HPP

#include "asset_pipeline/compiler/asset_compiler.hpp"
#include "plugin_system/plugin_loader.hpp"

BW_BEGIN_NAMESPACE

class TestCompiler : public AssetCompiler
				   , public PluginLoader
{
public:
	TestCompiler( bool recursive, 
				  int numThreads, 
				  const StringRef & intermediatePath, 
				  const StringRef & outputPath );
	virtual ~TestCompiler();

	virtual bool shouldIterateDirectory( const BW::StringRef & directory );

	virtual void onTaskStarted( ConversionTask & conversionTask );
	virtual void onTaskCompleted( ConversionTask & conversionTask );

	virtual bool handleMessage( DebugMessagePriority messagePriority, 
		const char * pCategory,
		DebugMessageSource messageSource,
		const LogMetaData & metaData,
		const char * pFormat, 
		va_list argPtr );

	virtual bool resolveRelativePath( BW::string & path ) const;
	virtual bool resolveSourcePath( BW::string & path ) const;
	virtual bool resolveIntermediatePath( BW::string & path ) const;
	virtual bool resolveOutputPath( BW::string & path ) const;

	// Test that a conversion rule can be found for a source file
	bool testConversionRule( const StringRef & sourceFile, 
							 const StringRef & outputFile, 
							 const StringRef & converterName, 
							 const StringRef & converterParams );
	// Trigger a test build for a single file
	bool testBuildFile( const StringRef & file );
	// Trigger a test build for a directory
	bool testBuildDirectory( const StringRef & directory );

	// Set the directory where the source files for this test can be found
	bool setSourceFileDirectory( const StringRef & sourceFileDirectory );
	// Set the directory where the output files for this test can be found
	bool setOutputFileDirectory( const StringRef & outputFileDirectory );
	// Add a file from the source file directory that should be used in this test
	bool addSourceFile( const StringRef & sourceFile );
	// Add a file from the output file directory that should be built in this test
	bool addOutputFile( const StringRef & outputFile );

	// Get the number of tasks that were processed by this test
	long getTaskCount() const { return taskCount_; }
	// Get the number of tasks that were failed by this test
	long getTaskFailedCount() const { return taskFailedCount_; }
	// Returns if this test encountered a cyclic error
	bool hasCyclicError() const { return hasCyclicError_; }
	// Returns if this test encountered a dependency error
	bool hasDependencyError() const { return hasDependencyError_; }
	// Returns if this test encountered a conversion error
	bool hasConversionError() const { return hasConversionError_; }

private:
	// Setup the test files
	bool setupTest();
	// Verify the results of the test
	bool verifyTest();
	// Cleanup the test files
	bool cleanupTest();

private:
	BW::string testingDirectory_;
	BW::string sourceFileDirectory_;
	BW::string outputFileDirectory_;
	BW::vector<BW::string> sourceFiles_;
	BW::vector<BW::string> outputFiles_;

	volatile long	taskCount_;
	volatile long	taskFailedCount_;
	bool			hasCyclicError_;
	bool			hasDependencyError_;
	bool			hasConversionError_;

};

BW_END_NAMESPACE

#endif //TEST_COMPILER_HPP
