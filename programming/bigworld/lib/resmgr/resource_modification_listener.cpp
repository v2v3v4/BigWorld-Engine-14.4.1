#include "pch.hpp"

#include "resource_modification_listener.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace
{
/**
 * canOpenExclusivelyFromWrite:
 *
 *	desc:
 *		Checks to see if the filename passed in can be opened exclusively from writing.
 *		This is used for texture files that have recently been modified, 
 *		as the writing of the texture file may not be complete when the file
 *		modification event is received. This checks to see if the writing 
 *		has completed before starting processing.
 *
 *		@param filename full pathname of file to check
 *
 *		@return	whether the file could be opened exclusively from write.
 */
bool canOpenExclusivelyFromWrite( const BW::StringRef &fileName )
{
	WCHAR buffer[BW_MAX_PATH];
	if (!bw_utf8tow(fileName.data(), fileName.length(), buffer, BW_MAX_PATH))
	{
		return false;
	}

	HANDLE pathHandle_ = ::CreateFileW( buffer,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,  
		0, 
		NULL );

	bool successfulOpen = false;
	if (pathHandle_ != INVALID_HANDLE_VALUE)
	{
		CloseHandle( pathHandle_ ); // close the file
		successfulOpen = true;
	}

	return successfulOpen;
}

} // End anon namespace


ResourceModificationListener::ReloadTask::ReloadTask(
	const BW::StringRef& basePath, 				 
	const BW::StringRef& resourceID,
	ResourceModificationListener* owner  ) :
	BackgroundTask( "ReloadTask_IOPoll" ),
	basePath_( basePath.to_string() ),
	resourceID_( resourceID.to_string() ),
	pOwner_( owner ),
	timesRequeued_( 0 )
{
}


ResourceModificationListener::ReloadTask::~ReloadTask()
{
}


const BW::string & ResourceModificationListener::ReloadTask::basePath() const
{
	return basePath_;
}


const BW::string & ResourceModificationListener::ReloadTask::resourceID() const
{
	return resourceID_;
}


void ResourceModificationListener::ReloadTask::doBackgroundTask( 
	TaskManager & mgr )
{
	// check to see if the process writing to the file has completed by 
	// attempting to open it exclusively.
	BW::string fullName = basePath_ + resourceID_;

	if (canOpenExclusivelyFromWrite( fullName ))
	{
		// Time to queue for main thread and execute the reload
		BgTaskManager::instance().addMainThreadTask( this );
	}
	else
	{
		// If the file is not ready yet, requeue this task to be done
		// in the future (don't stop other files from being able to get through)
		
		++timesRequeued_;

		TRACE_MSG( "ResourceModificationListener::ReloadTask: Requeued "
			"reload task %d times. (%s)\n",
			timesRequeued_,
			fullName.c_str());

		BgTaskManager::instance().addBackgroundTask( this );
	}
}


void ResourceModificationListener::ReloadTask::doMainThreadTask( 
	TaskManager & mgr )
{
	executeReload();

	// Clear this task from our owner
	pOwner_->notifyCompletedTask( this );
}


void ResourceModificationListener::queueReloadTask( 
	const ReloadTaskPtr& task )
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	// Add this task to the list of things we're in charge of doing:
	pendingTasks_.push_back( task );

	BgTaskManager::instance().addBackgroundTask( task );
}


void ResourceModificationListener::cancelReloadTasks()
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	for (TaskList::iterator iter = pendingTasks_.begin();
		iter != pendingTasks_.end();
		++iter)
	{
		BackgroundTaskPtr pTask = *iter;
		pTask->stop();
	}
	pendingTasks_.clear();
}


void ResourceModificationListener::notifyCompletedTask( ReloadTask* pTask )
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	TaskList::iterator iter = 
		std::find( pendingTasks_.begin(), pendingTasks_.end(), pTask );
	if (iter == pendingTasks_.end())
	{
		return;
	}

	*iter = pendingTasks_.back();
	pendingTasks_.pop_back();
}


BW_END_NAMESPACE

// resource_modification_listener.cpp
