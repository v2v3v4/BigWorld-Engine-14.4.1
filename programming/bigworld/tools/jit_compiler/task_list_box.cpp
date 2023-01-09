#include "task_list_box.hpp"

#include "task_info.hpp"
#include "task_store.hpp"
#include "message_loop.hpp"
#include "details_dialog.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

TaskListBox::TaskListBox(TaskStore & store, MainMessageLoop & messageLoop) : 
	TaskListBoxBase(store, messageLoop)
{
	contextMenu_.LoadMenuW(MAKEINTRESOURCE(IDR_TASK_CONTEXT_MENU));
}

void TaskListBox::addTask(TaskInfoPtr task)
{
	const BW::wstring name = task->getFormattedName();
	const int index = AddString(name.c_str());
	void* pointer = const_cast<void*>(static_cast<const void *>(task->conversionTask()));
	SetItemDataPtr(index, pointer);
}

void TaskListBox::removeTask(TaskInfoPtr task)
{
	auto targetTask = task->conversionTask();
	const int indexes = GetCount();
	for (int index = 0; index < indexes; ++index)
	{
		auto testTask = taskAtIndex(index);
		if (testTask == targetTask)
		{
			DeleteString(index);
			break;
		}
	}
}

const ConversionTask * TaskListBox::taskAtIndex(const int index) const
{
	return static_cast<const ConversionTask *>(GetItemDataPtr(index));
}

void TaskListBox::onDoubleClick(UINT flags, ::CPoint clientPoint)
{
	BOOL outside = TRUE;
	uint index = ItemFromPoint(clientPoint, outside);
	if (outside == TRUE)
	{
		return;
	}

	auto task = taskAtIndex(index);
	MF_ASSERT(task != nullptr);

	auto taskInfo = store_.findTaskInfo(task);
	if (taskInfo)
	{
		createDetailsDialog(GetParent(), taskInfo);
	}
}

void TaskListBox::onContextMenu(CWindow handle, ::CPoint screenPoint)
{
	if (contextMenu_.IsNull())
	{
		return;
	}

	::CPoint clientPoint = screenPoint;
	ScreenToClient(&clientPoint);

	BOOL outside = TRUE;
	uint index = ItemFromPoint(clientPoint, outside);
	if (outside == TRUE)
	{
		return;
	}

	auto task = taskAtIndex(index);
	MF_ASSERT(task != nullptr);

	auto taskInfo = store_.findTaskInfo(task);
	if (!taskInfo)
	{
		return;
	}

	WTL::CMenuHandle popupMenu = contextMenu_.GetSubMenu(0);
	UINT id = popupMenu.TrackPopupMenu(TPM_NONOTIFY | TPM_RETURNCMD, screenPoint.x, screenPoint.y, GetParent());
	switch (id)
	{
	case ID_TASK_DETAILS:
		createDetailsDialog(GetParent(), taskInfo);
		break;
	case ID_TASK_OPEN:
		openTaskApplication(taskInfo);
		break;
	case ID_TASK_OPEN_FOLDER:
		openTaskFolder(taskInfo);
		break;
	};
}

BW_END_NAMESPACE
