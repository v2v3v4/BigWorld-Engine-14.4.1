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


#include "pch.hpp"
#include "page_my_panel.hpp"
#include "common/user_messages.hpp"

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageMyPanel::contentID = L"PageMyPanel";


/**
 *  Constructor, doing standard MFC stuff at the moment
 */
PageMyPanel::PageMyPanel() :
	CDialog( PageMyPanel::IDD )
{
}


/**
 *  Destructor, does nothing so far.
 */
PageMyPanel::~PageMyPanel()
{
}


/**
 *	Standard MFC data exchange method, where you can hook up member variables
 *	to actuall controls in the dialog, among other things.
 */
void PageMyPanel::DoDataExchange( CDataExchange * pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_BUTTONTEST, buttonTest ); // sample button
}


/**
 *	In this MFC override of the OnInitDialog method we initialise BigWorld's
 *	tooltips.
 */
BOOL PageMyPanel::OnInitDialog()
{
	CDialog::OnInitDialog();
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


//
//	Standard MFC message map, with a handler to BigWorld's WM_UPDATE_CONTROLS
//	custom message that gets sent every frame.
//
BEGIN_MESSAGE_MAP( PageMyPanel, CDialog )
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	ON_MESSAGE( WM_UPDATE_CONTROLS, OnUpdateControls )
	ON_BN_CLICKED( IDC_BUTTONTEST, OnBnClickedTest )
END_MESSAGE_MAP()


/**
 *	Standard WM_SIZE handler.
 */
afx_msg void PageMyPanel::OnSize( UINT nType, int cx, int cy )
{
	CDialog::OnSize( nType, cx, cy );
	// Here you could reposition your controls, redraw, etc.
}


/**
 *	BigWorld's WM_UPDATE_CONTROLS handler.
 */
afx_msg LRESULT PageMyPanel::OnUpdateControls( WPARAM wParam, LPARAM lParam )
{
	// Do GUI related stuff you want to do per frame.  For example, you could
	// have a FPS counteror a # of triangles counter that needs to be updated
	// frequently.
	return 0;
}


/**
 *	Standard button click handler, as a test.
 */
void PageMyPanel::OnBnClickedTest()
{
	MessageBox( L"This is a test!!!", L"Test Button in MyPanel", 0 );
}

BW_END_NAMESPACE

