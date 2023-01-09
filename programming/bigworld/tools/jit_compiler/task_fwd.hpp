#ifndef TASK_FOWRWARD_DEFS_HPP
#define TASK_FOWRWARD_DEFS_HPP

#include "cstdmf/bw_namespace.hpp"

#include <memory>
#include <functional>

BW_BEGIN_NAMESPACE

enum TaskInfoState
{
	NONE,
	CHECKING,
	GENERATING_DEPENDENCIES,
	CONVERTING,
};

class TaskInfo;
typedef std::shared_ptr<TaskInfo> TaskInfoPtr;


enum TaskStoreAction
{
	ADDED,
	REMOVED
};

class TaskStore;

BW_END_NAMESPACE

#endif // TASK_FOWRWARD_DEFS_HPP
