#include "pch.hpp"
#include "gui_action_maker.hpp"
#include "gui_textor_maker.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

struct OpenFileAction : public ActionMaker<OpenFileAction>
{
	OpenFileAction() : ActionMaker<OpenFileAction>( "OpenFile", 
		&OpenFileAction::openFile )
	{}
	bool openFile( ItemPtr item )
	{
		BW_GUARD;

		MessageBox( NULL, L"Open File", L"Open File", MB_OK );
		return true;
	}
}
OpenFileActionInstance;

struct TimeTextor : public TextorMaker<TimeTextor>
{
	TimeTextor() : TextorMaker<TimeTextor>( "time", &TimeTextor::time )
	{}
	BW::string time( ItemPtr item )
	{
		BW_GUARD;

		static int i = 0;
		char s[1024];
		bw_snprintf( s, ARRAY_SIZE(s), "%d", i++ );
		return s;
	}
}
TimeTextorInstance;

END_GUI_NAMESPACE
BW_END_NAMESPACE

