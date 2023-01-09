#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include <utility> // Need std::pair

#include "conversion_rule.hpp"
#include "task_finder.hpp"

DECLARE_DEBUG_COMPONENT2( ASSET_PIPELINE_CATEGORY, 0 )

BW_BEGIN_NAMESPACE

TaskFinder::TaskFinder( Compiler& compiler,
						ConversionRules& rules ) :
	compiler_(compiler),
	rules_(rules)
{
}


TaskFinder::~TaskFinder() 
{
	// clear out the tasks
	//	memory for tasks is owned by TaskFinder
	tasksLock_.beginWrite();
	StringHashMap<ConversionTask *>::iterator taskIter = tasks_.begin();
	while ( taskIter != tasks_.end() )
	{
		ConversionTask *pTask = taskIter->second;
		MF_ASSERT( pTask );
		bw_safe_delete( pTask );
		++taskIter;
	}
	tasks_.clear();
	tasksLock_.endWrite();
}

/* iterates recursively over directories to find root tasks. */
void TaskFinder::findTasks( const BW::StringRef& directory )
{
	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::FileType ft = fs->getFileType( directory );
	if ( ft == IFileSystem::FT_NOT_FOUND )
	{
		ERROR_MSG( "Directory passed in (%s) not found\n", directory.to_string().c_str() );
	}
	else if (ft == IFileSystem::FT_FILE)
	{
		iterateFile( directory );
	}
	else if (ft == IFileSystem::FT_DIRECTORY)
	{
		iterateDirectory( directory );
	}
}

void TaskFinder::iterateFile( const BW::StringRef& file )
{
	if (!compiler_.shouldIterateFile( file ))
	{
		return;
	}

	ConversionTask * task = getTask( file, true );
	if ( task != NULL )
	{
		compiler_.queueTask( *task );
	}
}

void TaskFinder::iterateDirectory( const BW::StringRef& directory )
{
	BW::string path = BWUtil::formatPath( directory );
	if (!compiler_.shouldIterateDirectory( path ))
	{
		return;
	}

	BW::string relativeDirectory = BWResource::dissolveFilename( path );
	bool isBWPath = relativeDirectory.length() < path.length();
	if (isBWPath)
	{
		INFO_MSG( "Searching %s\n", path.c_str() );
	}

	char pathBuffer[BW_MAX_PATH];
	StringBuilder pathBuilder( pathBuffer, BW_MAX_PATH );

	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::Directory dir;
	fs->readDirectory( dir, path );
	for( IFileSystem::Directory::iterator it = dir.begin();
		it != dir.end(); ++it )
	{
		pathBuilder.clear();
		pathBuilder.append( path );
		pathBuilder.append( *it );
		const char * subPath = pathBuilder.string();

		IFileSystem::FileType ft = fs->getFileType( subPath );
		if (ft == IFileSystem::FT_FILE && isBWPath)
		{
			// Don't iterate files that aren't in one of our resource paths
			iterateFile( subPath );
		}
		else if (ft == IFileSystem::FT_DIRECTORY)
		{
			iterateDirectory( subPath );
		}
	}
}

ConversionTask & TaskFinder::getTask( const BW::StringRef & filename )
{
	BW::string directory = BWResource::getFilePath( filename );
	if (!compiler_.shouldIterateDirectory( directory ) ||
		!compiler_.shouldIterateFile( filename ))
	{
		BW::string source = filename.to_string();
		ConversionTask *pNewTask = new ConversionTask;
		pNewTask->source_ = source;
		pNewTask->converterId_ = ConversionTask::s_unknownId; // unknown converter type
		pNewTask->converterVersion_ = "";
		pNewTask->converterParams_ = "";
		pNewTask->status_ = ConversionTask::FAILED;
		pNewTask->threadId_ = 0;

		tasksLock_.beginWrite();
		std::pair< StringHashMap<ConversionTask *>::iterator, bool > insertIt = 
			tasks_.insert( std::make_pair( source, pNewTask ) );
		if (!insertIt.second)
		{
			delete pNewTask;
			pNewTask = insertIt.first->second;
		}
		tasksLock_.endWrite();

		return *pNewTask;
	}

	ConversionTask * task = getTask( filename, false );
	MF_ASSERT( task );
	return *task;
}

// TaskFinder::GetTask
// matches file conversion rules to task. 
// creates a new task if it doesn't exit yet.
//
ConversionTask * TaskFinder::getTask( const BW::StringRef & filename, const bool bRoot )
{
	BW_GUARD;

	// Make sure we are working with absolute filenames
	MF_ASSERT( !BWResource::pathIsRelative( filename ) );

	// check to see if filename already has a task, and if so, return it

	BW::string source = filename.to_string();
	ConversionTask *pTask = NULL;
	
	tasksLock_.beginRead();
	if (!this->tasks_.empty())
	{
		StringHashMap<ConversionTask* >::iterator taskIter = 
			this->tasks_.find( source );
		if (taskIter != this->tasks_.end())
		{
			pTask = taskIter->second;
		}
	}
	tasksLock_.endRead();

	if (pTask != NULL)
	{
		return pTask;
	}

	// else create one from the rules.

	ConversionTask *pNewTask = new ConversionTask;
	MF_ASSERT( pNewTask );
	//  if we have been called from findTasks (bRoot == true),
	//		we only want to create tasks for files on disk that have root rules,
	//		not tasks for files on disk that have tasks created via dependencies.
	//	
	ConversionRules::const_reverse_iterator ruleIter = rules_.crbegin();
	ConversionRules::const_reverse_iterator ruleEnd = rules_.crend();
	for ( ; ruleIter != ruleEnd; ++ruleIter )
	{
		if (bRoot)
		{
			if ((*ruleIter)->createRootTask( filename, *pNewTask ))
			{
				break;
			}
		}
		else
		{
			if ((*ruleIter)->createTask( filename, *pNewTask ))
			{
				break;
			}
		}
	}

	// if we didn't find a matching rule,
	//	create an empty task with an unknown id?
	if ( ruleIter == ruleEnd )
	{
		if ( bRoot )
		{
			bw_safe_delete( pNewTask );
			return NULL;
		}

		WARNING_MSG( "Could not find a rule to create a task for file %s\n", 
			source.c_str() );
		pNewTask->source_ = source;
		pNewTask->converterId_ = ConversionTask::s_unknownId; // unknown converter type
		pNewTask->converterVersion_ = "";
		pNewTask->converterParams_ = "";
		pNewTask->status_ = ConversionTask::FAILED;
		pNewTask->threadId_ = 0;
	}
	else
	{
		if (bRoot)
		{
			INFO_MSG( "  Created root task: %s\n", source.c_str() );
		}
		else
		{
			INFO_MSG( "  Created task: %s\n", source.c_str() );
		}

		pNewTask->source_ = source;
		pNewTask->status_ = ConversionTask::NEW;
		pNewTask->threadId_ = 0;
	}

	// add it to the tasks hashmap
	tasksLock_.beginWrite();
	std::pair< StringHashMap<ConversionTask *>::iterator, bool > insertIt = 
		tasks_.insert( std::make_pair( source, pNewTask ) );
	if (!insertIt.second)
	{
		delete pNewTask;
		pNewTask = insertIt.first->second;
	}
	tasksLock_.endWrite();

	return pNewTask;
}

BW_END_NAMESPACE
