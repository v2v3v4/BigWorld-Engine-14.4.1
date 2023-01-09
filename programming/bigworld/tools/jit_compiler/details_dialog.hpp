#ifndef DETAILS_DIALOG_HPP
#define DETAILS_DIALOG_HPP

#include "wtl.hpp"
#include "signal.hpp"
#include "task_fwd.hpp"
#include "task_list_box.hpp"

#include "asset_pipeline/conversion/conversion_task.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class MainMessageLoop;

class DetailsDialog : 
	public ATL::CDialogImpl<DetailsDialog>,
	public WTL::CDialogResize<DetailsDialog>
{
public:
	enum { IDD = IDR_DETAILS_DIALOG };

public:

	static void createDialog(CWindow parentWindow, 
		TaskInfoPtr task, MainMessageLoop & messageLoop, TaskStore & store);

	BEGIN_MSG_MAP_EX(DetailsDialog)
		MSG_WM_INITDIALOG(onInitDialog)
		MSG_WM_CLOSE(onClose)
		COMMAND_ID_HANDLER_EX(ID_FILE_CLOSE, onMenuClose)
		COMMAND_HANDLER_EX(IDC_DETAILS_LOG_EDIT, EN_SETFOCUS, onLogSetFocus)
		COMMAND_HANDLER_EX(IDC_LOG_DETAIL, CBN_SELCHANGE, onLogDetailSelChange)
		CHAIN_MSG_MAP(WTL::CDialogResize<DetailsDialog>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(DetailsDialog)
		DLGRESIZE_CONTROL(ID_STATUS_TEXT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_LOG_LABEL, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_DETAILS_LOG_EDIT, DLSZ_SIZE_X|DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_OUTPUTS_LABEL, DLSZ_SIZE_X|DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_OUTPUTS_LIST, DLSZ_SIZE_X|DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DEPENDS_LABEL, DLSZ_SIZE_X|DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_DEPENDS_LIST, DLSZ_SIZE_X|DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(ID_FILE_CLOSE, DLSZ_MOVE_X|DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_LOG_DETAIL, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

private:
	DetailsDialog(TaskInfoPtr info, MainMessageLoop & messageLoop, TaskStore & store);
	~DetailsDialog();


	void updateControls();
	void onTaskInfoChanged(TaskInfoPtr taskInfo);

	// Message Map Functions
	int onInitDialog(ATL::CWindow handle, LPARAM data);
	void onClose();
	void onMenuClose(UINT notifyCode, int id, CWindow handle);
	void onLogSetFocus(UINT notifyCode, int id, CWindow handle);
	void onLogDetailSelChange(UINT notifyCode, int id, CWindow handle);

	virtual void OnFinalMessage(HWND handle);

private:

	TaskStore & store_;
	MainMessageLoop & messageLoop_;

	TaskInfoPtr info_;

	Connection updateConnection_;

	WTL::CListBox outputsList_;
	TaskListBox dependsList_;
	WTL::CEdit logEdit_;
	WTL::CStatic statusText_;
	WTL::CComboBox logDetailCombo_;

};

BW_END_NAMESPACE

#endif // DETAILS_DIALOG_HPP
