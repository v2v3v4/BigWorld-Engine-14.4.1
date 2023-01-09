#include "pch.hpp"
#include "critical_handler.hpp"


#include "romp/progress.hpp"


#include "debug_app.hpp"
#include "device_app.hpp"


BW_BEGIN_NAMESPACE

void CriticalHandler::handleCritical( const char * msg )
{
	if (!DebugApp::instance.shouldBreakOnCritical_) return;

	bool runningFullScreen = !Moo::rc().windowed();
	if (Moo::rc().device() && runningFullScreen)
	{
		Moo::rc().changeMode( Moo::rc().modeIndex(), true );
	}
	ShowCursor( TRUE );

	char buffer[2*BUFSIZ];

	#if ENABLE_ENTER_DEBUGGER_MESSAGE
	bw_snprintf( buffer, sizeof(buffer),
		"%s\nEnter debugger?\nCancel will exit the program.\n", msg );
	#endif

	if (DeviceApp::s_pProgress_)
	{
		BW::string errorMsg = "Critical Error: ";
		DeviceApp::s_pProgress_->add(errorMsg + buffer);
	}

	#if ENABLE_ENTER_DEBUGGER_MESSAGE
	wchar_t wbuffer[ 2*BUFSIZ ];
	MultiByteToWideChar( CP_ACP, 0, buffer, 2*BUFSIZ, wbuffer, 2*BUFSIZ );
	switch (::MessageBox( NULL, wbuffer, L"Critical Error", MB_YESNOCANCEL ))
	{
		case IDYES:		ENTER_DEBUGGER();		break;
		case IDNO:								break;
		case IDCANCEL:	exit( EXIT_FAILURE );	break;
	}
	#endif

	ShowCursor( FALSE );
	if (Moo::rc().device() && runningFullScreen)
	{
		Moo::rc().changeMode( Moo::rc().modeIndex(), false );
	}
}

BW_END_NAMESPACE

// critical_handler.cpp
