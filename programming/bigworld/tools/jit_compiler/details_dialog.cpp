#include "details_dialog.hpp"
#include "task_info.hpp"

#include "cstdmf/string_utils.hpp"
#include "cstdmf/bw_stack_container.hpp"
#include "cstdmf/debug.hpp"

namespace std
{
	using namespace placeholders;
}

BW_BEGIN_NAMESPACE

void DetailsDialog::createDialog(CWindow parentWindow, TaskInfoPtr task, 
								 MainMessageLoop & messageLoop, TaskStore & store)
{
	auto dialog = new DetailsDialog(task, messageLoop, store);
	MF_ASSERT(dialog != nullptr);
	if (dialog->Create(parentWindow) == NULL)
	{
		ERROR_MSG("Could not create Details Dialog for: %s\r\n", task->source().c_str());
		return;
	}

	dialog->ShowWindow(SW_SHOW);
}

void DetailsDialog::OnFinalMessage(HWND handle)
{
	// Gets called from the message loop once all messages have been processed for this window
	// Hence we should destroy the object here as the dialog is already closed
	delete this;
}

DetailsDialog::DetailsDialog(TaskInfoPtr info, MainMessageLoop & messageLoop, TaskStore & store) : 
	store_(store),
	messageLoop_(messageLoop),
	info_(info),
	dependsList_(store, messageLoop)
{
}

DetailsDialog::~DetailsDialog()
{
}


void DetailsDialog::updateControls()
{
	const WCHAR * text = L"";
	switch (info_->state())
	{
	case TaskInfoState::CHECKING:
		text = L"Checking";
		break;
	case TaskInfoState::GENERATING_DEPENDENCIES:
		text = L"Generating Dependencies";
		break;
	case TaskInfoState::CONVERTING:
		text = L"Converting";
		break;
	case TaskInfoState::NONE:
	default:
		text = L"Idle";
		break;
	};
	statusText_.SetWindowTextW(text);


	TaskInfo::LogDetailLevel detailLevel = TaskInfo::LOG_DETAIL_ALL;
	switch (logDetailCombo_.GetCurSel())
	{
	case 1:
		detailLevel = TaskInfo::LOG_DETAIL_WARNINGS;
		break;
	case 2:
		detailLevel = TaskInfo::LOG_DETAIL_ERRORS;
		break;
	}

	BW::wstring logString = info_->getFormattedLog( detailLevel );
	logEdit_.SetWindowTextW(logString.c_str());


	outputsList_.ResetContent();
	for (const auto & output : info_->outputs())
	{
		BW::wstring name = bw_utf8tow(output);
		outputsList_.AddString(name.c_str());
	}

	info_->resolveSubTasks(store_);
	dependsList_.ResetContent();
	for (const auto & depend : info_->subTasks())
	{
		dependsList_.addTask(depend);
	}
}

void DetailsDialog::onTaskInfoChanged(TaskInfoPtr taskInfo)
{
	if (info_ != taskInfo)
	{
		return;
	}

	updateControls();
}

int DetailsDialog::onInitDialog(ATL::CWindow handle, LPARAM data)
{
	DlgResize_Init();

	statusText_.Attach(GetDlgItem(ID_STATUS_TEXT));
	logEdit_.Attach(GetDlgItem(IDC_DETAILS_LOG_EDIT));
	outputsList_.Attach(GetDlgItem(IDC_OUTPUTS_LIST));
	dependsList_.SubclassWindow(GetDlgItem(IDC_DEPENDS_LIST));

	{
		BW::WStackString<500> titleString;
		titleString->append(L"Task Details - ");
		titleString->append(info_->getFormattedName());
		SetWindowTextW(titleString->c_str());
	}

	logDetailCombo_.Attach(GetDlgItem(IDC_LOG_DETAIL));
	logDetailCombo_.AddString(L"Show All");
	logDetailCombo_.AddString(L"Show Warnings");
	logDetailCombo_.AddString(L"Show Errors");
	logDetailCombo_.SetCurSel(0);

	updateControls();

	updateConnection_ = info_->registerChangedCallback(
		std::bind(&DetailsDialog::onTaskInfoChanged, this, std::_1));

	return FALSE;
}

void DetailsDialog::onClose()
{
	updateConnection_.disconnect();
	DestroyWindow();
}

void DetailsDialog::onMenuClose(UINT notifyCode, int id, CWindow handle)
{
	SendMessage(WM_CLOSE);
}

void DetailsDialog::onLogSetFocus(UINT notifyCode, int id, CWindow handle)
{
	logEdit_.HideCaret();
}

void DetailsDialog::onLogDetailSelChange(UINT notifyCode, int id, CWindow handle)
{
	updateControls();
}

BW_END_NAMESPACE
