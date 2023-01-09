#ifndef BATCH_COMPILER
#define BATCH_COMPILER

#include "asset_pipeline/compiler/asset_compiler.hpp"
#include "plugin_system/plugin_loader.hpp"

#include "resmgr/datasection.hpp"

#include "cstdmf/debug_message_callbacks.hpp"

BW_BEGIN_NAMESPACE

class BatchCompiler : public AssetCompiler
					, public PluginLoader
{
public:
	BatchCompiler();
	virtual ~BatchCompiler();

	void build( const BW::vector< BW::string > & paths );
	void outputReport( const StringRef & filename );

	long getFailedCount() const { return taskFailedCount_; }

private:
	DataSectionPtr writeSection( DataSectionPtr pDataSection, const StringRef & id, const StringRef & classType );
	DataSectionPtr writeBreak( DataSectionPtr pDataSection );
	DataSectionPtr writeHeading( DataSectionPtr pDataSection, const StringRef & heading, uint8 size = 1 );
	DataSectionPtr writeText( DataSectionPtr pDataSection, const StringRef & text, const StringRef & style );
	DataSectionPtr writeLink( DataSectionPtr pDataSection, const StringRef & link, const StringRef & text, const StringRef & style );

public:
	virtual bool shouldIterateFile( const StringRef & file );
	virtual bool shouldIterateDirectory( const StringRef & directory );

	virtual void onTaskStarted( ConversionTask & conversionTask );
	virtual void onTaskResumed( ConversionTask & conversionTask );
	virtual void onTaskSuspended( ConversionTask & conversionTask );
	virtual void onTaskCompleted( ConversionTask & conversionTask );

	virtual void onPreCreateDependencies( ConversionTask & conversionTask );

	virtual void onPreConvert( ConversionTask & conversionTask );

	virtual void onOutputGenerated( const BW::string & filename );

	virtual void onCacheRead( const BW::string & filename );
	virtual void onCacheReadMiss( const BW::string & filename );
	virtual void onCacheWrite( const BW::string & filename );
	virtual void onCacheWriteMiss( const BW::string & filename );

	virtual bool handleMessage( DebugMessagePriority messagePriority, 
		const char * pCategory,
		DebugMessageSource messageSource,
		const LogMetaData & metaData,
		const char * pFormat,
		va_list argPtr );

public:
	struct TaskRecord
	{
		TaskRecord()
			: id_( -1 )
			, upToDate_( true )
			, skipped_( true ) 
			, hasError_( false )
			, hasWarning_( false )
			, duration_( 0 ) {}

		long id_;
		bool upToDate_;
		bool skipped_;
		bool hasError_;
		bool hasWarning_;
		double duration_;
		BW::vector<BW::string> outputs_;
		BW::string log_;
	};

private:
	// Records for every processed task
	BW::map<const ConversionTask *, TaskRecord *>	taskRecords_;
	SimpleMutex										taskRecordsMutex_;
	static THREADLOCAL( TaskRecord * )				s_currentTaskRecord;
	static THREADLOCAL( uint64 )					s_currentTaskStartTime;

	// General stats
	double			totalDuration_;
	volatile long	cacheReadCount_;
	volatile long	cacheReadMissCount_;
	volatile long	cacheWriteCount_;
	volatile long	cacheWriteMissCount_;

	// Discovery stats
	long			filesIterated_;
	long			directoriesIterated_;

	// Conversion stats
	volatile long	taskCount_;
	volatile long	taskFailedCount_;
	// A task is up to date if the dependencies are generated but
	// conversion is skipped
	volatile long	taskUpToDateCount_;
	// A task is skipped if both dependencies are up to date
	// and conversion are skipped
	volatile long	taskSkippedCount_;

};

BW_END_NAMESPACE

#endif //BATCH_COMPILER
