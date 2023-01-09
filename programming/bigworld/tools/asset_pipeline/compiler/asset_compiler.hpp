#ifndef ASSET_COMPILER
#define ASSET_COMPILER

#include "asset_pipeline/conversion/conversion_task_queue.hpp"
#include "asset_pipeline/conversion/task_processor.hpp"
#include "asset_pipeline/discovery/task_finder.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "compiler.hpp"
#include "generic_conversion_rule.hpp"

#define ASSET_PIPELINE_CAPTURE_ASSERTS 0

BW_BEGIN_NAMESPACE

/// Base class for all asset compilers.
/// Provides implementation for the essential call backs of the asset pipeline.
class AssetCompiler : public Compiler
					, public DebugMessageCallback
					, public CriticalMessageCallback
{
public:
	AssetCompiler();
	virtual ~AssetCompiler();

	void initCompiler();
	void finiCompiler();

	bool paused() const { return state_ == PAUSED; }
	bool executing() const { return state_ == EXECUTING; }
    bool terminating() const { return state_ == TERMINATING; }
	bool operating() const { return state_ == PAUSED || state_ == EXECUTING; }
	bool pause();
	bool resume();
	bool terminate();

	int getThreadCount() const;
	void setThreadCount(int numThreads);

	const BW::string & getIntermediatePath() const;
	void setIntermediatePath( const StringRef & path );

	const BW::string & getOutputPath() const;
	void setOutputPath( const StringRef & path );

	bool getForceRebuild() const;
	void setForceRebuild( bool force );

	bool getRecursive() const;
	void setRecursive( bool recurse );

	virtual void registerConversionRule( ConversionRule& conversionRule );
	virtual void registerConverter( ConverterInfo& converterInfo );
	virtual void registerResourceCallbacks( ResourceCallbacks & resourceCallbacks );

	virtual const BW::vector< BW::string > & getResourcePaths() const;

	virtual bool ensureUpToDate( const Dependency & dependency, const ConversionTask *& conversionTask );
	virtual bool getSourceFile( const BW::StringRef & file, BW::string & sourceFile );
	virtual uint64 getHash( const Dependency & dependency );
	virtual uint64 getFileHash( const BW::string & fileName, bool forceUpdate );
	virtual bool checkFileHashUpToDate( const BW::string & fileName );
	virtual uint64 getDirectoryHash( const BW::string & directory, const BW::string & pattern, bool regex, bool recursive );

	bool ensureCompiled( const BW::string & compiledFileName, bool output, const ConversionTask *& conversionTask );

	virtual void setError();
	virtual void setWarning();
	virtual void resetErrorFlags();
	virtual bool hasError() const;
	virtual bool hasWarning() const;

	virtual bool hasTasks();
	virtual ConversionTask * getNextTask();
	virtual void queueTask( ConversionTask & conversionTask );

	virtual bool shouldIterateFile( const BW::StringRef & file );
	virtual bool shouldIterateDirectory( const BW::StringRef & directory );

	virtual void onTaskStarted( ConversionTask & conversionTask );
	virtual void onTaskResumed( ConversionTask & conversionTask );
	virtual void onTaskSuspended( ConversionTask & conversionTask );
	virtual void onTaskCompleted( ConversionTask & conversionTask );

	virtual void onPreCreateDependencies( ConversionTask & conversionTask );
	virtual void onPostCreateDependencies( ConversionTask & conversionTask );

	virtual void onPreConvert( ConversionTask & conversionTask );
	virtual void onPostConvert( ConversionTask & conversionTask );

	virtual void onOutputGenerated( const BW::string & filename ) {}

	virtual void onCacheRead( const BW::string & filename ) {}
	virtual void onCacheReadMiss( const BW::string & filename ) {}
	virtual void onCacheWrite( const BW::string & filename ) {}
	virtual void onCacheWriteMiss( const BW::string & filename ) {}

	virtual bool handleMessage(
		DebugMessagePriority messagePriority, 
		const char * pCategory,
		DebugMessageSource messageSource, 
		const LogMetaData & metaData,
		const char * pFormat, 
		va_list argPtr );

	virtual void handleCritical( const char * msg );

	virtual bool resolveRelativePath( BW::string & path ) const;
	virtual bool resolveSourcePath( BW::string & path ) const;
	virtual bool resolveIntermediatePath( BW::string & path ) const;
	virtual bool resolveOutputPath( BW::string & path ) const;

	static BW::string sanitisePathParam( const char * pathParam );

protected:
	void addToolsResourcePaths();

	bool resolveCustomPath( const BW::string & basePath, BW::string & path ) const;

	BW::vector< HANDLE > compilerMutexes_;

	enum CompilerState
	{
		INVALID,
		EXECUTING,
		PAUSED,
		TERMINATING
	} state_;

	bool			recursive_;
	bool			forceRebuild_;
	int				numThreads_;
	BW::string		intermediatePath_;
	BW::string		outputPath_;

	BW::vector< BW::string > resourcePaths_;

	TaskFinder		taskFinder_;
	TaskProcessor	taskProcessor_;

	GenericConversionRule genericConversionRule_;
	ConversionRules conversionRules_;
	ConverterMap	converterMap_;
	BW::vector< ResourceCallbacks * > resourceCallbacks_;

	ConversionTaskQueue taskQueue_;
	SimpleMutex			taskQueueMutex_;

	HANDLE				taskSemaphore_;

	static THREADLOCAL( bool ) 					s_error;
	static THREADLOCAL( bool ) 					s_warning;
	static THREADLOCAL( ConversionTask* )		s_currentTask;
	static THREADLOCAL( ConversionTaskQueue* )	s_currentTaskQueue;
	static THREADLOCAL( bool )					s_CreatingDependencies;
	static THREADLOCAL( bool )					s_Converting;

	StringHashMap< uint64 >	fileHashes_;
	mutable ReadWriteLock	fileHashesLock_;

	/// an invalid task returned when the asset compiler is asked for a task to convert TO an 
	/// invalid compiled file. This is different to unknown tasks which are returned when the 
	/// asset pipeline is asked for a task to convert FROM an invalid source file.
	static const ConversionTask s_invalidTask;
};

BW_END_NAMESPACE

#endif
