//
//	Sample barebones panel for integrating into a BigWorld tool.
//	This panel can also be easily put into ModelEditor or ParticleEditor.
//
//	IMPORTANT:
//	For this panel to work you will need a few things:
//
//	- Create a panel with id IDD_MY_PANEL in VisualStudio's resource editor.
//	An example panel would be like this in the .rc file:
//		IDD_MY_PANEL DIALOGEX 0, 0, 186, 145
//					STYLE DS_SETFONT | DS_FIXEDSYS | WS_POPUP | WS_SYSMENU
//					FONT 8, "MS Shell Dlg", 400, 0, 0x1
//		BEGIN
//		    PUSHBUTTON      "Test",IDC_BUTTONTEST,69,57,50,14
//		END
//
//	- And of course, you will need to have IDD_MY_PANEL and IDC_BUTTONTEST
//	defined in the app's resources.h.
//
//	Other things you need to do to make the panel show up in the tool:
//	- Register the factory in PanelManager::initPanels()
//	- Insert the panel into the default layout in
//	PanelManager::loadDefaultPanels().
//	- Also add menu items for showing it (check how the other panels do).
//


#ifndef PAGE_MY_PANEL_HPP
#define PAGE_MY_PANEL_HPP


#include "resource.h"
#include "controls/auto_tooltip.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>
#include <afxcmn.h>

//-----------------------------------------------------------------------------
//	Dummy definitions to make it compile.  Remove these when you create your
//	actual dialog resources in Visual Studio resources editor.
#ifndef IDD_MY_PANEL
#define IDD_MY_PANEL	0
#define IDC_BUTTONTEST	0
#endif
//-----------------------------------------------------------------------------

BW_BEGIN_NAMESPACE

/**
 *	This class implements a panel in a BigWorld tool, using BigWorld's panel
 *	system.
 */
class PageMyPanel : public CDialog, public GUITABS::Content
{
	// There are some macros that help in defining the behaviour of your panel
	// under some panel-system-related circumstances.
	// See the other macros for more options (src\lib\guitabs\content.hpp).
	IMPLEMENT_BASIC_CONTENT( L"MyPanel", L"My Blank Panel", 290, 250, NULL )

public:
	PageMyPanel();
	virtual ~PageMyPanel();

// Dialog Data
	enum { IDD = IDD_MY_PANEL };

protected:
	// This is optiona, BigWorld's automatic tooltips, which use the IDs of 
	// controls in the dialog and gets the string matching that ID from the
	// resource string table to display it as the tooltip for the control.
	DECLARE_AUTO_TOOLTIP( PageMyPanel, CDialog );

	// standard MFC stuff
	virtual void DoDataExchange( CDataExchange * pDX );    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

private:
	// Some methods, again, standard MFC stuff
	virtual BOOL OnInitDialog();

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg LRESULT OnUpdateControls( WPARAM wParam, LPARAM lParam );

	// This sample dialog has a button, and it's handled as you would in a
	// normal MFC dialog.
	CButton buttonTest;
	afx_msg void OnBnClickedTest();
};


// GUITABS needs a factory for your dialog so it can create an instance when
// loading from disk.  Fortunatelly, we have some straightforward macros for
// this.
IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageMyPanel, PageMyPanel::IDD )

BW_END_NAMESPACE

#endif // PAGE_MY_PANEL_HPP
