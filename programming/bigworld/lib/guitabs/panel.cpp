
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
 *	@param parent	Parent window.
 */
Panel::Panel( CWnd* parent ) :
	activeTab_( 0 ),
	isFloating_( false ),
	isExpanded_( true ),
	expandedSize_( 100 ),
	isActive_( false ),
	buttonDown_( 0 ),
	lastX_( 300 ),
	lastY_( 200 ),
	tempTab_( 0 )
{
	BW_GUARD;

	CreateEx(
		0,
		AfxRegisterWndClass( CS_OWNDC, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetSysColorBrush(COLOR_BTNFACE) ),
		L"Panel",
		WS_CHILD | WS_CLIPCHILDREN,
		CRect(0,0,1,1),
		parent,
		0,
		0);

	tabBar_ = new TabCtrl( this, TabCtrl::TOP );

	tabBar_->setEventHandler( this );
	tabBar_->ShowWindow( SW_SHOW );

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	captionFont_.CreateFontIndirect( &metrics.lfSmCaptionFont );
}


/**
 *	Destructor.
 */
Panel::~Panel()
{
	BW_GUARD;

	if ( tabBar_ )
		tabBar_->DestroyWindow();

	activeTab_ = 0;
	tabList_.clear();

	DestroyWindow();
}


/**
 *	This method returns the CWnd for the panel.
 *
 *	@return	This panel's CWnd.
 */
CWnd* Panel::getCWnd()
{
	return this;
}


/**
 *	This method adds a new Content to a tab in this panel.
 *
 *	@param contentID	Content ID for the new tab's Content.
 */
void Panel::addTab( const BW::wstring contentID )
{
	BW_GUARD;

	TabPtr newTab = new Tab( this, contentID );
	tabList_.push_back( newTab );
	tabBar_->insertItem( newTab->getTabDisplayString(), newTab->getIcon(), newTab.getObject() );
	newTab->show( true );
	setActiveTab( newTab );
}


/**
 *	This method puts an existing tab into this panel.
 *
 *	@param tab	Tab to dock into this panel.
 */
void Panel::addTab( TabPtr tab )
{
	BW_GUARD;

	tab->getCWnd()->SetParent( this );
	tabList_.push_back( tab );
	if ( tab->isVisible() )
	{
		tabBar_->insertItem( tab->getTabDisplayString(), tab->getIcon(), tab.getObject() );
		setActiveTab( tab );
	}
}


/**
 *	This method removes a tab from this panel.
 *
 *	@param tab	Tab to be detached from the panel.
 */
void Panel::detachTab( TabPtr tab )
{
	BW_GUARD;

	for(TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( (*i) == tab )
		{
			if ( activeTab_ == tab )
				activeTab_ = 0;
			tab->getCWnd()->ShowWindow( SW_HIDE );
			tab->getCWnd()->SetParent( 0 );
			tabBar_->removeItem( tab.getObject() );
			tabList_.erase( i );
			for( i = tabList_.begin(); i != tabList_.end(); ++i )
			{
				if ( tabBar_->contains( (*i).getObject() ) )
				{
					setActiveTab( *i );
					break;
				}
			}
			if ( i == tabList_.end() )
				setActiveTab( 0 );

			break;
		}
	}
}


/**
 *	This method finds a Content in the list of tabs and removes the tab from
 *	this panel.
 *
 *	@param contentID	Content ID of the tab(s) to remove.
 */
void Panel::detachTab( const BW::wstring contentID )
{
	BW_GUARD;

	for(TabItr i = tabList_.begin(); i != tabList_.end(); )
	{
		if ( contentID.compare( (*i)->getContent()->getContentID() ) == 0 )
		{
			// restart iterator since detachTab will remove the tab from the list
			TabItr old = i;
			++i;
			detachTab( *old );
		}
		else
			++i;
	}
}


/**
 *	This method simply detaches the first tab and retirns it.
 *
 *	@return The former first tab of the panel, after being detached.
 */
TabPtr Panel::detachFirstTab()
{
	BW_GUARD;

	if ( tabList_.empty() )
		return 0;

	TabPtr tab = *(tabList_.begin());

	detachTab( *(tabList_.begin()) );

	return tab;
}


/**
 *	This method loads the panel's position and layout info from the layout xml
 *	file, and creates and loads its tabs.
 *
 *	@param section	Data section containing the panel and tab layout info.
 *	@return		True if successful, false if not.
 */
bool Panel::load( DataSectionPtr section )
{
	BW_GUARD;

	lastX_ = section->readInt( "lastX", lastX_ );
	lastY_ = section->readInt( "lastY", lastY_ );
	isExpanded_ = section->readBool( "expanded", isExpanded_ );
	expandedSize_ = section->readInt( "expandedSize", expandedSize_ );
	isFloating_ = section->readBool( "floating", isFloating_ );

	int w;
	int h;
	getPreferredSize( w, h );
	SetWindowPos( 0, 0, 0,
		section->readInt( "lastWidth", w ), section->readInt( "lastHeight", h ),
		SWP_NOMOVE | SWP_NOZORDER );

	BW::vector<DataSectionPtr> tabs;
	section->openSections( "Tab", tabs );
	if ( tabs.empty() )
		return false;
	activeTab_ = 0;
	TabPtr firstTab = 0;
	for( BW::vector<DataSectionPtr>::iterator i = tabs.begin(); i != tabs.end(); ++i )
	{
		BW::wstring contentID = (*i)->readWideString( "contentID" );
		if ( contentID.empty() )
			continue;

		TabPtr newTab = new Tab( this, contentID );

		if ( !newTab->getContent() )
			continue;

		newTab->setVisible( (*i)->readBool( "visible", true ) );

		// ignoring if loading a tab returns false
		newTab->load( *i );

		addTab( newTab );

		newTab->getCWnd()->ShowWindow( SW_HIDE );

		if ( !firstTab && !!activeTab_ )
			firstTab = activeTab_;
	}
	if ( firstTab )
		setActiveTab( firstTab );

	if ( activeTab_ )
	{
		updateTabBar();

		if ( isExpanded_ )
			activeTab_->getCWnd()->ShowWindow( SW_SHOW );
		else
			activeTab_->getCWnd()->ShowWindow( SW_HIDE );
	}

	return true;
}


/**
 *	This method saves the panel's position and layout info to the layout xml
 *	file, and saves its tabs info.
 *
 *	@param section	Data section to save the panel and tab layout info to.
 *	@return		True if successful, false if not.
 */
bool Panel::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	// save properties
	section->writeInt( "lastX", lastX_ );
	section->writeInt( "lastY", lastY_ );
	CRect rect;
	GetWindowRect( &rect );
	section->writeInt( "lastWidth", rect.Width() );
	section->writeInt( "lastHeight", rect.Height() );
	section->writeBool( "expanded", isExpanded_ );
	section->writeInt( "expandedSize", expandedSize_ );
	section->writeBool( "floating", isFloating_ );

	// save tab order in a temporary vector
	BW::vector<TabPtr> tabOrder;
	for( int i = 0; i < tabBar_->itemCount(); ++i )
		tabOrder.push_back( (Tab*)tabBar_->getItemData( i ) );
	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
		if ( std::find<BW::vector<TabPtr>::iterator,TabPtr>(
				tabOrder.begin(), tabOrder.end(), *i ) == tabOrder.end() )
			tabOrder.push_back( *i );

	// save tabs
	for( BW::vector<TabPtr>::iterator i = tabOrder.begin(); i != tabOrder.end(); ++i )
	{
		DataSectionPtr tabSec = section->newSection( "Tab" );
		if ( !tabSec )
			return false;

		// have to save visibility at this level, not in the tab...
		tabSec->writeBool( "visible", (*i)->isVisible() );
		tabSec->writeWideString( "contentID", (*i)->getContent()->getContentID() );

		if ( !(*i)->save( tabSec ) )
			return false;
	}

	return true;
}


/**
 *	This method activates the panel, in turn activating its active tab.
 */
void Panel::activate()
{
	BW_GUARD;

	isActive_ = true;
	paintCaptionBar();
	if ( activeTab_ )
		activeTab_->getCWnd()->SetFocus();
	Manager::instance().dock()->setActivePanel( this );
}


/**
 *	This method deactivates this pane.
 */
void Panel::deactivate()
{
	BW_GUARD;

	isActive_ = false;
	paintCaptionBar();
}


/**
 *	This method returns whether the panel is expanded (true) or rolled up
 *	(false).
 *
 *	@return		Whether the panel is expanded (true) or rolled up (false).
 */
bool Panel::isExpanded()
{
	BW_GUARD;

	return isExpanded_;
}


/**
 *	This method sets whether a panel is expanded (true) or rolled up (false).
 *
 *	@param expanded		Sets the panel to expanded (true) or rolled up (false).
 */
void Panel::setExpanded( bool expanded )
{
	BW_GUARD;

	isExpanded_ = expanded;
	Manager::instance().dock()->rollupPanel( this );
	if ( activeTab_ )
	{
		updateTabBar();

		if ( isExpanded_ )
		{
			activeTab_->getCWnd()->ShowWindow( SW_SHOW );
			activeTab_->getCWnd()->RedrawWindow();
		}
		else
			activeTab_->getCWnd()->ShowWindow( SW_HIDE );
	}
}


/**
 *	This method returns whether the panel is floating (true) or docked (false).
 *
 *	@return		Whether the panel is floating (true) or docked (false).
 */
bool Panel::isFloating()
{
	BW_GUARD;

	return isFloating_;
}


/**
 *	This method returns sets the panel is floating (true) or docked (false).
 *
 *	@param floating		Whether the panel is floating (true) or docked (false).
 */
void Panel::setFloating( bool floating )
{
	BW_GUARD;

	isFloating_ = floating;
}


/**
 *	This method returns the panels preferred default size, which is the max of
 *	its tabs preferred sizes.
 *
 *	@param width	Return param, preferred width.
 *	@param height	Return param, preferred height.
 */
void Panel::getPreferredSize( int& width, int& height )
{
	BW_GUARD;

	width = 0;
	height = 0;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		int w;
		int h;

		(*i)->getPreferredSize( w, h );
		if ( w > width )
			width = w;
		if ( h > height )
			height = h;
	}

	if ( !isExpanded() )
		height = PANEL_ROLLUP_SIZE;
}


/**
 *	This method returns the height of the caption bar for this panel.
 *
 *	@return	The height of the caption bar for this panel.
 */
int Panel::getCaptionSize()
{
	BW_GUARD;

	return CAPTION_HEIGHT;
}


/**
 *	This method returns the height of its tabs control, which can be multiline.
 *
 *	@return	The height of the tabs control for this panel.
 */
int Panel::getTabCtrlSize()
{
	BW_GUARD;

	if ( tabBar_->itemCount() > 1 )
		return tabBar_->getHeight();
	else
		return 0;	
}


/**
 *	This method returns whether or not the tabs control is at the top or the
 *	bottom.
 *
 *	@return	True if the tabs control is at the top, false if at the bottom.
 */
bool Panel::isTabCtrlAtTop()
{
	BW_GUARD;

	if ( tabBar_->getAlignment() == TabCtrl::TOP )
		return true;
	else
		return false;
}


/**
 *	This method clears the list of saved positions for this panel.
 *	@see insertPos, PanelPos
 *
 *	@param docked	Whether or not the panel is docked (true) or not (false).
 */
void Panel::clearPosList( bool docked )
{
	BW_GUARD;

	if ( docked )
		dockedPosList_.clear();
	else
		floatingPosList_.clear();

	resetPosList( docked );
}


/**
 *	This method resets the iterators that point to the current pos in the 
 *	positions lists.
 *	@see insertPos, PanelPos
 *
 *	@param docked	Whether or not the panel is docked (true) or not (false).
 */
void Panel::resetPosList( bool docked )
{
	BW_GUARD;

	if ( docked )
		dockedPosItr_ = dockedPosList_.begin();
	else
		floatingPosItr_ = floatingPosList_.begin();
}


/**
 *	This method inserts a panel position to the panel's list of insertion
 *	positions.
 *	This list is used for when, for example, double clicking on a docked panel,
 *	which results in the panel toggling from docked to floating.  This list
 *	stores information on how the panel was inserted in the dock tree so, if 
 *	it has to be docked again, it makes the best guess.  Each PanelPos entry
 *	contains info about this panels position relative to others, like "Left to
 *	the Asset Browser", "Top of the Properties", "As a tab in the Object panel"
 *	and so on.
 *
 *	@param docked	Whether or not the panel is docked (true) or not (false).
 */
void Panel::insertPos( bool docked, PanelPos pos )
{
	BW_GUARD;

	if ( docked )
		dockedPosList_.push_back( pos );
	else
		floatingPosList_.push_back( pos );
}


/**
 *	This method returns the next relative position fo the panel.
 *	@see insertPos, PanelPos
 *
 *	@param docked	Whether or not the panel is docked (true) or not (false).
 *	@param pos		Return param, next relative position going up the tree.
 *	@return		True if a position was returned in pos, false if no more info.
 */
bool Panel::getNextPos( bool docked, PanelPos& pos )
{
	BW_GUARD;

	if ( docked )
	{
		if ( dockedPosItr_ == dockedPosList_.end() || dockedPosList_.empty() )
			return false;

		pos = *dockedPosItr_++;
	}
	else
	{
		if ( floatingPosItr_ == floatingPosList_.end() || floatingPosList_.empty() )
			return false;

		pos = *floatingPosItr_++;
	}

	return true;
}


/**
 *	This method returns the last screen position of the panel.
 *
 *	@param x	Return param, last X position of the panel.
 *	@param y	Return param, last Y position of the panel.
 */
void Panel::getLastPos( int& x, int& y )
{
	BW_GUARD;

	x = lastX_;
	y = lastY_;
}


/**
 *	This method sets the last screen position of the panel.
 *
 *	@param x	The last X position of the panel.
 *	@param y	The last Y position of the panel.
 */
void Panel::setLastPos( int x, int y )
{
	BW_GUARD;

	lastX_ = x;
	lastY_ = y;
}


/**
 *	This method returns whether or not a specified tab contains a Content.  If
 *	the tab is a content container, the we ask the content container if it
 *	contains the specified content itself.
 *
 *	@param t	Tab we want to test.
 *	@param content	Content we want to know whether or not it's in tab "t".
 *	@return		True if tab "t" contains content "content", false if not.
 */
bool Panel::tabContains( TabPtr t, ContentPtr content )
{
	BW_GUARD;

	Content* tcontent = t->getContent().getObject();
	if ( !tcontent )
		return 0;
	
	return tcontent == content ||
			( tcontent->getContentID() == ContentContainer::contentID &&
			((ContentContainer*)tcontent)->contains( content ) );
}


/**
 *	This method returns whether or not a specified tab contains a Content.  If
 *	the tab is a content container, the we ask the content container if it
 *	contains the specified content itself.
 *
 *	@param t	Tab we want to test.
 *	@param contentID	Content we want to know whether or not is in tab "t".
 *	@return		Number of times "contentID" was found inside tab "t".
 */
int Panel::tabContains( TabPtr t, const BW::wstring contentID )
{
	BW_GUARD;

	Content* tcontent = t->getContent().getObject();
	if ( !tcontent )
		return 0;
	
	int cnt = 0;

	if ( contentID.compare( tcontent->getContentID() ) == 0 )
		++cnt;
	else if ( tcontent->getContentID() == ContentContainer::contentID )
		cnt += ((ContentContainer*)tcontent)->contains( contentID );
	
	return cnt;
}


/**
 *	This method returns whether or not this panel contains a Content.
 *
 *	@param content	Content we want to know whether or not it's in the panel.
 *	@return		True if the panel contains content "content", false if not.
 */
bool Panel::contains( ContentPtr content )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( tabContains( *i, content ) )
			return true;
	}

	return false;
}


/**
 *	This method returns whether or not this panel contains a Content.
 *
 *	@param contentID	Content we want to know whether or not is in the panel.
 *	@return		Number of times "contentID" was found inside this panel.
 */
int Panel::contains( const BW::wstring contentID )
{
	BW_GUARD;

	int cnt = 0;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
		cnt += tabContains( *i, contentID );

	return cnt;
}


/**
 *	This method returns the first Content specified by the contentID if it's
 *	contained within this panel, or NULL if it's not.
 *
 *	@param contentID	Content ID of the desired Content.
 *	
 *	@return		Pointer to the desired Content, or NULL if it's not in this
 *				panel.
 */
ContentPtr Panel::getContent( const BW::wstring contentID )
{
	BW_GUARD;

	int index = 0;
	return getContent( contentID, index );
}


/**
 *	This method returns the "index" occurrence of the Content specified by the
 *	contentID contained in this panel, or NULL if the content is not contained
 *	or there are less than "index" occurrences of the content.
 *
 *	@param contentID	Content ID of the desired Content.
 *	@param index		Zero-based index, 0 for first occurrence, 1 for the
 *						second occurrence, etc.
 *	@return		Pointer to the desired Content, or NULL if it's not in this
 *				panel or there are less than "index" occurrences.
 */
ContentPtr Panel::getContent( const BW::wstring contentID, int& index )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		Content* content = (*i)->getContent().getObject();
		if ( !content )
			continue;
		if ( contentID.compare( content->getContentID() ) == 0 )
		{
			if ( index <= 0 )
				return content;
			else
				index--;
		}
		else if ( content->getContentID() == ContentContainer::contentID &&
			((ContentContainer*)content)->contains( contentID ) )
		{
			content = ((ContentContainer*)content)->getContent( contentID, index ).getObject();
			if ( content )
				return content;
			// index already decremented by ContentContainer::getContent
		}
	}

	return 0;
}


/**
 *	This method forwards a message to all the tabs in this panel.
 *
 *	@param msg	Windows message.
 *	@param wParam	Windows message WPARAM.
 *	@param lParam	Windows message LPARAM.
 */
void Panel::broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		Content* content = (*i)->getContent().getObject();
		if ( !content )
			continue;

		if ( content->getContentID() == ContentContainer::contentID )
			((ContentContainer*)content)->broadcastMessage( msg, wParam, lParam );
		else if ((*i)->getCWnd()->m_hWnd)
		{
			(*i)->getCWnd()->SendMessage( msg, wParam, lParam );
		}
	}
}


/**
 *	This method handles a click on a tab event sent from the tab control, to
 *	either activate a tab or to start dragging it to tear it off the panel.
 *
 *	@param itemData	Callback specific data, the Tab pointer in this case.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Panel::clickedTab( void* itemData, int x, int y )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( (*i).getObject() == itemData )
		{
			setActiveTab( *i );
			break;
		}
	}

	if ( activeTab_ )
	{
		CRect rect;
		tabBar_->GetWindowRect( &rect );
		Manager::instance().dragManager()->startDrag( rect.left + x, rect.top + y, this, activeTab_ );
	}
}


/**
 *	This method handles a double-click on a tab event sent from the tab control
 *	which results in activating or floating the tab.
 *
 *	@param itemData	Callback specific data, the Tab pointer in this case.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Panel::doubleClickedTab( void* itemData, int x, int y )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( (*i).getObject() == itemData )
		{
			setActiveTab( *i );
			break;
		}
	}

	if ( activeTab_ )
	{
		Manager::instance().dock()->toggleTabPos( this, activeTab_ );
		setActiveTab( *tabList_.begin() );
	}
}


/**
 *	This method handles a right-click on a tab event sent from the tab control,
 *	which simply forwards the event to the Tab's Content.
 *
 *	@param itemData	Callback specific data, the Tab pointer in this case.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Panel::rightClickedTab( void* itemData, int x, int y )
{
	BW_GUARD;

	if ( activeTab_ && activeTab_.getObject() == itemData )
		activeTab_->handleRightClick( x, y );
}


//
// private members
//


/**
 *	This method repaints the panel's caption bar and its buttons.
 */
void Panel::paintCaptionBar()
{
	BW_GUARD;

	paintCaptionBarOnly();
	paintCaptionButtons();
}


/**
 *	This method repaints the panel's caption bar only, no buttons.
 */
void Panel::paintCaptionBarOnly()
{
	BW_GUARD;

	CBrush brush;
	COLORREF textColor;

	// Draw the caption bar
	if( isActive_ )
	{
		brush.CreateSolidBrush( GetSysColor(COLOR_ACTIVECAPTION) );
		textColor = GetSysColor(COLOR_CAPTIONTEXT);
	}
	else
	{
		brush.CreateSolidBrush( GetSysColor(COLOR_BTNFACE) );
		textColor = GetSysColor(COLOR_BTNTEXT);
	}

	CWindowDC dc(this);

	CRect rect;
	GetWindowRect(&rect);
	
	rect.right = rect.Width();
	rect.top = 0;
	rect.left = 0;
	rect.bottom = rect.top + CAPTION_HEIGHT;
	dc.FillRect(&rect,&brush);

	// Draw caption text
	int oldBkMode = dc.SetBkMode(TRANSPARENT);
	CFont* oldFont = dc.SelectObject( &captionFont_ );
	COLORREF oldColor = dc.SetTextColor( textColor );

	BW::wstring text;
	if ( !isExpanded_ && visibleTabCount() > 1 )
	{
		for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
		{
			if ( tabBar_->contains( (*i).getObject() ) )
			{
				if ( text.length() > 0 )
					text = text + L", ";
				text = text + (*i)->getContent()->getTabDisplayString();
			}
		}
	}
	else
	{
		if ( activeTab_ )
			text = activeTab_->getDisplayString();
		else
			text = Localise(L"GUITABS/PANEL/NO_TAB_SELECTED");
	}

	CRect tRect = rect;
	tRect.left += CAPTION_LEFTMARGIN;
	tRect.top += CAPTION_TOPMARGIN;
	if ( activeTab_ && activeTab_->isClonable() )
		tRect.right -= CAPTION_HEIGHT*3;
	else
		tRect.right -= CAPTION_HEIGHT*2;
	dc.DrawText( CString( text.c_str() ) , &tRect, DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS );

	if ( !isActive_ && tabBar_->itemCount() > 1 ) 
	{
		CPen btnBottomLine( PS_SOLID, 1, isExpanded_?::GetSysColor(COLOR_BTNSHADOW):GetSysColor(COLOR_BTNFACE) );
		CPen* oldPen = dc.SelectObject( &btnBottomLine );
		dc.MoveTo( rect.left, rect.bottom - 1 );
		dc.LineTo( rect.right, rect.bottom - 1 );
		dc.SelectObject( oldPen );
	}

	// Restore old DC objects
	dc.SelectObject( oldFont );
	dc.SetBkMode( oldBkMode );
	dc.SetTextColor( oldColor );
}


/**
 *	This method repaints the panel's caption bar buttons.
 */
void Panel::paintCaptionButtons( UINT hitButton )
{
	BW_GUARD;

	CBrush brush;
	CPen pen;

	CWindowDC dc(this);

	if ( isActive_ )
	{
		brush.CreateSolidBrush( GetSysColor(COLOR_ACTIVECAPTION) );
		pen.CreatePen( PS_SOLID, 1, GetSysColor(COLOR_CAPTIONTEXT) );
	}
	else
	{
		brush.CreateSolidBrush( GetSysColor(COLOR_BTNFACE) );
		pen.CreatePen( PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT) );
	}

	// Draw background
	CRect rect;
	GetWindowRect(&rect);
	
	rect.right = rect.Width();
	if ( activeTab_ && activeTab_->isClonable() )
		rect.left = rect.right - CAPTION_HEIGHT*3;
	else
		rect.left = rect.right - CAPTION_HEIGHT*2;
	rect.top = 0;
	rect.bottom = rect.top + CAPTION_HEIGHT - 1;
	dc.FillRect(&rect,&brush);

	// Draw buttons
	CPen* oldPen = dc.SelectObject( &pen );
	rect.top += CAPTION_TOPMARGIN;

	// Draw the "Close" button
	rect.left = rect.right - CAPTION_HEIGHT;
	CRect butRect( rect );
	butRect.DeflateRect( 0, 0, 1, 1 );
	if ( hitButton == BUT_CLOSE )
		if ( buttonDown_ == hitButton )
			dc.Draw3dRect( &butRect, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT) );
		else
			dc.Draw3dRect( &butRect, GetSysColor(COLOR_BTNHIGHLIGHT), GetSysColor(COLOR_BTNSHADOW) );

	butRect.DeflateRect( 5, 4, 5, 5 );
	dc.MoveTo( butRect.left, butRect.top );
	dc.LineTo( butRect.right, butRect.bottom+1 );
	dc.MoveTo( butRect.left, butRect.bottom );
	dc.LineTo( butRect.right, butRect.top-1 );
	
	// Draw the "Rollup" button
	rect.OffsetRect( -CAPTION_HEIGHT, 0 );
	butRect = rect;
	butRect.DeflateRect( 0, 0, 1, 1 );
	if ( hitButton == BUT_ROLLUP )
		if ( buttonDown_ == hitButton )
			dc.Draw3dRect( &butRect, GetSysColor(COLOR_BTNSHADOW), GetSysColor(COLOR_BTNHIGHLIGHT) );
		else
			dc.Draw3dRect( &butRect, GetSysColor(COLOR_BTNHIGHLIGHT), GetSysColor(COLOR_BTNSHADOW) );

	butRect.DeflateRect( 5, 4, 5, 5 );
	if ( isExpanded_ )
	{
		dc.MoveTo( ( butRect.left + butRect.right ) /2, butRect.top );
		dc.LineTo( butRect.left, butRect.bottom );
		dc.MoveTo( ( butRect.left + butRect.right ) /2, butRect.top );
		dc.LineTo( butRect.right, butRect.bottom );
		dc.MoveTo( butRect.left, butRect.bottom );
		dc.LineTo( butRect.right, butRect.bottom );
	}
	else
	{
		dc.MoveTo( ( butRect.left + butRect.right ) /2, butRect.bottom );
		dc.LineTo( butRect.left, butRect.top );
		dc.MoveTo( ( butRect.left + butRect.right ) /2, butRect.bottom );
		dc.LineTo( butRect.right, butRect.top );
		dc.MoveTo( butRect.left, butRect.top );
		dc.LineTo( butRect.right, butRect.top );
	}
	
	// Draw the "Clone" button
	if ( activeTab_ && activeTab_->isClonable() )
	{
		rect.OffsetRect( -CAPTION_HEIGHT, 0 );
		butRect = rect;
		butRect.DeflateRect( 0, 0, 1, 1 );
		if ( hitButton == BUT_CLONE )
			if ( buttonDown_ == hitButton )
				dc.Draw3dRect( &butRect, ::GetSysColor(COLOR_BTNSHADOW), ::GetSysColor(COLOR_BTNHIGHLIGHT) );
			else
				dc.Draw3dRect( &butRect, ::GetSysColor(COLOR_BTNHIGHLIGHT), ::GetSysColor(COLOR_BTNSHADOW) );
	
		butRect.DeflateRect( 5, 4, 5, 5 );
		dc.MoveTo( butRect.left, ( butRect.top + butRect.bottom ) / 2 );
		dc.LineTo( butRect.right, ( butRect.top + butRect.bottom ) / 2 );
		dc.MoveTo( ( butRect.left + butRect.right ) /2, butRect.bottom );
		dc.LineTo( ( butRect.left + butRect.right ) /2, butRect.top -1 );
	}

	// Restore old DC objects
	dc.SelectObject( oldPen );

}


/**
 *	This method finds out if a point is on top of one of the caption bar
 *	buttons.
 *
 *	@param point	Screen point being tested (after a click, etc).
 *	@return		Button being hit, or if no button is hit, whether the point is
 *				in the caption or client areas.
 */
UINT Panel::hitTest( const CPoint point )
{
	BW_GUARD;

	CRect winRect;
	GetWindowRect(&winRect);
	
	int numBut = 0;

	CRect closeRect = winRect;
	closeRect.DeflateRect( winRect.Width() - CAPTION_HEIGHT, 0, 0, winRect.Height() - CAPTION_HEIGHT );
	numBut++;

	CRect rollupRect = closeRect;
	rollupRect.OffsetRect( -CAPTION_HEIGHT, 0 );
	numBut++;

	CRect cloneRect( 0, 0, 0, 0 );
	if ( activeTab_ && activeTab_->isClonable() )
	{
		cloneRect = rollupRect;
		cloneRect.OffsetRect( -CAPTION_HEIGHT, 0 );
		numBut++;
	}

	// deflate caption to exclude buttons
	winRect.DeflateRect( 0, CAPTION_HEIGHT * numBut, 0, 0 );

	// do the hit test.
	UINT ret = HTCAPTION;

	if( closeRect.PtInRect( point ) )
		ret =  BUT_CLOSE;
	else if( rollupRect.PtInRect( point ) )
		ret = BUT_ROLLUP;
	else if( activeTab_ && activeTab_->isClonable() && cloneRect.PtInRect( point ) )
		ret = BUT_CLONE;
	else if ( winRect.PtInRect( point ) )
		ret = HTCLIENT;
	else
		ret = HTCAPTION;

	return ret;
}


/**
 *	This method either shows or hides the tab bar depending on the panel's
 *	configuration.
 */
void Panel::updateTabBar()
{
	BW_GUARD;

	if ( isExpanded_ && tabBar_->itemCount() > 1 )
		tabBar_->ShowWindow( SW_SHOW );
	else
		tabBar_->ShowWindow( SW_HIDE );
}


/**
 *	This method sets the active tab for the panel.
 *
 *	@param tab	The new active tab (must be contained in the panel).
 */
void Panel::setActiveTab( TabPtr tab )
{
	BW_GUARD;

	if ( activeTab_ ) 
		activeTab_->getCWnd()->ShowWindow( SW_HIDE );

	activeTab_ = tab;

	updateTabBar();

	if ( tab )
	{
		if ( isExpanded_ )
			tab->getCWnd()->ShowWindow( SW_SHOW );
		tabBar_->setCurItem( activeTab_.getObject() );
		tab->getCWnd()->SetFocus();
	}

	recalcSize();

	paintCaptionBar();
}


/**
 *	This method recalculates the size and position of the tab bar control and 
 *	the current tab, to match the current panel size.
 */
void Panel::recalcSize()
{
	BW_GUARD;

	CRect rect;
	GetClientRect( &rect );
	recalcSize( rect.Width(), rect.Height() );
}


/**
 *	This method recalculates the size and position of the tab bar control and 
 *	the current tab, to the specified width and height.
 *
 *	@param w	New width.
 *	@param h	New height.
 */
void Panel::recalcSize( int w, int h )
{
	BW_GUARD;

	int tabBarHeight = 0;
	if ( tabBar_->GetSafeHwnd() )
	{
		tabBar_->SetWindowPos( 0, 0, 0, w, 1, SWP_NOZORDER );
		tabBar_->recalcHeight(); // force a recalc of the height/number of lines, based on the width
		if ( tabBar_->getAlignment() == TabCtrl::TOP )
			tabBar_->SetWindowPos( 0, 0, 0, w, tabBar_->getHeight(), SWP_NOZORDER );
		else
			tabBar_->SetWindowPos( 0, 0, h - tabBar_->getHeight(), w, tabBar_->getHeight(), SWP_NOZORDER );

		if ( tabBar_->itemCount() > 1 )
			tabBarHeight = tabBar_->getHeight() + 3;
	}

	if ( activeTab_ )
	{
		if ( tabBar_->getAlignment() == TabCtrl::TOP )
			activeTab_->getCWnd()->SetWindowPos( 0, 0, tabBarHeight, w, h - tabBarHeight, SWP_NOZORDER );
		else
			activeTab_->getCWnd()->SetWindowPos( 0, 0, 0, w, h - tabBarHeight, SWP_NOZORDER );
	}
}


/**
 *	This method inserts a temporary tab, useful for showing darg & drop
 *	feedback to the user without actually having a tab in the panel.
 *
 *	@param tab	Temporary tab object.
 */
void Panel::insertTempTab( TabPtr tab )
{
	BW_GUARD;

	if ( activeTab_ && isExpanded_ )
		activeTab_->getCWnd()->ShowWindow( SW_HIDE );
	tempTab_ = tab;
	tabBar_->insertItem( tab->getTabDisplayString(), tab->getIcon(), tab.getObject() );
	updateTabBar();
	recalcSize();
	UpdateWindow();
}


/**
 *	This method updates the position of a temporary tab, showing feedback as
 *	the user moves the dragged temporary tab over the tab control of the panel.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Panel::updateTempTab( int x, int y )
{
	BW_GUARD;

	if ( !tempTab_ )
		return;
	CRect rect;
	tabBar_->GetWindowRect( &rect );
	tabBar_->updateItemPosition( tempTab_.getObject(), x - rect.left, y - rect.top );
}


/**
 *	This method removes the temporary tab added during drag and drop.
 */
void Panel::removeTempTab()
{
	BW_GUARD;

	if ( !tempTab_ )
		return;
	if ( activeTab_ && isExpanded_ )
		activeTab_->getCWnd()->ShowWindow( SW_SHOW );
	tabBar_->removeItem( tempTab_.getObject() );
	if ( activeTab_ )
		tabBar_->setCurItem( activeTab_.getObject() );
	updateTabBar();
	recalcSize();
	UpdateWindow();
	tempTab_ = 0;
}


/**
 *	This method returns the currently active tab in this panel.
 *
 *	@return	The currently active tab in this panel.
 */
TabPtr Panel::getActiveTab()
{
	BW_GUARD;

	return activeTab_;
}


/**
 *	This method shows or hides a tab that is in the panel.
 *
 *	@param tab	Tab that will be shown or hidden.
 *	@param show	True to show the tab, false to hide it.
 */
void Panel::showTab( TabPtr tab, bool show )
{
	BW_GUARD;

	TabItr i;
	for( i = tabList_.begin(); i != tabList_.end(); ++i )
		if ( (*i) == tab )
			break;

	if ( i == tabList_.end() )
		return;

	if ( show )
	{
		if ( !tabBar_->contains( tab.getObject() ) )
		{
			tab->setVisible( true );
			tabBar_->insertItem( tab->getTabDisplayString(), tab->getIcon(), tab.getObject() );
		}

		setActiveTab( tab );
	}
	else
	{
		if ( tabBar_->contains( tab.getObject() ) )
		{
			tab->show( false );
			tabBar_->removeItem( tab.getObject() );
		}

		if ( activeTab_ == tab )
		{
			for( i = tabList_.begin(); i != tabList_.end(); ++i )
			{
				if ( tabBar_->contains( (*i).getObject() ) )
				{
					setActiveTab( (*i) );
					break;
				}
			}
		}
	}

	updateTabBar();

	if ( tabBar_->itemCount() == 0 )
		Manager::instance().dock()->showPanel( this, false );
}


/**
 *	This method shows or hides a content that is in a tab in this panel.
 *
 *	@param contentID	Content ID of the content that will be shown or hidden.
 *	@param show	True to show the content, false to hide it.
 */
void Panel::showTab( const BW::wstring contentID, bool show )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( tabContains( *i, contentID ) )
		{
			showTab( *i, show );
			Content* tcontent = (*i)->getContent().getObject();
			if ( tcontent && tcontent->getContentID() == ContentContainer::contentID )
				((ContentContainer*)tcontent)->currentContent( contentID );
		}
	}
}


/**
 *	This method shows or hides a content that is in a tab in this panel.
 *
 *	@param content	Content that will be shown or hidden.
 *	@param show	True to show the content, false to hide it.
 */
void Panel::showTab( ContentPtr content, bool show )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
	{
		if ( tabContains( *i, content ) )
		{
			showTab( *i, show );
			Content* tcontent = (*i)->getContent().getObject();
			if ( tcontent && tcontent->getContentID() == ContentContainer::contentID )
				((ContentContainer*)tcontent)->currentContent( content );
			break;
		}
	}
}


/**
 *	This method returns whether or not a content in this panel is visible.
 *
 *	@return Whether or not a content in this panel is visible.
 */
bool Panel::isTabVisible( const BW::wstring contentID )
{
	BW_GUARD;

	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
		if ( contentID.compare( (*i)->getContent()->getContentID() ) == 0 )
			return tabBar_->contains( (*i).getObject() );

	return false;
}


/**
 *	This method clones a tab after the user presses the "clone" button on the
 *	panel's caption bar.
 *
 *	@param content	Content that needs to be cloned.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 *	@return New Content, put in a newly created tab (and floating panel).
 */
ContentPtr Panel::cloneTab( ContentPtr content, int x, int y )
{
	BW_GUARD;

	if ( !content )
		return 0;

	content = content->clone(); 

	TabPtr tab = new Tab( this, content );
	tab->show( true );
	addTab( tab );
	CRect rect;
	GetWindowRect( &rect );
	Manager::instance().dock()->dockTab( this, tab, 0, FLOATING,
		rect.left, rect.top, x, y );

	return content;
}


/**
 *	This method returns the number of tabs in the panel.
 *
 *	@return The number of tabs in the panel.
 */
int Panel::tabCount()
{
	BW_GUARD;

	return (int)tabList_.size();
}


/**
 *	This method returns the number of visible tabs in the panel.
 *
 *	@return The number of visible tabs in the panel.
 */
int Panel::visibleTabCount()
{
	BW_GUARD;

	return tabBar_->itemCount();
}


/**
 *	This method updates the position of a tab in the panel's tab control to
 *	show feedback to the user when dragging a tab around.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void Panel::updateTabPosition( int x, int y )
{
	BW_GUARD;

	if ( !activeTab_ )
		return;

	CRect rect;
	tabBar_->GetWindowRect( &rect );
	tabBar_->updateItemPosition( activeTab_.getObject(), x - rect.left, y - rect.top );
}


/**
 *	This method returns the position in the tab control list for a tab if it
 *	was dropped at the specified x,y screen position after dragging.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 *	@return		The position in the tab control list
 */
int Panel::getTabInsertionIndex( int x, int y )
{
	BW_GUARD;

	CRect rect;
	tabBar_->GetWindowRect( &rect );
	return tabBar_->getItemIndex( 0, x - rect.left, y - rect.top );
}


/**
 *	This method mvoes the active tab to a new position in the tab control list.
 *
 *	@param index	New position for the active tab in the tab control list.
 */
void Panel::setActiveTabIndex( int index )
{
	BW_GUARD;

	if ( !activeTab_ )
		return;

	tabBar_->updateItemPosition( activeTab_.getObject(), index );
}


/**
 *	This method returns the position of the active tab in the tab control list.
 *
 *	@return		The position of the active tab in the tab control list.
 */
int Panel::getActiveTabIndex()
{
	BW_GUARD;

	if ( !activeTab_ )
		return -1;

	return tabBar_->getItemIndex( activeTab_.getObject() );
}


/**
 *	This method handles closing a tab.
 *
 *	@param tab	Tab that needs to be closed.
 *	@return		True to actually close the tab, false to abort closing.
 */
bool Panel::onTabClose( TabPtr tab )
{
	BW_GUARD;

	bool doClose = false;

	int cnt = Manager::instance().dock()->getContentCount( tab->getContent()->getContentID() );
	ASSERT( cnt != 0 );

	Content::OnCloseAction response = tab->getContent()->onClose( (cnt == 1) );
	if ( response == Content::CONTENT_KEEP )
	{
		doClose = false;
	}
	else if ( response == Content::CONTENT_HIDE )
	{
		showTab( tab, false );
		doClose = true;
	}
	else if ( response == Content::CONTENT_DESTROY )
	{
		detachTab( tab );
		doClose = true;
	}
	else
		ASSERT( 0 );

	return doClose;
}


/**
 *	This method handles a click on the "close" caption bar button of the panel.
 *
 *	@return		True if the close was successful, false to abort close.
 */
bool Panel::onClose()
{
	BW_GUARD;

	bool doClose;

	FloaterPtr floater = Manager::instance().dock()->getFloaterByWnd( getCWnd() );
	
	if ( floater )
	{
		// Hack: To avoid that the CFrameWnd floater still points to a deleted
		// panel as its view, which later results in assertion failed/crash.
		floater->SetActiveView( 0 );
	}

	// Hack: needed to avoid that the MainFrame thinks a FormView tab
	// contained in the panel is the active view, which results in
	// windows no longer receiving messages properly for some reason.
	Manager::instance().dock()->getMainFrame()->SetActiveView(
		(CView*)Manager::instance().dock()->getMainView()
		);

	if ( isFloating_ && ( !floater || floater->getRootNode()->getCWnd() == getCWnd() )  )
	{
		doClose = true;

		for( TabItr i = tabList_.begin(); i != tabList_.end(); )
		{
			// save iterator since onTabClose can potentially remove the tab from the list
			TabItr old = i;
			++i;
			if ( !onTabClose( (*old) ) )
				doClose = false;
		}
	}
	else
		doClose = onTabClose( activeTab_ );

	if ( doClose && tabBar_->itemCount() == 0 ) 
		Manager::instance().dock()->showPanel( this, false );

	return doClose;
}


/**
 *	This method returns the panel's index in the dock's panel list, useful for
 *	when saving.
 *
 *	@return		The panel's index in the dock's panel list.
 */
int Panel::getIndex()
{
	BW_GUARD;

	return Manager::instance().dock()->getPanelIndex( this );
}


//
// Windows events
//


// MFC message map
BEGIN_MESSAGE_MAP(Panel, CWnd)
	ON_WM_NCCALCSIZE()
	ON_WM_PAINT()
	ON_WM_NCPAINT()
	ON_WM_TIMER()
	ON_WM_NCHITTEST()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCLBUTTONUP()
	ON_WM_NCRBUTTONDOWN()
	ON_WM_NCMOUSEMOVE()
	ON_WM_MOUSEACTIVATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()


/**
 *	This MFC method is called by Windows to know the size of the non-client
 *	area (a.k.a caption bar).
 *
 *	@param bCalcValidRects	MFC param, ignored.
 *	@param lpncsp	MFC param, used to return the panel caption bar height.
 */
void Panel::OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp )
{
	BW_GUARD;

	if( !tabList_.empty() )
		lpncsp->rgrc[0].top += CAPTION_HEIGHT;
}


/**
 *	This MFC method is called by Windows paint non-client area (a.k.a caption
 *	bar).
 */
void Panel::OnNcPaint()
{
	BW_GUARD;

	if( !tabList_.empty() )
		paintCaptionBar();
}


/**
 *	This MFC method is called by Windows to paint the panel's client are.  It
 *	just forwards the call to the default implementation after updating the
 *	tabs and panel's size.
 */
void Panel::OnPaint()
{
	BW_GUARD;

	// refresh tab names, just in case
	for( TabItr i = tabList_.begin(); i != tabList_.end(); ++i )
		tabBar_->updateItemData( (*i).getObject(), (*i)->getTabDisplayString(), (*i)->getIcon() );
	recalcSize();
	CWnd::OnPaint();
}


/**
 *	This MFC method is called by Windows when a timer is triggered, used to
 *	remove the hover button graphics after the mouse has left the panel.
 *
 *	@param nIDEvent	ID of the timer.
 */
void Panel::OnTimer( UINT_PTR nIDEvent )
{
	BW_GUARD;

	CPoint point;

	GetCursorPos( &point );

	UINT ht = hitTest( point );

	if ( ht != BUT_CLOSE && ht != BUT_ROLLUP && ht != BUT_CLONE )
	{	
		paintCaptionButtons( ht );
		KillTimer( HOVER_TIMERID );
	}
	else
	{
		SetTimer( HOVER_TIMERID, HOVER_TIMERMILLIS, 0 );
	}
}


/**
 *	This MFC method is called by Windows when the mouse is moving over the
 *	non-client area (a.k.a caption bar).
 *
 *	@param point	MFC mouse position.
 *	@return		Which button the mouse is over, or whether it's over the client
 *				area or the caption bar (see hitTest).
 */
HITTESTRESULT Panel::OnNcHitTest( CPoint point )
{
	BW_GUARD;

	HITTESTRESULT ht = hitTest( point );

	if ( ht == BUT_CLOSE || ht == BUT_ROLLUP || ht == BUT_CLONE )
	{	
		paintCaptionButtons( ( UINT ) ht );
		SetTimer( HOVER_TIMERID, HOVER_TIMERMILLIS, 0 );
	}

	return ht;
}


/**
 *	This MFC method is called by Windows when the user presses down the left
 *	mouse button on the non-client area (a.k.a caption bar).
 *
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param point	Mouse position.
 */
void Panel::OnNcLButtonDown( UINT nHitTest, CPoint point )
{
	BW_GUARD;

	buttonDown_ = nHitTest;

	switch ( nHitTest )
	{
	case HTCAPTION:
		activate();
		paintCaptionButtons( nHitTest );
		Manager::instance().dragManager()->startDrag( point.x, point.y, this, 0 );
		return;
	}
	paintCaptionButtons( nHitTest );
}


/**
 *	This MFC method is called by Windows when the user releases up the left
 *	mouse button on the non-client area (a.k.a caption bar).
 *
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param point	Mouse position.
 */
void Panel::OnNcLButtonUp( UINT nHitTest, CPoint point )
{
	BW_GUARD;

	int lastBut = buttonDown_;
	buttonDown_ = 0;
	paintCaptionButtons( nHitTest );

	switch ( nHitTest )
	{
	case BUT_CLOSE:
		if ( lastBut != nHitTest ) break;
		if ( onClose() )
		{
			if ( tabCount() == 0 ) 
				Manager::instance().dock()->removePanel( this );
			return;
		}
		break;

	case BUT_ROLLUP:
		if ( lastBut != nHitTest ) break;
		setExpanded( !isExpanded_ );
		SetFocus();
		activate();
		break;

	case BUT_CLONE:
		if ( lastBut != nHitTest ) break;
		if ( activeTab_ )
		{
			CRect rect;
			GetWindowRect( &rect );
			// position is hand-hacked, but should work well in all cases
			cloneTab(
				activeTab_->getContent(),
				( rect.left + 10 ) % (GetSystemMetrics( SM_CXMAXIMIZED ) - 64),
				( rect.top ) % (GetSystemMetrics( SM_CYMAXIMIZED ) - 64) );
		}
		break;
	}
}


/**
 *	This MFC method is called by Windows when the user double-clicks the left
 *	mouse button on the non-client area (a.k.a caption bar).
 *
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param point	Mouse position.
 */
void Panel::OnNcLButtonDblClk( UINT nHitTest, CPoint point )
{
	BW_GUARD;

	if ( nHitTest == HTCAPTION )
	{
		Manager::instance().dock()->togglePanelPos( this );
	}
}


/**
 *	This MFC method is called by Windows when the user presses down the right
 *	mouse button on the non-client area (a.k.a caption bar).
 *
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param point	Mouse position.
 */
void Panel::OnNcRButtonDown( UINT nHitTest, CPoint point )
{
	BW_GUARD;

	if ( nHitTest == HTCAPTION && activeTab_ )
		activeTab_->handleRightClick( point.x, point.y );
}


/**
 *	This MFC method is called by Windows when the user moves the mouse over the
 *	non-client area (a.k.a caption bar).
 *
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param point	Mouse position.
 */
void Panel::OnNcMouseMove( UINT nHitTest, CPoint point )
{
	BW_GUARD;

	if ( buttonDown_ )
	{
		short mouseButton;

		if ( GetSystemMetrics( SM_SWAPBUTTON ) )
			mouseButton = GetAsyncKeyState( VK_RBUTTON );
		else
			mouseButton = GetAsyncKeyState( VK_LBUTTON );

		if ( mouseButton >= 0 )
			buttonDown_ = 0;
	}
}


/**
 *	This MFC method is called by Windows when the user activates the panel by
 *	clicking somewhere on it.
 *
 *	@param pDesktopWnd	MFC desktop window, ignored.
 *	@param nHitTest	Which area, result of OnNcHitTest (see OnNcHitTest).
 *	@param message	MFC activate message, ignored.
 *	@return	The result of the base class's OnMouseActivate.
 */
int Panel::OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message )
{
	BW_GUARD;

	if ( nHitTest == HTCAPTION || ( !isActive_ && nHitTest == HTCLIENT ) )
	{
		if ( Manager::instance().dock()->getMainFrame() )
		{
			if ( isFloating_ )
				Manager::instance().dock()->getMainFrame()->SetForegroundWindow();
		}
		activate();
		if ( Manager::instance().dock()->getMainFrame() )
		{
			// Hack: needed to avoid that the MainFrame thinks a FormView tab
			// contained in the panel is the active view, which results in
			// windows no longer receiving messages properly for some reason.
			Manager::instance().dock()->getMainFrame()->SetActiveView(
				(CView*)Manager::instance().dock()->getMainView()
				);
		}
		return MA_ACTIVATE;
	}

	return CWnd::OnMouseActivate( pDesktopWnd, nHitTest, message );
}


/**
 *	This MFC method is overriden to make sure the panel's tab control and 
 *	active tab get resized properly when the panel's own window is resized.
 *
 *	@param nType	MFC resize type.
 *	@param cx	New width.
 *	@param cy	New height.
 */
void Panel::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CWnd::OnSize( nType, cx, cy );

	if (cx > 0 && cy > 0)
	{
		recalcSize( cx, cy );
	}
}


}	// namespace
BW_END_NAMESPACE

