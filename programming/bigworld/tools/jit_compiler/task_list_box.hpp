#ifndef TASK_LIST_BOX_HPP
#define TASK_LIST_BOX_HPP

#include "task_list_box_base.hpp"

#include "wtl.hpp"
#include "task_fwd.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class TaskStore;
class MainMessageLoop;
class DetailsDialog;
struct ConversionTask;

class TaskListBox : public ATL::CWindowImpl<TaskListBox, WTL::CListBox>, public TaskListBoxBase
{
public:
	TaskListBox(TaskStore & store, MainMessageLoop & messageLoop);

	virtual void addTask(TaskInfoPtr task);
	virtual void removeTask(TaskInfoPtr task);

	BEGIN_MSG_MAP_EX(TaskListBox)
		MSG_WM_LBUTTONDBLCLK(onDoubleClick)
		MSG_WM_CONTEXTMENU(onContextMenu)
	END_MSG_MAP()

private:
	const ConversionTask * taskAtIndex(const int index) const;

	void onDoubleClick(UINT flags, ::CPoint clientPoint);
	void onContextMenu(CWindow handle, ::CPoint screenPoint);

	WTL::CMenu contextMenu_;
};

BW_END_NAMESPACE

#endif // TASK_LIST_BOX_HPP
