#include "main_window.hpp"
#include "details_dialog.hpp"
#include "options_dialog.hpp"
#include "config_dialog.hpp"
#include "task_store.hpp"
#include "task_info.hpp"
#include "message_loop.hpp"
#include "about_dialog.hpp"

#include "resmgr/bwresource.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/bw_stack_container.hpp"
#include "cstdmf/debug.hpp"

namespace std
{
	using namespace placeholders;
}

BW_BEGIN_NAMESPACE

MainWindow::MainWindow( TaskStore & store, MainMessageLoop & loop, 
	AssetCompiler & compiler, const PluginLoader & loader)
	: store_(store)
	, messageLoop_(loop)
	, compiler_(compiler)
	, loader_(loader)
	, requestedList_(store, loop)
	, currentList_(store, loop)
	, completedList_(store, loop)
	, lastFailedCount_(0)
	, lastWarningCount_(0)
	, showBalloonNotifications_(true)
{
}

MainWindow::~MainWindow()
{
}


bool MainWindow::init()
{
	Create(NULL);
	if (m_hWnd == NULL || m_hWnd == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	CenterWindow();

	{
		BW::WStackString<1024> titleString;
		titleString->append(L"BigWorld JIT Compiler");
		titleString->append(L" - ");
		titleString->append(bw_utf8tow(BWUtil::executableDirectory()));
		SetWindowTextW(titleString->data());
	}

	while (::ShowCursor(TRUE) < 0);

	// Load icons and set window title icon
	iconNormal_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_COMPILER), 0, 0, LR_DEFAULTSIZE);
	MF_ASSERT(!iconNormal_.IsNull());

	WTL::CIcon smallIconNormal;
	smallIconNormal.LoadIconW(MAKEINTRESOURCE(IDI_JIT_COMPILER), 16, 16, 0);
	MF_ASSERT(!smallIconNormal.IsNull());

	iconWarnings_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_WARNING), 0, 0, LR_DEFAULTSIZE);
	MF_ASSERT(!iconWarnings_.IsNull());

	WTL::CIcon smallIconWarnings;
	smallIconWarnings.LoadIconW(MAKEINTRESOURCE(IDI_JIT_WARNING), 16, 16, 0);
	MF_ASSERT(!smallIconWarnings.IsNull());

	iconErrors_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_ERROR), 0, 0, LR_DEFAULTSIZE);
	MF_ASSERT(!iconErrors_.IsNull());

	WTL::CIcon smallIconErrors;
	smallIconErrors.LoadIconW(MAKEINTRESOURCE(IDI_JIT_ERROR), 16, 16, 0);
	MF_ASSERT(!smallIconErrors.IsNull());

	SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconNormal_.m_hIcon));

	requestedList_.SubclassWindow(GetDlgItem(IDC_REQUESTED_LIST));
	currentList_.SubclassWindow(GetDlgItem(IDC_CURRENT_LIST));
	completedList_.SubclassWindow(GetDlgItem(IDC_COMPLETED_LIST));
	completedList_.updateItemHeight(); // Fix for WM_MEASUREITEM message being lost
	statusText_.Attach(GetDlgItem(ID_STATUS_TEXT));

	sortOption_.Attach(GetDlgItem(IDC_COMPLETED_SORT_COMBO));
	sortOption_.AddString(L"Completed");
	sortOption_.AddString(L"File Name");
	sortOption_.AddString(L"Converter");
	sortOption_.AddString(L"Result");
	sortOption_.SetCurSel(0);

	normalTasksToggle_.Attach(GetDlgItem(ID_NORMAL_TASKS));
	normalTasksToggle_.SetIcon(smallIconNormal);
	normalTasksToggle_.SetCheck(BST_UNCHECKED); // Don't show normal by default
	completedList_.showNormalTasks(false);

	warningTasksToggle_.Attach(GetDlgItem(ID_WARNING_TASKS));
	warningTasksToggle_.SetIcon(smallIconWarnings);
	warningTasksToggle_.SetCheck(BST_CHECKED);
	completedList_.showWarningTasks(true);

	errorTasksToggle_.Attach(GetDlgItem(ID_ERROR_TASKS));
	errorTasksToggle_.SetIcon(smallIconErrors);
	errorTasksToggle_.SetCheck(BST_CHECKED);
	completedList_.showErrorTasks(true);

	tooltip_.Create(m_hWnd, 0, nullptr, TTS_ALWAYSTIP | TTS_NOPREFIX);
	MF_ASSERT(tooltip_.m_hWnd != NULL);

	WTL::CToolInfo toolinfo(TTF_SUBCLASS, normalTasksToggle_, 0, nullptr, L"Show/Hide successfully completed tasks.");
	tooltip_.AddTool(toolinfo);
	toolinfo.Init(TTF_SUBCLASS, warningTasksToggle_, 0, nullptr, L"Show/Hide completed tasks with warnings.");
	tooltip_.AddTool(toolinfo);
	toolinfo.Init(TTF_SUBCLASS, errorTasksToggle_, 0, nullptr, L"Show/Hide completed tasks with errors.");
	tooltip_.AddTool(toolinfo);

	tooltip_.Activate(TRUE);

	setStatusText(BW::wstring(L"Initialising..."));

	hideWindow();

	if (!trayIcon_.init(*this))
	{
		return false;
	}

	store_.setStatusCallbacks(
		std::bind(&MainWindow::setStatusText, this, BW::wstring(L"Scanning Disk...")),
		std::bind(&MainWindow::setStatusText, this, BW::wstring(L"Processing..."))
		);

	connections_.push_back(
		store_.registerRequestedTaskCallback(
			std::bind(&MainWindow::changeTaskList, this, std::ref(requestedList_), std::_1, std::_2)));
	connections_.push_back(
		store_.registerCurrentTaskCallback(
			std::bind(&MainWindow::changeTaskList, this, std::ref(currentList_), std::_1, std::_2)));
	connections_.push_back(
		store_.registerCompletedTaskCallback(
			std::bind(&MainWindow::changeTaskList, this, std::ref(completedList_), std::_1, std::_2)));

	return true;
}

void MainWindow::fini()
{
	connections_.clear();

	trayIcon_.fini();
}


void MainWindow::hideWindow()
{
	ShowWindow(SW_HIDE);
}

void MainWindow::showWindow()
{
	ShowWindow(SW_SHOWNORMAL);
	BringWindowToTop();
}

void MainWindow::showBalloonPopup()
{
	if (!showBalloonNotifications_)
	{
		return;

	}

	const int failedCount = store_.failedCount();
	const int warningCount = store_.warningCount();

	if (failedCount == lastFailedCount_ && warningCount == lastWarningCount_)
	{
		return;
	}

	SystemTrayIcon::BalloonType type;
	std::wstring titleString;
	if (failedCount > 0)
	{
		type = SystemTrayIcon::BALLOON_ERROR;
		titleString = L"Failures encountered converting assets";
	}
	else if (warningCount > 0)
	{
		type = SystemTrayIcon::BALLOON_WARNING;
		titleString = L"Warnings encountered converting assets";
	}
	else
	{
		return;
	}

	std::wstring textString;
	if (failedCount > 0)
	{
		textString.append(std::to_wstring(failedCount));
		if (failedCount > 1)
		{
			textString.append(L" tasks failed.");
		}
		else
		{
			textString.append(L" task failed.");
		}
		textString.append(L"\r\n");
	}
	if (warningCount > 0)
	{
		textString.append(std::to_wstring(warningCount));
		if (warningCount > 1)
		{
			textString.append(L" tasks succeeded with warnings.");
		}
		else
		{
			textString.append(L" task succeeded with warnings.");
		}
	}

	trayIcon_.showBalloonPopup(titleString, textString, type);

	lastFailedCount_ = failedCount;
	lastWarningCount_ = warningCount;
}

void MainWindow::updateStatus()
{
	store_.updateErrorWarningCounts();
	if (store_.failuresExist())
	{
		trayIcon_.setError();
		SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconErrors_.m_hIcon));
		showBalloonPopup();
	}
	else if (store_.warningsExist())
	{
		trayIcon_.setWarning();
		SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconWarnings_.m_hIcon));
		showBalloonPopup();
	}
	else
	{
		trayIcon_.setNormal();
		SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(iconNormal_.m_hIcon));
	}
}


int MainWindow::onInitDialog(ATL::CWindow handle, LPARAM data)
{
	DlgResize_Init();
	return FALSE;
}

void MainWindow::onClose()
{
	hideWindow();
}

void MainWindow::onDestroy()
{
	PostQuitMessage(0);
}

void MainWindow::onButtonClicked(UINT notifyCode, int id, CWindow handle)
{
	if (id == ID_NORMAL_TASKS)
	{
		const bool checked = normalTasksToggle_.GetCheck() == BST_CHECKED;
		completedList_.showNormalTasks(checked);
	}
	else if (id == ID_WARNING_TASKS)
	{
		const bool checked = warningTasksToggle_.GetCheck() == BST_CHECKED;
		completedList_.showWarningTasks(checked);
	}
	else if (id == ID_ERROR_TASKS)
	{
		const bool checked = errorTasksToggle_.GetCheck() == BST_CHECKED;
		completedList_.showErrorTasks(checked);
	}
	SetMsgHandled(FALSE);
}

void MainWindow::onMenuClose(UINT notifyCode, int id, CWindow handle)
{
	hideWindow();
}

void MainWindow::onShowMainWindow(UINT notifyCode, int id, CWindow handle)
{
	showWindow();
}

void MainWindow::onMenuOptions(UINT notifyCode, int id, CWindow handle)
{
	JitCompilerOptions options(compiler_);

	// Gather options values
	options.enableBalloonNotifications(showBalloonNotifications_);

	OptionsDialog dialog(options);
	if (!dialog.run())
	{
		return;
	}

	// Update options values
	options.apply(compiler_);
	showBalloonNotifications_ = options.enableBalloonNotifications();
}

void MainWindow::onMenuQuit(UINT notifyCode, int id, CWindow handle)
{
	DestroyWindow();
}

void MainWindow::onConfig(UINT notifyCode, int id, CWindow handle)
{
	ConfigDialog config( compiler_, loader_ );
	config.run();
}

void MainWindow::onAbout(UINT notifyCode, int id, CWindow handle)
{
	AboutDialog about;
	about.DoModal();
}

void MainWindow::onSortSelChange(UINT notifyCode, int id, CWindow handle)
{
	const int current = sortOption_.GetCurSel();
	if (current == 0)
	{
		completedList_.setSortMode(LargeTaskListBox::SORT_NONE);
	}
	else if (current == 1)
	{
		completedList_.setSortMode(LargeTaskListBox::SORT_FILENAME);
	}
	else if (current == 2)
	{
		completedList_.setSortMode(LargeTaskListBox::SORT_FILETYPE);
	}
	else if (current == 3)
	{
		completedList_.setSortMode(LargeTaskListBox::SORT_RESULT);
	}
}

void MainWindow::setStatusText(BW::wstring text)
{
	messageLoop_.addAction([this, text]() 
	{
		statusText_.SetWindowTextW(text.c_str());
	});
}

void MainWindow::changeTaskList(TaskListBoxBase & list, TaskInfoPtr task, TaskStoreAction action)
{
	if (action == TaskStoreAction::ADDED)
	{
		messageLoop_.addAction( [&list, task]() { list.addTask(task); } );
	}
	else if (action == TaskStoreAction::REMOVED)
	{
		messageLoop_.addAction( [&list, task]() { list.removeTask(task); } );

		if (&list == &currentList_)
		{
			messageLoop_.addAction(std::bind(&MainWindow::updateStatus, this));
		}
	}
}


BW_END_NAMESPACE
