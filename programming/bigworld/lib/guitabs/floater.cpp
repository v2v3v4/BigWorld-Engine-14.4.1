
#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{


// Floater window constants
static const int MIN_VERTICAL_SIZE = 21;
static const int ROLLEDUP_SIZE = 16;



/**
 *	Constructor.
 */
Floater::Floater( CWnd* parentWnd ) :
	dockTreeRoot_( 0 ),
	lastRollupSize_( 0 )
{
	BW_GUARD;

	Create(
		0,
		L"",
		WS_POPUP|WS_THICKFRAME|MFS_SYNCACTIVE|WS_CAPTION|WS_SYSMENU,
		CRect( 0, 0, 100, 100 ),
		parentWnd,
		0
		);
}


/**
 *	Denstructor.
 */
Floater::~Floater()
{
	BW_GUARD;

	DestroyWindow();
}


/**
 *	This method checks that the desired position and size are acceptable for
 *	this floater window.  It can potentially adjust the position and the size.
 *
 *	@param posX		Floater X position.
 *	@param posY		Floater Y position.
 *	@param width	Desired window width.
 *	@param height	Desired window height.
 */
void Floater::validatePos( int& posX, int& posY, int& width, int& height )
{
	BW_GUARD;

	HMONITOR mon = MonitorFromRect(
		CRect( posX, posY, posX + width, posY + height ),
		MONITOR_DEFAULTTONEAREST );
	MONITORINFO mi;
	mi.cbSize = sizeof( MONITORINFO );
	GetMonitorInfo( mon, &mi );

	if ( posX + width < mi.rcWork.left )
		posX = mi.rcWork.left;
	if ( posX > mi.rcWork.right )
		posX = mi.rcWork.right - width;

	if ( posY + height < mi.rcWork.top )
		posY = mi.rcWork.top;
	if ( posY > mi.rcWork.bottom )
		posY = mi.rcWork.bottom - height;
}


/**
 *	This method loads a floater's configuration from a data section.
 *
 *	@param section	Data section containing the floater's layout.
 *	@return	True if successful, false if not.
 */
bool Floater::load( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	int posX = section->readInt( "posX", 300 );
	int posY = section->readInt( "posY", 200 );
	int width = section->readInt( "width", 300 );
	int height = section->readInt( "height", 400 );
	lastRollupSize_ = section->readInt( "lastRollupSize", 0 );

	validatePos( posX, posY, width, height );

	SetWindowPos( 0, posX, posY, width, height, SWP_NOZORDER );

	DockNodePtr node = Manager::instance().dock()->nodeFactory( section );
	if ( !node )
		return false;

	if ( !node->load( section, this, AFX_IDW_PANE_FIRST ) )
		return false;

	dockTreeRoot_ = node;
	dockTreeRoot_->recalcLayout();
	RecalcLayout();

	ShowWindow( SW_SHOW );

	updateStyle();

	return true;
}


/**
 *	This method loads a floater's configuration from a data section.
 *
 *	@param section	Data section containing the floater's layout.
 *	@return	True if successful, false if not.
 */
bool Floater::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	CRect rect( 0, 0, 200, 200 );
	GetWindowRect( &rect );

	section->writeInt( "posX", rect.left );
	section->writeInt( "posY", rect.top );
	section->writeInt( "width", rect.Width() );
	section->writeInt( "height", rect.Height() );
	section->writeInt( "lastRollupSize", lastRollupSize_ );

	if ( !dockTreeRoot_->save( section ) )
		return false;

	return true;
}


/**
 *	This method returns the floater's CWnd.
 *
 *	@return	The floater's CWnd.
 */
CWnd* Floater::getCWnd()
{
	return this;
}


/**
 *	This method returns the root node of the floater.
 *
 *	@return	The root node of the floater.
 */
DockNodePtr Floater::getRootNode()
{
	BW_GUARD;

	return dockTreeRoot_;
}


/**
 *	This method sets the root node of the floater.
 *
 *	@param node	The root node of the floater.
 */
void Floater::setRootNode( DockNodePtr node )
{
	BW_GUARD;

	dockTreeRoot_ = node;

	if ( node )
	{
		node->setParentWnd( this );
		node->getCWnd()->SetDlgCtrlID( AFX_IDW_PANE_FIRST );
		node->getCWnd()->ShowWindow( SW_SHOW );

		updateStyle();

		RecalcLayout();
	}
}


/**
 *	This method is called to make sure the style of the window is modified
 *	appropriately depending on whether it has 1 or many panels.
 */
void Floater::updateStyle()
{
	BW_GUARD;

	int count = 0;

	countVisibleNodes( dockTreeRoot_, count );

	if ( count == 1 )
		ModifyStyle( WS_CAPTION|WS_SYSMENU, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED|SWP_DRAWFRAME );
	else
		ModifyStyle( 0, WS_CAPTION|WS_SYSMENU, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED|SWP_DRAWFRAME );
}


/**
 *	This method returns the last size the floater had before being rolled up.
 *
 *	@return The last size the floater had before being rolled up.
 */
int Floater::getLastRollupSize()
{
	BW_GUARD;

	return lastRollupSize_;
}


/**
 *	This method sets the last size the floater had before being rolled up.
 *
 *	@param size The last size the floater had before being rolled up.
 */
void Floater::setLastRollupSize( int size )
{
	BW_GUARD;

	lastRollupSize_ = size;
}


/**
 *	This method adjusts the size of the floater to try to fit all its internal
 *	panels.
 *
 *	@param rollUp	True to roll up the whole floater, false for normal resize.
 */
void Floater::adjustSize( bool rollUp )
{
	BW_GUARD;

	int w = 0;
	int h = 0;
	dockTreeRoot_->getPreferredSize( w, h );
	if ( rollUp )
	{
		if ( h > ROLLEDUP_SIZE )
		{
			int fh = getLastRollupSize();
			if ( fh )
				h = fh - GetSystemMetrics( SM_CYFIXEDFRAME ) * 2;
		}
		else
		{
			CRect rect;
			GetWindowRect( &rect );
			setLastRollupSize( rect.Height() );
		}
		CRect rect;
		GetWindowRect( &rect );
		w = rect.Width();
	}
	else
	{
		w += GetSystemMetrics( SM_CXFIXEDFRAME ) * 2;
		setLastRollupSize( 0 );
	}
	if ( !dockTreeRoot_->isLeaf() )
		h += GetSystemMetrics( SM_CYCAPTION );
	h += GetSystemMetrics( SM_CYFIXEDFRAME ) * 2;
	SetWindowPos( 0, 0, 0, w, h, SWP_NOMOVE|SWP_NOZORDER );
}


/**
 *	This method simply counts the number of visible nodes in the floater.
 *
 *	@param node	Recursive param, initially the floater's root node.
 *	@param count Recursive and return param, number of visible panels found.
 */
void Floater::countVisibleNodes( DockNodePtr node, int& count )
{
	BW_GUARD;

	if ( node->isLeaf() )
	{
		if ( node->isVisible() )
			count++;
	}
	else
	{
		countVisibleNodes( node->getLeftChild(), count );
		countVisibleNodes( node->getRightChild(), count );
	}
}


/**
 *	This MFC method overrides the default behaviour of "delete this", so we can
 *	rely on smartpointers for objects of this class.
 */
void Floater::PostNcDestroy()
{
	// do nothing. The standard behaviour is "delete this", which conflicts with smartpointers.
}


// MFC message map.
BEGIN_MESSAGE_MAP(Floater,CMiniFrameWnd)
	ON_WM_CLOSE()
	ON_WM_SIZING()
END_MESSAGE_MAP()


/**
 *	This method is called when the WM_CLOSE message is received so it closes
 *	all it's contained panels.
 *
 *	@param node	Recursive param, initially the floater's root node.
 */
bool Floater::onClosePanels( DockNodePtr node )
{
	BW_GUARD;

	if ( node->isLeaf() )
	{
		PanelPtr panel = Manager::instance().dock()->getPanelByWnd( node->getCWnd() );
		if ( panel )
			return panel->onClose();
		else
			return true;
	}

	bool l = onClosePanels( node->getLeftChild() );
	bool r = onClosePanels( node->getRightChild() );

	return l && r;
}


/**
 *	This MFC method handles the floater window being closed by the user.
 */
void Floater::OnClose()
{
	BW_GUARD;

	FloaterPtr pThis( this );
	if ( !onClosePanels( dockTreeRoot_ ) )
		return;

	Manager::instance().dock()->destroyFloater( pThis );
}


/**
 *	This MFC method is called as the window is resized in order to update the
 *	sizes of the internal panels.
 *
 *	@param nSide	Size of the floater window being moved.
 *	@param rect	Floater window's new size and position.
 */
void Floater::OnSizing( UINT nSide, LPRECT rect )
{
	BW_GUARD;

	// control the size of a floater, specially if it's rolled up
	int w = 0;
	int h = 0;
	dockTreeRoot_->getPreferredSize( w, h );

	int count = 0;
	countVisibleNodes( dockTreeRoot_, count );
	int minH;
	if ( dockTreeRoot_ && !dockTreeRoot_->isExpanded() )
		minH = h + GetSystemMetrics( SM_CYFIXEDFRAME ) * 2;
	else
		minH = MIN_VERTICAL_SIZE;
	if ( count > 1 )
		minH += GetSystemMetrics( SM_CYCAPTION );

	if ( rect && (
		rect->bottom - rect->top - 1 < minH ||
		( dockTreeRoot_ && !dockTreeRoot_->isExpanded() ) ) )
		if ( nSide == WMSZ_TOP || nSide == WMSZ_TOPLEFT || nSide == WMSZ_TOPRIGHT )
			rect->top = rect->bottom - minH - 1;
		else
			rect->bottom = rect->top + minH + 1;
	CWnd::OnSizing( nSide, rect );
}


}	// namespace
BW_END_NAMESPACE



