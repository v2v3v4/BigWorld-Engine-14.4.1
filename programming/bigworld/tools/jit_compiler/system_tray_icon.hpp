#ifndef SYSTEM_TRAY_ICON_HPP
#define SYSTEM_TRAY_ICON_HPP

#include "wtl.hpp"

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class SystemTrayIcon
{
private:
	enum { WM_TRAYICON = WM_APP + 55 };

public:
	enum BalloonType
	{
		BALLOON_NORMAL,
		BALLOON_WARNING,
		BALLOON_ERROR
	};

public:
	SystemTrayIcon();
	~SystemTrayIcon();

	bool init(CWindow & window);
	void fini();

	void setNormal();
	void setError();
	void setWarning();

	void showBalloonPopup(const BW::WStringRef & title, const BW::WStringRef & text, BalloonType type);

	BEGIN_MSG_MAP_EX(SystemTrayIcon)
		MESSAGE_HANDLER_EX(WM_TRAYICON, onTrayIcon)
	END_MSG_MAP()

private:
	void changeIcon(WTL::CIcon & icon);

	LRESULT onTrayIcon(UINT uMsg, WPARAM wParam, LPARAM lParam);

	CWindow ownerWindow_;

	NOTIFYICONDATA data_;
	WTL::CIcon iconNormal_;
	WTL::CIcon iconWarnings_;
	WTL::CIcon iconErrors_;
	WTL::CMenu iconMenu_;
	bool installed_;
};

BW_END_NAMESPACE

#endif // SYSTEM_TRAY_ICON_HPP
