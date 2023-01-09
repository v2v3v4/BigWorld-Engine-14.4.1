#include "task_store.hpp"

#include "task_info.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE


TaskStore::TaskStore() : 
	failedCount_(0),
	warningCount_(0)
{
}


void TaskStore::scanningStarted()
{
	if (scanningStartedCallback_)
	{
		scanningStartedCallback_();
	}
}

void TaskStore::scanningStopped()
{
	if (scanningStoppedCallback_)
	{
		scanningStoppedCallback_();
	}
}

TaskInfoPtr TaskStore::findTaskInfo(const ConversionTask * task) const
{
	SimpleMutexHolder lock(allTasksGuard_);

	auto iter = allTasks_.find(task);
	if ( iter != std::end(allTasks_) ) 
	{
		return iter->second;
	}

	return nullptr;
}

TaskInfoPtr TaskStore::findOrCreateTaskInfo(const ConversionTask * task)
{
	TaskInfoPtr taskInfo = findTaskInfo(task);
	if (!taskInfo)
	{
		taskInfo = std::make_shared<TaskInfo>(task);

		SimpleMutexHolder lock(allTasksGuard_);
		allTasks_[task] = taskInfo;
	}

	return taskInfo;
}

void TaskStore::resetTask(ConversionTask * task)
{
	MF_ASSERT(task != nullptr);

	TRACE_MSG("Reseting task: %s\n", task->source_.c_str());

	auto taskInfo = findOrCreateTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);

	bool removedFromCompleted = false;

	{
		SimpleMutexHolder lock(currentGuard_);

		MF_ASSERT(currentTasks_.find(task) == std::end(currentTasks_));

		removedFromCompleted = completedTasks_.erase(task) != 0;
	}

	taskInfo->clearLog();
	taskInfo->clearOutputs();
	taskInfo->clearSubTasks();
	taskInfo->clearErrorFlags();

	if (removedFromCompleted)
	{
		completedSignal_.call(taskInfo, REMOVED);
	}
}

void TaskStore::addRequestedTask(ConversionTask * task)
{
	MF_ASSERT(task != nullptr);

	TaskInfoPtr taskInfo;
	{
		SimpleMutexHolder lock(requestedGuard_);
		if (requestedTasks_.find(task) != std::end(requestedTasks_))
		{
			TRACE_MSG("Task already in Requested list: %s\n", task->source_.c_str());
			return;
		}

		TRACE_MSG("Adding task to Requested list: %s\n", task->source_.c_str());
		taskInfo = findOrCreateTaskInfo(task);
		requestedTasks_[task] = taskInfo;
	}

	requestedSignal_.call(taskInfo, ADDED);
}

void TaskStore::setTaskCurrent(ConversionTask * task)
{
	MF_ASSERT(task != nullptr);

	TRACE_MSG("Moving task to Current list: %s\n", task->source_.c_str());

	auto taskInfo = findOrCreateTaskInfo(task);

	{
		SimpleMutexHolder lock(currentGuard_);

		MF_ASSERT(currentTasks_.find(task) == std::end(currentTasks_));
		MF_ASSERT(completedTasks_.find(task) == std::end(completedTasks_));

		currentTasks_[task] = taskInfo;
	}

	currentSignal_.call(taskInfo, ADDED);
}

void TaskStore::setTaskComplete(ConversionTask * task)
{
	MF_ASSERT(task != nullptr);

	TRACE_MSG("Moving task to Completed list: %s\n", task->source_.c_str());

	auto taskInfo = findTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);

	{
		SimpleMutexHolder lock(currentGuard_);

		MF_ASSERT(currentTasks_.find(task) != std::end(currentTasks_));
		MF_ASSERT(completedTasks_.find(task) == std::end(completedTasks_));

		currentTasks_.erase(task);
		completedTasks_[task] = taskInfo;
	}

	bool removeFromRequested = false;
	{
		SimpleMutexHolder lock(requestedGuard_);

		removeFromRequested = requestedTasks_.erase(task) != 0;
	}

	if (removeFromRequested)
	{
		requestedSignal_.call(taskInfo, REMOVED);
	}

	currentSignal_.call(taskInfo, REMOVED);
	completedSignal_.call(taskInfo, ADDED);
}


bool TaskStore::handleTaskMessage(ConversionTask * task, 
								  DebugMessagePriority priority, 
								  const char * pCategory,
								  BW::WStringRef message)
{
	auto taskInfo = findTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);

	BW::wstring outputString;
	outputString.reserve(message.length() + 20);

	if (taskInfo->state() == GENERATING_DEPENDENCIES ||
		taskInfo->state() == CONVERTING)
	{
		if (priority == DebugMessagePriority::MESSAGE_PRIORITY_CRITICAL || 
			priority == DebugMessagePriority::MESSAGE_PRIORITY_ERROR)
		{
			taskInfo->setErrors();
			outputString.append(L"ERROR: ");
		}
		else if (priority == DebugMessagePriority::MESSAGE_PRIORITY_WARNING)
		{
			taskInfo->setWarnings();
			outputString.append(L"WARNING: ");
		}
	}
	else if (pCategory == NULL || strcmp( pCategory, ASSET_PIPELINE_CATEGORY ) != 0)
	{
		return false;
	}

	outputString.append(message.data(), message.length());

	taskInfo->appendToLog(outputString);

	return true;
}

void TaskStore::appendLogToTask(ConversionTask * task, BW::WStringRef message)
{
	auto taskInfo = findTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);

	BW::wstring outputString(message.data(), message.length());
	
	taskInfo->appendToLog(outputString);
}

void TaskStore::addOutputToTask(ConversionTask * task, const BW::StringRef & output)
{
	auto taskInfo = findTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);
	taskInfo->addOutput(output);
}

void TaskStore::setCurrentTaskState(ConversionTask * task, TaskInfoState state)
{
	auto taskInfo = findOrCreateTaskInfo(task);
	MF_ASSERT(taskInfo != nullptr);
	taskInfo->setState(state);
}


void TaskStore::setStatusCallbacks(CallbackFunction scanningStartedCallback, 
								   CallbackFunction scanningStoppedCallback)
{
	scanningStartedCallback_ = scanningStartedCallback;
	scanningStoppedCallback_ = scanningStoppedCallback;
}


Connection TaskStore::registerRequestedTaskCallback(TaskCallbackFunction callback)
{
	return requestedSignal_.connect(callback);
}

Connection TaskStore::registerCurrentTaskCallback(TaskCallbackFunction callback)
{
	return currentSignal_.connect(callback);
}

Connection TaskStore::registerCompletedTaskCallback(TaskCallbackFunction callback)
{
	return completedSignal_.connect(callback);
}


void TaskStore::updateErrorWarningCounts()
{
	SimpleMutexHolder lock(allTasksGuard_);

	failedCount_ = warningCount_ = 0;
	for (const auto & taskPair : allTasks_)
	{
		const TaskInfoPtr taskInfo = taskPair.second;
		if (taskInfo->hasErrors() || taskInfo->hasFailed())
		{
			++failedCount_;
		}
		else if (taskInfo->hasWarnings())
		{
			++warningCount_;
		}
	}
}

bool TaskStore::failuresExist() const
{
	return failedCount_ != 0;
}

bool TaskStore::warningsExist() const
{
	return warningCount_ != 0;
}

int TaskStore::failedCount() const
{
	return failedCount_;
}

int TaskStore::warningCount() const
{
	return warningCount_;
}



BW_END_NAMESPACE
