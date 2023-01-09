#include "jit_compiler.hpp"

#include "task_store.hpp"

#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/dependency/dependency.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "asset_pipeline/dependency/directory_dependency.hpp"
#include "asset_pipeline/dependency/intermediate_file_dependency.hpp"
#include "asset_pipeline/dependency/output_file_dependency.hpp"
#include "asset_pipeline/dependency/source_file_dependency.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/string_builder.hpp"

#include "re2/re2.h"

#include <memory>

DECLARE_DEBUG_COMPONENT2( ASSET_PIPELINE_CATEGORY, 0 )

BW_BEGIN_NAMESPACE

THREADLOCAL( ConversionTask * ) JITCompiler::st_currentTask_ = nullptr;

JITCompiler::JITCompiler( TaskStore & store ) :
	AssetCompiler(),
	event_( false ),
	store_( store )
{
	addToolsResourcePaths();
	// Create a monitor thread for all resource paths so that the JIT Compiler
	// can respond to file changes.
	BW::BWResource::instance().enableModificationMonitor( true ); 
	BWResource::instance().addModificationListener( this );
}

JITCompiler::~JITCompiler()
{
	BWResource::instance().removeModificationListener( this );
}

void JITCompiler::scanningThreadMain()
{
	store_.scanningStarted();

	// search for tasks in all resource paths
	int numPaths = BWResource::getPathNum();
	for (int i = numPaths; i > 0 && !terminating(); --i)
	{
		BW::string path = BWResource::getPath( i - 1 );
		taskFinder_.findTasks( path );
	}

	store_.scanningStopped();
}

void JITCompiler::managingThreadMain()
{
	// process the discovered tasks, flush the modification monitor and repeat
	while (!terminating())
	{
		event_.wait( 1000 );
		taskProcessor_.processTasks();

		MF_VERIFY( WaitForSingleObject( taskSemaphore_, INFINITE ) == 
			WAIT_OBJECT_0 );
		BWResource::instance().flushModificationMonitor();
		MF_VERIFY( ReleaseSemaphore( taskSemaphore_, 1, NULL ) );
	}
}

void JITCompiler::stop()
{	
	if (terminating())
	{
		return;
	}

	// tell the asset compiler to terminate. Will allow it to finish up any currently processing tasks
	terminate();

	event_.set();

	return;
}

void JITCompiler::addReverseDependency( const BW::string & path,
									    bool isOutput,
									    ConversionTask & conversionTask )
{
	ReverseDependencyMap::iterator it = 
		reverseDependencyMap_.find( path );
	if (it == reverseDependencyMap_.end())
	{
		it = reverseDependencyMap_.insert( 
			reverseDependencyMap_.end(), 
			std::make_pair( path, ReverseDependencies() ) );
	}

	ReverseDependencies & reverseDependencies = it->second;
	reverseDependencies.push_back( std::make_pair( &conversionTask, isOutput ) );

	ForwardDependencies & forwardDependencies = forwardDependencyMap_[&conversionTask];
	forwardDependencies.push_back( it->first );
}

void JITCompiler::addReverseDependency( const BW::string & directory,
									    const BW::string & pattern,
										bool regex,
										bool recursive,
									    ConversionTask & conversionTask )
{
	BW::string path = bw_format( "%s>%s>%s>%s", 
		directory.c_str(), 
		pattern.c_str(),
		regex ? "1" : "0",
		recursive ? "1" : "0" );
	if (std::find( directoryDependencies_.begin(), directoryDependencies_.end(), path ) ==
		directoryDependencies_.end())
	{
		directoryDependencies_.push_back( path );
	}
	addReverseDependency( path, false, conversionTask );
}

void JITCompiler::addReverseDependency( const Dependency & dependency, 
									    ConversionTask & conversionTask )
{
	switch (dependency.getType())
	{
	case SourceFileDependencyType:
		{
			const SourceFileDependency & sourcefileDependency = 
				static_cast< const SourceFileDependency & >( dependency );
			BW::string filename = sourcefileDependency.getFileName();
			resolveSourcePath( filename );
			addReverseDependency( filename, false, conversionTask );
		}
		break;

	case IntermediateFileDependencyType:
		{
			const IntermediateFileDependency & intermediatefileDependency = 
				static_cast< const IntermediateFileDependency & >( dependency );
			BW::string filename = intermediatefileDependency.getFileName();
			resolveIntermediatePath( filename );
			addReverseDependency( filename, false, conversionTask );
		}
		break;

	case OutputFileDependencyType:
		{
			// The hash of a compiled file dependency is a hash of its compiled file on disk
			const OutputFileDependency & outputfileDependency = 
				static_cast< const OutputFileDependency & >( dependency );
			BW::string filename = outputfileDependency.getFileName();
			resolveOutputPath( filename );
			addReverseDependency( filename, false, conversionTask );
		}
		break;

	case DirectoryDependencyType:
		{
			const DirectoryDependency & directoryDependency = 
				static_cast< const DirectoryDependency & >( dependency );
			BW::string directory = directoryDependency.getDirectory();
			if (directory.empty())
			{
				int numPaths = BWResource::getPathNum();
				for (int i = numPaths; i > 0; --i)
				{
					addReverseDependency( BWResource::getPath( i - 1 ), 
										  directoryDependency.getPattern(),
										  directoryDependency.isRegex(),
										  directoryDependency.isRecursive(),
										  conversionTask );
				}
			}
			else
			{
				resolveSourcePath( directory );
				addReverseDependency( directory, 
									  directoryDependency.getPattern(),
									  directoryDependency.isRegex(),
									  directoryDependency.isRecursive(),
									  conversionTask );
			}
		}
	}
}

void JITCompiler::collectReverseDependencies( const BW::string & path, 
											  const BW::StringRef & file, 
											  BW::vector< ConversionTask * > & conversionTasks )
{
	for (DirectoryDependencies::iterator it = directoryDependencies_.begin();
		it != directoryDependencies_.end(); )
	{
		typedef BW::vector< BW::StringRef > StringArray;
		StringArray tokens;
		bw_tokenise( *it, ">", tokens );
		MF_ASSERT( tokens.size() == 4 );

		BW::string directory = tokens[0] + "/";
		StringRef pattern = tokens[1];
		bool regex = (tokens[2] == "1");
		bool recursive = (tokens[3] == "1");

		if (( !recursive && path == directory ) ||
			( recursive && path.substr( 0, directory.length() ) == directory))
		{
			if (regex)
			{
				if (RE2::FullMatch( re2::StringPiece( file.data(), static_cast< int >( file.length() ) ),
					re2::StringPiece( pattern.data(), static_cast< int >( pattern.length() ) ) ))
				{
					collectReverseDependencies( *it, false, conversionTasks );
					it = directoryDependencies_.erase( it );
					continue;
				}
			}
			else
			{
				if (file == pattern)
				{
					collectReverseDependencies( *it, false, conversionTasks );
					it = directoryDependencies_.erase( it );
					continue;
				}
			}
		}

		++it;
	}
}

void JITCompiler::collectReverseDependencies( const BW::string & path,
											  bool includeOutputs,
											  BW::vector< ConversionTask * > & conversionTasks )
{
	ReverseDependencyMap::iterator it = reverseDependencyMap_.find( path );
	if (it == reverseDependencyMap_.end())
	{
		return;
	}
	
	ReverseDependencies & reverseDependencies = it->second;
	for (size_t i = 0; i < reverseDependencies.size();)
	{
		if (!includeOutputs && reverseDependencies[i].second)
		{
			++i;
			continue;
		}

		// Use the forward dependency map to clear this task out of all
		// the reverse dependency lists. We need to do this as we are about to
		// requeue this task and on completion of the task the reverse dependencies
		// will be populated again.
		ConversionTask * task = reverseDependencies[i].first;
		ForwardDependencies & forwardDependencies = forwardDependencyMap_[task];
		for ( ForwardDependencies::iterator
			forwardIt = forwardDependencies.begin(); forwardIt != forwardDependencies.end(); ++forwardIt )
		{
			ReverseDependencies & tasks = reverseDependencyMap_[forwardIt->to_string()];
			ReverseDependencies::iterator taskIt;
			for (taskIt = tasks.begin(); taskIt != tasks.end(); ++taskIt)
			{
				if (taskIt->first == task)
					break;
			}
			MF_ASSERT( taskIt != tasks.end() );
			tasks.erase( taskIt );
		}
		forwardDependencies.clear();

		conversionTasks.push_back( task );
	}
}

void JITCompiler::purgeResource( const StringRef & resourceId )
{
	BWResource::instance().purge( resourceId );
	for (BW::vector< ResourceCallbacks * >::iterator 
		it = resourceCallbacks_.begin(); it != resourceCallbacks_.end(); ++it)
	{
		( *it )->purgeResource( resourceId );
	}
}

void JITCompiler::onAssetRequested( const StringRef & asset )
{
	if (!executing())
	{
		return;
	}

	BW::string sourceFile;
	bool found = getSourceFile( asset, sourceFile );

	if (!found)
	{
		// Cannot build this asset - just broadcast it as done
		broadcastAsset( asset );
		return;
	}

	// Get the task for this source file.
	ConversionTask & task = taskFinder_.getTask( sourceFile );
	if (task.converterId_ == ConversionTask::s_unknownId)
	{
		broadcastAsset( asset );
		return;
	}

	// Try to push the task to the start of the queue
	{
		SimpleMutexHolder taskQueueMutexHolder( taskQueueMutex_ );
		if (task.status_ == ConversionTask::QUEUED)
		{
			ConversionTaskQueue::iterator it = 
				std::find( taskQueue_.begin(), taskQueue_.end(), &task );
			if (it != taskQueue_.end())
			{
				taskQueue_.erase( it );
				// Set the status of this task back to new as it is no longer in the queue.
				task.status_ = ConversionTask::NEW;
			}
		}

		if (task.status_ == ConversionTask::NEW)
		{
			taskQueue_.push_front( &task );
			// Set the task status as queued.
			task.status_ = ConversionTask::QUEUED;
		}
	}

	if (task.status_ >= ConversionTask::DONE)
	{
		BW::string relativeSourceFile = BWResolver::dissolveFilename( sourceFile );
		if (!BWResource::instance().hasPendingModification( relativeSourceFile ))
		{
			// task is already up to date. Broadcast it as done
			broadcastAsset( asset );
			return;
		}
	}

	BW::string request = asset.to_string();
	{
		SimpleMutexHolder smh( requestMutex_ );
		// need to store this request so we can broadcast when it is done
		BW::vector<std::pair<ConversionTask *, BW::string>>::iterator requestIt =
			std::find( requests_.begin(), requests_.end(), std::make_pair( &task, request ) );
		if (requestIt == requests_.end())
		{
			requests_.push_back( std::make_pair( &task, request ) );
		}
	}

	store_.addRequestedTask( &task );

	// notify the processing thread that there is something to build
	event_.set();
}

void JITCompiler::lock()
{
	// this will block until the asset compiler has been successfully paused
	pause();
}

void JITCompiler::unlock()
{
	resume();
}

void JITCompiler::onResourceModified( const StringRef & basePath,
									  const StringRef & resourceID,
									  Action modType )
{
	if (terminating())
	{
		return;
	}

	BW::string fullPath = basePath + resourceID;
	if (BWResource::pathIsRelative( fullPath ))
	{
		return;
	}

	// Purge the data census & cache
	purgeResource( resourceID );
	purgeResource( fullPath );

	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::FileType ft = fs->getFileType( fullPath );
	if (ft == IFileSystem::FT_DIRECTORY)
	{
		return;
	}

	BW::vector< ConversionTask * > tasks;

	// If the file was added, we may need to create a new root task
	if (modType == Action::ACTION_ADDED)
	{
		ConversionTask * rootTask = taskFinder_.getTask( fullPath, true );
		if (rootTask != NULL)
		{
			tasks.push_back( rootTask );
		}
	}

	// Collect all the tasks that depend on this file
	const BW::string path = BWResource::instance().getFilePath( fullPath );
	const BW::StringRef filename = BWResource::instance().getFilename( fullPath );
	bool includeOutputs = !checkFileHashUpToDate( fullPath );
	collectReverseDependencies( fullPath, includeOutputs, tasks );
	collectReverseDependencies( path, filename, tasks );

	for ( BW::vector< ConversionTask * >::iterator
		it = tasks.begin(); it != tasks.end(); ++it )
	{
		ConversionTask & task = **it;
		if (task.converterId_ == ConversionTask::s_unknownId)
		{
			continue;
		}

		if ( task.status_ == ConversionTask::QUEUED )
		{
			continue;
		}

		MF_ASSERT( task.status_ == ConversionTask::NEW ||
			task.status_ == ConversionTask::DONE ||
			task.status_ == ConversionTask::FAILED );

		bool modifiedIsSource = ( task.source_ == fullPath );
		bool sourceExists = BWResource::fileExists( task.source_ );

		// Don't touch this task if its source has been deleted,
		// but not by this modification
		if (!modifiedIsSource && !sourceExists)
		{
			continue;
		}

		// Reset the task
		task.status_ = ConversionTask::NEW;
		task.subTasks_.clear();
		store_.resetTask( &task );

		// Don't re-queue the task if we deleted the source
		if (modifiedIsSource && !sourceExists)
		{
			continue;
		}

		// Queue the task
		queueTask( task );

		// notify the processing thread that there is something to build
		event_.set();
	}
}

void JITCompiler::onTaskStarted( ConversionTask & conversionTask )
{
	// add the task to our current tasks and set it to processing
	store_.setTaskCurrent(&conversionTask);
	store_.setCurrentTaskState(&conversionTask, TaskInfoState::CHECKING);

	MF_ASSERT(st_currentTask_ == nullptr);
	st_currentTask_ = &conversionTask;

	AssetCompiler::onTaskStarted( conversionTask );
}

void JITCompiler::onTaskResumed( ConversionTask & conversionTask )
{
	MF_ASSERT(st_currentTask_ == nullptr);
	st_currentTask_ = &conversionTask;

	AssetCompiler::onTaskResumed(conversionTask);
}

void JITCompiler::onTaskSuspended( ConversionTask & conversionTask )
{
	MF_ASSERT(st_currentTask_ == &conversionTask);
	st_currentTask_ = nullptr;

	AssetCompiler::onTaskSuspended(conversionTask);
}

void JITCompiler::onTaskCompleted( ConversionTask & conversionTask )
{
	store_.setCurrentTaskState(&conversionTask, TaskInfoState::NONE);

	{
		SimpleMutexHolder smh( requestMutex_ );
		// broadcast the task as done
		for ( BW::vector<std::pair<ConversionTask *, BW::string>>::iterator
			it = requests_.begin(); it != requests_.end(); )
		{
			if (it->first == &conversionTask)
			{
				broadcastAsset( it->second );
				it = requests_.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	store_.setTaskComplete(&conversionTask);

	MF_ASSERT(&conversionTask == st_currentTask_);
	st_currentTask_ = nullptr;

	DependencyList depList(*this);
	BW::StringBuilder depListFileNameBuilder( MAX_PATH );
	depListFileNameBuilder.append( conversionTask.source_ );
	depListFileNameBuilder.append( ".deps" );
	BW::string depListFileName = depListFileNameBuilder.string();
	MF_VERIFY(resolveIntermediatePath( depListFileName ));
	DataResource depListResource( depListFileName, RESOURCE_TYPE_XML );
	DataSectionPtr depListRoot = depListResource.getRootSection();
	MF_ASSERT( depListRoot != NULL );
	depList.serialiseIn( depListRoot );

	{
		static SimpleMutex reverseDependencyMutex;
		SimpleMutexHolder reverseDependencyMutexHolder( reverseDependencyMutex );

		if (depList.primaryInputs().empty())
		{
			// Add a reverse dependency to the task's source file
			addReverseDependency( conversionTask.source_, false, conversionTask );
		}

		const BW::vector< DependencyList::Input > & primaryInputs = depList.primaryInputs();
		for ( BW::vector< DependencyList::Input >::const_iterator
			it = primaryInputs.begin(); it != primaryInputs.end(); ++it )
		{
			addReverseDependency( *it->first, conversionTask );
		}

		const BW::vector< DependencyList::Input > & secondaryInputs = depList.secondaryInputs();
		for ( BW::vector< DependencyList::Input >::const_iterator
			it = secondaryInputs.begin(); it != secondaryInputs.end(); ++it )
		{
			addReverseDependency( *it->first, conversionTask );
		}

		if (conversionTask.status_ != ConversionTask::FAILED)
		{
			const BW::vector< DependencyList::Output > & intermediateOutputs = depList.intermediateOutputs();
			for ( BW::vector< DependencyList::Output >::const_iterator
				it = intermediateOutputs.begin(); it != intermediateOutputs.end(); ++it )
			{
				BW::string filename = it->first;
				resolveIntermediatePath( filename );
				addReverseDependency( filename, true, conversionTask );
			}

			const BW::vector< DependencyList::Output > & outputs = depList.outputs();
			for ( BW::vector< DependencyList::Output >::const_iterator
				it = outputs.begin(); it != outputs.end(); ++it )
			{
				BW::string filename = it->first;
				resolveOutputPath( filename );
				addReverseDependency( filename, true, conversionTask );
			}
		}
	}

	AssetCompiler::onTaskCompleted( conversionTask );
}

void JITCompiler::onPreCreateDependencies( ConversionTask & conversionTask )
{
	store_.setCurrentTaskState( &conversionTask, TaskInfoState::GENERATING_DEPENDENCIES );
	AssetCompiler::onPreCreateDependencies( conversionTask );
}

void JITCompiler::onPostCreateDependencies( ConversionTask & conversionTask )
{
	store_.setCurrentTaskState( &conversionTask, TaskInfoState::CHECKING );
	AssetCompiler::onPostCreateDependencies( conversionTask );
}

void JITCompiler::onPreConvert( ConversionTask & conversionTask )
{
	store_.setCurrentTaskState( &conversionTask, TaskInfoState::CONVERTING );
	AssetCompiler::onPreConvert( conversionTask );
}

void JITCompiler::onPostConvert( ConversionTask & conversionTask )
{
	store_.setCurrentTaskState( &conversionTask, TaskInfoState::CHECKING );
	AssetCompiler::onPostConvert( conversionTask );
}

void JITCompiler::onOutputGenerated( const BW::string & filename )
{
	if (st_currentTask_ != nullptr)
	{
		store_.addOutputToTask( st_currentTask_, filename );
	}

	AssetCompiler::onOutputGenerated( filename );
}

bool JITCompiler::handleMessage( DebugMessagePriority messagePriority, 
								 const char * pCategory,
								 DebugMessageSource messageSource,
								 const LogMetaData & metaData,
								 const char * pFormat,
								 va_list argPtr )
{
	BW_GUARD;

#if !ASSET_PIPELINE_CAPTURE_ASSERTS
	if (messagePriority == DebugMessagePriority::MESSAGE_PRIORITY_CRITICAL)
	{
		return false;
	}
#endif

	bool handled = AssetCompiler::handleMessage( messagePriority,
												pCategory,
												messageSource,
												metaData,
												pFormat,
												argPtr);

	if (st_currentTask_ != nullptr)
	{
		const size_t count = _vscprintf(pFormat, argPtr) + 1;
		auto buffer = std::unique_ptr<char[]>(new char[count]);
		_vsnprintf(buffer.get(), count - 1, pFormat, argPtr); // Shouldn't truncate but don't care if it does
		buffer.get()[count - 1] = '\0';

		BW::wstring message;
		bw_utf8tow(buffer.get(), message);
		if (store_.handleTaskMessage(st_currentTask_, 
								 	 messagePriority, 
									 pCategory,
									 message))
		{
			handled = true;
		}
	}

	return handled;
}

BW_END_NAMESPACE
