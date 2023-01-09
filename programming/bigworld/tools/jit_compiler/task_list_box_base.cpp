#include "task_list_box_base.hpp"

#include "task_info.hpp"
#include "task_store.hpp"
#include "message_loop.hpp"
#include "details_dialog.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

TaskListBoxBase::TaskListBoxBase(TaskStore & store, MainMessageLoop & messageLoop) : 
	store_(store),
	messageLoop_(messageLoop)
{
}

void TaskListBoxBase::createDetailsDialog(HWND parentWindow, TaskInfoPtr taskInfo)
{
	DetailsDialog::createDialog(parentWindow, taskInfo, messageLoop_, store_);
}

void TaskListBoxBase::openTaskApplication(TaskInfoPtr taskInfo)
{
	BW::wstring fileName = bw_utf8tow(taskInfo->source());
	::ShellExecute(NULL, NULL, fileName.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void TaskListBoxBase::openTaskFolder(TaskInfoPtr taskInfo)
{
	BW::wstring filePath = bw_utf8tow(BWUtil::getFilePath(taskInfo->source()));
	::ShellExecute(NULL, NULL, filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

BW_END_NAMESPACE
