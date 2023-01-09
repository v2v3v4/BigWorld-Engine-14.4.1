#include "options_dialog.hpp"

// For AssetCompiler::sanitisePathParam
#include "asset_pipeline/compiler/asset_compiler.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "cstdmf/string_utils.hpp"
#include "cstdmf/debug.hpp"

#include <memory>

namespace std
{
	using namespace placeholders;
}

BW_BEGIN_NAMESPACE

OptionsDialog::OptionsDialog(JitCompilerOptions & options)
	: options_(options)
{
}

OptionsDialog::~OptionsDialog()
{
}

int OptionsDialog::onInitDialog(ATL::CWindow handle, LPARAM data)
{
	balloonCheckbox_.Attach(GetDlgItem(IDC_NOTIFICATIONS));
	threadCountSlider_.Attach(GetDlgItem(IDC_THREADS_SLIDER));
	threadCountLabel_.Attach(GetDlgItem(IDC_THREADS_COUNT_LABEL));
	cachePathTextbox_.Attach(GetDlgItem(IDC_CACHE_PATH));
	outputPathTextbox_.Attach(GetDlgItem(IDC_OUTPUT_PATH));
	interPathTextbox_.Attach(GetDlgItem(IDC_INTER_PATH));
	cacheReadCheckbox_.Attach(GetDlgItem(IDC_CACHE_READ_ENABLE));
	cacheWriteCheckbox_.Attach(GetDlgItem(IDC_CACHE_WRITE_ENABLE));
	forceRebuildCheckbox_.Attach(GetDlgItem(IDC_FORCE_REBUILD));

	// Write options to the dialog
	balloonCheckbox_.SetCheck(options_.enableBalloonNotifications() ? BST_CHECKED : BST_UNCHECKED);
	cacheReadCheckbox_.SetCheck(options_.enableCacheRead() ? BST_CHECKED : BST_UNCHECKED);
	cacheWriteCheckbox_.SetCheck(options_.enableCacheWrite() ? BST_CHECKED : BST_UNCHECKED);
	forceRebuildCheckbox_.SetCheck(options_.forceRebuild() ? BST_CHECKED : BST_UNCHECKED);

	// For now just hard code the range
	// Need to check how many is a good maximum, 
	// though >= 64 has been found to jam up the system
	threadCountSlider_.SetRange(1, 32);
	const int currentThreads = options_.threadCount();
	threadCountSlider_.SetPos(currentThreads);
	updateThreadsLabel(currentThreads);

	setPathSetting( cachePathTextbox_, options_.cachePath() );
	setPathSetting( interPathTextbox_, options_.intermediatePath() );
	setPathSetting( outputPathTextbox_, options_.outputPath() );

	return FALSE;
}

void OptionsDialog::onClose()
{
	EndDialog(IDCANCEL);
}

void OptionsDialog::onOk(UINT notifyCode, int controlID, CWindow handle)
{
	const bool balloonNotifications = balloonCheckbox_.GetCheck() == BST_CHECKED;
	options_.enableBalloonNotifications(balloonNotifications);

	const bool enableCacheRead = cacheReadCheckbox_.GetCheck() == BST_CHECKED;
	options_.enableCacheRead(enableCacheRead);

	const bool enableCacheWrite = cacheWriteCheckbox_.GetCheck() == BST_CHECKED;
	options_.enableCacheWrite(enableCacheWrite);

	const bool rebuild = forceRebuildCheckbox_.GetCheck() == BST_CHECKED;
	options_.forceRebuild(rebuild);

	const int threadCount = threadCountSlider_.GetPos();
	options_.threadCount(threadCount);

	options_.cachePath( getPathSetting( cachePathTextbox_ ) );
	options_.intermediatePath( getPathSetting( interPathTextbox_ ) );
	options_.outputPath( getPathSetting( outputPathTextbox_ ) );

	EndDialog(IDOK);
}

void OptionsDialog::onCancel(UINT notifyCode, int controlID, CWindow handle)
{
	EndDialog(IDCANCEL);
}

LRESULT OptionsDialog::onTrackbar(UINT message, WPARAM wParam, LPARAM lParam)
{
	int position = 0;
	const DWORD thumbMessage = LOWORD(wParam);
	if (thumbMessage == TB_THUMBPOSITION || thumbMessage == TB_THUMBTRACK)
	{
		position = HIWORD(wParam);
	}
	else
	{
		position = threadCountSlider_.GetPos();
	}

	updateThreadsLabel(position);

	return 0;
}

void OptionsDialog::updateThreadsLabel(const int threadCount)
{
	threadCountLabel_.SetWindowTextW(std::to_wstring(threadCount).c_str());
}

void OptionsDialog::setPathSetting(WTL::CEdit & control, const BW::string & path)
{
	BW::wstring wpath = bw_utf8tow( path );
	std::replace( wpath.begin(), wpath.end(), L'/', L'\\' );
	control.SetWindowTextW( wpath.c_str() );
}

BW::string OptionsDialog::getPathSetting(const WTL::CEdit & control) const
{
	wchar_t buffer[BW_MAX_PATH + 1];
	control.GetWindowTextW(buffer, BW_MAX_PATH);

	const int length = control.GetWindowTextLengthW();
	buffer[length] = L'\0';

	BW::string path;
	if (!bw_wtoutf8(buffer, path))
	{
		WARNING_MSG("Could not convert path: %S\n.", buffer);
	}

	return path;
}

bool OptionsDialog::run()
{
	const INT_PTR result = DoModal();
	return result == IDOK;
}


BW_END_NAMESPACE
