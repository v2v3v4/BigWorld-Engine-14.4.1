#ifndef TASK_LIST_BOX_BASE_HPP
#define TASK_LIST_BOX_BASE_HPP

#include "wtl.hpp"
#include "task_fwd.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class TaskStore;
class MainMessageLoop;
class DetailsDialog;
struct ConversionTask;

class TaskListBoxBase
{
public:
	virtual void addTask(TaskInfoPtr task) = 0;
	virtual void removeTask(TaskInfoPtr task) = 0;

protected:
	TaskListBoxBase(TaskStore & store, MainMessageLoop & messageLoop);

	void createDetailsDialog(HWND parentWindow, TaskInfoPtr taskInfo);
	void openTaskApplication(TaskInfoPtr taskInfo);
	void openTaskFolder(TaskInfoPtr taskInfo);

	TaskStore & store_;
	MainMessageLoop & messageLoop_;
};

BW_END_NAMESPACE

#endif // TASK_LIST_BOX_BASE_HPP
