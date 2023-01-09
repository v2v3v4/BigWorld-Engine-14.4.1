#include "asset_pipeline/asset_pipe.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/debug_filter.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/win_file_system.hpp"
#include "re2/re2.h"
#include "resmgr/xml_section.hpp"

#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/converter.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "asset_pipeline/dependency/intermediate_file_dependency.hpp"
#include "asset_pipeline/dependency/output_file_dependency.hpp"
#include "asset_pipeline/dependency/source_file_dependency.hpp"
#include "asset_pipeline/dependency/converter_dependency.hpp"
#include "asset_pipeline/dependency/converter_params_dependency.hpp"
#include "asset_pipeline/dependency/directory_dependency.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_compiler.hpp"

// How long the task will wait to create a handle to the source
// before failing on a timeout
#define TASK_START_TIMEOUT 5
#define TASK_TRY_MAX_TIMES 5

DECLARE_DEBUG_COMPONENT2( ASSET_PIPELINE_CATEGORY, 0 )

BW_BEGIN_NAMESPACE

THREADLOCAL( bool ) AssetCompiler::s_error = false;
THREADLOCAL( bool ) AssetCompiler::s_warning = false;
THREADLOCAL( ConversionTask* ) AssetCompiler::s_currentTask = NULL;
THREADLOCAL( ConversionTaskQueue* ) AssetCompiler::s_currentTaskQueue = NULL;
THREADLOCAL( bool )	AssetCompiler::s_CreatingDependencies = false;
THREADLOCAL( bool )	AssetCompiler::s_Converting = false;

const ConversionTask AssetCompiler::s_invalidTask = 
	{ "InvalidTask", ConversionTask::s_unknownId, "", "", ConversionTask::FAILED };

AssetCompiler::AssetCompiler()
	: state_( INVALID )
	, recursive_( false )
	, forceRebuild_( false )
	, numThreads_( 1 )
	, intermediatePath_()
	, outputPath_()
	, taskFinder_( *this, conversionRules_ )
	, taskProcessor_( *this, converterMap_, true )
	, genericConversionRule_( converterMap_ )
{
	BW::vector< BW::wstring > baseResourcePaths = AssetPipe::getBaseResourcePaths();
	for (BW::vector< BW::wstring >::iterator 
		it = baseResourcePaths.begin(); it != baseResourcePaths.end(); ++it)
	{
		BW::wstring mutexName = L"Local\\AssetPipeline" + *it;
		HANDLE compilerMutex = CreateMutexW( 0, TRUE, mutexName.c_str() );
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			::MessageBox( NULL, 
				L"Only one instance of the AssetPipeline is allowed to be active per resource path.",
				L"AssetPipeline",
				MB_OK );
			exit(-1);
		}
		compilerMutexes_.push_back( compilerMutex );

		BW::wstring subPath = *it;
		while (true)
		{
			BW::wstring::size_type pos = subPath.find_last_of( L"\\/" );
			if (pos == BW::wstring::npos)
			{
				break;
			}
			subPath = subPath.substr( 0, pos );

			mutexName = L"Local\\AssetPipeline" + subPath;
			compilerMutex = CreateMutexW( 0, FALSE, mutexName.c_str() );
			if (WaitForSingleObject( compilerMutex, 100 ) != WAIT_OBJECT_0)
			{
				::MessageBox( NULL, 
					L"Only one instance of the AssetPipeline is allowed to be active per resource path.",
					L"AssetPipeline",
					MB_OK );
				exit(-1);
			}
			ReleaseMutex( compilerMutex );
			compilerMutexes_.push_back( compilerMutex );
		}
	}
}

AssetCompiler::~AssetCompiler()
{
	MF_ASSERT( state_ == INVALID );

	for ( BW::vector< HANDLE >::const_iterator 
		it = compilerMutexes_.begin(); it != compilerMutexes_.end(); ++it )
	{
		CloseHandle( *it );
	}
	compilerMutexes_.clear();
}

void AssetCompiler::initCompiler()
{
	WinFileSystem::useFileTypeCache( true );

	// Some game projects modify the defaults of these settings in XmlSection.
	// Make sure they're set appropriately for Batch/Jit compiler.
	XMLSection::shouldReadXMLAttributes( true );
	XMLSection::shouldWriteXMLAttributes( true );

	genericConversionRule_.load( "asset_rules.xml" );
	conversionRules_.push_back( &genericConversionRule_ );

	if (forceRebuild_)
	{
		taskProcessor_.forceRebuild( true );
	}

    // Semaphore for pausing processing threads
	taskSemaphore_ = ::CreateSemaphore( 
		NULL,
		numThreads_,
		numThreads_,
		NULL );

	// Start the background task manager
	BgTaskManager::init();
	BgTaskManager::instance().startThreads( ASSET_PIPELINE_CATEGORY, numThreads_ );

	// Register ourself for message callbacks
	DebugFilter::instance().addMessageCallback(this);
	DebugFilter::instance().addCriticalCallback(this);

	state_ = EXECUTING;
}

void AssetCompiler::finiCompiler()
{
	terminate();

	MF_ASSERT( s_currentTask == NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );

	CloseHandle( taskSemaphore_ );

	// Stop the background task manager
	BgTaskManager::instance().stopAll();
	BgTaskManager::instance().fini();

	// De-register ourself for message callbacks
	DebugFilter::instance().deleteMessageCallback(this);
	DebugFilter::instance().deleteCriticalCallback(this);

	state_ = INVALID;
}

bool AssetCompiler::pause()
{
	if (!executing())
	{
		return false;
	}

    // This will block until all processing threads have been paused
	for (int i = 0; i < numThreads_; ++i)
	{
		MF_VERIFY( WaitForSingleObject( taskSemaphore_, INFINITE ) == 
			WAIT_OBJECT_0 );
	}
	state_ = PAUSED;
	return true;
}

bool AssetCompiler::resume()
{
	if (!paused())
	{
		return false;
	}

	ReleaseSemaphore( taskSemaphore_, numThreads_, NULL );
	state_ = EXECUTING;
	return true;
}

bool AssetCompiler::terminate()
{
	if (paused())
	{
		resume();
	}

	if (!executing())
	{
		return false;
	}

	state_ = TERMINATING;
	return true;
}

int AssetCompiler::getThreadCount() const
{
	return numThreads_;
}

void AssetCompiler::setThreadCount(int numThreads)
{
	numThreads_ = numThreads;

	if ( operating() )
	{
		BgTaskManager::instance().setNumberOfThreads(numThreads_);
	}
}

const BW::string & AssetCompiler::getIntermediatePath() const
{
	return intermediatePath_;
}

void AssetCompiler::setIntermediatePath( const StringRef & path )
{
	intermediatePath_ = path.to_string();
}

const BW::string & AssetCompiler::getOutputPath() const
{
	return outputPath_;
}

void AssetCompiler::setOutputPath( const StringRef & path )
{
	outputPath_ = path.to_string();
}

bool AssetCompiler::getForceRebuild() const
{
	return forceRebuild_;
}

void AssetCompiler::setForceRebuild( bool force )
{
	forceRebuild_ = force;
	taskProcessor_.forceRebuild( forceRebuild_ );
}

bool AssetCompiler::getRecursive() const
{
	return recursive_;
}

void AssetCompiler::setRecursive( bool recurse )
{
	recursive_ = recurse;
}

void AssetCompiler::registerConversionRule( ConversionRule& conversionRule )
{
	conversionRules_.push_back( &conversionRule );
}

void AssetCompiler::registerConverter( ConverterInfo& converterInfo )
{
	// Store the converter info in a map indexed by converter id.
	// Assert that no other converter has been registered with the same id.
	std::pair<ConverterMap::iterator, bool> insertIt = 
		converterMap_.insert( std::make_pair( converterInfo.typeId_, &converterInfo ) );
	MF_ASSERT( insertIt.second );
}

void AssetCompiler::registerResourceCallbacks( ResourceCallbacks& resourceCallbacks )
{
	resourceCallbacks_.push_back( &resourceCallbacks );
}

const BW::vector< BW::string > & AssetCompiler::getResourcePaths() const
{
	return resourcePaths_;
}

bool AssetCompiler::ensureUpToDate( const Dependency & dependency, const ConversionTask *& conversionTask )
{
	switch (dependency.getType())
	{
	case SourceFileDependencyType:
		{
			const SourceFileDependency & sourcefileDependency = static_cast< const SourceFileDependency & >( dependency );
			return BWResource::fileExists( sourcefileDependency.getFileName() );
		}

	case IntermediateFileDependencyType:
		{
			const IntermediateFileDependency & intermediatefileDependency = static_cast< const IntermediateFileDependency & >( dependency );
			return ensureCompiled( intermediatefileDependency.getFileName(), false, conversionTask );
		}

	case OutputFileDependencyType:
		{
			const OutputFileDependency & outputfileDependency = static_cast< const OutputFileDependency & >( dependency );
			return ensureCompiled( outputfileDependency.getFileName(), true, conversionTask );
		}

	default:
		return true;
	}
}

bool AssetCompiler::getSourceFile( const BW::StringRef & file, BW::string & sourceFile )
{
	ConversionRules::const_reverse_iterator it;
	for ( it = conversionRules_.crbegin(); it != conversionRules_.crend(); ++it )
	{
		if (!(*it)->getSourceFile( file, sourceFile ))
		{
			continue;
		}

		if (BWResource::pathIsRelative( sourceFile ))
		{
			ERROR_MSG( "Conversion rule returned a relative source file %s", sourceFile.c_str() );
		}
		return true;
	}
	return false;
}

uint64 AssetCompiler::getHash( const Dependency & dependency )
{
	switch (dependency.getType())
	{
	case SourceFileDependencyType:
		{
			// The hash of a file dependency is a hash of its file on disk
			const SourceFileDependency & sourcefileDependency = static_cast< const SourceFileDependency & >( dependency );
			BW::string filename = sourcefileDependency.getFileName();
			uint64 hash = Hash64::compute( filename );
			resolveSourcePath( filename );
			Hash64::combine( hash, getFileHash( filename, false ) );
			return hash;
		}

	case IntermediateFileDependencyType:
		{
			// The hash of a compiled file dependency is a hash of its compiled file on disk
			const IntermediateFileDependency & intermediatefileDependency = static_cast< const IntermediateFileDependency & >( dependency );
			BW::string filename = intermediatefileDependency.getFileName();
			uint64 hash = Hash64::compute( filename );
			resolveIntermediatePath( filename );
			Hash64::combine( hash, getFileHash( filename, false ) );
			return hash;
		}

	case OutputFileDependencyType:
		{
			// The hash of a compiled file dependency is a hash of its compiled file on disk
			const OutputFileDependency & outputfileDependency = static_cast< const OutputFileDependency & >( dependency );
			BW::string filename = outputfileDependency.getFileName();
			uint64 hash = Hash64::compute( filename );
			resolveOutputPath( filename );
			Hash64::combine( hash, getFileHash( filename, false ) );
			return hash;
		}

	case ConverterDependencyType:
		{
			// The hash of a converter dependency is a combination of its id and version
			const ConverterDependency & converterDependency = static_cast< const ConverterDependency & >( dependency );
			uint64 hash = converterDependency.getConverterId();
			Hash64::combine( hash, converterDependency.getConverterVersion() );
			return hash;
		}

	case ConverterParamsDependencyType:
		{
			// The hash of a converter params dependency is a hash of the parameter string
			const ConverterParamsDependency & converterParamsDependency = static_cast< const ConverterParamsDependency & >( dependency );
			return Hash64::compute( converterParamsDependency.getConverterParams() );
		}

	case DirectoryDependencyType:
		{
			const DirectoryDependency & directoryDependency = static_cast< const DirectoryDependency & >( dependency );
			BW::string directory = directoryDependency.getDirectory();
			uint64 hash = Hash64::compute( directory );
			if (directory.empty())
			{
				int numPaths = BWResource::getPathNum();
				for (int i = numPaths; i > 0; --i)
				{
					BW::string path = BWResource::getPath( i - 1 );
					Hash64::combine( hash, getDirectoryHash( path, 
						directoryDependency.getPattern(),
						directoryDependency.isRegex(),
						directoryDependency.isRecursive() ) );
				}
			}
			else
			{
				resolveSourcePath( directory );
				Hash64::combine( hash, getDirectoryHash( directory, 
					directoryDependency.getPattern(), 
					directoryDependency.isRegex(),
					directoryDependency.isRecursive() ) );
			}
			return hash;
		}

	default:
		MF_ASSERT( false );
		return 0;
	}
}

uint64 AssetCompiler::getFileHash( const BW::string & fileName, bool forceUpdate )
{
	MF_ASSERT( !BWResource::pathIsRelative( fileName ) );

	if (!forceUpdate)
	{
		// If not forcing an update check our cache for an existing hash.
		// Need to lock the cache for read.
		ReadWriteLock::ReadGuard lockGuard( fileHashesLock_ );
		StringHashMap< uint64 >::const_iterator hashIt = fileHashes_.find( fileName );
		if (hashIt != fileHashes_.cend())
		{
			// A cached hash was found
			return hashIt->second;
		}
	}

	// Could not find the hash in the cache or we are forcing an update,
	// So open the file on disk and hash its contents.
	// If the file does not exist it will get a hash of 0.
	uint64 hash = 0;
	BinaryPtr sourceFilePtr = BWResource::instance().fileSystem()->readFile( fileName );
	if (sourceFilePtr.hasObject())
	{
		hash = Hash64::compute( sourceFilePtr->data(), sourceFilePtr->len() );
	}

	// Lock the cache for write
	ReadWriteLock::WriteGuard lockGuard( fileHashesLock_ );
	// Store the hash in the cache
	fileHashes_[fileName] = hash;

	return hash;
}

bool AssetCompiler::checkFileHashUpToDate( const BW::string & fileName )
{
	uint64 oldHash = 0;

	{
		// Lock the cache for write
		ReadWriteLock::ReadGuard lockGuard( fileHashesLock_ );
		StringHashMap<uint64>::iterator it = fileHashes_.find( fileName );
		if (it == fileHashes_.end())
		{
			// Cannot find the file in our stored hashes so it must be out of date.
			return false;
		}
		oldHash = it->second;
	}

	uint64 newHash = getFileHash( fileName, true );
	return oldHash == newHash;
}

uint64 AssetCompiler::getDirectoryHash( const BW::string & directory, const BW::string & pattern, bool regex, bool recursive )
{
	uint64 hash = 0;

	BW::string path = BWUtil::formatPath( directory );

	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::Directory dir;
	fs->readDirectory( dir, directory );
	std::sort( dir.begin(), dir.end() );

	for( IFileSystem::Directory::iterator it = dir.begin();
		it != dir.end(); ++it )
	{
		BW::string subPath = path + (*it);
		IFileSystem::FileType ft = fs->getFileType( subPath );
		if (ft == IFileSystem::FT_FILE)
		{
			if (regex)
			{
				if (RE2::FullMatch( re2::StringPiece( it->c_str() ),
					re2::StringPiece( pattern.c_str() ) ))
				{
					Hash64::combine( hash, *it );
					Hash64::combine( hash, getFileHash( subPath, false ));
				}
			}
			else
			{
				if (*it == pattern)
				{
					Hash64::combine( hash, *it );
					Hash64::combine( hash, getFileHash( subPath, false ));
				}
			}
		}
		else if (ft == IFileSystem::FT_DIRECTORY && recursive)
		{
			uint64 subHash = getDirectoryHash( subPath, pattern, regex, recursive );
			if (subHash != 0)
			{
				Hash64::combine( hash, subHash );
			}
		}
	}

	return hash;
}

bool AssetCompiler::ensureCompiled( const BW::string & compiledFileName, bool output, const ConversionTask *& o_conversionTask )
{
	BW::string sourceFile;
	if (getSourceFile( compiledFileName, sourceFile ))
	{
		// Get the task for this source file.
		ConversionTask & task = taskFinder_.getTask( sourceFile );

		if (recursive_)
		{
			bool shouldProcess = false;
			if ( task.status_ == ConversionTask::NEW )
			{
				// Grab a mutex to ensure that only we are going to trigger a build for this task.
				static SimpleMutex taskMutex;
				taskMutex.grab();
				shouldProcess = (task.status_ == ConversionTask::NEW);
				if (shouldProcess)
				{
					task.status_ = ConversionTask::PROCESSING;
				}
				taskMutex.give();
			}
			if (task.status_ == ConversionTask::QUEUED)
			{
				taskQueueMutex_.grab();
				shouldProcess = (task.status_ == ConversionTask::QUEUED);
				if (shouldProcess)
				{
					ConversionTaskQueue::iterator it;
					for ( it = taskQueue_.begin(); it != taskQueue_.end(); ++it )
					{
						if (*it == &task)
						{
							break;
						}
					}
					MF_ASSERT( it != taskQueue_.end() );
					taskQueue_.erase( it );
					task.status_ = ConversionTask::PROCESSING;
				}
				taskQueueMutex_.give();
			}

			// If shouldProcess is false it means another thread managed to trigger a build
			// for this task before we did.
			if (shouldProcess)
			{
				// Suspend the current task
				ConversionTask & currentTask = *s_currentTask;
				onTaskSuspended( currentTask );
				MF_ASSERT( s_currentTask == NULL );

				// Start the child task
				MF_ASSERT( task.status_ == ConversionTask::PROCESSING );
				onTaskStarted( task );
				MF_ASSERT( s_currentTask == &task );

				taskProcessor_.processTask( task );

				// Complete the child task
				MF_ASSERT( task.status_ >= ConversionTask::DONE );
				onTaskCompleted( task );
				MF_ASSERT( s_currentTask == NULL );

				// Resume the current task
				onTaskResumed( currentTask );
				MF_ASSERT( s_currentTask == &currentTask );
			}

			while (task.status_ != ConversionTask::DONE &&
					task.status_ != ConversionTask::FAILED)
			{
				if (numThreads_ == 1)
				{
					// We can't be waiting on a task on a different thread as we aren't
					// running multi-threaded. Must have encountered a cyclic dependency.
					ERROR_MSG( "Cyclic dependency found %s\n", task.source_.c_str() );
					break;
				}
				else if (task.threadId_ != 0)
				{
					// Set our task thread id to the thread id of the task we are waiting for.
					s_currentTask->threadId_ = task.threadId_;
					if (s_currentTask->threadId_ == GetCurrentThreadId())
					{
						ERROR_MSG( "Cyclic dependency found %s\n", task.source_.c_str() );
						break;
					}
				}
				else
				{
					s_currentTask->threadId_ = GetCurrentThreadId();
				}
				// If the task is not new and its not completed it means another
				// thread is processing it. In this case we need to
				// block until it is done.
				// TODO: use an event?
				Sleep(0);
			}

			// Set our current task thread id back to the current process id
			// as we are no longer waiting for another task to complete
			s_currentTask->threadId_ = GetCurrentThreadId();
		}
		else
		{
			if (task.threadId_ == GetCurrentThreadId())
			{
				ERROR_MSG( "Cyclic dependency found %s\n", task.source_.c_str() );
			}
			else if (task.status_ != ConversionTask::DONE &&
					    task.status_ != ConversionTask::FAILED)
			{
				if (s_currentTaskQueue == NULL)
				{
					s_currentTaskQueue = new ConversionTaskQueue();
				}
				s_currentTaskQueue->push_front( &task );
			}
		}

		// The asset is compiled if its task is done.
		o_conversionTask = &task;
		return task.status_ == ConversionTask::DONE;
	}

	if (output)
	{
		// Don't know how to compile this file, check if we can copy it from somewhere
		BW::string sourcePath = compiledFileName;
		resolveSourcePath( sourcePath );
		if (BWResource::fileExists( sourcePath ))
		{
			if (outputPath_.empty())
			{
				// A source for this file exists and we are not using an output path.
				// We can just use this file in place.
				return true;
			}
	
			BW::string outputPath = compiledFileName;
			resolveOutputPath( outputPath );
			if (BWResource::fileExists( outputPath ))
			{
				if (getFileHash( sourcePath, false ) == getFileHash( outputPath, false ))
				{
					// The output file exists and it is the same as the source.
					// No need to copy it
					return true;
				}
			}
	
			// Copy the source file to the output file
			if (BWResource::instance().fileSystem()->copyFileOrDirectory( sourcePath, outputPath ))
			{
				// Update the hash of the file we just copied
				getFileHash( outputPath, true );
				return true;
			}
		}
	}

	// Don't know how to compile this file.
	o_conversionTask = &s_invalidTask;
	return false;
}

void AssetCompiler::setError() 
{ 
	s_error = true; 
}

void AssetCompiler::setWarning() 
{ 
	s_warning = true; 
}


void AssetCompiler::resetErrorFlags() 
{
	s_error = false; 
	s_warning = false;
}

bool AssetCompiler::hasError() const 
{ 
	return s_error;
}

bool AssetCompiler::hasWarning() const 
{ 
	return s_warning;
}

bool AssetCompiler::shouldIterateFile( const BW::StringRef & file )
{
	return !terminating();
}

bool AssetCompiler::shouldIterateDirectory( const BW::StringRef & directory )
{
	if (terminating())
	{
		return false;
	}

	// Don't iterate into the intermediate or output paths
	if (intermediatePath_.length() && 
		directory.substr( 0, intermediatePath_.length() ) == intermediatePath_)
	{
		return false;
	}
	if (outputPath_.length() && 
		directory.substr( 0, outputPath_.length() ) == outputPath_)
	{
		return false;
	}

	// Don't iterate into svn folders
	StringRef svn = "/.svn";
	if (directory.find( svn ) != StringRef::npos)
	{
		return false;
	}

	// Don't iterate into the unit test testfiles directory
	StringRef testfiles = "/tools/asset_pipeline/testfiles";
	if (directory.find( testfiles ) != StringRef::npos )
	{
		return false;
	}

	return true;
}

bool AssetCompiler::hasTasks()
{
	if (terminating())
	{
		return false;
	}

	// We have tasks if there are tasks in our queue.
	taskQueueMutex_.grab();
	bool hasTasks = !taskQueue_.empty();
	taskQueueMutex_.give();

	return hasTasks;
}

ConversionTask * AssetCompiler::getNextTask()
{
	if (terminating())
	{
		return NULL;
	}

	ConversionTask * task = NULL;
	taskQueueMutex_.grab();
	if (!taskQueue_.empty())
	{
		// If the task queue is not empty return the first task in the queue.
		// Otherwise return NULL.
		task = taskQueue_.front();
		taskQueue_.pop_front();
		MF_ASSERT( task->status_ > ConversionTask::NEW );
		if (task->status_ == ConversionTask::QUEUED)
		{
			// Set the task status as processing.
			task->status_ = ConversionTask::PROCESSING;
		}
	}
	taskQueueMutex_.give();

	return task;
}

void AssetCompiler::queueTask( ConversionTask & conversionTask )
{
	// Push the task onto the back of the queue.
	taskQueueMutex_.grab();
	if ( conversionTask.status_ == ConversionTask::NEW )
	{
		taskQueue_.push_back( &conversionTask );
		// Set the task status as queued.
		conversionTask.status_ = ConversionTask::QUEUED;
	}
	taskQueueMutex_.give();
}

void AssetCompiler::onTaskStarted( ConversionTask & conversionTask )
{
	// Set the current task for this thread.
	MF_ASSERT( !hasError() );
	MF_ASSERT( s_currentTask == NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_VERIFY( WaitForSingleObject( taskSemaphore_, INFINITE ) == 
		WAIT_OBJECT_0 );
	s_currentTask = &conversionTask;

	// Update the tasks thread id
	MF_ASSERT( s_currentTask->threadId_ == 0 );
	s_currentTask->threadId_ = GetCurrentThreadId();

	// Acquire a handle to the source
	size_t triedTimes = 0;
	TimeStamp ts = timestamp();
	while (true)
	{
		s_currentTask->fileHandle_ = ::CreateFileW( 
			bw_utf8tow( conversionTask.source_ ).c_str(), 
			GENERIC_READ, 
			FILE_SHARE_READ, 
			0, 
			OPEN_EXISTING, 
			0, 
			0 );
		if (s_currentTask->fileHandle_ != INVALID_HANDLE_VALUE ||
			!BWResource::fileExists( conversionTask.source_ ) ||
			triedTimes >= TASK_TRY_MAX_TIMES )
		{
			break;
		}

		if (ts.ageInSeconds() > TASK_START_TIMEOUT)
		{
			INFO_MSG( "Waiting to acquire source handle...\n" );
			ts = timestamp();
			++triedTimes;
		}
		Sleep( 100 );
	}

	// Reset the hash of the source file
	getFileHash( conversionTask.source_, true );
}

void AssetCompiler::onTaskResumed( ConversionTask & conversionTask )
{
	// Set the current task for this thread.
	MF_ASSERT( !hasError() );
	MF_ASSERT( s_currentTask == NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_VERIFY( WaitForSingleObject( taskSemaphore_, INFINITE ) == 
		WAIT_OBJECT_0 );
	s_currentTask = &conversionTask;

	// Update the tasks thread id
	if (recursive_)
	{
		MF_ASSERT( s_currentTask->threadId_ == GetCurrentThreadId() );
	}
	else
	{
		MF_ASSERT( s_currentTask->threadId_ == 0 );
		s_currentTask->threadId_ = GetCurrentThreadId();
		for ( ConversionTask::SubTaskList::const_iterator
			it = s_currentTask->subTasks_.begin(); it != s_currentTask->subTasks_.end(); ++it )
		{
			const ConversionTask & task = *it->first;
			while (task.status_ != ConversionTask::DONE &&
				   task.status_ != ConversionTask::FAILED)
			{
				if (numThreads_ == 1)
				{
					// We can't be waiting on a task on a different thread as we aren't
					// running multi-threaded. Must have encountered a cyclic dependency.
					ERROR_MSG( "Cyclic dependency found %s\n", task.source_.c_str() );
					break;
				}
				else if (task.threadId_ != 0)
				{
					// Set our task thread id to the thread id of the task we are waiting for.
					s_currentTask->threadId_ = task.threadId_;
					if (s_currentTask->threadId_ == GetCurrentThreadId())
					{
						ERROR_MSG( "Cyclic dependency found %s\n", task.source_.c_str() );
						break;
					}
				}
				else
				{
					s_currentTask->threadId_ = GetCurrentThreadId();
				}
				// If the task is not new and its not completed it means another
				// thread is recursively processing it. In this case we need to
				// block until it is done.
				// TODO: use an event?
				Sleep(0);
			}
		}
		s_currentTask->threadId_ = GetCurrentThreadId();
	}
}

void AssetCompiler::onTaskSuspended( ConversionTask & conversionTask )
{
	MF_ASSERT( !hasError() );
	MF_ASSERT( s_currentTask == &conversionTask );

	// Update the tasks thread id
	if (recursive_)
	{
		MF_ASSERT( s_currentTask->threadId_ == GetCurrentThreadId() );
	}
	else
	{
		MF_ASSERT( s_currentTask->threadId_ == GetCurrentThreadId() );
		s_currentTask->threadId_ = 0;
	}

	// Clear the current task and push it back in the queue if not recursive
	if (recursive_)
	{
		MF_ASSERT( s_currentTaskQueue == NULL );
		s_currentTask = NULL;
	}
	else
	{
		MF_ASSERT( s_currentTaskQueue != NULL );
		taskQueueMutex_.grab();

		ConversionTaskQueue::iterator it;
		ConversionTaskQueue::iterator itQueued;

		// Find the tasks that we depend on that are already queued and 
		// remove them so that we can push them to the front
		for ( it = s_currentTaskQueue->begin(); it != s_currentTaskQueue->end(); ++it )
		{
			if ((*it)->status_ != ConversionTask::QUEUED)
			{
				continue;
			}

			for ( itQueued = taskQueue_.begin(); itQueued != taskQueue_.end(); ++itQueued )
			{
				if ( *it == *itQueued )
				{
					taskQueue_.erase( itQueued );
					// Set the status of this task back to new as it is no longer in the queue.
					(*it)->status_ = ConversionTask::NEW;
					break;
				}
			}
		}

		// Find the tasks that we depend on that have already begun processing and 
		// make sure that we insert our task behind them in the queue.
		// We cannot remove these tasks from the queue and push them to the front because
		// these tasks may in turn be dependent on other tasks before them in the queue.
		ConversionTaskQueue::iterator itInsert = taskQueue_.begin();
		for ( it = s_currentTaskQueue->begin(); it != s_currentTaskQueue->end(); ++it )
		{
			if ((*it)->status_ <= ConversionTask::QUEUED ||
				(*it)->status_ >= ConversionTask::DONE)
			{
				continue;
			}

			for ( itQueued = itInsert; itQueued != taskQueue_.end(); ++itQueued )
			{
				if ( *it == *itQueued )
				{
					itInsert = itQueued + 1;
					break;
				}
			}
		}

		// Insert our task into the correct location in the queue.
		taskQueue_.insert( itInsert, s_currentTask );

		// Go through all our current 
		for ( it = s_currentTaskQueue->begin(); it != s_currentTaskQueue->end(); ++it )
		{
			if ((*it)->status_ != ConversionTask::NEW)
			{
				continue;
			}

			// Push this dependent task to the front of the queue
			taskQueue_.push_front( *it );
			// Set the task status to queued.
			(*it)->status_ = ConversionTask::QUEUED;
		}

		s_currentTask = NULL;
		bw_safe_delete( s_currentTaskQueue );
		taskQueueMutex_.give();
	}

	MF_VERIFY( ReleaseSemaphore( taskSemaphore_, 1, NULL ) );
}

void AssetCompiler::onTaskCompleted( ConversionTask & conversionTask )
{
	MF_ASSERT( s_currentTask == &conversionTask );
	MF_ASSERT( s_currentTaskQueue == NULL || !recursive_ );
	MF_ASSERT( !hasError() || conversionTask.status_ == ConversionTask::FAILED );
	resetErrorFlags();

	// Update the tasks thread id
	MF_ASSERT( s_currentTask->threadId_ == GetCurrentThreadId() );
	s_currentTask->threadId_ = 0;

	// Close the handle to the source
	::CloseHandle( s_currentTask->fileHandle_ );
	s_currentTask->fileHandle_ = NULL;

	// Clear the current task for this thread.
	s_currentTask = NULL;
	bw_safe_delete( s_currentTaskQueue );

	MF_VERIFY( ReleaseSemaphore( taskSemaphore_, 1, NULL ) );
}

void AssetCompiler::onPreCreateDependencies( ConversionTask & conversionTask )
{
	// Set a flag indicating that this thread is creating dependencies.
	MF_ASSERT( s_currentTask != NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_ASSERT( s_CreatingDependencies == false );
	MF_ASSERT( s_Converting == false );
	s_CreatingDependencies = true;
}

void AssetCompiler::onPostCreateDependencies( ConversionTask & conversionTask )
{
	// Clear the flag indicating that this thread is creating dependencies.
	MF_ASSERT( s_currentTask != NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_ASSERT( s_CreatingDependencies == true );
	MF_ASSERT( s_Converting == false );
	s_CreatingDependencies = false;
}

void AssetCompiler::onPreConvert( ConversionTask & conversionTask )
{
	// Set a flag indicating that this thread is converting.
	MF_ASSERT( s_currentTask != NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_ASSERT( s_CreatingDependencies == false );
	MF_ASSERT( s_Converting == false );
	s_Converting = true;
}

void AssetCompiler::onPostConvert( ConversionTask & conversionTask )
{
	// Clear the flag indicating that this thread is converting.
	MF_ASSERT( s_currentTask != NULL );
	MF_ASSERT( s_currentTaskQueue == NULL );
	MF_ASSERT( s_CreatingDependencies == false );
	MF_ASSERT( s_Converting == true );
	s_Converting = false;
}

bool AssetCompiler::handleMessage( DebugMessagePriority messagePriority,
								  const char * /*pCategory*/,
								  DebugMessageSource /*messageSource*/,
								  const LogMetaData & /*metaData*/,
								  const char * /*pFormat*/,
								  va_list /*argPtr*/ )
{
	if (!s_CreatingDependencies && !s_Converting)
	{
		return false;
	}

	// If this thread is creating dependencies or converting and an error or warning
	// message was triggered, Notify the asset pipeline that an error or warning
	// was detected for the current task.
	MF_ASSERT( s_currentTask != NULL );

	if (messagePriority == DebugMessagePriority::MESSAGE_PRIORITY_ERROR ||
		messagePriority == DebugMessagePriority::MESSAGE_PRIORITY_CRITICAL)
	{
		setError();
	}
	else if (messagePriority == DebugMessagePriority::MESSAGE_PRIORITY_WARNING)
	{
		setWarning();
	}
	return true;
}

void AssetCompiler::handleCritical( const char * msg )
{
	if (!s_CreatingDependencies && !s_Converting)
	{
		return;
	}

#if ASSET_PIPELINE_CAPTURE_ASSERTS
	// If an assert was fired while creating dependencies or converting, throw an exception
	// to cause the current task to error but not crash the application.
	throw msg;
#endif
}

bool AssetCompiler::resolveRelativePath( BW::string & path ) const
{
	if (BWResource::pathIsRelative( path ))
	{
		// Already relative
		return true;
	}

	if (!intermediatePath_.empty() &&
		strncmp( path.c_str(), intermediatePath_.c_str(), intermediatePath_.length() ) == 0)
	{
		// The path provided is an absolute intermediate path
		size_t begin = intermediatePath_.length();
		size_t end = path.length();
		path = path.substr( begin, end - begin );
		return true;
	}

	if (!outputPath_.empty() &&
		strncmp( path.c_str(), outputPath_.c_str(), outputPath_.length() ) == 0)
	{
		// The path provided is an absolute output path
		size_t begin = outputPath_.length();
		size_t end = path.length();
		path = path.substr( begin, end - begin );
		return true;
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

bool AssetCompiler::resolveSourcePath( BW::string & path ) const
{
	if (!resolveRelativePath( path ))
	{
		return false;
	}

	// Resolve the path
	path = BWResource::resolveFilename( path );
	return true;
}

bool AssetCompiler::resolveIntermediatePath( BW::string & path ) const
{
	return resolveCustomPath( intermediatePath_, path );
}

bool AssetCompiler::resolveOutputPath( BW::string & path ) const
{
	return resolveCustomPath( outputPath_, path );
}

bool AssetCompiler::resolveCustomPath( const BW::string & basePath, BW::string & path ) const
{
	BW::string actualBasePath = basePath;

	if (basePath.empty())
	{
		// Resolve the base path to be the source path component within the
		// resource tree. (Absolute version of relative path in paths.xml).
		MF_ASSERT(s_currentTask != NULL);
		const BW::string & sourcePath = s_currentTask->source_;

		for (BW::vector<BW::string>::const_iterator it = resourcePaths_.begin(),
			end = resourcePaths_.end(); it != end; ++it)
		{
			if (strncmp( sourcePath.c_str(), it->c_str(), it->length() ) == 0)
			{
				if (it->length() > actualBasePath.length())
				{
					actualBasePath = BW::string(it->c_str(), it->length());
					if (*actualBasePath.rbegin() != '/')
					{
						actualBasePath += '/';
					}
				}
			}
		}
	}

	if (strncmp( path.c_str(), actualBasePath.data(), actualBasePath.length() ) == 0)
	{
		// The path provided is already an absolute custom path
		return true;
	}

	if (!resolveRelativePath( path ))
	{
		return false;
	}

	path = actualBasePath + path;
	return true;
}

void AssetCompiler::addToolsResourcePaths()
{
	// Then read the paths already loaded in BWResource
	const int numPaths = BWResource::getPathNum();
	for (int i = 0; i < numPaths; ++i)
	{
		resourcePaths_.push_back( BWResource::getPath( i ) );
	}
}

#include <shlwapi.h>

BW::string AssetCompiler::sanitisePathParam( const char * pathParam )
{
	BW::string path = pathParam;
	if (path.empty() || BWResource::pathIsRelative( path ))
	{
		path = BWResource::getCurrentDirectory() + path;
	}

	std::replace( path.begin(), path.end(), '/', '\\' );
	BW::wstring wPath;
	bw_utf8tow( path, wPath );
	wchar_t wCanonicalizedPath[MAX_PATH];
	if ( PathCanonicalizeW( wCanonicalizedPath, wPath.c_str() ) )
	{
		bw_wtoutf8( wCanonicalizedPath, path );
	}
	path = BWResource::correctCaseOfPath( path );
	BWUtil::sanitisePath( path );

	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::FileType ft = fs->getFileType( path );
	if ((ft == IFileSystem::FT_DIRECTORY || ft == IFileSystem::FT_NOT_FOUND) &&
		path.back() != '/')
	{
		path += "/";
	}

	return path;
}

BW_END_NAMESPACE
