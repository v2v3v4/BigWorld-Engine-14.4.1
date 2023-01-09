#include "system_tray_icon.hpp"

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

SystemTrayIcon::SystemTrayIcon() : 
	data_(),
	installed_(false)
{
}

SystemTrayIcon::~SystemTrayIcon()
{
}

bool SystemTrayIcon::init(CWindow & window)
{
	ownerWindow_ = window;

	const int iconSizeX = ::GetSystemMetrics(SM_CXSMICON);
	const int iconSizeY = ::GetSystemMetrics(SM_CYSMICON);

	iconNormal_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_COMPILER), iconSizeX, iconSizeY, 0);
	if (iconNormal_.IsNull())
	{
		return false;
	}

	iconMenu_.LoadMenuW(MAKEINTRESOURCE(IDR_SYSTEM_TRAY_MENU));
	if (iconMenu_.IsNull())
	{
		return false;
	}

	::ZeroMemory(&data_, sizeof(NOTIFYICONDATA));
	data_.cbSize = sizeof(NOTIFYICONDATA);
	data_.hWnd = ownerWindow_;
	data_.uID = 1; // Only having one tray icon for now
	data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	data_.uCallbackMessage = WM_TRAYICON;
	data_.hIcon = iconNormal_;
	wcscpy_s(data_.szTip, L"BigWorld JIT Asset Compiler");
	data_.uVersion = NOTIFYICON_VERSION;
	
	installed_ = Shell_NotifyIcon(NIM_ADD, &data_) != FALSE;
	if (installed_)
	{
		if (Shell_NotifyIcon(NIM_SETVERSION, &data_) == FALSE)
		{
			fini();
		}
	}

	iconWarnings_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_WARNING), iconSizeX, iconSizeY, 0);
	if (iconWarnings_.IsNull())
	{
		ERROR_MSG("Could not load the Warning Icon from the resource.\n");
		// Fall back to using the normal icon
		iconWarnings_ = iconNormal_.CopyIcon();
	}

	iconErrors_.LoadIconW(MAKEINTRESOURCE(IDI_JIT_ERROR), iconSizeX, iconSizeY, 0);
	if (iconErrors_.IsNull())
	{
		ERROR_MSG("Could not load the Error Icon from the resource.\n");
		// Fall back to using the normal icon
		iconErrors_ = iconNormal_.CopyIcon();
	}

	return installed_;
}

void SystemTrayIcon::fini()
{
	if (installed_)
	{
		Shell_NotifyIcon(NIM_DELETE, &data_);
	}
}


void SystemTrayIcon::changeIcon(WTL::CIcon & icon)
{
	if (installed_ && data_.hIcon != icon)
	{
		data_.hIcon = icon;
		Shell_NotifyIcon(NIM_MODIFY, &data_);
	}
}

void SystemTrayIcon::setNormal()
{
	changeIcon(iconNormal_);
}

void SystemTrayIcon::setError()
{
	changeIcon(iconErrors_);
}

void SystemTrayIcon::setWarning()
{
	changeIcon(iconWarnings_);
}


void SystemTrayIcon::showBalloonPopup(const BW::WStringRef & title, const BW::WStringRef & text, BalloonType type)
{
	NOTIFYICONDATA data;
	::ZeroMemory(&data, sizeof(NOTIFYICONDATA));
	data.cbSize = sizeof(NOTIFYICONDATA);
	data.hWnd = ownerWindow_;
	data.uID = data_.uID;
	data.uVersion = NOTIFYICON_VERSION;
	data.uFlags = NIF_INFO;

	switch (type)
	{
	case BALLOON_NORMAL:	data.dwInfoFlags = NIIF_NONE;		break;
	case BALLOON_WARNING:	data.dwInfoFlags = NIIF_WARNING;	break;
	case BALLOON_ERROR:		data.dwInfoFlags = NIIF_ERROR;		break;
	};

	wcsncpy_s(data.szInfoTitle, title.data(), title.size());
	wcsncpy_s(data.szInfo, text.data(), text.size());

	::Shell_NotifyIcon(NIM_MODIFY, &data);
}


LRESULT SystemTrayIcon::onTrayIcon(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_CONTEXTMENU:
		{
			WTL::CMenuHandle popupMenu = iconMenu_.GetSubMenu(0);
			::CPoint popupPoint;
			GetCursorPos(&popupPoint);
			popupMenu.TrackPopupMenu(0, popupPoint.x, popupPoint.y, ownerWindow_);
		}
		break;

	case NIN_BALLOONUSERCLICK:
	case WM_LBUTTONUP:
		ownerWindow_.ShowWindow(SW_SHOWNORMAL);
		SetForegroundWindow(ownerWindow_);
		break;
	};

	return 0;
}

BW_END_NAMESPACE
