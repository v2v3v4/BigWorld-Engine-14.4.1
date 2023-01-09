
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/string_provider.hpp"
#include "content_container.hpp"
#include "cstdmf/guard.hpp"
#include "manager.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{

//	Special content ID for the ContentContainer panel.
const BW::wstring ContentContainer::contentID = L"GUITABS::ContentContainer";


/**
 *	Constructor.
 */
ContentContainer::ContentContainer() :
	currentContent_( 0 )
{
}


/**
 *	Destructor.
 */
ContentContainer::~ContentContainer()
{
	BW_GUARD;
}


/**
 *	This method creates the actual window for a subcontent, if necessary.
 *
 *	@param content	Subcontent that needs its window created.
 */
void ContentContainer::createContentWnd( ContentPtr content )
{
	BW_GUARD;

	if ( !content )
		return;

	CWnd* wnd = content->getCWnd();

	if ( !wnd )
		ASSERT( 0 );

	if ( !IsWindow( wnd->GetSafeHwnd() ) )
	{
		wnd->Create(
			AfxRegisterWndClass( CS_OWNDC, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetSysColorBrush(COLOR_BTNFACE) ),
			L"GUITABS-Created-CWnd",
			WS_CHILD,
			CRect( 0, 0, 300, 400 ),
			this,
			0,
			0);
		if ( !IsWindow( wnd->GetSafeHwnd() ) )
			ASSERT( 0 );
	}
	else
	{
		wnd->SetParent( this );
	}

	wnd->UpdateData( FALSE );
}


/**
 *	This method adds a subcontent that will be part of this container panel.
 *
 *	@param content	Subcontent being added.
 */
void ContentContainer::addContent( ContentPtr content )
{
	BW_GUARD;

	if ( !content )
		return;

	createContentWnd( content );

	contents_.push_back( content );

	if ( !currentContent_ )
		currentContent( content );
}


/**
 *	This method adds a subcontent that will be part of this container panel by
 *	contentID.
 *
 *	@param subcontentID	ContentID for the subcontent to add to this container.
 */
void ContentContainer::addContent( BW::wstring subcontentID )
{
	BW_GUARD;

	addContent( Manager::instance().createContent( subcontentID ) );
}


/**
 *	This method makes the specified subcontent the currently active subcontent
 *	in this container panel.
 *
 *	@param content	The subcontent to activate.
 */
void ContentContainer::currentContent( ContentPtr content )
{
	BW_GUARD;

	ContentVecIt i = std::find( contents_.begin(), contents_.end(), content );
	if ( i == contents_.end() )
		return;

	CWnd* oldContent = 0;
	if ( !!currentContent_ && currentContent_ != content )
		oldContent = currentContent_->getCWnd();

	currentContent_ = content;
	CRect rect;
	GetClientRect( &rect );
	currentContent_->getCWnd()->SetWindowPos( 0, 0, 0, rect.Width(), rect.Height(), SWP_NOZORDER );
	currentContent_->getCWnd()->ShowWindow( SW_SHOW );
	if ( oldContent )
		oldContent->ShowWindow( SW_HIDE );
	CWnd* parent = GetParent();
	if ( parent )
		parent->RedrawWindow( 0, 0, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN );
}


/**
 *	This method makes the specified subcontent the currently active subcontent
 *	in this container panel.
 *
 *	@param subcontentID	ContentID of the subcontent to activate.
 */
void ContentContainer::currentContent( BW::wstring subcontentID )
{
	BW_GUARD;

	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		if ( (*i)->getContentID() == subcontentID )
		{
			currentContent( *i );
			break;
		}
	}
}


/**
 *	This method makes the specified subcontent the currently active subcontent
 *	in this container panel.
 *
 *	@param index	Index of the subcontent to activate.
 */
void ContentContainer::currentContent( int index )
{
	BW_GUARD;

	int count = 0;
	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		if ( count == index )
		{
			currentContent( *i );
			break;
		}
		count++;
	}
}


/**
 *	This method returns the currently active subcontent in the container.
 *
 *	@return		The currently active subcontent in the container.
 */
ContentPtr ContentContainer::currentContent()
{
	BW_GUARD;

	return currentContent_;
}


/**
 *	This method returns whether or not the container contains the specified
 *	subcontent.
 *
 *	@param content	Subcontent to test.
 *	@return		True of the subcontent is part of this container, false if not.
 */
bool ContentContainer::contains( ContentPtr content )
{
	BW_GUARD;

	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
		if ( (*i) == content )
			return true;

	return false;
}


/**
 *	This method returns the number of instances of a subcontainer in this
 *	container.
 *
 *	@param subcontentID	ContentID of the desired subcontent.
 *	@return		The number of instances of the contentID in this container.
 */
int ContentContainer::contains( const BW::wstring subcontentID )
{
	BW_GUARD;

	int cnt = 0;
	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
		if ( (*i)->getContentID() == subcontentID )
			cnt++;

	return cnt;
}


/**
 *	This method returns the first occurrence of a contentID in this container.
 *
 *	@param subcontentID	ContentID of the desired subcontent.
 *	@return		The first occurrence of a contentID in this container.
 */
ContentPtr ContentContainer::getContent( const BW::wstring subcontentID )
{
	BW_GUARD;

	int index = 0;
	return getContent( subcontentID, index );
}


/**
 *	This method returns the occurrence number "index" of a contentID in this
 *	container.
 *
 *	@param subcontentID	ContentID of the desired subcontent.
 *	@return		The occurrence number "index" of a contentID in this container.
 */
ContentPtr ContentContainer::getContent( const BW::wstring subcontentID, int& index )
{
	BW_GUARD;

	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		if ( subcontentID.compare( (*i)->getContentID() ) == 0 )
		{
			if ( index <= 0 )
				return *i;
			else
				index--;
		}
	}

	return 0;
}


/**
 *	This method returns this content container's contentID.
 *
 *	@return		This container's contentID.
 */
BW::wstring ContentContainer::getContentID()
{
	BW_GUARD;

	return contentID;
}


/**
 *	This method forwards a message to all of it's subcontents.
 *
 *	@param msg	Windows message id.
 *	@param wParam	Windows message WPARAM.
 *	@param lParam	Windows message LPARAM.
 */
void ContentContainer::broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		CWnd* window = (*i)->getCWnd();

		if (window->GetSafeHwnd())
		{
			window->SendMessage( msg, wParam, lParam );
		}
	}
}


/**
 *	This method loads this container and its subcontents from XML.
 *
 *	@param section	Data section from the layout file.
 *	@return True if successful, false on failure.
 */
bool ContentContainer::load( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	BW::vector<DataSectionPtr> sections;
	section->openSections( "Subcontent", sections );
	if ( sections.empty() )
		return true;

	ContentPtr firstContent = 0;

	for( BW::vector<DataSectionPtr>::iterator s = sections.begin();
		s != sections.end(); ++s )
	{
		ContentPtr content = Manager::instance().createContent( (*s)->asWideString() );
		if ( !content )
			continue;

		DataSectionPtr subsec = (*s)->openSection( "SubcontentData" );
		if ( !subsec )
			continue;

		addContent( content );
		if ( !firstContent || (*s)->readBool( "current" ) )
			firstContent = content;

		content->load( subsec );
	}

	if ( !!firstContent )
	{
		currentContent( firstContent );
	}

	return true;
}


/**
 *	This method saves this container and its subcontents to XML.
 *
 *	@param section	Data section from the layout file.
 *	@return True if successful, false on failure.
 */
bool ContentContainer::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		DataSectionPtr sec = section->newSection( "Subcontent" );
		if ( !sec )
			continue;

		sec->setWideString( (*i)->getContentID() );
		if ( currentContent_ == (*i) )
			sec->writeBool( "current", true );
		DataSectionPtr subsec = sec->newSection( "SubcontentData" );
		if ( !subsec )
			continue;

		(*i)->save( subsec );
	}

	return true;
}


/**
 *	This method forwards the call to the active subcontainer so its caption
 *	text gets displayed.
 *
 *	@return Active subcontainer's caption text.
 */
BW::wstring ContentContainer::getDisplayString()
{
	BW_GUARD;

	if ( !currentContent_ )
		return Localise(L"GUITABS/CONTENT_CONTAINER/NO_CONTENT");

	return currentContent_->getDisplayString();
}


/**
 *	This method forwards the call to the active subcontainer so its tab caption
 *	text gets displayed.
 *
 *	@return Active subcontainer's tab caption text.
 */
BW::wstring ContentContainer::getTabDisplayString()
{
	BW_GUARD;

	if ( !currentContent_ )
		return Localise(L"GUITABS/CONTENT_CONTAINER/NO_CONTENT");

	return currentContent_->getTabDisplayString();
}


/**
 *	This method forwards the call to the active subcontainer so its icon gets
 *	used.
 *
 *	@return Active subcontainer's icon.
 */
HICON ContentContainer::getIcon()
{
	BW_GUARD;

	if ( !currentContent_ )
		return 0;

	return currentContent_->getIcon();
}


/**
 *	This method returns the "this" pointer as its CWnd pointer.
 *
 *	@return The "this" pointer as its CWnd pointer.
 */
CWnd* ContentContainer::getCWnd()
{
	return this;
}


/**
 *	This method calculates the preferred size by getting the maximum of all the
 *	subcontainers preferred sizes.
 *
 *	@param width	Return param, prefered width for the panel.
 *	@param height	Return param, prefered height for the panel.
 */
void ContentContainer::getPreferredSize( int& width, int& height )
{
	BW_GUARD;

	width = 0;
	height = 0;
	for( ContentVecIt i = contents_.begin(); i != contents_.end(); ++i )
	{
		int w;
		int h;

		(*i)->getPreferredSize( w, h );

		width = max( width, w );
		height = max( height, h );
	}
}


/**
 *	This method is called when this container panel is being closed.
 *
 *	@param isLastContent	True if this is the last instance of this panel.
 *	@return	Always hide this panel on close.
 */
Content::OnCloseAction ContentContainer::onClose( bool isLastContent )
{
	// never destroy, only hide
	return CONTENT_HIDE;
}


/**
 *	This method is called when the user right-clicks on the non-client area of
 *	this container panel, and it simply forwards that call to the active
 *	subcontent.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void ContentContainer::handleRightClick( int x, int y )
{
	BW_GUARD;

	if ( !currentContent_ )
		return;

	currentContent_->handleRightClick( x, y );
}


// MFC message map
BEGIN_MESSAGE_MAP(ContentContainer, CDialog)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()


/**
 *	This method forwards the resize event to the active subcontent.
 *
 *	@param nType	MFC resize type.
 *	@param cx	MFC new width.
 *	@param cy	MFC new height.
 */
void ContentContainer::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	if ( !!currentContent_ )
		currentContent_->getCWnd()->SetWindowPos( 0, 0, 0, cx, cy, SWP_NOZORDER );
}


/**
 *	This method forwards the focus event to the active subcontent.
 *
 *	@param pOldWnd	MFC previously focused window.
 */
void ContentContainer::OnSetFocus( CWnd* pOldWnd )
{
	BW_GUARD;

	CDialog::OnSetFocus( pOldWnd );

	if ( !!currentContent_ )
		currentContent_->getCWnd()->SetFocus();
}


} // namespace
BW_END_NAMESPACE
