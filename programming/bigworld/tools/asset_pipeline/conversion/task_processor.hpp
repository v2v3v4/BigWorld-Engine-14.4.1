#ifndef ASSET_PIPELINE_TASK_PROCESSOR
#define ASSET_PIPELINE_TASK_PROCESSOR

#include "asset_pipeline/conversion/converter_map.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "resmgr/dataresource.hpp"

BW_BEGIN_NAMESPACE

class Compiler;
struct ConversionTask;
class DependencyList;
class TaskProcessorTask;

/// \brief Utility class for processing conversion tasks
///
/// Processes conversion tasks invoking the appropriate converter and
/// processing the tasks for all sub dependencies.
class TaskProcessor
{
public:
	TaskProcessor( Compiler & compiler,
				   ConverterMap & converters,
				   bool multiThreaded );
	~TaskProcessor();

	void forceRebuild( bool forceRebuild = true ) { forceRebuild_ = forceRebuild; }

	/// Process a specified conversion task.
	/// \param conversionTask the task to process.
	void processTask( ConversionTask & conversionTask );
	/// Process all conversion tasks in the compilers queue.
	void processTasks();

private:
	struct TaskContext
	{
		TaskContext( TaskProcessor & taskProcessor,
					 ConversionTask & conversionTask );

		ConversionTask & conversionTask_;
		BW::string relativeSource_;
	};

	struct DependencyContext
	{
		DependencyContext( TaskProcessor & taskProcessor,
						   ConversionTask & conversionTask );

		bool loadFromCache( bool secondaryDependencies, Compiler & compiler );

		DependencyList depList_;
		BW::string depListFileName_;
		DataResource depListResource_;
		DataSectionPtr depListRoot_;
	};

	struct ConversionContext
	{
		ConversionContext( TaskProcessor & taskProcessor,
						   ConversionTask & conversionTask );
		~ConversionContext();

		bool initConverter();

		ConverterInfo & converterInfo_;
		BW::string & converterParams_;
		Converter * converter_;

	private:
		ConversionContext( ConversionContext & o )
			: converterInfo_( o.converterInfo_ )
			, converterParams_( o.converterParams_ )
			, converter_( NULL )
		{
			MF_ASSERT( false );
		}
	};

	bool processNewTask( TaskContext & taskContext );

	bool processPrimaryDependencies( TaskContext & taskContext,
									 DependencyContext & dependencyContext,
									 ConversionContext & conversionContext );

	bool processSecondaryDependencies( TaskContext & taskContext,
									   DependencyContext & dependencyContext,
									   ConversionContext & conversionContext  );

	bool processConversion( TaskContext & taskContext,
							DependencyContext & dependencyContext,
							ConversionContext & conversionContext );

	bool checkPrimaryDependenciesUpToDate( TaskContext & taskContext,
										   DependencyContext & dependencyContext );

	bool retrievePrimaryDependencyListFromCache( DependencyContext & dependencyContext,
											     ConversionContext & conversionContext );

	bool hasSubTaskErrors( TaskContext & taskContext );

	bool hasOutputs( DependencyContext & dependencyContext );

	bool checkSecondaryDependenciesUpToDate( DependencyContext & dependencyContext );

	bool retrieveSecondaryDependencyListFromCache( DependencyContext & dependencyContext,
												   ConversionContext & conversionContext );

	bool checkIntermediateOutputsUpToDate( DependencyContext & dependencyContext,
										  ConversionContext & conversionContext );

	bool checkOutputsUpToDate( DependencyContext & dependencyContext,
							   ConversionContext & conversionContext );

	void updateSecondaryDependencies( DependencyContext & dependencyContext );

	void updateIntermediateOutputs( BW::vector< BW::string > & intermediateFiles,
									TaskContext & taskContext,
									DependencyContext & dependencyContext,
									ConversionContext & conversionContext );

	void updateOutputs( BW::vector< BW::string > & outputFiles,
						TaskContext & taskContext,
						DependencyContext & dependencyContext,
						ConversionContext & conversionContext );

	friend class TaskProcessorTask;
	/// Process all conversion tasks on a single thread.
	void processTasksOnSingleThread();
	/// Spawn multiple threads to process all conversion tasks in the compilers queue.
	void processTasksOnMultipleThreads();

	bool isThreadKillRequested();

private:
	Compiler &		compiler_;
	ConverterMap &	converterMap_;
	bw_atomic32_t	threadKillCount_;
	bool			multiThreaded_;
	bool			forceRebuild_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_TASK_PROCESSOR
