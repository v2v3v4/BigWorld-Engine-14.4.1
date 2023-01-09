#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/dependency/converter_dependency.hpp"
#include "asset_pipeline/dependency/converter_params_dependency.hpp"
#include "asset_pipeline/dependency/source_file_dependency.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/xml_section.hpp"
#include <xmemory>

#include "content_addressable_cache.hpp"
#include "conversion_task.hpp"
#include "converter.hpp"
#include "task_processor.hpp"

DECLARE_DEBUG_COMPONENT2( ASSET_PIPELINE_CATEGORY, 0 )

BW_BEGIN_NAMESPACE

/// Background task for processing conversion tasks on a thread.
class TaskProcessorTask : public BackgroundTask
{
public:
	TaskProcessorTask( TaskProcessor & taskProcessor )
		: BackgroundTask( "Task Processor" )
		, taskProcessor_( taskProcessor ) {}

	virtual ~TaskProcessorTask() {}

protected:
	virtual void doBackgroundTask( TaskManager & mgr )
	{
		// simply call process tasks on the task processor which will
		// iterate over all tasks in the compilers queue.
		taskProcessor_.processTasksOnSingleThread();
	}

private:
	TaskProcessor & taskProcessor_;
};

class ConverterGuard
{
public:
	ConverterGuard( ConverterInfo & converterInfo )
	{
		threadSafe_ = ( (converterInfo.flags_ & ConverterInfo::THREAD_SAFE) != 0 );

		if (threadSafe_)
		{
			while (s_pendingWrites > 0)
			{
				// If we have pending exclusive tasks allow them to run first.
				Sleep( 0 );
			}
			// This task is thread safe so acquire a shared read lock
			s_lock_.beginRead();
			return;
		}
		
		InterlockedIncrement( &s_pendingWrites );
		// This task is not thread safe so acquire an exclusive write lock
		s_lock_.beginWrite();
		InterlockedDecrement( &s_pendingWrites );
	}

	~ConverterGuard()
	{
		if (threadSafe_)
		{
			s_lock_.endRead();
		}
		else
		{
			s_lock_.endWrite();
		}
	}

private:
	bool threadSafe_;
	static ReadWriteLock s_lock_;
	static volatile long s_pendingWrites;
};

ReadWriteLock ConverterGuard::s_lock_;
volatile long ConverterGuard::s_pendingWrites = 0;

TaskProcessor::TaskProcessor( Compiler & compiler,
							  ConverterMap & converterMap,
							  bool multiThreaded )
: converterMap_( converterMap )
, compiler_( compiler )
, threadKillCount_( 0 )
, multiThreaded_( multiThreaded )
, forceRebuild_( false )
{

}

TaskProcessor::~TaskProcessor()
{

}

TaskProcessor::TaskContext::TaskContext( TaskProcessor & taskProcessor,
										 ConversionTask & conversionTask )
	: conversionTask_( conversionTask )
{
	relativeSource_ = conversionTask_.source_;
	if (!taskProcessor.compiler_.resolveRelativePath( relativeSource_ ))
	{
		relativeSource_ = "";
	}
}

TaskProcessor::DependencyContext::DependencyContext( TaskProcessor & taskProcessor,
													 ConversionTask & conversionTask )
	: depList_( taskProcessor.compiler_ )
{
	// Attempt to load the dependency list associated with this task.
	// If a .deps file does not exist an empty file will be created.
	depListFileName_ = conversionTask.source_ + ".deps";
	MF_VERIFY( taskProcessor.compiler_.resolveIntermediatePath( depListFileName_ ) );
	BWResource::ensureAbsolutePathExists( depListFileName_ );
	depListResource_.load( depListFileName_, RESOURCE_TYPE_XML, true );
	depListRoot_ = depListResource_.getRootSection();
	MF_ASSERT( depListRoot_ != NULL );
	depList_.serialiseIn( depListRoot_ );
}

bool TaskProcessor::DependencyContext::loadFromCache( bool secondaryDependencies, Compiler & compiler )
{
	const BW::string file = BWResource::getFilename( depListFileName_ ).to_string();
	const BW::string directory = BWResource::getFilePath( depListFileName_ );

	// Make a backup of the dependency list
	XMLSectionPtr tempDepListRoot_ = new XMLSection( "root" );
	depList_.serialiseOut( tempDepListRoot_ );

	uint64 hash = depList_.getInputHash( secondaryDependencies );
	if (ContentAddressableCache::readFromCache( depListFileName_, hash, compiler ))
	{
		depListResource_.reload();
		depListRoot_ = depListResource_.getRootSection();

		bool depListValid = true;
		if (depListRoot_ != NULL)
		{
			depList_.serialiseIn( depListRoot_ );

			BW::vector< DependencyList::Input > & primaryDeps = depList_.primaryInputs_;
			for ( BW::vector< DependencyList::Input >::iterator it = primaryDeps.begin(); 
				it != primaryDeps.end() && depListValid; ++it )
			{
				// The hashes of all our primary dependencies should match
				uint64 hash = compiler.getHash( *it->first );
				depListValid &= (it->second == hash);
			}

			BW::vector< DependencyList::Input > & secondaryDeps = depList_.secondaryInputs_;
			for ( BW::vector< DependencyList::Input >::iterator it = secondaryDeps.begin(); 
				it != secondaryDeps.end() && depListValid; ++it )
			{
				// The hashes of all our secondary dependencies should be zero
				uint64 hash = secondaryDependencies ? compiler.getHash( *it->first ) : 0;
				depListValid &= (it->second == hash);
			}
		}
		else
		{
			depListValid = false;
		}

		if (!depListValid)
		{
			WARNING_MSG( "Corrupted dependency list retrieved from the cache: %s.%IX\n",
				file.c_str(), hash );

			// the cache file was corrupted. try to restore the backup
			depListRoot_->copy( tempDepListRoot_ );
			depListResource_.save();

			depList_.serialiseIn( depListRoot_ );
			return false;
		}
	}

	return false;
}

TaskProcessor::ConversionContext::ConversionContext( TaskProcessor & taskProcessor,
													 ConversionTask & conversionTask )
	// Get the converter info associated with the task
	: converterInfo_( *taskProcessor.converterMap_[conversionTask.converterId_] )
	, converterParams_( conversionTask.converterParams_ )
	, converter_( NULL )
{
}

bool TaskProcessor::ConversionContext::initConverter()
{
	// Create a converter if we haven't done so already
	if (converter_ == NULL)
	{
		converter_ = converterInfo_.creator_( converterParams_ );
	}

	return converter_ != NULL;
}

TaskProcessor::ConversionContext::~ConversionContext()
{
	delete converter_;
}

void TaskProcessor::processTask( ConversionTask & conversionTask )
{
	if (conversionTask.fileHandle_ == INVALID_HANDLE_VALUE)
	{
		ERROR_MSG( "FAILED: Could not acquire source handle.\n" );
		conversionTask.status_ = ConversionTask::FAILED;
		return;
	}

	// Sanity check that the task being processed is actually in a processing stage
	MF_ASSERT( conversionTask.status_ >= ConversionTask::PROCESSING )
	MF_ASSERT( conversionTask.status_ < ConversionTask::DONE )
	if (conversionTask.status_ >= ConversionTask::DONE)
	{
		return;
	}

	// Setup the task context
	TaskContext taskContext( *this, conversionTask );

	if (conversionTask.status_ == ConversionTask::PROCESSING)
	{
		if (!processNewTask( taskContext ))
		{
			MF_ASSERT( conversionTask.status_ == ConversionTask::FAILED );
			return;
		}
		MF_ASSERT( conversionTask.status_ == ConversionTask::NEEDS_PRIMARY_DEPS );
	}
	
	DependencyContext dependencyContext( *this, conversionTask );
	ConversionContext conversionContext( *this, conversionTask );

	if (conversionTask.status_ == ConversionTask::NEEDS_PRIMARY_DEPS)
	{
		if (!processPrimaryDependencies( taskContext, dependencyContext, conversionContext ))
		{
			MF_ASSERT( conversionTask.status_ == ConversionTask::FAILED );
			return;
		}
		MF_ASSERT( conversionTask.status_ == ConversionTask::NEEDS_SECONDARY_DEPS );
	}

	if (conversionTask.status_ == ConversionTask::NEEDS_SECONDARY_DEPS)
	{
		if (!processSecondaryDependencies( taskContext, dependencyContext, conversionContext ))
		{
			MF_ASSERT( conversionTask.status_ == ConversionTask::NEEDS_CONVERSION ||
					   conversionTask.status_ == ConversionTask::FAILED );
			return;
		}
		MF_ASSERT( conversionTask.status_ == ConversionTask::NEEDS_CONVERSION );
	}

	if (conversionTask.status_ == ConversionTask::NEEDS_CONVERSION)
	{
		if (!processConversion( taskContext, dependencyContext, conversionContext ))
		{
			MF_ASSERT( conversionTask.status_ == ConversionTask::FAILED );
			return;
		}
		MF_ASSERT( conversionTask.status_ == ConversionTask::DONE );
	}
}

bool TaskProcessor::processNewTask( TaskContext & taskContext )
{
	if (taskContext.relativeSource_.empty())
	{
		ERROR_MSG( "FAILED: Could not resolve relative path for %s.\n",
			taskContext.conversionTask_.source_.c_str() );
		taskContext.conversionTask_.status_ = ConversionTask::FAILED;
		return false;
	}

	BW::string resolvedSource = BWResource::resolveFilename( taskContext.relativeSource_ );
	if (taskContext.conversionTask_.source_ != resolvedSource)
	{
		ERROR_MSG( "FAILED: Found %s while trying to compile %s. This asset will be ignored.\n",
			resolvedSource.c_str(), taskContext.conversionTask_.source_.c_str() );
		taskContext.conversionTask_.status_ = ConversionTask::FAILED;
		return false;
	}

	taskContext.conversionTask_.status_ = ConversionTask::NEEDS_PRIMARY_DEPS;
	return true;
}

bool TaskProcessor::processPrimaryDependencies( TaskContext & taskContext,
											    DependencyContext & dependencyContext,
											    ConversionContext & conversionContext  )
{
	INFO_MSG( "Checking primary dependencies...\n" );

	// Check if the primary dependencies are up to date
	bool needsDeps = forceRebuild_;

	if (!needsDeps)
	{
		// We need to regenerate our dependencies if any of our 
		// primary dependencies are not up to date
		needsDeps = !checkPrimaryDependenciesUpToDate( taskContext, 
													   dependencyContext );
	}

	if (needsDeps)
	{
		// Setup the primary dependencies for this task.
		// This is the same for all assets so we don't need to ask the converter to do it for us.
		dependencyContext.depList_.initialise( taskContext.relativeSource_,
			taskContext.conversionTask_.converterId_, 
			taskContext.conversionTask_.converterVersion_,
			taskContext.conversionTask_.converterParams_ );

		// We don't need to regenerate our dependencies if we can retrieve them
		// from the cache
		needsDeps = !retrievePrimaryDependencyListFromCache( dependencyContext, 
														     conversionContext );
	}

	if (needsDeps)
	{
		INFO_MSG( "  Primary dependencies out of date\n" );
		// One of our primary dependencies have changed or previously did not exist on disk.
		// Thus we can't trust the dependency list we loaded and must ask the converter to
		// generate a new one for us.

		// Initialise the converter
		MF_VERIFY( conversionContext.initConverter() );

		MF_ASSERT( !compiler_.hasError() );
		compiler_.onPreCreateDependencies( taskContext.conversionTask_ );
		bool res = true;
		// Wrap the call to create dependencies in a try catch block.
		// While in the converter any assert will throw an exception rather than aborting the application.
		// We use this to catch errors and fail the task.
		try
		{
			INFO_MSG( "Creating dependencies...\n" );
			ConverterGuard converterGuard( conversionContext.converterInfo_ );
			res = conversionContext.converter_->createDependencies( taskContext.conversionTask_.source_, compiler_, dependencyContext.depList_ );
		}
		catch (...)
		{
			res = false;
		}
		compiler_.onPostCreateDependencies( taskContext.conversionTask_ );

		if (!res || compiler_.hasError())
		{
			ERROR_MSG( "FAILED: Errors creating dependencies\n" );
			// The converter returned false, an assert was triggered or someone used the ERROR_MSG macro
			taskContext.conversionTask_.status_ = ConversionTask::FAILED;
			return false;
		}

		// Save out the generated dependency list if there were no warnings.
		if (!compiler_.hasWarning() && dependencyContext.depListRoot_ != NULL)
		{
			dependencyContext.depList_.serialiseOut( dependencyContext.depListRoot_ );
			dependencyContext.depListResource_.save();

			// Write our generated dependency list to the cache without the secondary dependencies
			if ((conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_DEPENDENCIES) != 0)
			{
				ContentAddressableCache::writeToCache( dependencyContext.depListFileName_, 
					dependencyContext.depList_.getInputHash( false ), compiler_);
			}
		}
	}
	else
	{
		INFO_MSG( "  Primary dependencies up to date\n" );
		// All our primary dependencies match what is the dependency list on disk
		// so all our secondary dependencies should already be in the dependency list.
		// Thus we don't need to ask the converter to generate our dependencies.
	}

	taskContext.conversionTask_.status_ = ConversionTask::NEEDS_SECONDARY_DEPS;
	return true;
}

bool TaskProcessor::processSecondaryDependencies( TaskContext & taskContext,
												  DependencyContext & dependencyContext,
												  ConversionContext & conversionContext  )
{
	INFO_MSG( "Processing secondary dependencies...\n" );

	// We now need to go through all our secondary dependencies and make sure they are up to date
	// before we can convert our task.
	bool blocked = false;
	bool subTaskError = false;

	BW::vector< DependencyList::Input > & secondaryDeps = dependencyContext.depList_.secondaryInputs_;
	BW::vector< DependencyList::Input >::const_iterator it;
	for ( it = secondaryDeps.begin(); it != secondaryDeps.end(); ++it )
	{
		const ConversionTask * secondaryTask = NULL;
		bool upToDate = compiler_.ensureUpToDate( *it->first, secondaryTask );
		// The secondary dependency has failed if we couldn't get it up to date
		bool failed = !upToDate;
		if (secondaryTask != NULL)
		{
			if (secondaryTask->status_ == ConversionTask::FAILED ||
				secondaryTask->threadId_ == GetCurrentThreadId())
			{
				// If the secondary dependency is a task and it has failed, or if it
				// has been blocked by the current thread, then mark it as up to date. 
				// We can't do anything else with it.
				upToDate = true;
			}
			else
			{
				// If the secondary dependency is a task and it's still processing then
				// it actually hasn't failed and is just not up to date yet.
				failed = false;
			}

			// Add this secondary task to our sub tasks
			taskContext.conversionTask_.subTasks_.push_back( std::make_pair( secondaryTask, it->first->isCritical() ) );
		}

		if (failed)
		{
			if (it->first->isCritical())
			{
				if (secondaryTask != NULL)
				{
					ERROR_MSG( "Errors processing critical dependent task %s\n", secondaryTask->source_.c_str() );
				}
				// A critical secondary dependency has failed.
				// We cannot convert this task without this dependency so fail.
				subTaskError = true;
			}
			else
			{
				upToDate = true;
			}
		}

		// We are blocked if our secondary dependency is not up to date
		blocked |= !upToDate;
	}

	if (subTaskError)
	{
		ERROR_MSG( "FAILED: Errors processing dependencies\n" );
		taskContext.conversionTask_.status_ = ConversionTask::FAILED;
		return false;
	}

	// If we are blocked then return from being processed. This will put our task back into
	// queue but we will be guaranteed that all our secondary dependencies will be up to date
	// the next time we process this task.
	taskContext.conversionTask_.status_ = ConversionTask::NEEDS_CONVERSION;
	return !blocked;
}

bool TaskProcessor::processConversion( TaskContext & taskContext,
									   DependencyContext & dependencyContext,
									   ConversionContext & conversionContext  )
{
	// check if any of our critical sub tasks failed to build
	if (hasSubTaskErrors( taskContext ))
	{
		ERROR_MSG( "FAILED: Errors processing dependencies\n" );
		taskContext.conversionTask_.status_ = ConversionTask::FAILED;
		return false;
	}

	INFO_MSG( "Checking conversion...\n" );

	bool needsConversion = forceRebuild_;

	if (!needsConversion)
	{
		// we need to convert if our dependency list has no outputs
		needsConversion = !hasOutputs( dependencyContext );
	}

	if (!needsConversion)
	{
		// we need to convert if any of secondary dependencies are not up to date
		needsConversion = !checkSecondaryDependenciesUpToDate( dependencyContext );
	}

	if (needsConversion)
	{
		// update the stored hashes of our secondary dependencies
		updateSecondaryDependencies( dependencyContext );

		// we don't need to convert if we can retrieve an up to date dependency list
		// from the cache
		needsConversion = !retrieveSecondaryDependencyListFromCache( dependencyContext,
																	 conversionContext );
	}

	if (!needsConversion)
	{
		// after potentially retrieve a dependency list from the cache,
		// check if we still don't have outputs
		needsConversion = !hasOutputs( dependencyContext );
	}

	if (!needsConversion)
	{
		// we need to convert if our intermediate outputs are not up to date
		// and we can't retrieve them from the cache
		needsConversion = !checkIntermediateOutputsUpToDate( dependencyContext,
															 conversionContext );
	}

	if (!needsConversion)
	{
		// we need to convert if our outputs are not up to date
		// and we can't retrieve them from the cache
		needsConversion = !checkOutputsUpToDate( dependencyContext,
												 conversionContext );
	}

	if (needsConversion)
	{
		BW::vector< BW::string > intermediateFiles;
		BW::vector< BW::string > outputFiles;

		// Initialise the converter
		MF_VERIFY( conversionContext.initConverter() );

		// If the task is an upgrade, we need to release the file handle
		// so that the source can be written to by the converter
		if (( conversionContext.converterInfo_.flags_ & ConverterInfo::UPGRADE_CONVERSION ) != 0)
		{
			::CloseHandle( taskContext.conversionTask_.fileHandle_ );
			taskContext.conversionTask_.fileHandle_ = NULL;
		}

		MF_ASSERT( !compiler_.hasError() );
		compiler_.onPreConvert( taskContext.conversionTask_ );
		bool res = true;
		// Wrap the call to convert in a try catch block.
		// While in the converter any assert will throw an exception rather than aborting the application.
		// We use this to catch errors and fail the task.
		try
		{
			INFO_MSG( "Converting task...\n" );
			ConverterGuard converterGuard( conversionContext.converterInfo_ );
			res = conversionContext.converter_->convert( taskContext.conversionTask_.source_, compiler_, intermediateFiles, outputFiles );
		}
		catch (...)
		{
			res = false;
		}
		compiler_.onPostConvert( taskContext.conversionTask_ );

		// update the stored hashes of our intermediate outputs
		// and push them to the cache
		updateIntermediateOutputs( intermediateFiles,
								   taskContext,
								   dependencyContext,
								   conversionContext );
		// update the stored hashes of our outputs
		// and push them to the cache
		updateOutputs( outputFiles,
					   taskContext,
					   dependencyContext,
					   conversionContext );

		if (!res || compiler_.hasError())
		{
			ERROR_MSG( "FAILED: Errors converting task\n" );
			// The converter returned false, an assert was triggered or someone used the ERROR_MSG macro
			taskContext.conversionTask_.status_ = ConversionTask::FAILED;
			return false;
		}

		if (intermediateFiles.empty() && outputFiles.empty())
		{
			ERROR_MSG( "FAILED: No outputs were generated\n" );
			// The converter did not add any output files to the output list
			taskContext.conversionTask_.status_ = ConversionTask::FAILED;
			return false;
		}

		// Serialise out our updated dependency list if there were no warnings
		if (!compiler_.hasWarning() && dependencyContext.depListRoot_ != NULL)
		{
			dependencyContext.depList_.serialiseOut( dependencyContext.depListRoot_ );
			dependencyContext.depListResource_.save();

			// Write our generated dependency list to the cache with the secondary dependencies
			if ((conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION) != 0)
			{
				ContentAddressableCache::writeToCache( dependencyContext.depListFileName_, 
					dependencyContext.depList_.getInputHash( true ), compiler_);
			}
		}

		INFO_MSG( "DONE: Task completed\n" );
	}
	else
	{
		// Don't need to convert this task. Just mark it as done.
		TRACE_MSG( "Current output files were built with existing input files. Skipping conversion\n" );
		INFO_MSG( "DONE: Task skipped\n" );
	}

	taskContext.conversionTask_.status_ = ConversionTask::DONE;
	return true;
}

bool TaskProcessor::checkPrimaryDependenciesUpToDate( TaskContext & taskContext,
												      DependencyContext & dependencyContext )
{
	BW::vector< DependencyList::Input > & primaryDeps = dependencyContext.depList_.primaryInputs_;
	size_t numPrimaryDeps = primaryDeps.size();
	if (numPrimaryDeps < 3)
	{
		// Our deplist should always have at least 3 primary dependencies.
		// If it doesn't, we need to regenerate it.
		return false;
	}

	for (size_t i = 0; i < numPrimaryDeps; ++i)
	{
		DependencyList::Input & dependency = primaryDeps[i];
		if (dependency.second != compiler_.getHash( *dependency.first ))
		{
			// The stored dependency hash does not match what is on disk
			return false;
		}

		switch (i)
		{
		case 0:
			{
				SourceFileDependency * sourcefileDependency = dynamic_cast< SourceFileDependency * >( dependency.first );
				if (sourcefileDependency == NULL ||
					sourcefileDependency->getFileName() != taskContext.relativeSource_)
				{
					// The first of the primary dependencies should always be a dependency on the source file.
					return false;
				}
			}
			break;

		case 1:
			{
				ConverterDependency * converterDependency = dynamic_cast< ConverterDependency * >( dependency.first );
				if (converterDependency == NULL ||
					converterDependency->getConverterId() != taskContext.conversionTask_.converterId_ ||
					converterDependency->getConverterVersion() != taskContext.conversionTask_.converterVersion_)
				{
					// The secondary of the primary dependencies should always be a converter dependency.
					return false;
				}
			}
			break;

		case 2:
			{
				ConverterParamsDependency * converterParamsDependency = dynamic_cast< ConverterParamsDependency * >( dependency.first );
				if (converterParamsDependency == NULL ||
					converterParamsDependency->getConverterParams() != taskContext.conversionTask_.converterParams_)
				{
					// The last of the primary dependencies should always be a converter params dependency.
					return false;
				}
			}
			break;

		default:
			break;
		}
	}

	return true;
}

bool TaskProcessor::retrievePrimaryDependencyListFromCache( DependencyContext & dependencyContext,
														    ConversionContext & conversionContext )
{
	// Check if we can pull the deplist from the cache
	if (!forceRebuild_ &&
		( conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_DEPENDENCIES ) != 0 )
	{
		if (dependencyContext.loadFromCache( false, compiler_ ))
		{
			INFO_MSG( "  Dependency list retrieved from cache\n" );
			// We have found a deplist in the cache and copied to our local drive.
			// Need to reload the deplist from disk.
			return true;
		}
	}

	return false;
}

bool TaskProcessor::hasSubTaskErrors( TaskContext & taskContext )
{
	bool subTaskError = false;
	for ( ConversionTask::SubTaskList::const_iterator it = taskContext.conversionTask_.subTasks_.begin(); 
		it != taskContext.conversionTask_.subTasks_.end(); ++it )
	{
		if (it->first->status_ < ConversionTask::DONE)
		{
			// All sub tasks should have been completed by this point.
			// Something has gone wrong
			subTaskError = true;
		}

		if (it->second == true && it->first->status_ == ConversionTask::FAILED)
		{
			// A critical secondary dependency has failed.
			// We cannot convert this task without this dependency so fail.
			ERROR_MSG( "Errors processing critical dependent task %s\n", it->first->source_.c_str() );
			subTaskError = true;
		}
	}
	return subTaskError;
}

bool TaskProcessor::hasOutputs( DependencyContext & dependencyContext )
{
	return !dependencyContext.depList_.intermediateOutputs_.empty() || 
		!dependencyContext.depList_.outputs_.empty();
}

bool TaskProcessor::checkSecondaryDependenciesUpToDate( DependencyContext & dependencyContext )
{
	BW::vector< DependencyList::Input > & secondaryDeps = dependencyContext.depList_.secondaryInputs_;
	for ( BW::vector< DependencyList::Input >::iterator it = secondaryDeps.begin(); 
		it != secondaryDeps.end(); ++it )
	{
		// If the hash of any of our secondary dependencies do not match what is stored
		// in our dependency list then we need to convert this task
		if (it->second != compiler_.getHash( *it->first ))
		{
			return false;
		}
	}

	return true;
}

bool TaskProcessor::retrieveSecondaryDependencyListFromCache( DependencyContext & dependencyContext,
											  ConversionContext & conversionContext )
{
	//Check the cache for a dependency list with our secondary dependencies
	if (!forceRebuild_ &&
		( conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION ) != 0 )
	{
		if (dependencyContext.loadFromCache( true, compiler_ ))
		{
			INFO_MSG( "  Output list retrieved from cache\n" );
			// We have found a deplist in the cache and copied to our local drive.
			// Need to reload the deplist from disk.

			return true;
		}
	}

	return false;
}

bool TaskProcessor::checkIntermediateOutputsUpToDate( DependencyContext & dependencyContext,
													  ConversionContext & conversionContext )
{
	BW::vector< DependencyList::Output > & intermediateOutputs = dependencyContext.depList_.intermediateOutputs_;

	BW::vector< DependencyList::Output >::const_iterator it;
	for ( it = intermediateOutputs.begin(); it != intermediateOutputs.end(); ++it )
	{
		// If the hash of any of our output files do not match what is stored
		// in our dependency list then we need to convert this task
		BW::string output = it->first;
		compiler_.resolveIntermediatePath( output );
		uint64 hash = compiler_.getFileHash( output, false );
		if (it->second != hash)
		{
			if (( conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION ) != 0 &&
				ContentAddressableCache::readFromCache( output, it->second, compiler_ ))
			{
				if (it->second != compiler_.getFileHash( output, true ))
				{
					WARNING_MSG( "Corrupted intermediate output retrieved from the cache: %s.%IX\n",
						BWResource::getFilename( output ).to_string().c_str(), hash );
					return false;
				}

				INFO_MSG( "  Output %s retrieved from cache\n", output.c_str() );
				continue;
			}
			return false;
		}
	}

	return true;
}

bool TaskProcessor::checkOutputsUpToDate( DependencyContext & dependencyContext,
										  ConversionContext & conversionContext )
{
	BW::vector< DependencyList::Output > & outputs = dependencyContext.depList_.outputs_;

	BW::vector< DependencyList::Output >::const_iterator it;
	for ( it = outputs.begin(); it != outputs.end(); ++it )
	{
		// If the hash of any of our output files do not match what is stored
		// in our dependency list then we need to convert this task
		BW::string output = it->first;
		compiler_.resolveOutputPath( output );
		uint64 hash = compiler_.getFileHash( output, false );
		if (it->second != hash)
		{
			if (( conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION ) != 0&&
				ContentAddressableCache::readFromCache( output, it->second, compiler_ ))
			{
				if (it->second != compiler_.getFileHash( output, true ))
				{
					WARNING_MSG( "Corrupted output retrieved from the cache: %s.%IX\n",
						BWResource::getFilename( output ).to_string().c_str(), hash );
					return false;
				}
				
				INFO_MSG( "  Output %s retrieved from cache\n", output.c_str() );
				continue;
			}
			return false;
		}
	}

	return true;
}

void TaskProcessor::updateSecondaryDependencies( DependencyContext & dependencyContext )
{
	BW::vector< DependencyList::Input > & secondaryDeps = dependencyContext.depList_.secondaryInputs_;
	for ( BW::vector< DependencyList::Input >::iterator it = secondaryDeps.begin(); 
		it != secondaryDeps.end(); ++it )
	{
		it->second = compiler_.getHash( *it->first );
	}
}

void TaskProcessor::updateIntermediateOutputs( BW::vector< BW::string > & intermediateFiles,
											   TaskContext & taskContext,
											   DependencyContext & dependencyContext,
											   ConversionContext & conversionContext )
{
	BW::vector< DependencyList::Output > & intermediateOutputs = dependencyContext.depList_.intermediateOutputs_;
	intermediateOutputs.clear();

	for ( BW::vector< BW::string >::const_iterator it = intermediateFiles.begin(); 
		it != intermediateFiles.end(); ++it )
	{
		// Convert all the intermediate files to relative paths and push them into
		// our intermediate output list
		BW::string output = *it;
		BW::string relativeOutput = *it;
		compiler_.resolveIntermediatePath( output );
		compiler_.resolveRelativePath( relativeOutput );

		INFO_MSG( "  %s ->\n", taskContext.conversionTask_.source_.c_str() );
		INFO_MSG( "      %s\n", output.c_str() );
		// Update the hash of all our output files. Make sure we specify force update
		// to ensure the hash gets recalculated.
		uint64 hash = compiler_.getFileHash( output, true );
		intermediateOutputs.push_back( std::make_pair( relativeOutput, hash ) );

		// tell the compiler that we just generated an output file
		compiler_.onOutputGenerated( output );

		// save the output to the cache
		if ((conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION) != 0)
		{
			ContentAddressableCache::writeToCache( output, hash, compiler_ );
		}
	}
}

void TaskProcessor::updateOutputs( BW::vector< BW::string > & outputFiles,
								   TaskContext & taskContext,
								   DependencyContext & dependencyContext,
								   ConversionContext & conversionContext )
{
	BW::vector< DependencyList::Output > & outputs = dependencyContext.depList_.outputs_;
	outputs.clear();

	for ( BW::vector< BW::string >::const_iterator it = outputFiles.begin(); 
		it != outputFiles.end(); ++it )
	{
		// Convert all the converted files to relative paths and push them into
		// our output list
		BW::string output = *it;
		BW::string relativeOutput = *it;
		compiler_.resolveOutputPath( output );
		compiler_.resolveRelativePath( relativeOutput );

		INFO_MSG( "  %s ->\n", taskContext.conversionTask_.source_.c_str() );
		INFO_MSG( "      %s\n", output.c_str() );
		// Update the hash of all our output files. Make sure we specify force update
		// to ensure the hash gets recalculated.
		uint64 hash = compiler_.getFileHash( output, true );
		outputs.push_back( std::make_pair( relativeOutput, hash ) );

		// tell the compiler that we just generated an output file
		compiler_.onOutputGenerated( output );

		// save the output to the cache
		if ((conversionContext.converterInfo_.flags_ & ConverterInfo::CACHE_CONVERSION) != 0)
		{
			ContentAddressableCache::writeToCache( output, hash, compiler_ );
		}
	}
}

void TaskProcessor::processTasks()
{
	if (multiThreaded_)
	{
		processTasksOnMultipleThreads();
	}
	else
	{
		processTasksOnSingleThread();
	}
}

bool TaskProcessor::isThreadKillRequested()
{
	LONG killCount = BW_ATOMIC32_DEC_AND_FETCH( &threadKillCount_ );
	if (killCount >= 0)
	{
		return true;
	}

	BW_ATOMIC32_INC_AND_FETCH( &threadKillCount_ );

	return false;
}

void TaskProcessor::processTasksOnSingleThread()
{
	// This function is called by multiple threads simultaneously and needs to be thread safe.

	while (true)
	{
		ConversionTask * task = compiler_.getNextTask();
		if (task == NULL)
		{
			break;
		}

		task->status_ == ConversionTask::PROCESSING ?
			compiler_.onTaskStarted( *task ) :
			compiler_.onTaskResumed( *task );

		this->processTask( *task );

		task->status_ >= ConversionTask::DONE ?
			compiler_.onTaskCompleted( *task ) :
			compiler_.onTaskSuspended( *task );

		if (isThreadKillRequested())
		{
			break;
		}
	}
		
}

void TaskProcessor::processTasksOnMultipleThreads()
{
	while (true)
	{
		// Get the number of potential available threads
		const int numThreads = BgTaskManager::instance().numUnstoppedThreads();
		// Get the number of background tasks still to be run
		const int numTasks = BgTaskManager::instance().numBgTasksLeft();

		// Check if we have tasks still to process.
		if (compiler_.hasTasks())
		{
			if ( numThreads > numTasks )
			{
				for ( int i = 0; i < numThreads - numTasks; ++i )
				{
					// For every available thread create a background task to process tasks.
					BgTaskManager::instance().addBackgroundTask( new TaskProcessorTask( *this ) );
				}
			}
			else if ( numThreads < numTasks )
			{
				InterlockedExchange(&threadKillCount_, numTasks - numThreads);
			}
		}
		else if (numTasks == 0)
		{
			// If the compiler has no tasks left and all our background tasks have completed then we are done.
			break;
		}

		// Sleep for a while and then try to queue some more background tasks.
		// One of our background tasks may have finished prematurely or a background task we were not
		// managing may have completed, freeing up another thread.
		Sleep(100);
	}
}

BW_END_NAMESPACE
