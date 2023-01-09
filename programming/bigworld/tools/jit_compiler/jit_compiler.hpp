#ifndef JIT_COMPILER
#define JIT_COMPILER

#include "asset_pipeline/asset_server.hpp"
#include "asset_pipeline/compiler/asset_compiler.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/resource_modification_listener.hpp"

BW_BEGIN_NAMESPACE

class TaskStore;

class JITCompiler : public AssetCompiler
				  , public AssetServer
				  , public ResourceModificationListener
				  , public PluginLoader
{
public:
	explicit JITCompiler( TaskStore & store );
	virtual ~JITCompiler();

	void scanningThreadMain();
	void managingThreadMain();

	void stop();

private:
	void addReverseDependency( const BW::string & path,
		                       bool isOutput,
							   ConversionTask & conversionTask );
	void addReverseDependency( const BW::string & directory,
							   const BW::string & pattern,
							   bool regex,
							   bool recursive,
							   ConversionTask & conversionTask );
	void addReverseDependency( const Dependency & dependency, 
							   ConversionTask & conversionTask );
	void collectReverseDependencies( const BW::string & path, 
									 const BW::StringRef & filename, 
									 BW::vector< ConversionTask * > & conversionTasks );
	void collectReverseDependencies( const BW::string & path,
		                             bool includeOutputs,
									 BW::vector< ConversionTask * > & conversionTasks );

	// Resource callbacks
	void purgeResource( const StringRef & resourceId );

public:
	virtual void onAssetRequested( const StringRef & asset );
	virtual void lock();
	virtual void unlock();

	virtual void onResourceModified( const StringRef & basePath,
									 const StringRef & resourceID,
									 Action modType );

	virtual void onTaskStarted( ConversionTask & conversionTask );
	virtual void onTaskResumed( ConversionTask & conversionTask );
	virtual void onTaskSuspended( ConversionTask & conversionTask );
	virtual void onTaskCompleted( ConversionTask & conversionTask );

	virtual void onPreCreateDependencies( ConversionTask & conversionTask );
	virtual void onPostCreateDependencies( ConversionTask & conversionTask );

	virtual void onPreConvert( ConversionTask & conversionTask );
	virtual void onPostConvert( ConversionTask & conversionTask );

	virtual void onOutputGenerated( const BW::string & filename );

	virtual bool handleMessage( DebugMessagePriority messagePriority, 
		const char * pCategory,
		DebugMessageSource messageSource,
		const LogMetaData & metaData,
		const char * pFormat,
		va_list argPtr );

private:
	static THREADLOCAL( ConversionTask * ) st_currentTask_;

	SimpleMutex requestMutex_;
	BW::vector<std::pair<ConversionTask *, BW::string>> requests_;

	SimpleEvent event_;

	// reverse dependencies
	typedef BW::vector<BW::string> DirectoryDependencies;
	typedef BW::vector<std::pair<ConversionTask *, bool>> ReverseDependencies;
	typedef StringHashMap<ReverseDependencies> ReverseDependencyMap;
	typedef BW::vector<StringRef> ForwardDependencies;
	typedef BW::map<ConversionTask*, ForwardDependencies> ForwardDependencyMap;
	DirectoryDependencies directoryDependencies_;
	ReverseDependencyMap reverseDependencyMap_;
	ForwardDependencyMap forwardDependencyMap_;

	TaskStore & store_;
};

BW_END_NAMESPACE

#endif //JIT_COMPILER
