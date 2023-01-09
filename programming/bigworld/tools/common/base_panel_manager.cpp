#include "pch.hpp"
#include "base_panel_manager.hpp"

#include <afxdisp.h>

#include "appmgr/commentary.hpp"

#include "cstdmf/bw_stack_container.hpp"
#include "cstdmf/bwversion.hpp"

#include "resmgr/string_provider.hpp"
#include "editor_shared/cursor/wait_cursor.hpp"

#include "guimanager/gui_action_maker.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	bool openHelpFile( const char * name, const wchar_t * file )
	{
		WaitCursor wait;

		WStackString< 1024 > helpFile;
		LocaliseUTF8( helpFile.container(), file, BWVersion::majorNumber(), BWVersion::minorNumber() );

		// If the function succeeds, it returns a value greater than 32. If the
		// function fails, it returns an error value that indicates the cause
		// of the failure. The return value is cast as an HINSTANCE for
		// backward compatibility with 16-bit Windows applications. 
		intptr result = (intptr)ShellExecute(
			AfxGetMainWnd()->GetSafeHwnd(), L"open",
			helpFile->c_str() , NULL, NULL, SW_SHOWMAXIMIZED );
		if ( result < 32 )
		{
			Commentary::instance().addMsg(
				Localise(L"HELP/HELP_NOT_FOUND", helpFile->c_str(), name ), Commentary::WARNING );
		}

		return result >= 32;
	}
}


class PanelManagerFunctor
	: public GUI::ActionMaker<PanelManagerFunctor>
	, public GUI::ActionMaker<PanelManagerFunctor, 1>
{
public:
	PanelManagerFunctor();

private:
	bool OnToolsReferenceGuide( GUI::ItemPtr item );
	bool OnContentCreation( GUI::ItemPtr item );
};


//==============================================================================
PanelManagerFunctor::PanelManagerFunctor()
	: GUI::ActionMaker<PanelManagerFunctor>(
		"doToolsReferenceGuide", &PanelManagerFunctor::OnToolsReferenceGuide )
	, GUI::ActionMaker<PanelManagerFunctor, 1>(
		"doContentCreation", &PanelManagerFunctor::OnContentCreation )
{}


//==============================================================================
bool PanelManagerFunctor::OnToolsReferenceGuide( GUI::ItemPtr item )
{
	return openHelpFile(
		"toolsReferenceGuide", L"HELP/CONTENT_TOOLS_GUIDE_PATH" );
}


//==============================================================================
bool PanelManagerFunctor::OnContentCreation( GUI::ItemPtr item )
{
	return openHelpFile(
		"contentCreationManual" , L"HELP/CONTENT_CREATION_GUIDE_PATH" );
}


//==============================================================================
BasePanelManager::BasePanelManager()
	: pFunctor_( new PanelManagerFunctor() )
{

}


//==============================================================================
BasePanelManager::~BasePanelManager()
{
	delete pFunctor_;
}

BW_END_NAMESPACE
