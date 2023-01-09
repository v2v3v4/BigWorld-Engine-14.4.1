#ifndef TASK_STORE_HPP
#define TASK_STORE_HPP

#include "task_fwd.hpp"

#include "signal.hpp"

#include "cstdmf/stdmf.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_namespace.hpp"

#include <functional>
#include <utility>

BW_BEGIN_NAMESPACE

class TaskStore
{
public:
	typedef std::function<void ()> CallbackFunction;

	typedef void TaskCallbackSignature(TaskInfoPtr, TaskStoreAction);
	typedef std::function<TaskCallbackSignature> TaskCallbackFunction;

public:
	TaskStore();

	// Interface for JIT Compiler
	void scanningStarted();
	void scanningStopped();

	void resetTask(ConversionTask * task);
	void addRequestedTask(ConversionTask * task);
	void setTaskCurrent(ConversionTask * task);
	void setTaskComplete(ConversionTask * task);

	void setCurrentTaskState(ConversionTask * task, TaskInfoState state);
	bool handleTaskMessage(ConversionTask * task, DebugMessagePriority priority, 
		const char * pCategory, BW::WStringRef message);
	void appendLogToTask(ConversionTask * task, BW::WStringRef message);
	void addOutputToTask(ConversionTask * task, const BW::StringRef & output);

	// Interface for UI
	void setStatusCallbacks(CallbackFunction scanningStartedCallback, 
							CallbackFunction scanningStoppedCallback);

	Connection registerRequestedTaskCallback(TaskCallbackFunction callback);
	Connection registerCurrentTaskCallback(TaskCallbackFunction callback);
	Connection registerCompletedTaskCallback(TaskCallbackFunction callback);

	TaskInfoPtr findTaskInfo(const ConversionTask * task) const;
	TaskInfoPtr findOrCreateTaskInfo(const ConversionTask * task);

	void updateErrorWarningCounts();

	bool failuresExist() const;
	bool warningsExist() const;
	int failedCount() const;
	int warningCount() const;

private:

	typedef BW::map<const ConversionTask *, TaskInfoPtr> ConversionTaskMap;

	SimpleMutex requestedGuard_;
	SimpleMutex currentGuard_;
	mutable SimpleMutex allTasksGuard_;

	ConversionTaskMap requestedTasks_;
	ConversionTaskMap currentTasks_;
	ConversionTaskMap completedTasks_;
	ConversionTaskMap allTasks_;

	CallbackFunction scanningStartedCallback_;
	CallbackFunction scanningStoppedCallback_;

	typedef Signal<TaskCallbackSignature> TaskSignal;

	TaskSignal requestedSignal_;
	TaskSignal currentSignal_;
	TaskSignal completedSignal_;

	int failedCount_;
	int warningCount_;
};

BW_END_NAMESPACE

#endif // TASK_STORE_HPP
