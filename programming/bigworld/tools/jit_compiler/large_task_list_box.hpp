#ifndef LARGE_TASK_LIST_BOX_HPP
#define LARGE_TASK_LIST_BOX_HPP

#include "task_list_box_base.hpp"

#include "wtl.hpp"
#include "task_fwd.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class TaskStore;
class MainMessageLoop;
class DetailsDialog;
struct ConversionTask;

class LargeTaskListBox : public ATL::CWindowImpl<LargeTaskListBox, WTL::CListBox>, public TaskListBoxBase
{
public:
	enum SortMode
	{
		SORT_NONE,
		SORT_FILENAME,
		SORT_FILETYPE,
		SORT_RESULT,
	};

public:
	LargeTaskListBox(TaskStore & store, MainMessageLoop & messageLoop);

	void updateItemHeight();

	virtual void addTask(TaskInfoPtr task);
	virtual void removeTask(TaskInfoPtr task);

	void showNormalTasks(bool show);
	void showWarningTasks(bool show);
	void showErrorTasks(bool show);

	void setSortMode(SortMode mode);

	void pauseRedraw();
	void resumeRedraw();

	BEGIN_MSG_MAP_EX(TaskListBox)
		MSG_WM_LBUTTONDBLCLK(onDoubleClick)
		MSG_WM_CONTEXTMENU(onContextMenu)
		MSG_OCM_MEASUREITEM(onMeasureItem)
		MSG_OCM_DRAWITEM(onDrawItem)
		MSG_WM_ERASEBKGND(onEraseBackground)
		MSG_WM_PAINT(onPaint)
	END_MSG_MAP()

private:
	typedef BW::vector< TaskInfoPtr > TaskList;

	void updateCurrentCount(bool resetView = false);
	bool isTaskVisible(TaskInfoPtr taskInfo) const;
	void filterIntoTaskList(TaskList& list);
	void sortTaskList(TaskList& list);

	void regenerateCurrentList();

	void copyTaskDetails(TaskInfoPtr taskInfo);

	void onDoubleClick(UINT flags, ::CPoint clientPoint);
	void onContextMenu(CWindow handle, ::CPoint screenPoint);
	void onMeasureItem(int id, LPMEASUREITEMSTRUCT measureStruct);
	void onDrawItem(int id, LPDRAWITEMSTRUCT drawStruct);
	LRESULT onEraseBackground(WTL::CDCHandle dc);
	void onPaint(WTL::CDCHandle dc);

	TaskList allTasks_;
	TaskList currentTasks_;

	WTL::CMenu contextMenu_;

	SortMode sortMode_;
	bool showFilter_[3];
};

BW_END_NAMESPACE

#endif // LARGE_TASK_LIST_BOX_HPP
