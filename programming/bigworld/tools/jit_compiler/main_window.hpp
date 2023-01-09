#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "wtl.hpp"
#include "signal.hpp"
#include "task_list_box.hpp"
#include "large_task_list_box.hpp"
#include "task_list_box_base.hpp"
#include "system_tray_icon.hpp"
#include "details_dialog.hpp"

#include "task_fwd.hpp"

#include "asset_pipeline/conversion/conversion_task.hpp"

#include "cstdmf/bw_namespace.hpp"

#include <vector>
#include <memory>

BW_BEGIN_NAMESPACE

class TaskStore;
class MainMessageLoop;
class AssetCompiler;
class PluginLoader;

class MainWindow : 
	public ATL::CDialogImpl<MainWindow>,
	public WTL::CDialogResize<MainWindow>
{
public:
	enum { IDD = IDR_MAIN_DIALOG };

	MainWindow(TaskStore & store, MainMessageLoop & loop, 
		AssetCompiler & compiler, const PluginLoader & loader);
	~MainWindow();

	bool init();
	void fini();

	BEGIN_MSG_MAP_EX(MainWindow)
		MSG_WM_INITDIALOG(onInitDialog)
		MSG_WM_CLOSE(onClose)
		MSG_WM_DESTROY(onDestroy)
		COMMAND_CODE_HANDLER_EX(BN_CLICKED, onButtonClicked)
		COMMAND_ID_HANDLER_EX(ID_FILE_CLOSE, onMenuClose)
		COMMAND_ID_HANDLER_EX(ID_SHOW_MAIN_WINDOW, onShowMainWindow)
		COMMAND_ID_HANDLER_EX(ID_FILE_OPTIONS, onMenuOptions)
		COMMAND_ID_HANDLER_EX(ID_FILE_QUIT, onMenuQuit)
		COMMAND_ID_HANDLER_EX(ID_HELP_CONFIG, onConfig)
		COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, onAbout)
		COMMAND_HANDLER_EX(IDC_COMPLETED_SORT_COMBO, CBN_SELCHANGE, onSortSelChange)
		CHAIN_MSG_MAP_MEMBER(trayIcon_)
		CHAIN_MSG_MAP(WTL::CDialogResize<MainWindow>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(MainWindow)
		DLGRESIZE_CONTROL(ID_STATUS_TEXT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_REQUESTED_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_CURRENT_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(ID_NORMAL_TASKS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(ID_WARNING_TASKS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(ID_ERROR_TASKS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_COMPLETED_SORT_LABEL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_COMPLETED_SORT_COMBO, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_COMPLETED_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

private:
	void hideWindow();
	void showWindow();

	void showBalloonPopup();
	void updateStatus();

	// Message Map Functions
	int onInitDialog(ATL::CWindow handle, LPARAM data);
	void onClose();
	void onDestroy();
	void onButtonClicked(UINT notifyCode, int id, CWindow handle);
	void onMenuClose(UINT notifyCode, int id, CWindow handle);
	void onShowMainWindow(UINT notifyCode, int id, CWindow handle);
	void onMenuOptions(UINT notifyCode, int id, CWindow handle);
	void onMenuQuit(UINT notifyCode, int id, CWindow handle);
	void onConfig(UINT notifyCode, int id, CWindow handle);
	void onAbout(UINT notifyCode, int id, CWindow handle);
	void onSortSelChange(UINT notifyCode, int id, CWindow handle);

	// TaskStore Callbacks
	void setStatusText(BW::wstring text);

	void changeTaskList(TaskListBoxBase & list, TaskInfoPtr task, TaskStoreAction action);

private:
	TaskStore & store_;
	MainMessageLoop & messageLoop_;
	AssetCompiler & compiler_;
	const PluginLoader & loader_;

	std::vector<Connection> connections_;

	TaskListBox requestedList_;
	TaskListBox currentList_;
	LargeTaskListBox completedList_;
	WTL::CStatic statusText_;
	WTL::CButton normalTasksToggle_;
	WTL::CButton warningTasksToggle_;
	WTL::CButton errorTasksToggle_;
	WTL::CComboBox sortOption_;

	WTL::CToolTipCtrl tooltip_;

	WTL::CIcon iconNormal_;
	WTL::CIcon iconWarnings_;
	WTL::CIcon iconErrors_;

	SystemTrayIcon trayIcon_;
	int lastFailedCount_;
	int lastWarningCount_;
	bool showBalloonNotifications_;
};

BW_END_NAMESPACE

#endif // MAIN_WINDOW_HPP
