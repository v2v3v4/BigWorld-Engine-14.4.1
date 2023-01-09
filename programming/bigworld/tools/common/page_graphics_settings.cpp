// page_graphics_settings.cpp : implementation file
//

#include "pch.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/string_provider.hpp"
#include "page_graphics_settings.hpp"
#include "user_messages.hpp"
#include "utilities.hpp"

#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::string PageGraphicsSettings::contentID = "PageGraphicsSettings";


typedef SmartPointer<Moo::GraphicsSetting> GraphicsSettingPtr;


/**
 *	Constructor
 */
PageGraphicsSettings::PageGraphicsSettings()
	: GraphicsSettingsTable(PageGraphicsSettings::IDD)
{
}


/**
 *	Destructor
 */
PageGraphicsSettings::~PageGraphicsSettings()
{
}


/**
 *	Helper method let know the page that settings that take effect after the 
 *	application restarts. Must call the needsRestart method in the base class.
 *
 *  @param		graphics setting string (label or option)
 */
void PageGraphicsSettings::needsRestart( const BW::string& setting )
{
	GraphicsSettingsTable::needsRestart( setting );
	onSizeInternal();
	messageText_.SetWindowText(
		(BW::string( "* " ) + L("COMMON/PAGE_GRAPHICS_SETTINGS/NEED_RESTART")).c_str() );
	messageText_.ShowWindow( SW_SHOW );
	RedrawWindow();
}


/**
 *	DoDataExchange
 */
void PageGraphicsSettings::DoDataExchange(CDataExchange* pDX)
{
	GraphicsSettingsTable::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRAPHICS_SETTINGS_MSG, messageText_);
}


/**
 *	MFC Message Map
 */
BEGIN_MESSAGE_MAP(PageGraphicsSettings, GraphicsSettingsTable)
	ON_WM_SIZE()
END_MESSAGE_MAP()


/**
 *	The page has been resized.
 */
void PageGraphicsSettings::OnSize( UINT nType, int cx, int cy )
{
	GraphicsSettingsTable::OnSize( nType, cx, cy );
	onSizeInternal();
	RedrawWindow();
}


/**
 *	Helper method to layout the page's controls according to the page's size.
 */
void PageGraphicsSettings::onSizeInternal()
{
	// resize to correspond with the size of the wnd
	CRect rectPage;
	GetClientRect(rectPage);

	CRect msgRect;
	messageText_.GetWindowRect( msgRect );

	int yOffset = 5;
	if ( GraphicsSettingsTable::needsRestart() )
		yOffset += msgRect.Height();

	Utilities::stretchToBottomRight(
		this, pImpl_->propertyList,
		rectPage.Width(), 5,
		rectPage.Height(), yOffset );

	Utilities::moveToBottom( this, messageText_, rectPage.Height(), 5 );
	Utilities::stretchToRight( this, messageText_, rectPage.Width(), 5 );
}


