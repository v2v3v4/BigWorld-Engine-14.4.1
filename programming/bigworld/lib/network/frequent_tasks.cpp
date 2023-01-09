#include "pch.hpp"

#include "frequent_tasks.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/profiler.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{


/**
 *	Constructor.
 */
FrequentTasks::FrequentTasks() : 
	container_(),
	isDirty_( false ),
	pGotDestroyed_( NULL )
{
}


/**
 * 	Destructor.
 */
FrequentTasks::~FrequentTasks()
{
	if (pGotDestroyed_)
	{
		*pGotDestroyed_ = true; 
	}
}


/**
 *	Add a frequent task.
 *
 *	@param pTask 	The frequent task.
 */
void FrequentTasks::add( FrequentTask * pTask )
{
	isDirty_ = true;
	container_.push_back( pTask );
}


/**
 *	Remove a frequent task.
 *
 *	@param pTask 		The frequent task to remove.
 *
 *	@return			Whether the frequent task previously existed.
 */
bool FrequentTasks::cancel( FrequentTask * pTask )
{
	BW_GUARD_PROFILER( FrequentTasks_cancel );
	isDirty_ = true;
	Container::iterator iter =
		std::find( container_.begin(), container_.end(), pTask );

	if (iter != container_.end())
	{
		container_.erase( iter );
		return true;
	}

	return false;
}


/**
 * 	Process all frequent tasks now.
 */
void FrequentTasks::process()
{
	bool wasInProcess = pGotDestroyed_ != NULL;

	if (wasInProcess && isDirty_)
	{
		// not processing
		return;
	}
	isDirty_ = false;

	// This automatic boolean stores whether we get destroyed as a result of
	// executing a FrequentTask.
	bool gotDestroyed = false; 

	if (!wasInProcess)
	{
		pGotDestroyed_ = &gotDestroyed;
	}
	else
	{
		MF_ASSERT( pGotDestroyed_ != NULL );
	}

	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		FrequentTask * pTask = *iter;

		// Save away in this call's stack in case we are destructed in
		// doTask().
		bool * pGotDestroyed = pGotDestroyed_; 
		MF_ASSERT( pGotDestroyed != NULL );

#if ENABLE_PROFILER
		ScopedProfiler _taskProfile( pTask->taskName() );
#endif
		pTask->doTask();

		if (*pGotDestroyed)
		{
			// Don't access any member state, exit immediately.
			return;
		}

		// If the vector has been modified, the iterator is now invalid.
		if (isDirty_)
		{
			break;
		}

		++iter;
	}


	if (!wasInProcess)
	{
		pGotDestroyed_ = NULL;
	}
}


} // namespace Mercury

BW_END_NAMESPACE


// frequent_tasks.cpp

