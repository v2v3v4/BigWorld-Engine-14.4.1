
#include "pch.hpp"
#include "guitabs.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	Constructor.
 *
 *	@param parentWnd	Parent window for this tab.
 *	@param contentID	Content ID of the Tab's content.
 */
Tab::Tab( CWnd* parentWnd, BW::wstring contentID ) :
	isVisible_( false )
{
	BW_GUARD;

	content_ = Manager::instance().createContent( contentID );

	construct( parentWnd );
}


/**
 *	Constructor.
 *
 *	@param parentWnd	Parent window for this tab.
 *	@param contentID	Content to be contained in this tab.
 */
Tab::Tab( CWnd* parentWnd, ContentPtr content ) :
	isVisible_( false ),
	content_( content )
{
	BW_GUARD;

	construct( parentWnd );
}


/**
 *	This method creates and configures the MFC window for this tab.
 *
 *	@param parentWnd	Parent window for this tab.
 *	@param contentID	Content to be contained in this tab.
 */
void Tab::construct( CWnd* parentWnd )
{
	BW_GUARD;

	if ( content_ )
	{
		CWnd* wnd = content_->getCWnd();

		if ( !wnd )
			ASSERT( 0 );

		if ( !IsWindow( wnd->GetSafeHwnd() ) )
		{
			wnd->Create(
				AfxRegisterWndClass( CS_OWNDC, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetSysColorBrush(COLOR_BTNFACE) ),
				L"GUITABS-Created-CWnd",
				WS_CHILD,
				CRect( 0, 0, 300, 400 ),
				parentWnd,
				0,
				0);
			if ( !IsWindow( wnd->GetSafeHwnd() ) )
				ASSERT( 0 );
		}
		else
		{
			wnd->SetParent( parentWnd );
		}

		wnd->UpdateData( FALSE );
	}
}


/**
 *	Destructor.
 */
Tab::~Tab()
{
	BW_GUARD;

	if ( content_ && content_->getCWnd() )
		content_->getCWnd()->DestroyWindow();
}


/**
 *	This method simply forwards the call to its Content object.
 *
 *	@param section	Data section with the content's info.
 *	@return		True if successful, false if not.
 */
bool Tab::load( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr contentSec = section->openSection( "ContentData" );
	if ( !contentSec )
		return false;

	// for now, ignore if content returns false
	content_->load( contentSec );

	return true;
}


/**
 *	This method simply forwards the call to its Content object.
 *
 *	@param section	Data section to store content's info.
 *	@return		True if successful, false if not.
 */
bool Tab::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr contentSec = section->openSection( "ContentData", true );
	if ( !contentSec )
		return false;

	// for now, ignore if content returns false
	content_->save( contentSec );

	return true;
}


/**
 *	This method forwards the call to the Content to retrieve its caption
 *	text.
 *
 *	@return Content's caption text.
 */
BW::wstring Tab::getDisplayString()
{
	BW_GUARD;

	if (content_)
		return content_->getDisplayString();

	return Localise(L"GUITABS/TAB/NO_CONTENT");
}


/**
 *	This method forwards the call to the Content to retrieve its tab short
 *	caption text.
 *
 *	@return Content's tab short caption text.
 */
BW::wstring Tab::getTabDisplayString()
{
	BW_GUARD;

	if (content_)
		return content_->getTabDisplayString();

	return Localise(L"GUITABS/TAB/NO_CONTENT");
}


/**
 *	This method forwards the call to its Content's to retrieve its icon.
 *
 *	@return Content's icon.
 */
HICON Tab::getIcon()
{
	BW_GUARD;

	if (content_)
		return content_->getIcon();

	return 0;
}


/**
 *	This method forwards the call to its Content's to retrieve its CWnd.
 *
 *	@return Content's CWnd pointer.
 */
CWnd* Tab::getCWnd()
{
	BW_GUARD;

	if (content_)
		return content_->getCWnd();

	return 0;
}


/**
 *	This method forwards the call to its Content's to know if it's clonable.
 *
 *	@return True if the Content is clonable, false if not.
 */
bool Tab::isClonable()
{
	BW_GUARD;

	if (content_)
		return content_->isClonable();

	return false;
}


/**
 *	This method calculates the preferred size by getting the Content's
 *	preferred size.
 *
 *	@param width	Return param, prefered width for the panel.
 *	@param height	Return param, prefered height for the panel.
 */
void Tab::getPreferredSize( int& width, int& height )
{
	BW_GUARD;

	width = 0;
	height = 0;

	if (content_)
		content_->getPreferredSize( width, height );	
}


/**
 *	This method returns whether or not this tab is visible.
 *
 *	@return True if the tab is visible, false if not.
 */
bool Tab::isVisible()
{
	BW_GUARD;

	if ( !content_ )
		return false;

	return isVisible_;
}


/**
 *	This method sets whether or not this tab is visible.
 *
 *	@param visible True to set the tab is visible, false to hide it.
 */
void Tab::setVisible( bool visible )
{
	BW_GUARD;

	isVisible_ = visible;
}


/**
 *	This method sets whether or not this tab and its Content are visible.
 *
 *	@param visible True to set the tab and its Content visible.
 */
void Tab::show( bool show )
{
	BW_GUARD;

	if ( !content_ )
		return;

	isVisible_ = show;
	content_->getCWnd()->ShowWindow( show?SW_SHOW:SW_HIDE );
}


/**
 *	This method returns this tab's Content object.
 *
 *	@return This tab's Content object.
 */
ContentPtr Tab::getContent()
{
	BW_GUARD;

	return content_;
}


/**
 *	This method simply forwards the call to its Content.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Tab::handleRightClick( int x, int y )
{
	BW_GUARD;

	if ( content_ )
		content_->handleRightClick( x, y );
}


}	// namespace
BW_END_NAMESPACE

