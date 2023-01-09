/**
 *	FiltersCtrl: Manages a set of push-like checkbox buttons.
 */


#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "filters_ctrl.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
FiltersCtrl::FiltersCtrl() :
	eventHandler_( 0 ),
	lines_( 1 ),
	separatorWidth_( 10 ),
	butSeparation_( 4 ),
	pushlike_( false )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
FiltersCtrl::~FiltersCtrl()
{
	BW_GUARD;

	clear();
}


/**
 *	This method removes all filters from the control.
 */
void FiltersCtrl::clear()
{
	BW_GUARD;

	filters_.clear();
	lines_ = 1;
}


/**
 *	This method checks whether or not the control has any filters.
 *
 *	@return	True if the control is empty, has no filters. False otherwise.
 */
bool FiltersCtrl::empty()
{
	BW_GUARD;

	return filters_.empty();
}


/**
 *	This method adds a filter to the control.
 *
 *	@param name		Name of the filter.
 *	@param pushed	Initial state of the filter (true if active).
 *	@param data		Custom data the caller can use when receiving click events.
 */
void FiltersCtrl::add( const wchar_t* name, bool pushed, void* data )
{
	BW_GUARD;

	FilterPtr newFilter = new Filter();

	int id = static_cast<int>(filters_.size() + FILTERCTRL_ID_BASE);

	CWindowDC dc( this );
	CFont* oldFont = dc.SelectObject( GetParent()->GetFont() );
	CSize textSize = dc.GetTextExtent( name, static_cast<int>(wcslen( name )) );

	newFilter->name = name;
	newFilter->data = data;
	newFilter->button.Create(
		name,
		(pushlike_?BS_PUSHLIKE:0) | BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
		CRect( 0, 0, textSize.cx + (pushlike_?14:26), 20 ),
		this,
		id );
	if ( pushlike_ )
	{
		LONG style = GetWindowLong( newFilter->button.GetSafeHwnd(), GWL_EXSTYLE );
		SetWindowLong( newFilter->button.GetSafeHwnd(), GWL_EXSTYLE, style | WS_EX_STATICEDGE );
	}
	newFilter->button.SetFont( GetParent()->GetFont() );
	newFilter->button.SetWindowPos( 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

	CRect rect;
	GetWindowRect( &rect );
	recalcWidth( rect.Width() );

	if ( pushed )
		newFilter->button.SetCheck( BST_CHECKED );

	filters_.push_back( newFilter );

	dc.SelectObject( oldFont );
}


/**
 *	This method adds a separator to the control.
 */
void FiltersCtrl::addSeparator()
{
	BW_GUARD;

	FilterPtr newSep = new Filter();

	newSep->name = L"";
	newSep->data = 0;
	newSep->separator.Create(
		L"",
		WS_CHILD | WS_VISIBLE | WS_DISABLED,
		CRect( 0, 0, 2, 20 ),
		this,
		0 );
	LONG style = GetWindowLong( newSep->separator.GetSafeHwnd(), GWL_EXSTYLE );
	SetWindowLong( newSep->separator.GetSafeHwnd(), GWL_EXSTYLE, style | WS_EX_STATICEDGE );
	newSep->separator.SetWindowPos( 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
	filters_.push_back( newSep );
}


/**
 *	This method returns the height of the control, which depends on the number
 *	and width of the filters and the width of the control.
 *
 *	@return		Current height of the control in pixels.
 */
int FiltersCtrl::getHeight()
{
	BW_GUARD;

	return lines_*22;
}


/**
 *	This method recalculates the position of the filter and separator controls 
 *	based on the input width, wrapping buttons in multiple lines if necessary.
 *
 *	@param width	Desired width for the control.
 */
void FiltersCtrl::recalcWidth( int width )
{
	BW_GUARD;

	lines_ = 1;
	int x = 0;
	int y = 0;

	for( FilterItr i = filters_.begin(); i != filters_.end(); ++i )
	{
		if ( (*i)->button.GetSafeHwnd() )
		{
			CRect brect;
			(*i)->button.GetWindowRect( brect );
			if ( x && x + brect.Width() > width )
			{
				x = 0;
				y += 22;
				lines_++;
			}

			(*i)->button.SetWindowPos( 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
			(*i)->button.RedrawWindow();

			x += brect.Width() + butSeparation_;
		}
		else
		{
			CRect brect;
			(*i)->separator.GetWindowRect( brect );
			(*i)->separator.SetWindowPos( 0,
				x + (separatorWidth_ - butSeparation_ - brect.Width() )/2, y,
				0, 0, SWP_NOSIZE | SWP_NOZORDER );
			(*i)->separator.RedrawWindow();
			x += separatorWidth_;
		}

	}
}


/**
 *	This method enables or disables all the filter buttons.
 *
 *	@param enable	True to enable all buttons, false to disable.
 */
void FiltersCtrl::enableAll( bool enable )
{
	BW_GUARD;

	for( FilterItr i = filters_.begin(); i != filters_.end(); ++i )
		if ( (*i)->button.GetSafeHwnd() )
			(*i)->button.EnableWindow( enable?TRUE:FALSE );
}


/**
 *	This method enables or disables a single the filter button by name.
 *
 *	@param enable	True to enable the filter "name", false to disable it.
 */
void FiltersCtrl::enable( const BW::wstring& name, bool enable )
{
	BW_GUARD;

	if ( name.empty() )
		return;

	for( FilterItr i = filters_.begin(); i != filters_.end(); ++i )
		if ( (*i)->name == name && (*i)->button.GetSafeHwnd() )
		{
			(*i)->button.EnableWindow( enable?TRUE:FALSE );
			break;
		}
}


/**
 *	This method receives a pointer to an object wanting to receive events from
 *	this control.
 *
 *	@param eventHandler	Pointer to the object that wishes to receive events.
 */
void FiltersCtrl::setEventHandler( FiltersCtrlEventHandler* eventHandler )
{
	BW_GUARD;

	eventHandler_ = eventHandler;
}


// MFC message map
BEGIN_MESSAGE_MAP(FiltersCtrl, CWnd)
	ON_WM_SIZE()
	ON_COMMAND_RANGE( FILTERCTRL_ID_BASE, FILTERCTRL_ID_BASE + 100, OnFilterClicked )
END_MESSAGE_MAP()


/**
 *	This method handles click events on the filter buttons, and relays the
 *	event to the event handler, if any.
 *
 *	@param nID	MFC's control ID for the pressed button.
 */
void FiltersCtrl::OnFilterClicked( UINT nID )
{
	BW_GUARD;

	if ( !eventHandler_ )
		return;

	nID -= FILTERCTRL_ID_BASE;

	if ( nID >= 0 && nID < filters_.size() )
	{
		eventHandler_->filterClicked(
			filters_[ nID ]->name.c_str(),
			(filters_[ nID ]->button.GetCheck() == BST_CHECKED),
			filters_[ nID ]->data
			);
	}
}


/**
 *	This method handles MFC's resize events, wrapping filter buttons in
 *	multiple lines if necessary.
 *
 *	@param nType	Type of resize, ignored.
 *	@param cx		New width.
 *	@param cy		New height.
 */
void FiltersCtrl::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CWnd::OnSize( nType, cx, cy );

	recalcWidth( cx );
}

BW_END_NAMESPACE

